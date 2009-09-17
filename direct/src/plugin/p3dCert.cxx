// Filename: p3dCert.cxx
// Created by:  drose (11Sep09)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#include "p3dCert.h"
#include "wx/cmdline.h"
         
#ifdef __WXMAC__
#include <Carbon/Carbon.h>
extern "C" { void CPSEnableForegroundOperation(ProcessSerialNumber* psn); }
#endif

static const wxString
self_signed_cert_text = _T
  ("This Panda3D application has been signed by what's known as a "
   "self-signed certificate.  This means the name on the certificate can't "
   "be verified, and you have no way of knowing for sure who wrote it.\n\n"

   "We recommend you click Cancel to avoid running this application.");

static const wxString
unknown_auth_cert_text = _T
  ("This Panda3D application has been signed, but we don't recognize "
   "the authority that verifies the signature.  This means the name "
   "on the certificate can't be trusted, and you have no way of knowing "
   "for sure who wrote it.\n\n"

   "We recommend you click Cancel to avoid running this application.");

static const wxString
verified_cert_text = _T
  ("This Panda3D application has been signed by %s. "
   "If you trust %s, then click the Run button below "
   "to run this application on your computer.  This will also "
   "automatically approve this and any other applications signed by "
   "%s in the future.\n\n"

   "If you are unsure about this application, "
   "you should click Cancel instead.");

static const wxString
expired_cert_text = _T
  ("This Panda3D application has been signed by %s, "
   "but the certificate has expired.\n\n"

   "You should check the current date set on your computer's clock "
   "to make sure it is correct.\n\n"

   "If your computer's date is correct, we recommend "
   "you click Cancel to avoid running this application.");

static const wxString
generic_error_cert_text = _T
  ("This Panda3D application has been signed, but there is a problem "
   "with the certificate (OpenSSL error code %d).\n\n"

   "We recommend you click Cancel to avoid running this application.");

static const wxString
no_cert_text = _T
  ("This Panda3D application has not been signed.  This means you have "
   "no way of knowing for sure who wrote it.\n\n"

   "Click Cancel to avoid running this application.");

// the event tables connect the wxWidgets events with the functions
// (event handlers) which process them. It can be also done at
// run-time, but for the simple menu events like this the static
// method is much simpler.
/*
BEGIN_EVENT_TABLE(MyFrame, wxFrame)
    EVT_MENU(Minimal_Quit,  MyFrame::OnQuit)
    EVT_MENU(Minimal_About, MyFrame::OnAbout)
END_EVENT_TABLE()
*/

// Create a new application object: this macro will allow wxWidgets to
// create the application object during program execution (it's better
// than using a static object for many reasons) and also implements
// the accessor function wxGetApp() which will return the reference of
// the right type (i.e. P3DCertApp and not wxApp)
IMPLEMENT_APP(P3DCertApp)

