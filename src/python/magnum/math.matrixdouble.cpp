/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
              Vladimír Vondruš <mosra@centrum.cz>

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

#include "magnum/math.matrix.h"

namespace magnum {

void mathMatrixDouble(py::module& root) {
    py::class_<Matrix2x2d> matrix2x2d{root, "Matrix2x2d", "2x2 double matrix"};
    py::class_<Matrix2x3d> matrix2x3d{root, "Matrix2x3d", "2x3 double matrix"};
    py::class_<Matrix2x4d> matrix2x4d{root, "Matrix2x4d", "2x4 double matrix"};

    py::class_<Matrix3x2d> matrix3x2d{root, "Matrix3x2d", "3x2 double matrix"};
    py::class_<Matrix3x3d> matrix3x3d{root, "Matrix3x3d", "3x3 double matrix"};
    py::class_<Matrix3x4d> matrix3x4d{root, "Matrix3x4d", "3x4 double matrix"};

    py::class_<Matrix4x2d> matrix4x2d{root, "Matrix4x2d", "4x2 double matrix"};
    py::class_<Matrix4x3d> matrix4x3d{root, "Matrix4x3d", "4x3 double matrix"};
    py::class_<Matrix4x4d> matrix4x4d{root, "Matrix4x4d", "4x4 double matrix"};

    matrices<Double>(
        matrix2x2d, matrix2x3d, matrix2x4d,
        matrix3x2d, matrix3x3d, matrix3x4d,
        matrix4x2d, matrix4x3d, matrix4x4d);
}

}
