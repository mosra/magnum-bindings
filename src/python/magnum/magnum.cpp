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

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Magnum/Image.h>
#include <Magnum/ImageView.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/PixelStorage.h>
#include <Magnum/Sampler.h>
#include <Magnum/VertexFormat.h>

#include "Corrade/PythonBindings.h"
#include "Corrade/Containers/PythonBindings.h"
#include "Corrade/Containers/StridedArrayViewPythonBindings.h"
#include "Magnum/PythonBindings.h"

#include "magnum/bootstrap.h"

#ifdef MAGNUM_BUILD_STATIC
#include "magnum/staticconfigure.h"
#endif

namespace py = pybind11;

namespace magnum { namespace {

template<class T> void image(py::class_<T>& c) {
    c
        /* Constructors. Only the ones taking the generic format and *not*
           taking an Array, as Python has no way to "move" it in */
        .def(py::init<const PixelStorage&, PixelFormat>(), "Construct an image placeholder")
        .def(py::init<PixelFormat>(), "Construct an image placeholder")

        /* Properties */
        .def_property_readonly("storage", &T::storage, "Storage of pixel data")
        .def_property_readonly("format", &T::format, "Format of pixel data")
        /** @todo formatExtra() */
        .def_property_readonly("pixel_size", &T::pixelSize, "Pixel size (in bytes)")
        .def_property_readonly("size", [](T& self) {
            return PyDimensionTraits<T::Dimensions, Int>::from(self.size());
        }, "Image size")
        .def_property_readonly("data", [](T& self) {
            return Containers::pyArrayViewHolder(self.data(), self.data() ? py::cast(self) : py::none{});
        }, "Raw image data")
        .def_property_readonly("pixels", [](T& self) {
            return Containers::pyArrayViewHolder(Containers::PyStridedArrayView<T::Dimensions + 1, const char>{self.pixels()}, self.data() ? py::cast(self) : py::none{});
        }, "View on pixel data");
}

template<class T> void imageView(py::class_<T, PyImageViewHolder<T>>& c) {
    /*
        Missing APIs:

        Type, ErasedType, Dimensions
    */

    py::implicitly_convertible<Image<T::Dimensions>, T>();

    c
        /* Constructors. The variants *not* taking an array view have to be
           first, otherwise things fail on systems that don't have numpy
           installed:

            ===================================================================
            ERROR: test_init_empty (test.test.ImageView)
            -------------------------------------------------------------------
            Traceback (most recent call last):
              File ".../magnum/test/test.py", line 102, in test_init_empty
                b = ImageView2D(storage, PixelFormat.R32F, (8, 8))
            ModuleNotFoundError: No module named 'numpy'

           This is because of the order in which pybind processes arguments ---
           it would first try to match the (PixelFormat, Vector2i, ArrayView)
           variant and *somehow* getting all the way to the third argument,
           where, because ArrayView is marked as implicitly convertible from
           py::array for numpy compatibility, it ends up doing this in numpy.h:

            module m = module::import("numpy.core.multiarray");
            auto c = m.attr("_ARRAY_API");

           Wonderful, isn't it. */
        .def(py::init([](const PixelStorage& storage, PixelFormat format, const typename PyDimensionTraits<T::Dimensions, Int>::VectorType& size) {
            return T{storage, format, size};
        }), "Construct an empty view")
        .def(py::init([](PixelFormat format, const typename PyDimensionTraits<T::Dimensions, Int>::VectorType& size) {
            return T{format, size};
        }), "Construct an empty view")
        .def(py::init([](const PixelStorage& storage, PixelFormat format, const typename PyDimensionTraits<T::Dimensions, Int>::VectorType& size, const Containers::ArrayView<typename T::Type>& data) {
            return pyImageViewHolder(T{storage, format, size, data}, pyObjectHolderFor<Containers::PyArrayViewHolder>(data).owner);
        }), "Constructor")
        .def(py::init([](PixelFormat format, const typename PyDimensionTraits<T::Dimensions, Int>::VectorType& size, const Containers::ArrayView<typename T::Type>& data) {
            return pyImageViewHolder(T{format, size, data}, pyObjectHolderFor<Containers::PyArrayViewHolder>(data).owner);
        }), "Constructor")
        .def(py::init([](Image<T::Dimensions>& image) {
            return pyImageViewHolder(T{image}, image.data() ? py::cast(image) : py::none{});
        }), "Construct a view on an image")
        .def(py::init([](const ImageView<T::Dimensions, typename T::Type>& other) {
            return pyImageViewHolder(ImageView<T::Dimensions, typename T::Type>(other), pyObjectHolderFor<PyImageViewHolder>(other).owner);
        }), "Construct from any type convertible to an image view")

        /* Properties */
        .def_property_readonly("storage", &T::storage, "Storage of pixel data")
        .def_property_readonly("format", &T::format, "Format of pixel data")
        .def_property_readonly("pixel_size", &T::pixelSize, "Pixel size (in bytes)")
        .def_property_readonly("size", [](T& self) {
            return PyDimensionTraits<T::Dimensions, Int>::from(self.size());
        }, "Image size")
        .def_property("data", [](T& self) {
            return Containers::pyArrayViewHolder(self.data(), pyObjectHolderFor<PyImageViewHolder>(self).owner);
        }, [](T& self, const Containers::ArrayView<typename T::Type>& data) {
            self.setData(data);
            pyObjectHolderFor<PyImageViewHolder>(self).owner =
            pyObjectHolderFor<Containers::PyArrayViewHolder>(data).owner;
        }, "Raw image data")
        .def_property_readonly("pixels", [](T& self) {
            return Containers::pyArrayViewHolder(Containers::PyStridedArrayView<T::Dimensions + 1, typename T::Type>{self.pixels()}, pyObjectHolderFor<PyImageViewHolder>(self).owner);
        }, "View on pixel data")

        .def_property_readonly("owner", [](T& self) {
            return pyObjectHolderFor<PyImageViewHolder>(self).owner;
        }, "Memory owner");
}

template<class T> void imageViewFromMutable(py::class_<T, PyImageViewHolder<T>>& c) {
    py::implicitly_convertible<BasicMutableImageView<T::Dimensions>, T>();

    c
        .def(py::init([](const BasicMutableImageView<T::Dimensions>& other) {
            return pyImageViewHolder(BasicImageView<T::Dimensions>(other), pyObjectHolderFor<PyImageViewHolder>(other).owner);
        }), "Construct from a mutable view");
}

void magnum(py::module_& m) {
    m.attr("BUILD_STATIC") =
        #ifdef MAGNUM_BUILD_STATIC
        true
        #else
        false
        #endif
        ;
    m.attr("TARGET_GL") =
        #ifdef MAGNUM_TARGET_GL
        true
        #else
        false
        #endif
        ;
    m.attr("TARGET_GLES") =
        #ifdef MAGNUM_TARGET_GLES
        true
        #else
        false
        #endif
        ;
    m.attr("TARGET_GLES2") =
        #ifdef MAGNUM_TARGET_GLES2
        true
        #else
        false
        #endif
        ;
    /** @todo do we need TARGET_GLES3? i hope not */
    m.attr("TARGET_WEBGL") =
        #ifdef MAGNUM_TARGET_WEBGL
        true
        #else
        false
        #endif
        ;
    m.attr("TARGET_EGL") =
        #ifdef MAGNUM_TARGET_EGL
        true
        #else
        false
        #endif
        ;
    m.attr("TARGET_VK") =
        #ifdef MAGNUM_TARGET_VK
        true
        #else
        false
        #endif
        ;

    py::enum_<MeshPrimitive>{m, "MeshPrimitive", "Mesh primitive type"}
        .value("POINTS", MeshPrimitive::Points)
        .value("LINES", MeshPrimitive::Lines)
        .value("LINE_LOOP", MeshPrimitive::LineLoop)
        .value("LINE_STRIP", MeshPrimitive::LineStrip)
        .value("TRIANGLES", MeshPrimitive::Triangles)
        .value("TRIANGLE_STRIP", MeshPrimitive::TriangleStrip)
        .value("TRIANGLE_FAN", MeshPrimitive::TriangleFan);

    py::enum_<MeshIndexType>{m, "MeshIndexType", "Mesh index type"}
        .value("UNSIGNED_BYTE", MeshIndexType::UnsignedByte)
        .value("UNSIGNED_SHORT", MeshIndexType::UnsignedShort)
        .value("UNSIGNED_INT", MeshIndexType::UnsignedInt);

    py::enum_<PixelFormat>{m, "PixelFormat", "Format of pixel data"}
        .value("R8_UNORM", PixelFormat::R8Unorm)
        .value("RG8_UNORM", PixelFormat::RG8Unorm)
        .value("RGB8_UNORM", PixelFormat::RGB8Unorm)
        .value("RGBA8_UNORM", PixelFormat::RGBA8Unorm)
        .value("R8_SNORM", PixelFormat::R8Snorm)
        .value("RG8_SNORM", PixelFormat::RG8Snorm)
        .value("RGB8_SNORM", PixelFormat::RGB8Snorm)
        .value("RGBA8_SNORM", PixelFormat::RGBA8Snorm)
        .value("R8_SRGB", PixelFormat::R8Srgb)
        .value("RG8_SRGB", PixelFormat::RG8Srgb)
        .value("RGB8_SRGB", PixelFormat::RGB8Srgb)
        .value("RGBA8_SRGB", PixelFormat::RGBA8Srgb)
        .value("R8UI", PixelFormat::R8UI)
        .value("RG8UI", PixelFormat::RG8UI)
        .value("RGB8UI", PixelFormat::RGB8UI)
        .value("RGBA8UI", PixelFormat::RGBA8UI)
        .value("R8I", PixelFormat::R8I)
        .value("RG8I", PixelFormat::RG8I)
        .value("RGB8I", PixelFormat::RGB8I)
        .value("RGBA8I", PixelFormat::RGBA8I)
        .value("R16_UNORM", PixelFormat::R16Unorm)
        .value("RG16_UNORM", PixelFormat::RG16Unorm)
        .value("RGB16_UNORM", PixelFormat::RGB16Unorm)
        .value("RGBA16_UNORM", PixelFormat::RGBA16Unorm)
        .value("R16_SNORM", PixelFormat::R16Snorm)
        .value("RG16_SNORM", PixelFormat::RG16Snorm)
        .value("RGB16_SNORM", PixelFormat::RGB16Snorm)
        .value("RGBA16_SNORM", PixelFormat::RGBA16Snorm)
        .value("R16UI", PixelFormat::R16UI)
        .value("RG16UI", PixelFormat::RG16UI)
        .value("RGB16UI", PixelFormat::RGB16UI)
        .value("RGBA16UI", PixelFormat::RGBA16UI)
        .value("R16I", PixelFormat::R16I)
        .value("RG16I", PixelFormat::RG16I)
        .value("RGB16I", PixelFormat::RGB16I)
        .value("RGBA16I", PixelFormat::RGBA16I)
        .value("R32UI", PixelFormat::R32UI)
        .value("RG32UI", PixelFormat::RG32UI)
        .value("RGB32UI", PixelFormat::RGB32UI)
        .value("RGBA32UI", PixelFormat::RGBA32UI)
        .value("R32I", PixelFormat::R32I)
        .value("RG32I", PixelFormat::RG32I)
        .value("RGB32I", PixelFormat::RGB32I)
        .value("RGBA32I", PixelFormat::RGBA32I)
        .value("R16F", PixelFormat::R16F)
        .value("RG16F", PixelFormat::RG16F)
        .value("RGB16F", PixelFormat::RGB16F)
        .value("RGBA16F", PixelFormat::RGBA16F)
        .value("R32F", PixelFormat::R32F)
        .value("RG32F", PixelFormat::RG32F)
        .value("RGB32F", PixelFormat::RGB32F)
        .value("RGBA32F", PixelFormat::RGBA32F)
        .value("DEPTH16_UNORM", PixelFormat::Depth16Unorm)
        .value("DEPTH24_UNORM", PixelFormat::Depth24Unorm)
        .value("DEPTH32F", PixelFormat::Depth32F)
        .value("STENCIL8UI", PixelFormat::Stencil8UI)
        .value("DEPTH16_UNORM_STENCIL8UI", PixelFormat::Depth16UnormStencil8UI)
        .value("DEPTH24_UNORM_STENCIL8UI", PixelFormat::Depth24UnormStencil8UI)
        .value("DEPTH32F_STENCIL8UI", PixelFormat::Depth32FStencil8UI);

    py::class_<PixelStorage>{m, "PixelStorage", "Pixel storage parameters"}
        .def(py::init(), "Default constructor")

        /* Comparison */
        .def(py::self == py::self, "Equality comparison")
        .def(py::self != py::self, "Non-equality comparison")

        /* Properties */
        .def_property("alignment",
            &PixelStorage::alignment, &PixelStorage::setAlignment, "Row alignment")
        .def_property("row_length",
            &PixelStorage::rowLength, &PixelStorage::setRowLength, "Row length")
        .def_property("image_height",
            &PixelStorage::imageHeight, &PixelStorage::setImageHeight, "Image height")
        .def_property("skip",
            &PixelStorage::skip, &PixelStorage::setSkip, "Pixel, row and image skip");

    py::class_<Image1D> image1D{m, "Image1D", "One-dimensional image"};
    py::class_<Image2D> image2D{m, "Image2D", "Two-dimensional image"};
    py::class_<Image3D> image3D{m, "Image3D", "Three-dimensional image"};
    image(image1D);
    image(image2D);
    image(image3D);

    py::class_<ImageView1D, PyImageViewHolder<ImageView1D>> imageView1D{m, "ImageView1D", "One-dimensional image view"};
    py::class_<ImageView2D, PyImageViewHolder<ImageView2D>> imageView2D{m, "ImageView2D", "Two-dimensional image view"};
    py::class_<ImageView3D, PyImageViewHolder<ImageView3D>> imageView3D{m, "ImageView3D", "Three-dimensional image view"};
    py::class_<MutableImageView1D, PyImageViewHolder<MutableImageView1D>> mutableImageView1D{m, "MutableImageView1D", "One-dimensional mutable image view"};
    py::class_<MutableImageView2D, PyImageViewHolder<MutableImageView2D>> mutableImageView2D{m, "MutableImageView2D", "Two-dimensional mutable image view"};
    py::class_<MutableImageView3D, PyImageViewHolder<MutableImageView3D>> mutableImageView3D{m, "MutableImageView3D", "Three-dimensional mutable image view"};

    imageView(imageView1D);
    imageView(imageView2D);
    imageView(imageView3D);
    imageView(mutableImageView1D);
    imageView(mutableImageView2D);
    imageView(mutableImageView3D);

    imageViewFromMutable(imageView1D);
    imageViewFromMutable(imageView2D);
    imageViewFromMutable(imageView3D);

    py::enum_<SamplerFilter>{m, "SamplerFilter", "Texture sampler filtering"}
        .value("NEAREST", SamplerFilter::Nearest)
        .value("LINEAR", SamplerFilter::Linear);
    py::enum_<SamplerMipmap>{m, "SamplerMipmap", "Texture sampler mip level selection"}
        .value("BASE", SamplerMipmap::Base)
        .value("NEAREST", SamplerMipmap::Nearest)
        .value("LINEAR", SamplerMipmap::Linear);
    py::enum_<SamplerWrapping>{m, "SamplerWrapping", "Texture sampler wrapping"}
        .value("REPEAT", SamplerWrapping::Repeat)
        .value("MIRRORED_REPEAT", SamplerWrapping::MirroredRepeat)
        .value("CLAMP_TO_EDGE", SamplerWrapping::ClampToEdge)
        .value("CLAMP_TO_BORDER", SamplerWrapping::ClampToBorder)
        .value("MIRROR_CLAMP_TO_EDGE", SamplerWrapping::MirrorClampToEdge);

    py::enum_<VertexFormat>{m, "VertexFormat", "Vertex format"}
        .value("FLOAT", VertexFormat::Float)
        .value("HALF", VertexFormat::Half)
        .value("DOUBLE", VertexFormat::Double)
        .value("UNSIGNED_BYTE", VertexFormat::UnsignedByte)
        .value("UNSIGNED_BYTE_NORMALIZED", VertexFormat::UnsignedByteNormalized)
        .value("BYTE", VertexFormat::Byte)
        .value("BYTE_NORMALIZED", VertexFormat::ByteNormalized)
        .value("UNSIGNED_SHORT", VertexFormat::UnsignedShort)
        .value("UNSIGNED_SHORT_NORMALIZED", VertexFormat::UnsignedShortNormalized)
        .value("SHORT", VertexFormat::Short)
        .value("SHORT_NORMALIZED", VertexFormat::ShortNormalized)
        .value("UNSIGNED_INT", VertexFormat::UnsignedInt)
        .value("INT", VertexFormat::Int)
        .value("VECTOR2", VertexFormat::Vector2)
        .value("VECTOR2H", VertexFormat::Vector2h)
        .value("VECTOR2D", VertexFormat::Vector2d)
        .value("VECTOR2UB", VertexFormat::Vector2ub)
        .value("VECTOR2UB_NORMALIZED", VertexFormat::Vector2ubNormalized)
        .value("VECTOR2B", VertexFormat::Vector2b)
        .value("VECTOR2B_NORMALIZED", VertexFormat::Vector2bNormalized)
        .value("VECTOR2US", VertexFormat::Vector2us)
        .value("VECTOR2US_NORMALIZED", VertexFormat::Vector2usNormalized)
        .value("VECTOR2S", VertexFormat::Vector2s)
        .value("VECTOR2S_NORMALIZED", VertexFormat::Vector2usNormalized)
        .value("VECTOR2UI", VertexFormat::Vector2ui)
        .value("VECTOR2I", VertexFormat::Vector2i)
        .value("VECTOR3", VertexFormat::Vector3)
        .value("VECTOR3H", VertexFormat::Vector3h)
        .value("VECTOR3D", VertexFormat::Vector3d)
        .value("VECTOR3UB", VertexFormat::Vector3ub)
        .value("VECTOR3UB_NORMALIZED", VertexFormat::Vector3ubNormalized)
        .value("VECTOR3B", VertexFormat::Vector3b)
        .value("VECTOR3B_NORMALIZED", VertexFormat::Vector3bNormalized)
        .value("VECTOR3US", VertexFormat::Vector3us)
        .value("VECTOR3US_NORMALIZED", VertexFormat::Vector3usNormalized)
        .value("VECTOR3S", VertexFormat::Vector3s)
        .value("VECTOR3S_NORMALIZED", VertexFormat::Vector3usNormalized)
        .value("VECTOR3UI", VertexFormat::Vector3ui)
        .value("VECTOR3I", VertexFormat::Vector3i)
        .value("VECTOR4", VertexFormat::Vector4)
        .value("VECTOR4H", VertexFormat::Vector4h)
        .value("VECTOR4D", VertexFormat::Vector4d)
        .value("VECTOR4UB", VertexFormat::Vector4ub)
        .value("VECTOR4UB_NORMALIZED", VertexFormat::Vector4ubNormalized)
        .value("VECTOR4B", VertexFormat::Vector4b)
        .value("VECTOR4B_NORMALIZED", VertexFormat::Vector4bNormalized)
        .value("VECTOR4US", VertexFormat::Vector4us)
        .value("VECTOR4US_NORMALIZED", VertexFormat::Vector4usNormalized)
        .value("VECTOR4S", VertexFormat::Vector4s)
        .value("VECTOR4S_NORMALIZED", VertexFormat::Vector4usNormalized)
        .value("VECTOR4UI", VertexFormat::Vector4ui)
        .value("VECTOR4I", VertexFormat::Vector4i)
        .value("MATRIX2X2", VertexFormat::Matrix2x2)
        .value("MATRIX2X2H", VertexFormat::Matrix2x2h)
        .value("MATRIX2X2D", VertexFormat::Matrix2x2d)
        .value("MATRIX2X2B_NORMALIZED", VertexFormat::Matrix2x2bNormalized)
        .value("MATRIX2X2S_NORMALIZED", VertexFormat::Matrix2x2sNormalized)
        .value("MATRIX2X3", VertexFormat::Matrix2x3)
        .value("MATRIX2X3H", VertexFormat::Matrix2x3h)
        .value("MATRIX2X3D", VertexFormat::Matrix2x3d)
        .value("MATRIX2X3B_NORMALIZED", VertexFormat::Matrix2x3bNormalized)
        .value("MATRIX2X3S_NORMALIZED", VertexFormat::Matrix2x3sNormalized)
        .value("MATRIX2X4", VertexFormat::Matrix2x4)
        .value("MATRIX2X4H", VertexFormat::Matrix2x4h)
        .value("MATRIX2X4D", VertexFormat::Matrix2x4d)
        .value("MATRIX2X4B_NORMALIZED", VertexFormat::Matrix2x4bNormalized)
        .value("MATRIX2X4S_NORMALIZED", VertexFormat::Matrix2x4sNormalized)
        .value("MATRIX2X2B_NORMALIZED_ALIGNED", VertexFormat::Matrix2x2bNormalizedAligned)
        .value("MATRIX2X3H_ALIGNED", VertexFormat::Matrix2x3hAligned)
        .value("MATRIX2X3B_NORMALIZED_ALIGNED", VertexFormat::Matrix2x3bNormalizedAligned)
        .value("MATRIX2X3S_NORMALIZED_ALIGNED", VertexFormat::Matrix2x3sNormalizedAligned)
        .value("MATRIX3X2", VertexFormat::Matrix3x2)
        .value("MATRIX3X2H", VertexFormat::Matrix3x2h)
        .value("MATRIX3X2D", VertexFormat::Matrix3x2d)
        .value("MATRIX3X2B_NORMALIZED", VertexFormat::Matrix3x2bNormalized)
        .value("MATRIX3X2S_NORMALIZED", VertexFormat::Matrix3x2sNormalized)
        .value("MATRIX3X3", VertexFormat::Matrix3x3)
        .value("MATRIX3X3H", VertexFormat::Matrix3x3h)
        .value("MATRIX3X3D", VertexFormat::Matrix3x3d)
        .value("MATRIX3X3B_NORMALIZED", VertexFormat::Matrix3x3bNormalized)
        .value("MATRIX3X3S_NORMALIZED", VertexFormat::Matrix3x3sNormalized)
        .value("MATRIX3X4", VertexFormat::Matrix3x4)
        .value("MATRIX3X4H", VertexFormat::Matrix3x4h)
        .value("MATRIX3X4D", VertexFormat::Matrix3x4d)
        .value("MATRIX3X4B_NORMALIZED", VertexFormat::Matrix3x4bNormalized)
        .value("MATRIX3X4S_NORMALIZED", VertexFormat::Matrix3x4sNormalized)
        .value("MATRIX3X2B_NORMALIZED_ALIGNED", VertexFormat::Matrix3x2bNormalizedAligned)
        .value("MATRIX3X3H_ALIGNED", VertexFormat::Matrix3x3hAligned)
        .value("MATRIX3X3B_NORMALIZED_ALIGNED", VertexFormat::Matrix3x3bNormalizedAligned)
        .value("MATRIX3X3S_NORMALIZED_ALIGNED", VertexFormat::Matrix3x3sNormalizedAligned)
        .value("MATRIX4X2", VertexFormat::Matrix4x2)
        .value("MATRIX4X2H", VertexFormat::Matrix4x2h)
        .value("MATRIX4X2D", VertexFormat::Matrix4x2d)
        .value("MATRIX4X2B_NORMALIZED", VertexFormat::Matrix4x2bNormalized)
        .value("MATRIX4X2S_NORMALIZED", VertexFormat::Matrix4x2sNormalized)
        .value("MATRIX4X3", VertexFormat::Matrix4x3)
        .value("MATRIX4X3H", VertexFormat::Matrix4x3h)
        .value("MATRIX4X3D", VertexFormat::Matrix4x3d)
        .value("MATRIX4X3B_NORMALIZED", VertexFormat::Matrix4x3bNormalized)
        .value("MATRIX4X3S_NORMALIZED", VertexFormat::Matrix4x3sNormalized)
        .value("MATRIX4X4", VertexFormat::Matrix4x4)
        .value("MATRIX4X4H", VertexFormat::Matrix4x4h)
        .value("MATRIX4X4D", VertexFormat::Matrix4x4d)
        .value("MATRIX4X4B_NORMALIZED", VertexFormat::Matrix4x4bNormalized)
        .value("MATRIX4X4S_NORMALIZED", VertexFormat::Matrix4x4sNormalized)
        .value("MATRIX4X2B_NORMALIZED_ALIGNED", VertexFormat::Matrix4x2bNormalizedAligned)
        .value("MATRIX4X3H_ALIGNED", VertexFormat::Matrix4x3hAligned)
        .value("MATRIX4X3B_NORMALIZED_ALIGNED", VertexFormat::Matrix4x3bNormalizedAligned)
        .value("MATRIX4X3S_NORMALIZED_ALIGNED", VertexFormat::Matrix4x3sNormalizedAligned);
}

}}

