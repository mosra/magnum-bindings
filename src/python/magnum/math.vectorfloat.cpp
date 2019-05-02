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

template<class T> void vectorFloat(py::module& m, py::class_<T>& c) {
    m
        .def("angle", [](const T& a, const T& b) { return Radd(Math::angle(a, b)); },
            "Angle between normalized vectors", py::arg("normalized_a"), py::arg("normalized_b"));

    c
        .def("is_normalized", &T::isNormalized, "Whether the vector is normalized")
        .def("length", &T::length, "Vector length")

        /* Cast needed because these are enabled only for floats */
        .def("length_inverted", static_cast<typename T::Type(T::*)() const>(&T::lengthInverted), "Inverse vector length")
        .def("normalized", static_cast<T(T::*)() const>(&T::normalized),
             "Normalized vector (of unit length)")
        .def("resized", static_cast<T(T::*)(typename T::Type) const>(&T::resized),
             "Resized vector")
        .def("projected", [](const T& self, const T& line) {
            return self.projected(line);
        }, "Vector projected onto a line")
        .def("projected_onto_normalized", [](const T& self, const T& line) {
            return self.projectedOntoNormalized(line);
        }, "Vector projected onto a normalized line");
}

template<class T> void vectorsFloat(py::module& m, py::class_<Math::Vector2<T>>& vector2_, py::class_<Math::Vector3<T>>& vector3_, py::class_<Math::Vector4<T>>& vector4_) {
    vector2_
        .def("aspect_ratio", static_cast<T(Math::Vector2<T>::*)() const>(&Math::Vector2<T>::aspectRatio),
            "Aspect ratio")
        .def("cross", static_cast<T(*)(const Math::Vector2<T>&, const Math::Vector2<T>&)>(Math::cross),
            "2D cross product");
    vector<Math::Vector2<T>>(m, vector2_);
    vectorFloat<Math::Vector2<T>>(m, vector2_);
    vector2<T>(vector2_);

    vector3_
        .def("cross", static_cast<Math::Vector3<T>(*)(const Math::Vector3<T>&, const Math::Vector3<T>&)>(Math::cross),
            "Cross product");
    vector<Math::Vector3<T>>(m, vector3_);
    vectorFloat<Math::Vector3<T>>(m, vector3_);
    vector3<T>(vector3_);

    vector<Math::Vector4<T>>(m, vector4_);
    vectorFloat<Math::Vector4<T>>(m, vector4_);
    vector4<T>(vector4_);
}

}

void mathVectorFloat(py::module& root, py::module& m) {
    py::class_<Vector2> vector2{root, "Vector2", "Two-component float vector"};
    py::class_<Vector3> vector3{root, "Vector3", "Threee-component float vector"};
    py::class_<Vector4> vector4{root, "Vector4", "Four-component float vector"};
    py::class_<Vector2d> vector2d{root, "Vector2d", "Two-component double vector"};
    py::class_<Vector3d> vector3d{root, "Vector3d", "Threee-component double vector"};
    py::class_<Vector4d> vector4d{root, "Vector4d", "Four-component double vector"};
    vectorsFloat<Float>(m, vector2, vector3, vector4);
    vectorsFloat<Double>(m, vector2d, vector3d, vector4d);
}

}