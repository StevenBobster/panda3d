// Filename: graphicsChannel.h
// Created by:  mike (09Jan97)
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
#ifndef GRAPHICSCHANNEL_H
#define GRAPHICSCHANNEL_H

#include "pandabase.h"
#include "graphicsLayer.h"
#include "typedReferenceCount.h"
#include "pointerTo.h"
#include "pmutex.h"
#include "ordered_vector.h"
#include "indirectLess.h"

class GraphicsChannel;
class GraphicsPipe;
class GraphicsOutput;
class CullHandler;

////////////////////////////////////////////////////////////////////
//       Class : GraphicsChannel
// Description : This represents a single hardware output.  Typically
//               there is exactly one channel per GraphicsOutput, but
//               some implementations (e.g. SGI) support potentially
//               several different video channel ports connected to
//               different parts within a window.
////////////////////////////////////////////////////////////////////
class EXPCL_PANDA GraphicsChannel : public TypedReferenceCount {
protected:
  GraphicsChannel();

public:
  GraphicsChannel(GraphicsOutput *window);

private:
  GraphicsChannel(const GraphicsChannel &copy);
  void operator = (const GraphicsChannel &copy);

PUBLISHED:
  virtual ~GraphicsChannel();
  GraphicsLayer *make_layer(int sort = 0);
  bool remove_layer(GraphicsLayer *layer);
  bool move_layer(GraphicsLayer *layer, int sort);

  int get_num_layers() const;
  GraphicsLayer *get_layer(int index) const;

  GraphicsOutput *get_window() const;
  GraphicsPipe *get_pipe() const;

  void set_active(bool active);
  INLINE bool is_active() const;

public:
  virtual void window_resized(int x_size, int y_size);

private:
  void win_display_regions_changed();

protected:
  Mutex _lock;
  GraphicsOutput *_window;
  bool _is_active;

  typedef ov_multiset< PT(GraphicsLayer), IndirectLess<GraphicsLayer> > GraphicsLayers;
  GraphicsLayers _layers;

private:
  GraphicsLayers::iterator find_layer(GraphicsLayer *layer);

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TypedReferenceCount::init_type();
    register_type(_type_handle, "GraphicsChannel",
                  TypedReferenceCount::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }
  virtual TypeHandle force_init_type() {init_type(); return get_class_type();}

private:
  static TypeHandle _type_handle;

  friend class GraphicsOutput;
  friend class GraphicsLayer;
};

#include "graphicsChannel.I"

#endif /* GRAPHICSCHANNEL_H */