/* TODO: remove declaration when https://github.com/pybind/pybind11/pull/1863
   is released */
extern "C" PYBIND11_EXPORT PyObject* PyInit__magnum();
PYBIND11_MODULE(_magnum, m) {
    m.doc() = "Root Magnum module";

    /* We need ArrayView for images */
    py::module_::import("corrade.containers");

    py::module_ math = m.def_submodule("math");
    magnum::math(m, math);

    /* These need stuff from math, so need to be called after */
    magnum::magnum(m);

    /* In case Magnum is a bunch of static libraries, put everything into a
       single shared lib to make it easier to install (which is the point of
       static builds) and avoid issues with multiply-defined global symbols.

       These need to be defined in the order they depend on. */
    #ifdef MAGNUM_BUILD_STATIC
    #ifdef Magnum_GL_FOUND
    py::module_ gl = m.def_submodule("gl");
    magnum::gl(gl);
    #endif

    #ifdef Magnum_SceneGraph_FOUND
    py::module_ scenegraph = m.def_submodule("scenegraph");
    magnum::scenegraph(scenegraph);
    #endif

    #ifdef Magnum_Text_FOUND
    py::module_ text = m.def_submodule("text");
    magnum::text(text);
    #endif

    #ifdef Magnum_Trade_FOUND
    py::module_ trade = m.def_submodule("trade");
    magnum::trade(trade);
    #endif

    #ifdef Magnum_MeshTools_FOUND
    /* Depends on trade and gl */
    py::module_ meshtools = m.def_submodule("meshtools");
    magnum::meshtools(meshtools);
    #endif

    #ifdef Magnum_Primitives_FOUND
    /* Depends on trade */
    py::module_ primitives = m.def_submodule("primitives");
    magnum::primitives(primitives);
    #endif

    #ifdef Magnum_Shaders_FOUND
    /* Depends on gl */
    py::module_ shaders = m.def_submodule("shaders");
    magnum::shaders(shaders);
    #endif

    /* Keep the doc in sync with platform/__init__.py */
    py::module_ platform = m.def_submodule("platform");
    platform.doc() = "Platform-specific application and context creation";

    #ifdef Magnum_GlfwApplication_FOUND
    py::module_ glfw = platform.def_submodule("glfw");
    magnum::platform::glfw(glfw);
    #endif

    #ifdef Magnum_Sdl2Application_FOUND
    py::module_ sdl2 = platform.def_submodule("sdl2");
    magnum::platform::sdl2(sdl2);
    #endif

    #ifdef Magnum_WindowlessEglApplication_FOUND
    py::module_ egl = platform.def_submodule("egl");
    magnum::platform::egl(egl);
    #endif

    #ifdef Magnum_WindowlessGlxApplication_FOUND
    py::module_ glx = platform.def_submodule("glx");
    magnum::platform::glx(glx);
    #endif
    #endif
}
