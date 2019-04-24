#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
#             Vladimír Vondruš <mosra@centrum.cz>
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#

"""Root Magnum module"""

from ._magnum import *

__all__ = [
    'Deg', 'Rad',

    'BoolVector2', 'BoolVector3', 'BoolVector4',
    'Vector2', 'Vector3', 'Vector4',
    'Vector2d', 'Vector3d', 'Vector4d',
    'Vector2i', 'Vector3i', 'Vector4i',
    'Vector2ui', 'Vector3ui', 'Vector4ui',

    'Matrix2x2', 'Matrix2x3', 'Matrix2x4',
    'Matrix3x2', 'Matrix3x3', 'Matrix3x4',
    'Matrix4x2', 'Matrix4x3', 'Matrix4x4',
    'Matrix2x2d', 'Matrix2x3d', 'Matrix2x4d',
    'Matrix3x2d', 'Matrix3x3d', 'Matrix3x4d',
    'Matrix4x2d', 'Matrix4x3d', 'Matrix4x4d',
    'Matrix3', 'Matrix4', 'Matrix3d', 'Matrix4d',

    'Quaternion', 'Quaterniond'
]
