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
#include <Corrade/Containers/ArrayView.h>
#include <Magnum/GL/Attribute.h>
#include <Magnum/GL/Buffer.h>

#include "corrade/PyArrayView.h"
#include "magnum/bootstrap.h"

namespace magnum { namespace {

void gl(py::module& m) {
    py::module::import("corrade.containers");

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

    /* Buffer */
    py::enum_<GL::BufferUsage>{m, "BufferUsage", "Buffer usage"}
        .value("STREAM_DRAW", GL::BufferUsage::StreamDraw)
        #ifndef MAGNUM_TARGET_GLES2
        .value("STREAM_READ", GL::BufferUsage::StreamRead)
        .value("STREAM_COPY", GL::BufferUsage::StreamCopy)
        #endif
        .value("STATIC_DRAW", GL::BufferUsage::StaticDraw)
        #ifndef MAGNUM_TARGET_GLES2
        .value("STATIC_READ", GL::BufferUsage::StaticRead)
        .value("STATIC_COPY", GL::BufferUsage::StaticCopy)
        #endif
        .value("DYNAMIC_DRAW", GL::BufferUsage::DynamicDraw)
        #ifndef MAGNUM_TARGET_GLES2
        .value("DYNAMIC_READ", GL::BufferUsage::DynamicRead)
        .value("DYNAMIC_COPY", GL::BufferUsage::DynamicCopy)
        #endif
        ;

    py::class_<GL::Buffer> buffer{m, "Buffer", "Buffer"};

    py::enum_<GL::Buffer::TargetHint>{buffer, "TargetHint", "Buffer target"}
        .value("ARRAY", GL::Buffer::TargetHint::Array)
        #ifndef MAGNUM_TARGET_GLES2
        #ifndef MAGNUM_TARGET_WEBGL
        .value("ATOMIC_COUNTER", GL::Buffer::TargetHint::AtomicCounter)
        #endif
        .value("COPY_READ", GL::Buffer::TargetHint::CopyRead)
        .value("COPY_WRITE", GL::Buffer::TargetHint::CopyWrite)
        #ifndef MAGNUM_TARGET_WEBGL
        .value("DISPATCH_INDIRECT", GL::Buffer::TargetHint::DispatchIndirect)
        .value("DRAW_INDIRECT", GL::Buffer::TargetHint::DrawIndirect)
        #endif
        #endif
        .value("ELEMENT_ARRAY", GL::Buffer::TargetHint::ElementArray)
        #ifndef MAGNUM_TARGET_GLES2
        .value("PIXEL_PACK", GL::Buffer::TargetHint::PixelPack)
        .value("PIXEL_UNPACK", GL::Buffer::TargetHint::PixelUnpack)
        #ifndef MAGNUM_TARGET_WEBGL
        .value("SHADER_STORAGE", GL::Buffer::TargetHint::ShaderStorage)
        #endif
        #endif
        #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
        .value("TEXTURE", GL::Buffer::TargetHint::Texture)
        #endif
        #ifndef MAGNUM_TARGET_GLES2
        .value("TRANSFORM_FEEDBACK", GL::Buffer::TargetHint::TransformFeedback)
        .value("UNIFORM", GL::Buffer::TargetHint::Uniform)
        #endif
        ;

    buffer
        .def(py::init<GL::Buffer::TargetHint>(), "Constructor", py::arg("target_hint") = GL::Buffer::TargetHint::Array)
        .def_property_readonly("id", &GL::Buffer::id, "OpenGL buffer ID")
        .def_property("target_hint", &GL::Buffer::targetHint, &GL::Buffer::setTargetHint, "Target hint")
        /* Using lambdas to avoid method chaining getting into signatures */
        .def("set_data", [](GL::Buffer& self, const corrade::PyArrayView<const char>& data, GL::BufferUsage usage) {
            self.setData(data, usage);
        }, "Set buffer data", py::arg("data"), py::arg("usage") = GL::BufferUsage::StaticDraw)
        /** @todo more */;
}

}}

PYBIND11_MODULE(gl, m) {
    m.doc() = "OpenGL wrapping layer";

    magnum::gl(m);
}
