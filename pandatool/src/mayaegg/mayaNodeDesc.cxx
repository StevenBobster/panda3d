// Filename: mayaNodeDesc.cxx
// Created by:  drose (06Jun03)
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

#include "mayaNodeDesc.h"

TypeHandle MayaNodeDesc::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: MayaNodeDesc::Constructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
MayaNodeDesc::
MayaNodeDesc(MayaNodeDesc *parent, const string &name) :
  Namable(name),
  _parent(parent)
{
  _dag_path = (MDagPath *)NULL;
  _egg_group = (EggGroup *)NULL;
  _egg_table = (EggTable *)NULL;
  _anim = (EggXfmSAnim *)NULL;
  _joint_type = JT_none;

  // Add ourselves to our parent.
  if (_parent != (MayaNodeDesc *)NULL) {
    _parent->_children.push_back(this);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MayaNodeDesc::Destructor
//       Access: Public
//  Description: 
////////////////////////////////////////////////////////////////////
MayaNodeDesc::
~MayaNodeDesc() {
  if (_dag_path != (MDagPath *)NULL) {
    delete _dag_path;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MayaNodeDesc::from_dag_path
//       Access: Public
//  Description: Indicates an associated between the MayaNodeDesc and
//               some Maya instance.
////////////////////////////////////////////////////////////////////
void MayaNodeDesc::
from_dag_path(const MDagPath &dag_path) {
  if (_dag_path == (MDagPath *)NULL) {
    _dag_path = new MDagPath(dag_path);

    if (_dag_path->hasFn(MFn::kJoint)) {
      // This node is a joint.
      _joint_type = JT_joint;
      if (_parent != (MayaNodeDesc *)NULL) {
        _parent->mark_joint_parent();
      }
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MayaNodeDesc::has_dag_path
//       Access: Public
//  Description: Returns true if a Maya dag path has been associated
//               with this node, false otherwise.
////////////////////////////////////////////////////////////////////
bool MayaNodeDesc::
has_dag_path() const {
  return (_dag_path != (MDagPath *)NULL);
}

////////////////////////////////////////////////////////////////////
//     Function: MayaNodeDesc::get_dag_path
//       Access: Public
//  Description: Returns the dag path associated with this node.  It
//               is an error to call this unless has_dag_path()
//               returned true.
////////////////////////////////////////////////////////////////////
const MDagPath &MayaNodeDesc::
get_dag_path() const {
  nassertr(_dag_path != (MDagPath *)NULL, *_dag_path);
  return *_dag_path;
}

////////////////////////////////////////////////////////////////////
//     Function: MayaNodeDesc::is_joint
//       Access: Private
//  Description: Returns true if the node should be treated as a joint
//               by the converter.
////////////////////////////////////////////////////////////////////
bool MayaNodeDesc::
is_joint() const {
  return _joint_type == JT_joint || _joint_type == JT_pseudo_joint;
}

////////////////////////////////////////////////////////////////////
//     Function: MayaNodeDesc::is_joint_parent
//       Access: Private
//  Description: Returns true if the node is the parent or ancestor of
//               a joint.
////////////////////////////////////////////////////////////////////
bool MayaNodeDesc::
is_joint_parent() const {
  return _joint_type == JT_joint_parent;
}

////////////////////////////////////////////////////////////////////
//     Function: MayaNodeDesc::clear_egg
//       Access: Private
//  Description: Recursively clears the egg pointers from this node
//               and all children.
////////////////////////////////////////////////////////////////////
void MayaNodeDesc::
clear_egg() {
  _egg_group = (EggGroup *)NULL;
  _egg_table = (EggTable *)NULL;
  _anim = (EggXfmSAnim *)NULL;

  Children::const_iterator ci;
  for (ci = _children.begin(); ci != _children.end(); ++ci) {
    MayaNodeDesc *child = (*ci);
    child->clear_egg();
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MayaNodeDesc::mark_joint_parent
//       Access: Private
//  Description: Indicates that this node has at least one child that
//               is a joint or a pseudo-joint.
////////////////////////////////////////////////////////////////////
void MayaNodeDesc::
mark_joint_parent() {
  if (_joint_type == JT_none) {
    _joint_type = JT_joint_parent;
    if (_parent != (MayaNodeDesc *)NULL) {
      _parent->mark_joint_parent();
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MayaNodeDesc::check_pseudo_joints
//       Access: Private
//  Description: Walks the hierarchy, looking for non-joint nodes that
//               are both children and parents of a joint.  These
//               nodes are deemed to be pseudo joints, since the
//               converter must treat them as joints.
////////////////////////////////////////////////////////////////////
void MayaNodeDesc::
check_pseudo_joints(bool joint_above) {
  if (_joint_type == JT_joint_parent && joint_above) {
    // This is one such node: it is the parent of a joint
    // (JT_joint_parent is set), and it is the child of a joint
    // (joint_above is set).
    _joint_type = JT_pseudo_joint;
  }

  if (_joint_type == JT_joint) {
    // If this node is itself a joint, then joint_above is true for
    // all child nodes.
    joint_above = true;
  }

  // Don't bother traversing further if _joint_type is none, since
  // that means this node has no joint children.
  if (_joint_type != JT_none) {

    bool any_joints = false;
    Children::const_iterator ci;
    for (ci = _children.begin(); ci != _children.end(); ++ci) {
      MayaNodeDesc *child = (*ci);
      child->check_pseudo_joints(joint_above);
      if (child->is_joint()) {
        any_joints = true;
      }
    }

    // If any children qualify as joints, then any sibling nodes that
    // are parents of joints are also elevated to joints.
    if (any_joints) {
      bool all_joints = true;
      for (ci = _children.begin(); ci != _children.end(); ++ci) {
        MayaNodeDesc *child = (*ci);
        if (child->_joint_type == JT_joint_parent) {
          child->_joint_type = JT_pseudo_joint;
        } else if (child->_joint_type == JT_none) {
          all_joints = false;
        }
      }

      if (all_joints) {
        // Finally, if all children are joints, then we are too.
        if (_joint_type == JT_joint_parent) {
          _joint_type = JT_pseudo_joint;
        }
      }
    }
  }
}
