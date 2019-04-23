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

#include "magnum/math.vector.h"

namespace magnum {

namespace {

template<class T> void vectorIntegral(py::class_<T>& c) {
    c
        .def(py::self %= typename T::Type{}, "Do modulo of an integral vector and assign")
        .def(py::self % typename T::Type{}, "Modulo of an integral vector")
        .def(py::self %= py::self, "Do module of two integral vectors and assign")
        .def(py::self % py::self, "Modulo of two integral vectors")
        .def(~py::self, "Bitwise NOT of an integral vector")
        .def(py::self &= py::self, "Do bitwise AND of two integral vectors and assign")
        .def(py::self & py::self, "Bitwise AND of two integral vectors")
        .def(py::self |= py::self, "Do bitwise OR of two integral vectors and assign")
        .def(py::self | py::self, "Bitwise OR of two integral vectors")
        .def(py::self ^= py::self, "Do bitwise XOR of two integral vectors and assign")
        .def(py::self ^ py::self, "Bitwise XOR of two integral vectors")
        .def(py::self <<= typename T::Type{}, "Do bitwise left shift of an integral vector and assign")
        .def(py::self << typename T::Type{}, "Bitwise left shift of an integral vector")
        .def(py::self >>= typename T::Type{}, "Do bitwise right shift of an integral vector and assign")
        .def(py::self >> typename T::Type{}, "Bitwise right shift of an integral vector")
        .def(py::self *= Float{}, "Multiply an integral vector with a floating-point number and assign")
        .def(py::self * Float{}, "Multiply an integral vector with a floating-point number")
        .def(Float{} * py::self, "Multiply a floating-point number with an integral vector")
        .def(py::self /= Float{}, "Divide an integral vector with a floating-point number and assign")
        .def(py::self / Float{}, "Divide an integral vector with a floating-point number");
}

template<class T> void vectorsIntegral(py::module& m, py::class_<Math::Vector2<T>>& vector2_, py::class_<Math::Vector3<T>>& vector3_, py::class_<Math::Vector4<T>>& vector4_) {
    vector<Math::Vector2<T>>(m, vector2_);
    vectorIntegral<Math::Vector2<T>>(vector2_);
    vector2<T>(vector2_);

    vector<Math::Vector3<T>>(m, vector3_);
    vectorIntegral<Math::Vector3<T>>(vector3_);
    vector3<T>(vector3_);

    vector<Math::Vector4<T>>(m, vector4_);
    vectorIntegral<Math::Vector4<T>>(vector4_);
    vector4<T>(vector4_);
}

}

void mathVectorIntegral(py::module& root, py::module& m) {
    py::class_<Vector2i> vector2i{root, "Vector2i", "Two-component signed integer vector"};
    py::class_<Vector3i> vector3i{root, "Vector3i", "Threee-component signed integral vector"};
    py::class_<Vector4i> vector4i{root, "Vector4i", "Four-component signed integral vector"};
    py::class_<Vector2ui> vector2ui{root, "Vector2ui", "Two-component unsigned integral vector"};
    py::class_<Vector3ui> vector3ui{root, "Vector3ui", "Threee-component unsigned integral vector"};
    py::class_<Vector4ui> vector4ui{root, "Vector4ui", "Four-component unsigned integral vector"};
    vectorsIntegral<Int>(m, vector2i, vector3i, vector4i);
    vectorsIntegral<UnsignedInt>(m, vector2ui, vector3ui, vector4ui);
}

}
