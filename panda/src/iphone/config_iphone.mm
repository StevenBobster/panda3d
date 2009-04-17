// Filename: config_iphone.cxx
// Created by:  drose (08Apr09)
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

#include "config_iphone.h"
#include "iPhoneGraphicsPipe.h"
#include "iPhoneGraphicsStateGuardian.h"
#include "iPhoneGraphicsWindow.h"

#include "graphicsPipeSelection.h"
#include "dconfig.h"
#include "pandaSystem.h"


Configure(config_iphone);

NotifyCategoryDef(iphone, "display");

ConfigureFn(config_iphone) {
  init_libiphone();
}

////////////////////////////////////////////////////////////////////
//     Function: init_libiphone
//  Description: Initializes the library.  This must be called at
//               least once before any of the functions or classes in
//               this library can be used.  Normally it will be
//               called by the static initializers and need not be
//               called explicitly, but special cases exist.
////////////////////////////////////////////////////////////////////
void
init_libiphone() {
  static bool initialized = false;
  if (initialized) {
    return;
  }
  initialized = true;

  IPhoneGraphicsPipe::init_type();
  IPhoneGraphicsWindow::init_type();
  IPhoneGraphicsStateGuardian::init_type();

  GraphicsPipeSelection *selection = GraphicsPipeSelection::get_global_ptr();
  selection->add_pipe_type(IPhoneGraphicsPipe::get_class_type(), IPhoneGraphicsPipe::pipe_constructor);

  PandaSystem *ps = PandaSystem::get_global_ptr();
  ps->set_system_tag("OpenGL", "window_system", "IPhone");
  ps->set_system_tag("OpenGL ES", "window_system", "IPhone");
}

////////////////////////////////////////////////////////////////////
//     Function: get_pipe_type_iphone
//  Description: Returns the TypeHandle index of the recommended
//               graphics pipe type defined by this module.
////////////////////////////////////////////////////////////////////
int
get_pipe_type_iphone() {
  return IPhoneGraphicsPipe::get_class_type().get_index();
}
