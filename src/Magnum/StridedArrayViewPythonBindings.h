#ifndef Magnum_StridedArrayViewPythonBindings_h
#define Magnum_StridedArrayViewPythonBindings_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <Corrade/Containers/StridedArrayViewPythonBindings.h>
#include <Magnum/Magnum.h>

namespace Corrade { namespace Containers { namespace Implementation {

/* This expands Containers::Implementation::pythonFormatString() with Magnum
   types. Using e.g. "3f" instead of "fff" as that makes Numpy understand even
   the slices as arrays in a concrete type (or it at least repr()s them as
   such), with "fff" it gives back an untyped tuple. */

#define _c(type, string)                                                    \
    template<> constexpr const char* pythonFormatString<Magnum::type>() { return string; }
_c(Vector2,     "2f")
_c(Vector2d,    "2d")
_c(Vector2ub,   "2B")
_c(Vector2b,    "2b")
_c(Vector2us,   "2H")
_c(Vector2s,    "2h")
_c(Vector2ui,   "2I")
_c(Vector2i,    "2i")
_c(Vector3,     "3f")
_c(Vector3d,    "3d")
_c(Vector3ub,   "3B")
_c(Vector3b,    "3b")
_c(Vector3us,   "3H")
_c(Vector3s,    "3h")
_c(Vector3ui,   "3I")
_c(Vector3i,    "3i")
_c(Vector4,     "4f")
_c(Vector4d,    "4d")
_c(Vector4ub,   "4B")
_c(Vector4b,    "4b")
_c(Vector4us,   "4H")
_c(Vector4s,    "4h")
_c(Vector4ui,   "4I")
_c(Vector4i,    "4i")
#undef _c

}

}}

#endif
