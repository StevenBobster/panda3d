// Filename: qpgeomVertexReader.cxx
// Created by:  drose (25Mar05)
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

#include "qpgeomVertexReader.h"

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::set_data_type
//       Access: Published
//  Description: Sets up the reader to use the indicated data_type
//               description on the given array.
//
//               This also resets the current read vertex numbers to
//               the start vertex (the same value passed to a previous
//               call to set_vertex(), or 0 if set_vertex() was never
//               called.)
//
//               The return value is true if the data type is valid,
//               false otherwise.
////////////////////////////////////////////////////////////////////
bool qpGeomVertexReader::
set_data_type(int array, const qpGeomVertexDataType *data_type) {
  // Delete the old reader, if we've got one.
  if (_reader != (Reader *)NULL) {
    delete _reader;
    _reader = NULL;
  }

  if (array < 0 || array >= _vertex_data->get_num_arrays() || 
      data_type == (qpGeomVertexDataType *)NULL) {
    // Clear the data type.
    _array = -1;
    _data_type = NULL;
    _stride = 0;
    _read_vertex = _start_vertex;
    _num_vertices = 0;

    return false;

  } else {
    _array = array;
    _data_type = data_type;
    _stride = _vertex_data->get_format()->get_array(_array)->get_stride();

    set_pointer(_start_vertex);

    // Now set up a new reader.
    _reader = make_reader();
    _reader->_data_type = _data_type;

    return true;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::make_reader
//       Access: Private
//  Description: Returns a newly-allocated Reader object suitable for
//               reading the current data type.
////////////////////////////////////////////////////////////////////
qpGeomVertexReader::Reader *qpGeomVertexReader::
make_reader() const {
  switch (_data_type->get_contents()) {
  case qpGeomVertexDataType::C_point:
  case qpGeomVertexDataType::C_texcoord:
    // These types are read as a 4-d homogeneous point.
    switch (_data_type->get_numeric_type()) {
    case qpGeomVertexDataType::NT_float32:
      if (sizeof(float) == sizeof(PN_float32)) {
        // Use the native float type implementation for a tiny bit
        // more optimization.
        switch (_data_type->get_num_components()) {
        case 2:
          return new Reader_point_nativefloat_2;
        case 3:
          return new Reader_point_nativefloat_3;
        case 4:
          return new Reader_point_nativefloat_4;
        }
      } else {
        switch (_data_type->get_num_components()) {
        case 2:
          return new Reader_point_float32_2;
        case 3:
          return new Reader_point_float32_3;
        case 4:
          return new Reader_point_float32_4;
        }
      }
      break;
    default:
      break;
    }
    return new Reader_point;

  case qpGeomVertexDataType::C_rgba:
    switch (_data_type->get_numeric_type()) {
    case qpGeomVertexDataType::NT_uint8:
      switch (_data_type->get_num_components()) {
      case 4:
        return new Reader_rgba_uint8_4;
        
      default:
        break;
      }
      break;
    case qpGeomVertexDataType::NT_float32:
      switch (_data_type->get_num_components()) {
      case 4:
        if (sizeof(float) == sizeof(PN_float32)) {
          // Use the native float type implementation for a tiny bit
          // more optimization.
          return new Reader_rgba_nativefloat_4;
        } else {
          return new Reader_rgba_float32_4;
        }
        
      default:
        break;
      }
      break;
    default:
      break;
    }
    return new Reader_color;

  case qpGeomVertexDataType::C_argb:
    switch (_data_type->get_numeric_type()) {
    case qpGeomVertexDataType::NT_packed_8888:
      switch (_data_type->get_num_components()) {
      case 1:
        return new Reader_argb_packed_8888;
        
      default:
        break;
      }
      break;
    default:
      break;
    }
    return new Reader_color;

  default:
    // Otherwise, we just read it as a generic value.
    switch (_data_type->get_numeric_type()) {
    case qpGeomVertexDataType::NT_float32:
      switch (_data_type->get_num_components()) {
      case 3:
        if (sizeof(float) == sizeof(PN_float32)) {
          // Use the native float type implementation for a tiny bit
          // more optimization.
          return new Reader_nativefloat_3;
        } else {
          return new Reader_float32_3;
        }

      default:
        break;
      }
      break;
    default:
      break;
    }
    return new Reader;
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader::Destructor
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
qpGeomVertexReader::Reader::
~Reader() {
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader::get_data1f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
float qpGeomVertexReader::Reader::
get_data1f(const unsigned char *pointer) {
  switch (_data_type->get_numeric_type()) {
  case qpGeomVertexDataType::NT_uint8:
    return maybe_scale_color(*pointer);

  case qpGeomVertexDataType::NT_uint16:
    return *(const PN_uint16 *)pointer;

  case qpGeomVertexDataType::NT_packed_8888:
    {
      PN_uint32 dword = *(const PN_uint32 *)pointer;
      if (_data_type->get_contents() == qpGeomVertexDataType::C_argb) {
        return maybe_scale_color(qpGeomVertexData::unpack_8888_b(dword));
      } else {
        return maybe_scale_color(qpGeomVertexData::unpack_8888_a(dword));
      }
    }

  case qpGeomVertexDataType::NT_float32:
    return *(const PN_float32 *)pointer;
  }

  return 0.0f;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader::get_data2f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase2f &qpGeomVertexReader::Reader::
get_data2f(const unsigned char *pointer) {
  if (_data_type->get_num_values() == 1) {
    _v2.set(get_data1i(pointer), 0.0f);
    return _v2;

  } else {
    switch (_data_type->get_numeric_type()) {
    case qpGeomVertexDataType::NT_uint8:
      maybe_scale_color(pointer[0], pointer[1]);
      return _v2;
      
    case qpGeomVertexDataType::NT_uint16:
      {
        const PN_uint16 *pi = (const PN_uint16 *)pointer;
        _v2.set(pi[0], pi[1]);
      }
      return _v2;
      
    case qpGeomVertexDataType::NT_packed_8888:
      {
        PN_uint32 dword = *(const PN_uint32 *)pointer;
        if (_data_type->get_contents() == qpGeomVertexDataType::C_argb) {
          maybe_scale_color(qpGeomVertexData::unpack_8888_b(dword),
                            qpGeomVertexData::unpack_8888_c(dword));
        } else {
          maybe_scale_color(qpGeomVertexData::unpack_8888_a(dword),
                            qpGeomVertexData::unpack_8888_b(dword));
        }
      }
      return _v2;
      
    case qpGeomVertexDataType::NT_float32:
      {
        const PN_float32 *pi = (const PN_float32 *)pointer;
        _v2.set(pi[0], pi[1]);
      }
      return _v2;
    }
  }

  return _v2;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader::get_data3f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase3f &qpGeomVertexReader::Reader::
get_data3f(const unsigned char *pointer) {
  switch (_data_type->get_num_values()) {
  case 1:
    _v3.set(get_data1f(pointer), 0.0f, 0.0f);
    return _v3;

  case 2:
    {
      const LVecBase2f &v2 = get_data2f(pointer);
      _v3.set(v2[0], v2[1], 0.0f);
    }
    return _v3;

  default:
    switch (_data_type->get_numeric_type()) {
    case qpGeomVertexDataType::NT_uint8:
      maybe_scale_color(pointer[0], pointer[1], pointer[2]);
      return _v3;
      
    case qpGeomVertexDataType::NT_uint16:
      {
        const PN_uint16 *pi = (const PN_uint16 *)pointer;
        _v3.set(pi[0], pi[1], pi[2]);
      }
      return _v3;
      
    case qpGeomVertexDataType::NT_packed_8888:
      {
        PN_uint32 dword = *(const PN_uint32 *)pointer;
        if (_data_type->get_contents() == qpGeomVertexDataType::C_argb) {
          maybe_scale_color(qpGeomVertexData::unpack_8888_b(dword),
                            qpGeomVertexData::unpack_8888_c(dword),
                            qpGeomVertexData::unpack_8888_d(dword));
        } else {
          maybe_scale_color(qpGeomVertexData::unpack_8888_a(dword),
                            qpGeomVertexData::unpack_8888_b(dword),
                            qpGeomVertexData::unpack_8888_c(dword));
        }
      }
      return _v3;
      
    case qpGeomVertexDataType::NT_float32:
      {
        const PN_float32 *pi = (const PN_float32 *)pointer;
        _v3.set(pi[0], pi[1], pi[2]);
      }
      return _v3;
    }
  }

  return _v3;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader::get_data4f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase4f &qpGeomVertexReader::Reader::
get_data4f(const unsigned char *pointer) {
  switch (_data_type->get_num_values()) {
  case 1:
    _v4.set(get_data1i(pointer), 0.0f, 0.0f, 0.0f);
    return _v4;

  case 2:
    {
      const LVecBase2f &v2 = get_data2f(pointer);
      _v4.set(v2[0], v2[1], 0.0f, 0.0f);
    }
    return _v4;

  case 3:
    {
      const LVecBase3f &v3 = get_data3f(pointer);
      _v4.set(v3[0], v3[1], v3[2], 0.0f);
    }
    return _v4;

  default:
    switch (_data_type->get_numeric_type()) {
    case qpGeomVertexDataType::NT_uint8:
      maybe_scale_color(pointer[0], pointer[1], pointer[2], pointer[3]);
      return _v4;
      
    case qpGeomVertexDataType::NT_uint16:
      {
        const PN_uint16 *pi = (const PN_uint16 *)pointer;
        _v4.set(pi[0], pi[1], pi[2], pi[3]);
      }
      return _v4;
      
    case qpGeomVertexDataType::NT_packed_8888:
      {
        PN_uint32 dword = *(const PN_uint32 *)pointer;
        if (_data_type->get_contents() == qpGeomVertexDataType::C_argb) {
          maybe_scale_color(qpGeomVertexData::unpack_8888_b(dword),
                            qpGeomVertexData::unpack_8888_c(dword),
                            qpGeomVertexData::unpack_8888_d(dword),
                            qpGeomVertexData::unpack_8888_a(dword));
        } else {
          maybe_scale_color(qpGeomVertexData::unpack_8888_a(dword),
                            qpGeomVertexData::unpack_8888_b(dword),
                            qpGeomVertexData::unpack_8888_c(dword),
                            qpGeomVertexData::unpack_8888_d(dword));
        }
      }
      return _v4;
      
    case qpGeomVertexDataType::NT_float32:
      {
        const PN_float32 *pi = (const PN_float32 *)pointer;
        _v4.set(pi[0], pi[1], pi[2], pi[3]);
      }
      return _v4;
    }
  }

  return _v4;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader::get_data1i
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
int qpGeomVertexReader::Reader::
get_data1i(const unsigned char *pointer) {
  switch (_data_type->get_numeric_type()) {
  case qpGeomVertexDataType::NT_uint8:
    return *pointer;

  case qpGeomVertexDataType::NT_uint16:
    return *(const PN_uint16 *)pointer;

  case qpGeomVertexDataType::NT_packed_8888:
    {
      PN_uint32 dword = *(const PN_uint32 *)pointer;
      return qpGeomVertexData::unpack_8888_a(dword);
    }
    break;

  case qpGeomVertexDataType::NT_float32:
    return (int)*(const PN_float32 *)pointer;
  }

  return 0;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_point::get_data1f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
float qpGeomVertexReader::Reader_point::
get_data1f(const unsigned char *pointer) {
  if (_data_type->get_num_values() == 4) {
    const LVecBase4f &v4 = get_data4f(pointer);
    return v4[0] / v4[3];
  } else {
    return Reader::get_data1f(pointer);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_point::get_data2f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase2f &qpGeomVertexReader::Reader_point::
get_data2f(const unsigned char *pointer) {
  if (_data_type->get_num_values() == 4) {
    const LVecBase4f &v4 = get_data4f(pointer);
    _v2.set(v4[0] / v4[3], v4[1] / v4[3]);
    return _v2;
  } else {
    return Reader::get_data2f(pointer);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_point::get_data3f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase3f &qpGeomVertexReader::Reader_point::
get_data3f(const unsigned char *pointer) {
  if (_data_type->get_num_values() == 4) {
    const LVecBase4f &v4 = get_data4f(pointer);
    _v3.set(v4[0] / v4[3], v4[1] / v4[3], v4[2] / v4[3]);
    return _v3;
  } else {
    return Reader::get_data3f(pointer);
  }
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_point::get_data4f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase4f &qpGeomVertexReader::Reader_point::
get_data4f(const unsigned char *pointer) {
  switch (_data_type->get_num_values()) {
  case 1:
    _v4.set(get_data1i(pointer), 0.0f, 0.0f, 1.0f);
    return _v4;

  case 2:
    {
      const LVecBase2f &v2 = get_data2f(pointer);
      _v4.set(v2[0], v2[1], 0.0f, 1.0f);
    }
    return _v4;

  case 3:
    {
      const LVecBase3f &v3 = get_data3f(pointer);
      _v4.set(v3[0], v3[1], v3[2], 1.0f);
    }
    return _v4;

  default:
    switch (_data_type->get_numeric_type()) {
    case qpGeomVertexDataType::NT_uint8:
      maybe_scale_color(pointer[0], pointer[1], pointer[2], pointer[3]);
      return _v4;
      
    case qpGeomVertexDataType::NT_uint16:
      {
        const PN_uint16 *pi = (const PN_uint16 *)pointer;
        _v4.set(pi[0], pi[1], pi[2], pi[3]);
      }
      return _v4;
      
    case qpGeomVertexDataType::NT_packed_8888:
      {
        PN_uint32 dword = *(const PN_uint32 *)pointer;
        if (_data_type->get_contents() == qpGeomVertexDataType::C_argb) {
          maybe_scale_color(qpGeomVertexData::unpack_8888_b(dword),
                            qpGeomVertexData::unpack_8888_c(dword),
                            qpGeomVertexData::unpack_8888_d(dword),
                            qpGeomVertexData::unpack_8888_a(dword));
        } else {
          maybe_scale_color(qpGeomVertexData::unpack_8888_a(dword),
                            qpGeomVertexData::unpack_8888_b(dword),
                            qpGeomVertexData::unpack_8888_c(dword),
                            qpGeomVertexData::unpack_8888_d(dword));
        }
      }
      return _v4;
      
    case qpGeomVertexDataType::NT_float32:
      {
        const PN_float32 *pi = (const PN_float32 *)pointer;
        _v4.set(pi[0], pi[1], pi[2], pi[3]);
      }
      return _v4;
    }
  }

  return _v4;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_color::get_data4f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase4f &qpGeomVertexReader::Reader_color::
get_data4f(const unsigned char *pointer) {
  switch (_data_type->get_num_values()) {
  case 1:
    _v4.set(get_data1i(pointer), 0.0f, 0.0f, 1.0f);
    return _v4;

  case 2:
    {
      const LVecBase2f &v2 = get_data2f(pointer);
      _v4.set(v2[0], v2[1], 0.0f, 1.0f);
    }
    return _v4;

  case 3:
    {
      const LVecBase3f &v3 = get_data3f(pointer);
      _v4.set(v3[0], v3[1], v3[2], 1.0f);
    }
    return _v4;

  default:
    switch (_data_type->get_numeric_type()) {
    case qpGeomVertexDataType::NT_uint8:
      maybe_scale_color(pointer[0], pointer[1], pointer[2], pointer[3]);
      return _v4;
      
    case qpGeomVertexDataType::NT_uint16:
      {
        const PN_uint16 *pi = (const PN_uint16 *)pointer;
        _v4.set(pi[0], pi[1], pi[2], pi[3]);
      }
      return _v4;
      
    case qpGeomVertexDataType::NT_packed_8888:
      {
        PN_uint32 dword = *(const PN_uint32 *)pointer;
        if (_data_type->get_contents() == qpGeomVertexDataType::C_argb) {
          maybe_scale_color(qpGeomVertexData::unpack_8888_b(dword),
                            qpGeomVertexData::unpack_8888_c(dword),
                            qpGeomVertexData::unpack_8888_d(dword),
                            qpGeomVertexData::unpack_8888_a(dword));
        } else {
          maybe_scale_color(qpGeomVertexData::unpack_8888_a(dword),
                            qpGeomVertexData::unpack_8888_b(dword),
                            qpGeomVertexData::unpack_8888_c(dword),
                            qpGeomVertexData::unpack_8888_d(dword));
        }
      }
      return _v4;
      
    case qpGeomVertexDataType::NT_float32:
      {
        const PN_float32 *pi = (const PN_float32 *)pointer;
        _v4.set(pi[0], pi[1], pi[2], pi[3]);
      }
      return _v4;
    }
  }

  return _v4;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_float32_3::get_data3f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase3f &qpGeomVertexReader::Reader_float32_3::
get_data3f(const unsigned char *pointer) {
  const PN_float32 *pi = (const PN_float32 *)pointer;
  _v3.set(pi[0], pi[1], pi[2]);
  return _v3;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_point_float32_2::get_data2f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase2f &qpGeomVertexReader::Reader_point_float32_2::
get_data2f(const unsigned char *pointer) {
  const PN_float32 *pi = (const PN_float32 *)pointer;
  _v2.set(pi[0], pi[1]);
  return _v2;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_point_float32_3::get_data3f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase3f &qpGeomVertexReader::Reader_point_float32_3::
get_data3f(const unsigned char *pointer) {
  const PN_float32 *pi = (const PN_float32 *)pointer;
  _v3.set(pi[0], pi[1], pi[2]);
  return _v3;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_point_float32_4::get_data4f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase4f &qpGeomVertexReader::Reader_point_float32_4::
get_data4f(const unsigned char *pointer) {
  const PN_float32 *pi = (const PN_float32 *)pointer;
  _v4.set(pi[0], pi[1], pi[2], pi[3]);
  return _v4;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_nativefloat_3::get_data3f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase3f &qpGeomVertexReader::Reader_nativefloat_3::
get_data3f(const unsigned char *pointer) {
  return *(const LVecBase3f *)pointer;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_point_nativefloat_2::get_data2f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase2f &qpGeomVertexReader::Reader_point_nativefloat_2::
get_data2f(const unsigned char *pointer) {
  return *(const LVecBase2f *)pointer;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_point_nativefloat_3::get_data3f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase3f &qpGeomVertexReader::Reader_point_nativefloat_3::
get_data3f(const unsigned char *pointer) {
  return *(const LVecBase3f *)pointer;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_point_nativefloat_4::get_data4f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase4f &qpGeomVertexReader::Reader_point_nativefloat_4::
get_data4f(const unsigned char *pointer) {
  return *(const LVecBase4f *)pointer;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_argb_packed_8888::get_data4f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase4f &qpGeomVertexReader::Reader_argb_packed_8888::
get_data4f(const unsigned char *pointer) {
  PN_uint32 dword = *(const PN_uint32 *)pointer;
  _v4.set((float)qpGeomVertexData::unpack_8888_b(dword) / 255.0f,
          (float)qpGeomVertexData::unpack_8888_c(dword) / 255.0f,
          (float)qpGeomVertexData::unpack_8888_d(dword) / 255.0f,
          (float)qpGeomVertexData::unpack_8888_a(dword) / 255.0f);
  return _v4;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_rgba_uint8_4::get_data4f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase4f &qpGeomVertexReader::Reader_rgba_uint8_4::
get_data4f(const unsigned char *pointer) {
  _v4.set((float)pointer[0] / 255.0f,
          (float)pointer[1] / 255.0f,
          (float)pointer[2] / 255.0f,
          (float)pointer[3] / 255.0f);
  return _v4;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_rgba_float32_4::get_data4f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase4f &qpGeomVertexReader::Reader_rgba_float32_4::
get_data4f(const unsigned char *pointer) {
  const PN_float32 *pi = (const PN_float32 *)pointer;
  _v4.set(pi[0], pi[1], pi[2], pi[3]);
  return _v4;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_rgba_nativefloat_4::get_data4f
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
const LVecBase4f &qpGeomVertexReader::Reader_rgba_nativefloat_4::
get_data4f(const unsigned char *pointer) {
  return *(const LVecBase4f *)pointer;
}

////////////////////////////////////////////////////////////////////
//     Function: qpGeomVertexReader::Reader_uint16_1::get_data1i
//       Access: Public, Virtual
//  Description: 
////////////////////////////////////////////////////////////////////
int qpGeomVertexReader::Reader_uint16_1::
get_data1i(const unsigned char *pointer) {
  return *(const PN_uint16 *)pointer;
}
