// Filename: qpdialNode.h
// Created by:  drose (12Mar02)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001, Disney Enterprises, Inc.  All rights reserved
//
// All use of this software is subject to the terms of the Panda 3d
// Software license.  You should have received a copy of this license
// along with this source code; you will also find a current copy of
// the license at http://www.panda3d.org/license.txt .
//
// To contact the maintainers of this program write to
// panda3d@yahoogroups.com .
//
////////////////////////////////////////////////////////////////////

#ifndef qpDIALNODE_H
#define qpDIALNODE_H

#include "pandabase.h"

#include "clientBase.h"
#include "clientDialDevice.h"
#include "qpdataNode.h"


////////////////////////////////////////////////////////////////////
//       Class : qpDialNode
// Description : This is the primary interface to infinite dial type
//               devices associated with a ClientBase.  This creates a
//               node that connects to the named dial device, if it
//               exists, and provides hooks to the user to read the
//               state of any of the sequentially numbered dial
//               controls associated with that device.
//
//               A dial is a rotating device that does not have
//               stops--it can keep rotating any number of times.
//               Therefore it does not have a specific position at any
//               given time, unlike an AnalogDevice.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA qpDialNode : public qpDataNode {
PUBLISHED:
  qpDialNode(ClientBase *client, const string &device_name);
  virtual ~qpDialNode();

  INLINE bool is_valid() const;

  INLINE int get_num_dials() const;

  INLINE double read_dial(int index);
  INLINE bool is_dial_known(int index) const;

private:
  PT(ClientDialDevice) _dial;

protected:
  // Inherited from DataNode
  virtual void do_transmit_data(const DataNodeTransmit &input,
                                DataNodeTransmit &output);

private:
  // no inputs or outputs at the moment.

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    qpDataNode::init_type();
    register_type(_type_handle, "qpDialNode",
                  qpDataNode::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;
};

#include "qpdialNode.I"

#endif
