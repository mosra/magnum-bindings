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

#include <pybind11/pybind11.h>
#include <Magnum/GL/Attribute.h>

#include "magnum/bootstrap.h"

namespace magnum { namespace {

void gl(py::module& m) {
    /* (Dynamic) attribute */
    py::class_<GL::DynamicAttribute> attribute{m, "Attribute", "Vertex attribute location and type"};

    py::enum_<GL::DynamicAttribute::Kind>{attribute, "Kind", "Attribute kind"}
        .value("GENERIC", GL::DynamicAttribute::Kind::Generic)
        .value("GENERIC_NORMALIZED", GL::DynamicAttribute::Kind::GenericNormalized)
        #ifndef MAGNUM_TARGET_GLES2
        .value("INTEGRAL", GL::DynamicAttribute::Kind::Integral)
        #ifndef MAGNUM_TARGET_GLES
        .value("LONG", GL::DynamicAttribute::Kind::Long)
        #endif
        #endif
        ;

    py::enum_<GL::DynamicAttribute::Components>{attribute, "Components", "Component count"}
        .value("ONE", GL::DynamicAttribute::Components::One)
        .value("TWO", GL::DynamicAttribute::Components::Two)
        .value("THREE", GL::DynamicAttribute::Components::Three)
        .value("FOUR", GL::DynamicAttribute::Components::Four)
        #ifndef MAGNUM_TARGET_GLES
        .value("BGRA", GL::DynamicAttribute::Components::BGRA)
        #endif
        ;

    py::enum_<GL::DynamicAttribute::DataType>{attribute, "DataType", "Data type"}
        .value("UNSIGNED_BYTE", GL::DynamicAttribute::DataType::UnsignedByte)
        .value("BYTE", GL::DynamicAttribute::DataType::Byte)
        .value("UNSIGNED_SHORT", GL::DynamicAttribute::DataType::UnsignedShort)
        .value("SHORT", GL::DynamicAttribute::DataType::Short)
        .value("UNSIGNED_INT", GL::DynamicAttribute::DataType::UnsignedInt)
        .value("INT", GL::DynamicAttribute::DataType::Int)
        #if !(defined(MAGNUM_TARGET_WEBGL) && defined(MAGNUM_TARGET_GLES2))
        .value("HALF_FLOAT", GL::DynamicAttribute::DataType::HalfFloat)
        #endif
        .value("FLOAT", GL::DynamicAttribute::DataType::Float)
        #ifndef MAGNUM_TARGET_GLES
        .value("DOUBLE", GL::DynamicAttribute::DataType::Double)
        .value("UNSIGNED_INT_10F_11F_11F_REV", GL::DynamicAttribute::DataType::UnsignedInt10f11f11fRev)
        #endif
        #ifndef MAGNUM_TARGET_GLES2
        .value("UNSIGNED_INT_2_10_10_10_REV", GL::DynamicAttribute::DataType::UnsignedInt2101010Rev)
        .value("INT_2_10_10_10_REV", GL::DynamicAttribute::DataType::Int2101010Rev)
        #endif
        ;

    attribute
        .def(py::init<GL::DynamicAttribute::Kind, UnsignedInt, GL::DynamicAttribute::Components, GL::DynamicAttribute::DataType>(), "Constructor", py::arg("kind"), py::arg("location"), py::arg("components"), py::arg("data_type"))
        .def_property_readonly("kind", &GL::DynamicAttribute::kind,
            "Attribute kind")
        .def_property_readonly("location", &GL::DynamicAttribute::location,
            "Attribute location")
        .def_property_readonly("components", &GL::DynamicAttribute::components,
            "Component count")
        .def_property_readonly("data_type", &GL::DynamicAttribute::dataType,
            "Type of passed data");
}

}}

PYBIND11_MODULE(gl, m) {
    m.doc() = "OpenGL wrapping layer";

    magnum::gl(m);
}
