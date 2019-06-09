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
#include <Corrade/Utility/FormatStl.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Attribute.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>

#include "corrade/PyArrayView.h"
#include "corrade/EnumOperators.h"
#include "magnum/bootstrap.h"
#include "magnum/NonDestructible.h"
#include "magnum/PyMesh.h"

namespace magnum {

void gl(py::module& m) {
    m.doc() = "OpenGL wrapping layer";

    py::module::import("corrade.containers");

    /* Abstract shader program */
    NonDestructible<GL::AbstractShaderProgram>{m,
        "AbstractShaderProgram", "Base for shader program implementations"};
    /** @todo more */

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

    /* Framebuffers */
    py::enum_<GL::FramebufferClear> framebufferClear{m, "FramebufferClear", "Mask for framebuffer clearing"};
    framebufferClear
        .value("COLOR", GL::FramebufferClear::Color)
        .value("DEPTH", GL::FramebufferClear::Depth)
        .value("STENCIL", GL::FramebufferClear::Stencil);
    corrade::enumOperators(framebufferClear);

    NonDestructible<GL::AbstractFramebuffer> abstractFramebuffer{m,
        "AbstractFramebuffer", "Base for default and named framebuffers"};

    abstractFramebuffer
        /* Using lambdas to avoid method chaining getting into signatures */
        .def("clear", [](GL::AbstractFramebuffer& self, GL::FramebufferClear mask) {
            self.clear(mask);
        });

    NonDestructibleBase<GL::DefaultFramebuffer, GL::AbstractFramebuffer> defaultFramebuffer{m,
        "DefaultFramebuffer", "Default framebuffer"};

    /* An equivalent to this would be
        m.attr("default_framebuffer") = &GL::defaultFramebuffer;
       (have to use a & to make it choose return_value_policy::reference
       instead of return_value_policy::copy), but this is more explicit ---
       returning a raw pointer from functions makes pybind wrap it in an
       unique_ptr, which would cause double-free / memory corruption later */
    py::setattr(m, "default_framebuffer", py::cast(GL::defaultFramebuffer, py::return_value_policy::reference));

    /* Mesh */
    py::enum_<GL::MeshPrimitive>{m, "MeshPrimitive", "Mesh primitive type"}
        .value("POINTS", GL::MeshPrimitive::Points)
        .value("LINES", GL::MeshPrimitive::Lines)
        .value("LINE_LOOP", GL::MeshPrimitive::LineLoop)
        .value("LINE_STRIP", GL::MeshPrimitive::LineStrip)
        #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
        .value("LINES_ADJACENCY", GL::MeshPrimitive::LinesAdjacency)
        .value("LINE_STRIP_ADJACENCY", GL::MeshPrimitive::LineStripAdjacency)
        #endif
        .value("TRIANGLES", GL::MeshPrimitive::Triangles)
        .value("TRIANGLE_STRIP", GL::MeshPrimitive::TriangleStrip)
        .value("TRIANGLE_FAN", GL::MeshPrimitive::TriangleFan)
        #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
        .value("TRIANGLES_ADJACENCY", GL::MeshPrimitive::TrianglesAdjacency)
        .value("TRIANGLE_STRIP_ADJACENCY", GL::MeshPrimitive::TriangleStripAdjacency)
        .value("PATCHES", GL::MeshPrimitive::Patches)
        #endif
        ;

    py::class_<PyMesh>{m, "Mesh", "Mesh"}
        .def(py::init<GL::MeshPrimitive>(), "Constructor", py::arg("primitive") = GL::MeshPrimitive::Triangles)
        .def(py::init<MeshPrimitive>(), "Constructor")
        .def_property_readonly("id", &GL::Mesh::id, "OpenGL vertex array ID")
        .def_property("primitive", &GL::Mesh::primitive,
            [](PyMesh& self, py::object primitive) {
                if(py::isinstance<MeshPrimitive>(primitive))
                    self.setPrimitive(py::cast<MeshPrimitive>(primitive));
                else if(py::isinstance<GL::MeshPrimitive>(primitive))
                    self.setPrimitive(py::cast<GL::MeshPrimitive>(primitive));
                else throw py::type_error{Utility::formatString("expected MeshPrimitive or gl.MeshPrimitive, got {}", std::string(py::str{primitive.get_type()}))};
            }, "Primitive type")
        /* Have to use a lambda because it returns GL::Mesh which is not
           tracked (unlike PyMesh) */
        .def_property("count", &GL::Mesh::count, [](PyMesh& self, UnsignedInt count) {
            self.setCount(count);
        }, "Vertex/index count")

        /* Using lambdas to avoid method chaining getting into signatures */

        .def("add_vertex_buffer", [](PyMesh& self, GL::Buffer& buffer, GLintptr offset, GLsizei stride, const GL::DynamicAttribute& attribute) {
            self.addVertexBuffer(buffer, offset, stride, attribute);

            /* Keep a reference to the buffer to avoid it being deleted before
               the mesh */
            self.buffers.emplace_back(py::detail::get_object_handle(&buffer, py::detail::get_type_info(typeid(GL::Buffer))), true);
        }, "Add vertex buffer", py::arg("buffer"), py::arg("offset"), py::arg("stride"), py::arg("attribute"))
        .def("draw", [](PyMesh& self, GL::AbstractShaderProgram& shader) {
            self.draw(shader);
        }, "Draw the mesh")
        /** @todo more */
        ;

    /* Renderer */
    {
        py::class_<GL::Renderer> renderer{m, "Renderer", "Global renderer configuration"};

        py::enum_<GL::Renderer::Feature>{renderer, "Feature", "Feature"}
            #ifndef MAGNUM_TARGET_WEBGL
            .value("BLEND_ADVANCED_COHERENT", GL::Renderer::Feature::BlendAdvancedCoherent)
            #endif
            .value("BLENDING", GL::Renderer::Feature::Blending)
            #ifndef MAGNUM_TARGET_WEBGL
            .value("DEBUG_OUTPUT", GL::Renderer::Feature::DebugOutput)
            .value("DEBUG_OUTPUT_SYNCHRONOUS", GL::Renderer::Feature::DebugOutputSynchronous)
            #endif
            #ifndef MAGNUM_TARGET_GLES
            .value("DEPTH_CLAMP", GL::Renderer::Feature::DepthClamp)
            #endif
            .value("DEPTH_TEST", GL::Renderer::Feature::DepthTest)
            .value("DITHERING", GL::Renderer::Feature::Dithering)
            .value("FACE_CULLING", GL::Renderer::Feature::FaceCulling)
            #ifndef MAGNUM_TARGET_WEBGL
            .value("FRAMEBUFFER_SRGB", GL::Renderer::Feature::FramebufferSrgb)
            #endif
            #ifndef MAGNUM_TARGET_GLES
            .value("LOGIC_OPERATION", GL::Renderer::Feature::LogicOperation)
            .value("MULTISAMPLING", GL::Renderer::Feature::Multisampling)
            #endif
            .value("POLYGON_OFFSET_FILL", GL::Renderer::Feature::PolygonOffsetFill)
            #ifndef MAGNUM_TARGET_WEBGL
            .value("POLYGON_OFFSET_LINE", GL::Renderer::Feature::PolygonOffsetLine)
            .value("POLYGON_OFFSET_POINT", GL::Renderer::Feature::PolygonOffsetPoint)
            #endif
            #ifndef MAGNUM_TARGET_GLES
            .value("PROGRAM_POINT_SIZE", GL::Renderer::Feature::ProgramPointSize)
            #endif
            #ifndef MAGNUM_TARGET_GLES2
            .value("RASTERIZER_DISCARD", GL::Renderer::Feature::RasterizerDiscard)
            #endif
            #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
            .value("SAMPLE_SHADING", GL::Renderer::Feature::SampleShading)
            #endif
            #ifndef MAGNUM_TARGET_GLES
            .value("SEAMLESS_CUBE_MAP_TEXTURE", GL::Renderer::Feature::SeamlessCubeMapTexture)
            #endif
            .value("SCISSOR_TEST", GL::Renderer::Feature::ScissorTest)
            .value("STENCIL_TEST", GL::Renderer::Feature::StencilTest);

        renderer
            .def_static("enable", GL::Renderer::enable, "Enable a feature")
            .def_static("disable", GL::Renderer::disable, "Disable a feature")
            .def_static("set_feature", GL::Renderer::setFeature, "Enable or disable a feature");
    }
}

}

#ifndef MAGNUM_BUILD_STATIC
PYBIND11_MODULE(gl, m) {
    magnum::gl(m);
}
#endif
