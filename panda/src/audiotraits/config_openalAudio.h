// Filename: config_openalAudio.h
// Created by:  Ben Buchwald <bb2@alumni.cmu.edu>
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2004, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://etc.cmu.edu/panda3d/docs/license/ .
//
// To contact the maintainers of this program write to
// panda3d-general@lists.sourceforge.net .
//
////////////////////////////////////////////////////////////////////

#ifndef CONFIG_OPENALAUDIO_H
#define CONFIG_OPENALAUDIO_H

#include "pandabase.h"

#ifdef HAVE_OPENAL //[
#include "notifyCategoryProxy.h"
#include "dconfig.h"

ConfigureDecl(config_openalAudio, EXPCL_OPENAL_AUDIO, EXPTP_OPENAL_AUDIO);
NotifyCategoryDecl(openalAudio, EXPCL_OPENAL_AUDIO, EXPTP_OPENAL_AUDIO);

extern EXPCL_OPENAL_AUDIO void init_libOpenALAudio();

#endif //]

#endif // CONFIG_OPENALAUDIO_H
