// Filename: movieVideo.cxx
// Created by: jyelon (02Jul07)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) 2001 - 2007, Disney Enterprises, Inc.  All rights reserved
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

#include "movieVideoCursor.h"
#include "config_movies.h"

TypeHandle MovieVideoCursor::_type_handle;

////////////////////////////////////////////////////////////////////
//     Function: MovieVideoCursor::Constructor
//       Access: Public
//  Description: This constructor returns a null video stream --- a
//               stream of plain blue and white frames that last one
//               second each. To get more interesting video, you need
//               to construct a subclass of this class.
////////////////////////////////////////////////////////////////////
MovieVideoCursor::
MovieVideoCursor(PT(MovieVideo) src) :
  _source(src),
  _size_x(1),
  _size_y(1),
  _num_components(3),
  _length(1.0E10),
  _can_seek(true),
  _can_seek_fast(true),
  _aborted(false),
  _last_start(-1.0),
  _next_start(0.0)
{
}

////////////////////////////////////////////////////////////////////
//     Function: MovieVideoCursor::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
MovieVideoCursor::
~MovieVideoCursor() {
  if (_conversion_buffer != 0) {
    delete[] _conversion_buffer;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MovieVideoCursor::allocate_conversion_buffer
//       Access: Private
//  Description: The generic implementations of fetch_into_texture
//               and fetch_into_alpha require the use of a conversion
//               buffer.  This allocates the buffer.
////////////////////////////////////////////////////////////////////
void MovieVideoCursor::
allocate_conversion_buffer() {
  if (_conversion_buffer == 0) {
    _conversion_buffer = new unsigned char[size_x() * size_y() * 4];
  }
}
  
////////////////////////////////////////////////////////////////////
//     Function: MovieVideoCursor::fetch_into_bitbucket
//       Access: Published, Virtual
//  Description: Discards the next video frame.  Still sets
//               last_start and next_start.
//
//               See fetch_into_buffer for more details.
////////////////////////////////////////////////////////////////////
void MovieVideoCursor::
fetch_into_bitbucket(double time) {

  // This generic implementation is layered on fetch_into_buffer.
  // It will work for any derived class, so it is never necessary to
  // redefine this.  It is probably possible to make a faster
  // implementation, but since this function is rarely used, it
  // probably isn't worth the trouble.

  allocate_conversion_buffer();
  fetch_into_buffer(time, _conversion_buffer, false);
}

////////////////////////////////////////////////////////////////////
//     Function: MovieVideoCursor::fetch_into_texture
//       Access: Published, Virtual
//  Description: Reads the specified video frame into 
//               the specified texture.
//
//               See fetch_into_buffer for more details.
////////////////////////////////////////////////////////////////////
void MovieVideoCursor::
fetch_into_texture(double time, Texture *t, int page) {

  // This generic implementation is layered on fetch_into_buffer.
  // It will work for any derived class, so it is never necessary to
  // redefine this.  However, it may be possible to make a faster
  // implementation that uses fewer intermediate copies, depending
  // on the capabilities of the underlying codec software.

  nassertv(t->get_x_size() >= size_x());
  nassertv(t->get_y_size() >= size_y());
  nassertv((t->get_num_components() == 3) || (t->get_num_components() == 4));
  nassertv(t->get_component_width() == 1);
  nassertv(page < t->get_z_size());
  
  PTA_uchar img = t->modify_ram_image();
  
  unsigned char *data = img.p() + page * t->get_expected_ram_page_size();

  if (t->get_x_size() == size_x()) {
    fetch_into_buffer(time, data, t->get_num_components() == 4);
  } else {
    allocate_conversion_buffer();
    fetch_into_buffer(time, _conversion_buffer, t->get_num_components() == 4);
    int src_stride = size_x() * t->get_num_components();
    int dst_stride = t->get_x_size() * t->get_num_components();
    unsigned char *p = _conversion_buffer;
    for (int y=0; y<size_y(); y++) {
      memcpy(data, p, src_stride);
      data += dst_stride;
      p += src_stride;
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MovieVideoCursor::fetch_into_texture_alpha
//       Access: Published, Virtual
//  Description: Reads the specified video frame into 
//               the alpha channel of the supplied texture.  The
//               RGB channels of the texture are not touched.
//
//               See fetch_into_buffer for more details.
////////////////////////////////////////////////////////////////////
void MovieVideoCursor::
fetch_into_texture_alpha(double time, Texture *t, int page, int alpha_src) {

  // This generic implementation is layered on fetch_into_buffer.
  // It will work for any derived class, so it is never necessary to
  // redefine this.  However, it may be possible to make a faster
  // implementation that uses fewer intermediate copies, depending
  // on the capabilities of the underlying codec software.

  nassertv(t->get_x_size() >= size_x());
  nassertv(t->get_y_size() >= size_y());
  nassertv(t->get_num_components() == 4);
  nassertv(t->get_component_width() == 1);
  nassertv(page < t->get_z_size());
  nassertv((alpha_src >= 0) && (alpha_src <= 4));

  allocate_conversion_buffer();
  
  fetch_into_buffer(time, _conversion_buffer, true);
  
  PTA_uchar img = t->modify_ram_image();
  
  unsigned char *data = img.p() + page * t->get_expected_ram_page_size();
  
  int src_stride = size_x() * 4;
  int dst_stride = t->get_x_size() * 4;
  if (alpha_src == 0) {
    unsigned char *p = _conversion_buffer;
    for (int y=0; y<size_y(); y++) {
      for (int x=0; x<size_x(); x++) {
        data[x*4+3] = (p[x*4+0] + p[x*4+1] + p[x*4+2]) / 3;
      }
      data += dst_stride;
      p += src_stride;
    }
  } else {
    alpha_src -= 1;
    unsigned char *p = _conversion_buffer;
    for (int y=0; y<size_y(); y++) {
      for (int x=0; x<size_x(); x++) {
        data[x*4+3] = p[x*4+alpha_src];
      }
      data += dst_stride;
      p += src_stride;
    }
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MovieVideoCursor::fetch_into_texture_rgb
//       Access: Published, Virtual
//  Description: Reads the specified video frame into
//               the RGB channels of the supplied texture.  The alpha
//               channel of the texture is not touched.
//
//               See fetch_into_buffer for more details.
////////////////////////////////////////////////////////////////////
void MovieVideoCursor::
fetch_into_texture_rgb(double time, Texture *t, int page) {

  // This generic implementation is layered on fetch_into_buffer.
  // It will work for any derived class, so it is never necessary to
  // redefine this.  However, it may be possible to make a faster
  // implementation that uses fewer intermediate copies, depending
  // on the capabilities of the underlying codec software.

  nassertv(t->get_x_size() >= size_x());
  nassertv(t->get_y_size() >= size_y());
  nassertv(t->get_num_components() == 4);
  nassertv(t->get_component_width() == 1);
  nassertv(page < t->get_z_size());

  allocate_conversion_buffer();
  
  fetch_into_buffer(time, _conversion_buffer, true);
  
  PTA_uchar img = t->modify_ram_image();
  
  unsigned char *data = img.p() + page * t->get_expected_ram_page_size();
  
  int src_stride = size_x() * 4;
  int dst_stride = t->get_x_size() * 4;
  unsigned char *p = _conversion_buffer;
  for (int y=0; y<size_y(); y++) {
    for (int x=0; x<size_x(); x++) {
      data[x*4+0] = p[x*4+0];
      data[x*4+1] = p[x*4+1];
      data[x*4+2] = p[x*4+2];
    }
    data += dst_stride;
    p += src_stride;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: MovieVideoCursor::fetch_into_buffer
//       Access: Published, Virtual
//  Description: Reads the specified video frame into the supplied
//               BGR or BGRA buffer.  The frame's begin and end
//               times are stored in last_start and next_start.
//
//               If the movie reports that it can_seek, you may
//               also specify a timestamp less than next_start.
//               Otherwise, you may only specify a timestamp
//               greater than or equal to next_start.
//
//               If the movie reports that it can_seek, it doesn't
//               mean that it can do so quickly.  It may have to
//               rewind the movie and then fast forward to the
//               desired location.  Only if can_seek_fast returns
//               true can it seek rapidly.
////////////////////////////////////////////////////////////////////
void MovieVideoCursor::
fetch_into_buffer(double time, unsigned char *data, bool bgra) {
  
  // The following is the implementation of the null video stream, ie,
  // a stream of blinking red and blue frames.  This method must be
  // overridden by the subclass.
  
  _last_start = floor(time);
  _next_start = _last_start + 1;

  if (((int)_last_start) & 1) {
    data[0] = 255;
    data[1] = 128;
    data[2] = 128;
  } else {
    data[0] = 128;
    data[1] = 128;
    data[2] = 255;
  }
  if (bgra) {
    data[3] = 255;
  }
}