////////////////////////////////////////////////////////////////////
//     Function: P3DCertApp::OnInit
//       Access: Public, Virtual
//  Description: The "main" of a wx application.  This is the first
//               entry point.
////////////////////////////////////////////////////////////////////
bool P3DCertApp::
OnInit() {
  // call the base class initialization method, currently it only parses a
  // few common command-line options but it could be do more in the future
  if (!wxApp::OnInit()) {
    return false;
  }

  OpenSSL_add_all_algorithms();

#ifdef __WXMAC__
  // Enable the dialog to go to the foreground on Mac, even without
  // having to wrap it up in a bundle.
  ProcessSerialNumber psn;
  
  GetCurrentProcess(&psn);
  CPSEnableForegroundOperation(&psn);
  SetFrontProcess(&psn);
#endif

  AuthDialog *dialog = new AuthDialog(_cert_filename, _ca_filename);
  dialog->Show(true);

  // Return true to enter the main loop and wait for user input.
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: P3DCertApp::OnInitCmdLine
//       Access: Public, Virtual
//  Description: A callback to initialize the parser with the command
//               line options.
////////////////////////////////////////////////////////////////////
void P3DCertApp::
OnInitCmdLine(wxCmdLineParser &parser) {
  parser.AddParam();
  parser.AddParam();
}

////////////////////////////////////////////////////////////////////
//     Function: P3DCertApp::OnCmdLineParsed
//       Access: Public, Virtual
//  Description: A callback after the successful parsing of the
//               command line.
////////////////////////////////////////////////////////////////////
bool P3DCertApp::
OnCmdLineParsed(wxCmdLineParser &parser) {
  _cert_filename = parser.GetParam(0);
  _ca_filename = parser.GetParam(1);
  return true;
}

////////////////////////////////////////////////////////////////////
//     Function: AuthDialog::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
AuthDialog::
AuthDialog(const wxString &cert_filename, const wxString &ca_filename) : 
  wxDialog(NULL, wxID_ANY, _T("New Panda3D Application"), wxDefaultPosition)
{
  _cert = NULL;
  _stack = NULL;
  _verify_result = -1;

  read_cert_file(cert_filename);
  get_common_name();
  verify_cert(ca_filename);
  layout();
}

////////////////////////////////////////////////////////////////////
//     Function: AuthDialog::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
AuthDialog::
~AuthDialog() {
  if (_cert != NULL) { 
    X509_free(_cert);
    _cert = NULL;
  }
  if (_stack != NULL) { 
    sk_free(_stack);
    _stack = NULL;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: AuthDialog::read_cert_file
//       Access: Private
//  Description: Reads the list of certificates in the pem filename
//               passed on the command line into _cert and _stack.
////////////////////////////////////////////////////////////////////
void AuthDialog::
read_cert_file(const wxString &cert_filename) {
  FILE *fp = fopen(cert_filename.mb_str(), "r");
  if (fp == NULL) {
    cerr << "Couldn't read " << cert_filename << "\n";
    return;
  }
  _cert = PEM_read_X509(fp, NULL, NULL, (void *)"");
  if (_cert == NULL) {
    cerr << "Could not read certificate in " << cert_filename << ".\n";
    fclose(fp);
    return;
  }

  // Build up a STACK of the remaining certificates in the file.
  _stack = sk_new(NULL);
  X509 *c = PEM_read_X509(fp, NULL, NULL, (void *)"");
  while (c != NULL) {
    sk_push(_stack, (char *)c);
    c = PEM_read_X509(fp, NULL, NULL, (void *)"");
  }

  fclose(fp);
}

////////////////////////////////////////////////////////////////////
//     Function: AuthDialog::get_common_name
//       Access: Public
//  Description: Extracts the common_name from the certificate.
////////////////////////////////////////////////////////////////////
void AuthDialog::
get_common_name() {
  if (_cert == NULL) {
    _common_name.clear();
    return;
  }

  // A complex OpenSSL interface to extract out the common name in
  // utf-8.
  X509_NAME *xname = X509_get_subject_name(_cert);
  if (xname != NULL) {
    int pos = X509_NAME_get_index_by_NID(xname, NID_commonName, -1);
    if (pos != -1) {
      // We just get the first common name.  I guess it's possible to
      // have more than one; not sure what that means in this context.
      X509_NAME_ENTRY *xentry = X509_NAME_get_entry(xname, pos);
      if (xentry != NULL) {
        ASN1_STRING *data = X509_NAME_ENTRY_get_data(xentry);
        if (data != NULL) {
          // We use "print" to dump the output to a memory BIO.  Is
          // there an easier way to decode the ASN1_STRING?  Curse
          // these incomplete docs.
          BIO *mbio = BIO_new(BIO_s_mem());
          ASN1_STRING_print_ex(mbio, data, ASN1_STRFLGS_RFC2253 & ~ASN1_STRFLGS_ESC_MSB);

          char *pp;
          long pp_size = BIO_get_mem_data(mbio, &pp);
          _common_name = wxString(pp, wxConvUTF8, pp_size);
          BIO_free(mbio);
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: AuthDialog::verify_cert
//       Access: Public
//  Description: Checks whether the certificate is valid by the chain
//               and initializes _verify_status accordingly.
////////////////////////////////////////////////////////////////////
void AuthDialog::
verify_cert(const wxString &ca_filename) {
  if (_cert == NULL) {
    _verify_result = -1;
    return;
  }

  // Create a new X509_STORE.
  X509_STORE *store = X509_STORE_new();
  X509_STORE_set_default_paths(store);

  // Read the trusted certificates.
  FILE *fp = fopen(ca_filename.mb_str(), "r");
  if (fp == NULL) {
    cerr << "Couldn't read " << ca_filename << "\n";
  } else {
    X509 *c = PEM_read_X509(fp, NULL, NULL, (void *)"");
    while (c != NULL) {
      X509_STORE_add_cert(store, c);
      c = PEM_read_X509(fp, NULL, NULL, (void *)"");
    }
    fclose(fp);
  }

  // Create the X509_STORE_CTX for verifying the cert and chain.
  X509_STORE_CTX *ctx = X509_STORE_CTX_new();
  X509_STORE_CTX_init(ctx, store, _cert, _stack);
  X509_STORE_CTX_set_cert(ctx, _cert);

  if (X509_verify_cert(ctx)) {
    _verify_result = 0;
  } else {
    _verify_result = X509_STORE_CTX_get_error(ctx);
  }

  X509_STORE_CTX_cleanup(ctx);
  X509_STORE_CTX_free(ctx);

  X509_STORE_free(store);

  cerr << "Got certificate from " << _common_name
       << ", verify_result = " << _verify_result << "\n";
}

////////////////////////////////////////////////////////////////////
//     Function: AuthDialog::layout
//       Access: Private
//  Description: Arranges the text and controls within the dialog.
////////////////////////////////////////////////////////////////////
void AuthDialog::
layout() {
  wxString header, text;
  get_text(header, text);

  wxPanel *panel = new wxPanel(this);
  wxBoxSizer *vsizer = new wxBoxSizer(wxVERTICAL);

  wxFont font = panel->GetFont();
  wxFont *bold_font = wxTheFontList->FindOrCreateFont
    (font.GetPointSize() * 1.5,
     font.GetFamily(), font.GetStyle(), wxFONTWEIGHT_BOLD);

  if (!header.IsEmpty()) {
    wxStaticText *text0 = new wxStaticText
      (panel, wxID_ANY, header, wxDefaultPosition, wxDefaultSize,
       wxALIGN_CENTER);
    vsizer->Add(text0, 0, wxCENTER | wxALL, 10);
  }

  wxStaticText *text1 = new wxStaticText
    (panel, wxID_ANY, text, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER);
  text1->Wrap(400);
  vsizer->Add(text1, 0, wxCENTER | wxALL, 10);

  // Create the run / cancel buttons.
  wxBoxSizer *bsizer = new wxBoxSizer(wxHORIZONTAL);

  if (_verify_result == 0 && _cert != NULL) {
    wxButton *run_button = new wxButton(panel, wxID_OK, _T("Run"));
    bsizer->Add(run_button, 0, wxALIGN_CENTER | wxALL, 5);
  }

  if (_cert != NULL) {
    wxButton *view_button = new wxButton(panel, wxID_ANY, _T("View Certificate"));
    bsizer->Add(view_button, 0, wxALIGN_CENTER | wxALL, 5);
  }

  wxButton *cancel_button = new wxButton(panel, wxID_CANCEL, _T("Cancel"));
  bsizer->Add(cancel_button, 0, wxALIGN_CENTER | wxALL, 5);

  vsizer->Add(bsizer, 0, wxALIGN_CENTER | wxALL, 5);

  panel->SetSizer(vsizer);
  panel->SetAutoLayout(true);
  vsizer->Fit(this);
}

////////////////////////////////////////////////////////////////////
//     Function: AuthDialog::get_text
//       Access: Private
//  Description: Fills in the text appropriate to display in the
//               dialog box, based on the certificate read so far.
////////////////////////////////////////////////////////////////////
void AuthDialog::
get_text(wxString &header, wxString &text) {
  switch (_verify_result) {
  case -1:
    header = _T("No signature!");
    text = no_cert_text;
    break;

  case 0:
    text.Printf(verified_cert_text, _common_name.c_str(), _common_name.c_str(), _common_name.c_str());
    break;

  case X509_V_ERR_CERT_NOT_YET_VALID:
  case X509_V_ERR_CERT_HAS_EXPIRED:
  case X509_V_ERR_CRL_NOT_YET_VALID:
  case X509_V_ERR_CRL_HAS_EXPIRED:
    header = _T("Expired signatured!");
    text.Printf(expired_cert_text, _common_name.c_str());
    break;

  case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
    header = _T("Unverified signature!");
    text.Printf(unknown_auth_cert_text, _common_name.c_str());
    break;
      
  case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
  case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
    header = _T("Unverified signature!");
    text = self_signed_cert_text;
    break;

  default:
    header = _T("Unverified signature!");
    text.Printf(generic_error_cert_text, _verify_result);
  }
}