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
#include <pybind11/stl.h> /* for Mesh.buffers */
#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Utility/FormatStl.h>
#include <Magnum/ImageView.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Attribute.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/RenderbufferFormat.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>

#include "Corrade/Python.h"
#include "Magnum/Python.h"
#include "Magnum/GL/Python.h"

#include "corrade/EnumOperators.h"
#include "magnum/bootstrap.h"

namespace magnum { namespace {

/* Otherwise pybind yells that `generic_type: type "Framebuffer" has a
   non-default holder type while its base "Magnum::GL::AbstractFramebuffer"
   does not` -- we're using PyFramebufferHolder for it */
template<class T> struct NonDefaultFramebufferHolder: std::unique_ptr<T, PyNonDestructibleBaseDeleter<T, std::is_destructible<T>::value>> {
    explicit NonDefaultFramebufferHolder(T* object): std::unique_ptr<T, PyNonDestructibleBaseDeleter<T, std::is_destructible<T>::value>>{object} {}
};

}}

PYBIND11_DECLARE_HOLDER_TYPE(T, magnum::NonDefaultFramebufferHolder<T>)

namespace magnum {

namespace {

struct PublicizedAbstractShaderProgram: GL::AbstractShaderProgram {
    #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
    using GL::AbstractShaderProgram::setRetrievableBinary;
    #endif
    #ifndef MAGNUM_TARGET_WEBGL
    using GL::AbstractShaderProgram::setSeparable;
    #endif
    using GL::AbstractShaderProgram::attachShader;
    using GL::AbstractShaderProgram::bindAttributeLocation;
    #ifndef MAGNUM_TARGET_GLES
    using GL::AbstractShaderProgram::bindFragmentDataLocation;
    using GL::AbstractShaderProgram::bindFragmentDataLocationIndexed;
    #endif
    #ifndef MAGNUM_TARGET_GLES2
    using GL::AbstractShaderProgram::setTransformFeedbackOutputs;
    #endif
    using GL::AbstractShaderProgram::link;
    using GL::AbstractShaderProgram::uniformLocation;
    #ifndef MAGNUM_TARGET_GLES2
    using GL::AbstractShaderProgram::uniformBlockIndex;
    #endif
    using GL::AbstractShaderProgram::setUniform;
    #ifndef MAGNUM_TARGET_GLES2
    using GL::AbstractShaderProgram::setUniformBlockBinding;
    #endif
};

template<class T> void setUniform(GL::AbstractShaderProgram& self, Int location, T value) {
    static_cast<PublicizedAbstractShaderProgram&>(self).setUniform(location, value);
}

}

void gl(py::module& m) {
    /*
        Missing APIs:

        GL object labels
        limit queries
    */

    m.doc() = "OpenGL wrapping layer";

    /* Version and related utilities */
    py::enum_<GL::Version>{m, "Version", "OpenGL version"}
        .value("NONE", GL::Version::None)
        #ifndef MAGNUM_TARGET_GLES
        .value("GL210", GL::Version::GL210)
        .value("GL300", GL::Version::GL300)
        .value("GL310", GL::Version::GL310)
        .value("GL320", GL::Version::GL320)
        .value("GL330", GL::Version::GL330)
        .value("GL400", GL::Version::GL400)
        .value("GL410", GL::Version::GL410)
        .value("GL420", GL::Version::GL420)
        .value("GL430", GL::Version::GL430)
        .value("GL440", GL::Version::GL440)
        .value("GL450", GL::Version::GL450)
        .value("GL460", GL::Version::GL460)
        #endif
        .value("GLES200", GL::Version::GLES200)
        .value("GLES300", GL::Version::GLES300)
        #ifndef MAGNUM_TARGET_WEBGL
        .value("GLES310", GL::Version::GLES310)
        .value("GLES320", GL::Version::GLES320)
        #endif
        ;
    m
        .def("version", static_cast<GL::Version(*)(Int, Int)>(GL::version), "Enum value from major and minor version number", py::arg("major"), py::arg("minor"))
        .def("version", static_cast<std::pair<Int, Int>(*)(GL::Version)>(GL::version), "Major and minor version number from enum value", py::arg("version"))
        .def("is_version_es", GL::isVersionES, "Whether given version is OpenGL ES or WebGL");

    /* Shader (used by AbstractShaderProgram, so needs to be before) */
    {
        py::class_<GL::Shader> shader{m, "Shader", "Shader"};

        py::enum_<GL::Shader::Type>{shader, "Type", "Shader type"}
            .value("VERTEX", GL::Shader::Type::Vertex)
            #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
            .value("TESSELLATION_CONTROL", GL::Shader::Type::TessellationControl)
            .value("TESSELLATION_EVALUATION", GL::Shader::Type::TessellationEvaluation)
            .value("GEOMETRY", GL::Shader::Type::Geometry)
            .value("COMPUTE", GL::Shader::Type::Compute)
            #endif
            .value("FRAGMENT", GL::Shader::Type::Fragment);

        shader
            /** @todo limit queries */
            /* Constructors */
            .def(py::init<GL::Version, GL::Shader::Type>(), "Constructor", py::arg("version"), py::arg("type"))

            /* Interface */
            .def_property_readonly("id", &GL::Shader::id, "OpenGL shader ID")
            .def_property_readonly("type", &GL::Shader::type, "Shader type")
            .def_property_readonly("sources", &GL::Shader::sources, "Shader sources")
            /* Using lambdas to avoid method chaining leaking to Python */
            .def("add_source", [](GL::Shader& self, std::string source) {
                self.addSource(std::move(source));
            }, "Add shader source")
            .def("add_file", [](GL::Shader& self, const std::string& filename) {
                self.addFile(filename);
            }, "Add shader source file")
            .def("compile", static_cast<bool(GL::Shader::*)()>(&GL::Shader::compile), "Compile shader");
    }

    /* Abstract shader program */
    {
        /* The original class has protected functions and a pure virtual
           destructor to force people to subclass it. */
        struct PyAbstractShaderProgram: GL::AbstractShaderProgram {
            using GL::AbstractShaderProgram::AbstractShaderProgram;
        };
        py::class_<GL::AbstractShaderProgram, PyAbstractShaderProgram> abstractShaderProgram{m, "AbstractShaderProgram", "Base for shader program implementations"};

        #ifndef MAGNUM_TARGET_GLES2
        py::enum_<GL::AbstractShaderProgram::TransformFeedbackBufferMode>{abstractShaderProgram, "TransformFeedbackBufferMode", "Buffer mode for transform feedback"}
            .value("INTERLEAVED_ATTRIBUTES", GL::AbstractShaderProgram::TransformFeedbackBufferMode::InterleavedAttributes)
            .value("SEPARATE_ATTRIBUTES", GL::AbstractShaderProgram::TransformFeedbackBufferMode::SeparateAttributes);
        #endif

        abstractShaderProgram
            /** @todo limit queries */
            .def(py::init(), "Constructor")

            /* Public interface */
            .def_property_readonly("id", &GL::AbstractShaderProgram::id, "OpenGL program ID")
            .def("validate", &GL::AbstractShaderProgram::validate, "Validate program")
            #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
            .def("dispatch_compute", &GL::AbstractShaderProgram::dispatchCompute, "Dispatch compute")
            #endif

            /* Protected interface */
            #if !defined(MAGNUM_TARGET_GLES2) && !defined(MAGNUM_TARGET_WEBGL)
            .def_property("retrievable_binary", nullptr, &PublicizedAbstractShaderProgram::setRetrievableBinary, "Allow retrieving program binary")
            #endif
            #ifndef MAGNUM_TARGET_WEBGL
            .def_property("separable", nullptr, &PublicizedAbstractShaderProgram::setSeparable, "Allow the program to be bound to individual pipeline stages")
            #endif
            .def("attach_shader", &PublicizedAbstractShaderProgram::attachShader, "Attach a shader")
            /** @todo list-taking shader attach function */
            /* Somehow the overload static_casts don't work and it complains it
               can't bind a protected function, have to use lambdas */
            .def("bind_attribute_location", [](GL::AbstractShaderProgram& self, UnsignedInt location, const std::string& name) {
                static_cast<PublicizedAbstractShaderProgram&>(self).bindAttributeLocation(location, name);
            }, "Bind an attribute to given location", py::arg("location"), py::arg("name"))
            #ifndef MAGNUM_TARGET_GLES
            .def("bind_fragment_data_location_indexed", [](GL::AbstractShaderProgram& self, UnsignedInt location, UnsignedInt index, const std::string& name) {
                static_cast<PublicizedAbstractShaderProgram&>(self).bindFragmentDataLocationIndexed(location, index, name);
            }, "Bind fragment data to given location and first color input index", py::arg("location"), py::arg("index"), py::arg("name"))
            .def("bind_fragment_data_location", [](GL::AbstractShaderProgram& self, UnsignedInt location, const std::string& name) {
                static_cast<PublicizedAbstractShaderProgram&>(self).bindFragmentDataLocation(location, name);
            }, "Bind fragment data to given location and first color input index", py::arg("location"), py::arg("name"))
            #endif
            /** @todo setTransformFeedbackOutputs, list-taking link functions */
            /* Somehow the overload static_casts don't work and it complains it
               can't bind a protected function, have to use lambdas */
            .def("link", [](GL::AbstractShaderProgram& self) {
                return static_cast<PublicizedAbstractShaderProgram&>(self).link();
            }, "Link the shader")
            .def("uniform_location", [](GL::AbstractShaderProgram& self, const std::string& name) {
                return static_cast<PublicizedAbstractShaderProgram&>(self).uniformLocation(name);
            }, "Get uniform location")
            #ifndef MAGNUM_TARGET_GLES2
            .def("uniform_block_index", [](GL::AbstractShaderProgram& self, const std::string& name) {
                return static_cast<PublicizedAbstractShaderProgram&>(self).uniformBlockIndex(name);
            }, "Get uniform block index")
            #endif
            .def("set_uniform", setUniform<Float>, "Set uniform value")
            .def("set_uniform", setUniform<Int>, "Set uniform value")
            #ifndef MAGNUM_TARGET_GLES2
            /** @todo How to distinguish *this*? Python has just an int. */
            .def("set_uniform", setUniform<UnsignedInt>, "Set uniform value")
            #endif
            /** @todo double scalar uniform, how to distinguish? if I add it, it will get a priority over floats */
            .def("set_uniform", setUniform<Vector2>, "Set uniform value")
            .def("set_uniform", setUniform<Vector3>, "Set uniform value")
            .def("set_uniform", setUniform<Vector4>, "Set uniform value")
            .def("set_uniform", setUniform<Vector2i>, "Set uniform value")
            .def("set_uniform", setUniform<Vector3i>, "Set uniform value")
            .def("set_uniform", setUniform<Vector4i>, "Set uniform value")
            #ifndef MAGNUM_TARGET_GLES2
            .def("set_uniform", setUniform<Vector2ui>, "Set uniform value")
            .def("set_uniform", setUniform<Vector3ui>, "Set uniform value")
            .def("set_uniform", setUniform<Vector4ui>, "Set uniform value")
            #endif
            #ifndef MAGNUM_TARGET_GLES
            .def("set_uniform", setUniform<Vector2d>, "Set uniform value")
            .def("set_uniform", setUniform<Vector3d>, "Set uniform value")
            .def("set_uniform", setUniform<Vector4d>, "Set uniform value")
            #endif
            .def("set_uniform", setUniform<Matrix2x2>, "Set uniform value")
            .def("set_uniform", setUniform<Matrix3x3>, "Set uniform value")
            .def("set_uniform", setUniform<Matrix4x4>, "Set uniform value")
            #ifndef MAGNUM_TARGET_GLES2
            .def("set_uniform", setUniform<Matrix2x3>, "Set uniform value")
            .def("set_uniform", setUniform<Matrix3x2>, "Set uniform value")
            .def("set_uniform", setUniform<Matrix2x4>, "Set uniform value")
            .def("set_uniform", setUniform<Matrix4x2>, "Set uniform value")
            .def("set_uniform", setUniform<Matrix3x4>, "Set uniform value")
            .def("set_uniform", setUniform<Matrix4x3>, "Set uniform value")
            #endif
            #ifndef MAGNUM_TARGET_GLES
            .def("set_uniform", setUniform<Matrix2x3d>, "Set uniform value")
            .def("set_uniform", setUniform<Matrix3x2d>, "Set uniform value")
            .def("set_uniform", setUniform<Matrix2x4d>, "Set uniform value")
            .def("set_uniform", setUniform<Matrix4x2d>, "Set uniform value")
            .def("set_uniform", setUniform<Matrix3x4d>, "Set uniform value")
            .def("set_uniform", setUniform<Matrix4x3d>, "Set uniform value")
            #endif
            #ifndef MAGNUM_TARGET_GLES2
            .def("set_uniform_block_binding", &PublicizedAbstractShaderProgram::setUniformBlockBinding, "Set uniform block binding")
            #endif
            ;
    }

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
        /** @todo limit queries */

        .def(py::init<GL::Buffer::TargetHint>(), "Constructor", py::arg("target_hint") = GL::Buffer::TargetHint::Array)
        .def_property_readonly("id", &GL::Buffer::id, "OpenGL buffer ID")
        .def_property("target_hint", &GL::Buffer::targetHint, &GL::Buffer::setTargetHint, "Target hint")
        /* Using lambdas to avoid method chaining getting into signatures */
        .def("set_data", [](GL::Buffer& self, const Containers::ArrayView<const char>& data, GL::BufferUsage usage) {
            self.setData(data, usage);
        }, "Set buffer data", py::arg("data"), py::arg("usage") = GL::BufferUsage::StaticDraw)
        /** @todo more */;

    /* Renderbuffer */
    py::enum_<GL::RenderbufferFormat>{m, "RenderbufferFormat", "Internal renderbuffer format"}
        #ifndef MAGNUM_TARGET_GLES
        .value("RED", GL::RenderbufferFormat::Red)
        #endif
        #if !(defined(MAGNUM_TARGET_WEBGL) && defined(MAGNUM_TARGET_GLES2))
        .value("R8", GL::RenderbufferFormat::R8)
        #endif
        #ifndef MAGNUM_TARGET_GLES
        .value("RG", GL::RenderbufferFormat::RG)
        #endif
        #if !(defined(MAGNUM_TARGET_WEBGL) && defined(MAGNUM_TARGET_GLES2))
        .value("RG8", GL::RenderbufferFormat::RG8)
        #endif
        #ifndef MAGNUM_TARGET_GLES
        .value("RGBA", GL::RenderbufferFormat::RGBA)
        #endif
        #if !(defined(MAGNUM_TARGET_WEBGL) && defined(MAGNUM_TARGET_GLES2))
        .value("RGBA8", GL::RenderbufferFormat::RGBA8)
        #endif
        #ifndef MAGNUM_TARGET_GLES
        .value("R16", GL::RenderbufferFormat::R16)
        .value("RG16", GL::RenderbufferFormat::RG16)
        .value("RGB16", GL::RenderbufferFormat::RGB16)
        .value("RGBA16", GL::RenderbufferFormat::RGBA16)
        .value("R8UI", GL::RenderbufferFormat::R8UI)
        .value("RG8UI", GL::RenderbufferFormat::RG8UI)
        .value("RGBA8UI", GL::RenderbufferFormat::RGBA8UI)
        .value("R8I", GL::RenderbufferFormat::R8I)
        .value("RG8I", GL::RenderbufferFormat::RG8I)
        .value("RGBA8I", GL::RenderbufferFormat::RGBA8I)
        .value("R16UI", GL::RenderbufferFormat::R16UI)
        .value("RG16UI", GL::RenderbufferFormat::RG16UI)
        .value("RGBA16UI", GL::RenderbufferFormat::RGBA16UI)
        .value("R16I", GL::RenderbufferFormat::R16I)
        .value("RG16I", GL::RenderbufferFormat::RG16I)
        .value("RGBA16I", GL::RenderbufferFormat::RGBA16I)
        .value("R32UI", GL::RenderbufferFormat::R32UI)
        .value("RG32UI", GL::RenderbufferFormat::RG32UI)
        .value("RGBA32UI", GL::RenderbufferFormat::RGBA32UI)
        .value("R32I", GL::RenderbufferFormat::R32I)
        .value("RG32I", GL::RenderbufferFormat::RG32I)
        .value("RGBA32I", GL::RenderbufferFormat::RGBA32I)
        .value("R16F", GL::RenderbufferFormat::R16F)
        .value("RG16F", GL::RenderbufferFormat::RG16F)
        .value("RGBA16F", GL::RenderbufferFormat::RGBA16F)
        .value("R32F", GL::RenderbufferFormat::R32F)
        .value("RG32F", GL::RenderbufferFormat::RG32F)
        .value("RGBA32F", GL::RenderbufferFormat::RGBA32F)
        #endif
        #ifndef MAGNUM_TARGET_GLES2
        .value("RGB10A2", GL::RenderbufferFormat::RGB10A2)
        .value("RGB10A2UI", GL::RenderbufferFormat::RGB10A2UI)
        #endif
        .value("RGB5A1", GL::RenderbufferFormat::RGB5A1)
        .value("RGBA4", GL::RenderbufferFormat::RGBA4)
        #ifndef MAGNUM_TARGET_GLES
        .value("R11FG11FB10F", GL::RenderbufferFormat::R11FG11FB10F)
        #endif
        .value("RGB565", GL::RenderbufferFormat::RGB565)
        .value("SRGB8_ALPHA8", GL::RenderbufferFormat::SRGB8Alpha8)
        #ifndef MAGNUM_TARGET_GLES
        .value("DEPTH_COMPONENT", GL::RenderbufferFormat::DepthComponent)
        #endif
        .value("DEPTH_COMPONENT16", GL::RenderbufferFormat::DepthComponent16)
        #if !(defined(MAGNUM_TARGET_WEBGL) && defined(MAGNUM_TARGET_GLES2))
        .value("DEPTH_COMPONENT24", GL::RenderbufferFormat::DepthComponent24)
        #endif
        #ifndef MAGNUM_TARGET_WEBGL
        .value("DEPTH_COMPONENT32", GL::RenderbufferFormat::DepthComponent32)
        #endif
        #ifndef MAGNUM_TARGET_GLES2
        .value("DEPTH_COMPONENT32F", GL::RenderbufferFormat::DepthComponent32F)
        #endif
        #ifndef MAGNUM_TARGET_GLES
        .value("STENCIL_INDEX", GL::RenderbufferFormat::StencilIndex)
        #endif
        #ifndef MAGNUM_TARGET_WEBGL
        .value("STENCIL_INDEX1", GL::RenderbufferFormat::StencilIndex1)
        .value("STENCIL_INDEX4", GL::RenderbufferFormat::StencilIndex4)
        #endif
        .value("STENCIL_INDEX8", GL::RenderbufferFormat::StencilIndex8)
        #ifndef MAGNUM_TARGET_GLES
        .value("STENCIL_INDEX16", GL::RenderbufferFormat::StencilIndex16)
        #endif
        #if !defined(MAGNUM_TARGET_GLES) || (defined(MAGNUM_TARGET_WEBGL) && defined(MAGNUM_TARGET_GLES2))
        .value("DEPTH_STENCIL", GL::RenderbufferFormat::DepthStencil)
        #endif
        #if !(defined(MAGNUM_TARGET_WEBGL) && defined(MAGNUM_TARGET_GLES2))
        .value("DEPTH24_STENCIL8", GL::RenderbufferFormat::Depth24Stencil8)
        #endif
        #ifndef MAGNUM_TARGET_GLES2
        .value("DEPTH32F_STENCIL8", GL::RenderbufferFormat::Depth32FStencil8)
        #endif
        ;

    py::class_<GL::Renderbuffer>{m, "Renderbuffer", "Renderbuffer"}
        /** @todo limit queries */

        .def(py::init(), "Constructor")
        .def_property_readonly("id", &GL::Renderbuffer::id, "OpenGL renderbuffer ID")
        .def("set_storage", &GL::Renderbuffer::setStorage, "Set renderbuffer storage")
        .def("set_storage_multisample", &GL::Renderbuffer::setStorageMultisample, "Set multisample renderbuffer storage");

    /* Framebuffers */
    py::enum_<GL::FramebufferClear> framebufferClear{m, "FramebufferClear", "Mask for framebuffer clearing"};
    framebufferClear
        .value("COLOR", GL::FramebufferClear::Color)
        .value("DEPTH", GL::FramebufferClear::Depth)
        .value("STENCIL", GL::FramebufferClear::Stencil);
    corrade::enumOperators(framebufferClear);

    py::class_<GL::AbstractFramebuffer, NonDefaultFramebufferHolder<GL::AbstractFramebuffer>> abstractFramebuffer{m,
        "AbstractFramebuffer", "Base for default and named framebuffers"};

    abstractFramebuffer
        /** @todo limit queries */

        .def("bind", &GL::AbstractFramebuffer::bind,
            "Bind framebuffer for drawing")
        .def_property("viewport", &GL::AbstractFramebuffer::viewport, &GL::AbstractFramebuffer::setViewport,
            "Viewport")
        /* Using lambdas to avoid method chaining getting into signatures */
        .def("clear", [](GL::AbstractFramebuffer& self, GL::FramebufferClear mask) {
            self.clear(mask);
        }, "Clear specified buffers in the framebuffer")
        .def("read", static_cast<void(GL::AbstractFramebuffer::*)(const Range2Di&, const MutableImageView2D&)>(&GL::AbstractFramebuffer::read),
            "Read block of pixels from the framebuffer to an image view")
        /** @todo more */;

    py::class_<GL::DefaultFramebuffer, GL::AbstractFramebuffer, NonDefaultFramebufferHolder<GL::DefaultFramebuffer>> defaultFramebuffer{m,
        "DefaultFramebuffer", "Default framebuffer"};

    py::class_<GL::Framebuffer, GL::AbstractFramebuffer, GL::PyFramebufferHolder<GL::Framebuffer>> framebuffer{m,
        "Framebuffer", "Framebuffer"};

    py::class_<GL::Framebuffer::ColorAttachment>{framebuffer, "ColorAttachment", "Color attachment"}
        .def(py::init<UnsignedInt>(), "Constructor");

    py::class_<GL::Framebuffer::DrawAttachment>{framebuffer, "DrawAttachment", "Draw attachment"}
        .def(py::init<GL::Framebuffer::ColorAttachment>(), "Color attachment")
        .def_readonly_static("NONE", &GL::Framebuffer::DrawAttachment::None, "No attachment");

    py::implicitly_convertible<GL::Framebuffer::ColorAttachment, GL::Framebuffer::DrawAttachment>();

    py::class_<GL::Framebuffer::BufferAttachment>{framebuffer, "BufferAttachment", "Buffer attachment"}
        .def(py::init<GL::Framebuffer::ColorAttachment>(), "Color buffer")
        .def_readonly_static("DEPTH", &GL::Framebuffer::BufferAttachment::Depth, "Depth buffer")
        .def_readonly_static("STENCIL", &GL::Framebuffer::BufferAttachment::Stencil, "Stencil buffer")
        #if !defined(MAGNUM_TARGET_GLES2) || defined(MAGNUM_TARGET_WEBGL)
        .def_readonly_static("DEPTH_STENCIL", &GL::Framebuffer::BufferAttachment::DepthStencil, "Both depth and stencil buffer")
        #endif
        ;

    py::implicitly_convertible<GL::Framebuffer::ColorAttachment, GL::Framebuffer::BufferAttachment>();

    framebuffer
        .def(py::init<const Range2Di&>(), "Constructor")
        .def_property_readonly("id", &GL::Framebuffer::id, "OpenGL framebuffer ID")
        /* Using lambdas to avoid method chaining getting into signatures */
        .def("map_for_draw", [](GL::Framebuffer& self, GL::Framebuffer::DrawAttachment attachment) {
            self.mapForDraw(attachment);
        }, "Map shader output to an attachment")
        /** @todo list mapForDraw (neeeds a non-initlist variant on magnum side) */
        .def("map_for_read", [](GL::Framebuffer& self, GL::Framebuffer::ColorAttachment attachment) {
            self.mapForRead(attachment);
        }, "Map given color attachment for reading")
        .def("attach_renderbuffer", [](GL::Framebuffer& self, GL::Framebuffer::BufferAttachment attachment, GL::Renderbuffer& renderbuffer) {
            self.attachRenderbuffer(attachment, renderbuffer);

            /* Keep a reference to the renderbuffer to avoid it being deleted
               before the framebuffer */
            pyObjectHolderFor<GL::PyFramebufferHolder>(self).attachments.emplace_back(pyObjectFromInstance(renderbuffer));
        }, "Attach renderbuffer to given buffer")

        .def_property_readonly("attachments", [](GL::Framebuffer& self) {
            return pyObjectHolderFor<GL::PyFramebufferHolder>(self).attachments;
        }, "Renderbuffer and texture objects referenced by the framebuffer");

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

    py::class_<GL::Mesh, GL::PyMeshHolder<GL::Mesh>>{m, "Mesh", "Mesh"}
        .def(py::init<GL::MeshPrimitive>(), "Constructor", py::arg("primitive") = GL::MeshPrimitive::Triangles)
        .def(py::init<MeshPrimitive>(), "Constructor")
        .def_property_readonly("id", &GL::Mesh::id, "OpenGL vertex array ID")
        .def_property("primitive", &GL::Mesh::primitive,
            [](GL::Mesh& self, py::object primitive) {
                if(py::isinstance<MeshPrimitive>(primitive))
                    self.setPrimitive(py::cast<MeshPrimitive>(primitive));
                else if(py::isinstance<GL::MeshPrimitive>(primitive))
                    self.setPrimitive(py::cast<GL::MeshPrimitive>(primitive));
                else throw py::type_error{Utility::formatString("expected MeshPrimitive or gl.MeshPrimitive, got {}", std::string(py::str{primitive.get_type()}))};
            }, "Primitive type")
        /* Have to use a lambda because it returns GL::Mesh which is not
           tracked (unlike PyMesh) */
        .def_property("count", &GL::Mesh::count, [](GL::Mesh& self, UnsignedInt count) {
            self.setCount(count);
        }, "Vertex/index count")

        /* Using lambdas to avoid method chaining getting into signatures */

        .def("add_vertex_buffer", [](GL::Mesh& self, GL::Buffer& buffer, GLintptr offset, GLsizei stride, const GL::DynamicAttribute& attribute) {
            self.addVertexBuffer(buffer, offset, stride, attribute);

            /* Keep a reference to the buffer to avoid it being deleted before
               the mesh */
            pyObjectHolderFor<GL::PyMeshHolder>(self).buffers.emplace_back(pyObjectFromInstance(buffer));
        }, "Add vertex buffer", py::arg("buffer"), py::arg("offset"), py::arg("stride"), py::arg("attribute"))
        .def("draw", [](GL::Mesh& self, GL::AbstractShaderProgram& shader) {
            self.draw(shader);
        }, "Draw the mesh")
        /** @todo more */

        .def_property_readonly("buffers", [](GL::Mesh& self) {
            return pyObjectHolderFor<GL::PyMeshHolder>(self).buffers;
        }, "Buffer objects referenced by the mesh");

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
            .def_static("set_feature", GL::Renderer::setFeature, "Enable or disable a feature")

            .def_property_static("clear_color", nullptr,
                [](py::object, const Color4& color) {
                    /** @todo why can't it be just a single param? */
                    GL::Renderer::setClearColor(color);
                }, "Set clear color");
    }
}

}

#ifndef MAGNUM_BUILD_STATIC
/* TODO: remove declaration when https://github.com/pybind/pybind11/pull/1863
   is released */
extern "C" PYBIND11_EXPORT PyObject* PyInit_gl();
PYBIND11_MODULE(gl, m) {
    magnum::gl(m);
}
#endif
