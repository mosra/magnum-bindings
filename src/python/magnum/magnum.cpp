/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>

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

#include "magnum/acessorsForPixelFormat.h"
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
            const PixelFormat format = self.format();
            const std::size_t itemsize = pixelFormatSize(format);
            const Containers::Triple<const char*, py::object(*)(const char*), void(*)(char*, py::handle)> formatStringGetitemSetitem = accessorsForPixelFormat(format);
            if(!formatStringGetitemSetitem.first()) {
                PyErr_SetString(PyExc_NotImplementedError, "access to this pixel format is not implemented yet, sorry");
                throw py::error_already_set{};
            }
            return Containers::pyArrayViewHolder(Containers::PyStridedArrayView<T::Dimensions, char>{flattenPixelView(self.data(), self.pixels()), formatStringGetitemSetitem.first(), itemsize, formatStringGetitemSetitem.second(), formatStringGetitemSetitem.third()}, self.data() ? py::cast(self) : py::none{});
        }, "Pixel data");
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
            const PixelFormat format = self.format();
            const std::size_t itemsize = pixelFormatSize(format);
            const Containers::Triple<const char*, py::object(*)(const char*), void(*)(char*, py::handle)> formatStringGetitemSetitem = accessorsForPixelFormat(format);
            if(!formatStringGetitemSetitem.first()) {
                PyErr_SetString(PyExc_NotImplementedError, "access to this pixel format is not implemented yet, sorry");
                throw py::error_already_set{};
            }
            return Containers::pyArrayViewHolder(Containers::PyStridedArrayView<T::Dimensions, typename T::Type>{flattenPixelView(self.data(), self.pixels()), formatStringGetitemSetitem.first(), itemsize, formatStringGetitemSetitem.second(), formatStringGetitemSetitem.third()}, pyObjectHolderFor<PyImageViewHolder>(self).owner);
        }, "Pixel data")

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

template<class T> void compressedImage(py::class_<T>& c) {
    c
        /* Constructors. Only the ones that are *not* taking an Array, as
           Python has no way to "move" it in */
        .def(py::init(), "Construct an image placeholder")

        /* Properties */
        .def_property_readonly("format", &T::format, "Format of compressed pixel data")
        .def_property_readonly("size", [](T& self) {
            return PyDimensionTraits<T::Dimensions, Int>::from(self.size());
        }, "Image size")
        .def_property_readonly("data", [](T& self) {
            return Containers::pyArrayViewHolder(self.data(), self.data() ? py::cast(self) : py::none{});
        }, "Raw image data");
}

template<class T> void compressedImageView(py::class_<T, PyImageViewHolder<T>>& c) {
    /*
        Missing APIs:

        Type, ErasedType, Dimensions
    */

    py::implicitly_convertible<CompressedImage<T::Dimensions>, T>();

    c
        /* Constructors. The variants *not* taking an array view have to be
           first, otherwise things fail on systems that don't have numpy
           installed. See imageView() above for details */
        .def(py::init([](CompressedPixelFormat format, const typename PyDimensionTraits<T::Dimensions, Int>::VectorType& size) {
            return T{format, size};
        }), "Construct an empty view")
        .def(py::init([](CompressedPixelFormat format, const typename PyDimensionTraits<T::Dimensions, Int>::VectorType& size, const Containers::ArrayView<typename T::Type>& data) {
            return pyImageViewHolder(T{format, size, data}, pyObjectHolderFor<Containers::PyArrayViewHolder>(data).owner);
        }), "Constructor")
        .def(py::init([](CompressedImage<T::Dimensions>& image) {
            return pyImageViewHolder(T{image}, image.data() ? py::cast(image) : py::none{});
        }), "Construct a view on an image")
        .def(py::init([](const CompressedImageView<T::Dimensions, typename T::Type>& other) {
            return pyImageViewHolder(CompressedImageView<T::Dimensions, typename T::Type>(other), pyObjectHolderFor<PyImageViewHolder>(other).owner);
        }), "Construct from any type convertible to an image view")

        /* Properties */
        .def_property_readonly("format", &T::format, "Format of compressedpixel data")
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

        .def_property_readonly("owner", [](T& self) {
            return pyObjectHolderFor<PyImageViewHolder>(self).owner;
        }, "Memory owner");
}

template<class T> void compressedImageViewFromMutable(py::class_<T, PyImageViewHolder<T>>& c) {
    py::implicitly_convertible<BasicMutableCompressedImageView<T::Dimensions>, T>();

    c
        .def(py::init([](const BasicMutableCompressedImageView<T::Dimensions>& other) {
            return pyImageViewHolder(BasicCompressedImageView<T::Dimensions>(other), pyObjectHolderFor<PyImageViewHolder>(other).owner);
        }), "Construct from a mutable view");
}

void magnum(py::module_& m) {
    m.attr("BUILD_DEPRECATED") =
        #ifdef MAGNUM_BUILD_DEPRECATED
        true
        #else
        false
        #endif
        ;
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
        .value("DEPTH32F_STENCIL8UI", PixelFormat::Depth32FStencil8UI)
        .def_property_readonly("size", [](PixelFormat self) {
            if(isPixelFormatImplementationSpecific(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine size of an implementation-specific format");
                throw py::error_already_set{};
            }
            return pixelFormatSize(self);
        }, "Size of given pixel format")
        .def_property_readonly("channel_format", [](PixelFormat self) {
            if(isPixelFormatImplementationSpecific(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine channel format of an implementation-specific format");
                throw py::error_already_set{};
            }
            if(isPixelFormatDepthOrStencil(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine channel format of a depth/stencil format");
                throw py::error_already_set{};
            }
            return pixelFormatChannelFormat(self);
        }, "Channel format of given pixel format")
        .def_property_readonly("channel_count", [](PixelFormat self) {
            if(isPixelFormatImplementationSpecific(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine channel count of an implementation-specific format");
                throw py::error_already_set{};
            }
            if(isPixelFormatDepthOrStencil(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine channel count of a depth/stencil format");
                throw py::error_already_set{};
            }
            return pixelFormatChannelCount(self);
        }, "Channel format of given pixel format")
        .def_property_readonly("is_normalized", [](PixelFormat self) {
            if(isPixelFormatImplementationSpecific(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine type of an implementation-specific format");
                throw py::error_already_set{};
            }
            if(isPixelFormatDepthOrStencil(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine type of a depth/stencil format");
                throw py::error_already_set{};
            }
            return isPixelFormatNormalized(self);
        }, "Whether given pixel format is normalized")
        .def_property_readonly("is_integral", [](PixelFormat self) {
            if(isPixelFormatImplementationSpecific(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine type of an implementation-specific format");
                throw py::error_already_set{};
            }
            if(isPixelFormatDepthOrStencil(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine type of a depth/stencil format");
                throw py::error_already_set{};
            }
            return isPixelFormatIntegral(self);
        }, "Whether given pixel format is integral")
        .def_property_readonly("is_floating_point", [](PixelFormat self) {
            if(isPixelFormatImplementationSpecific(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine type of an implementation-specific format");
                throw py::error_already_set{};
            }
            if(isPixelFormatDepthOrStencil(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine type of a depth/stencil format");
                throw py::error_already_set{};
            }
            return isPixelFormatFloatingPoint(self);
        }, "Whether given pixel format is floating-point")
        .def_property_readonly("is_srgb", [](PixelFormat self) {
            if(isPixelFormatImplementationSpecific(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine colorspace of an implementation-specific format");
                throw py::error_already_set{};
            }
            if(isPixelFormatDepthOrStencil(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine colorspace of a depth/stencil format");
                throw py::error_already_set{};
            }
            return isPixelFormatSrgb(self);
        }, "Whether given pixel format is sRGB")
        .def_property_readonly("is_depth_or_stencil", [](PixelFormat self) {
            if(isPixelFormatImplementationSpecific(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine type of an implementation-specific format");
                throw py::error_already_set{};
            }
            return isPixelFormatDepthOrStencil(self);
        }, "Whether given pixel format is depth or stencil")
        .def_property_readonly("is_implementation_specific", [](PixelFormat self) {
            return isPixelFormatImplementationSpecific(self);
        }, "Whether given pixel format wraps an implementation-specific identifier")
        /** @todo wrap/unwrap, similarly to custom MeshAttribute etc in Trade */
        ;

    py::enum_<CompressedPixelFormat>{m, "CompressedPixelFormat", "Format of compressed pixel data"}
        .value("BC1_RGB_UNORM", CompressedPixelFormat::Bc1RGBUnorm)
        .value("BC1_RGB_SRGB", CompressedPixelFormat::Bc1RGBSrgb)
        .value("BC1_RGBA_UNORM", CompressedPixelFormat::Bc1RGBAUnorm)
        .value("BC1_RGBA_SRGB", CompressedPixelFormat::Bc1RGBASrgb)
        .value("BC2_RGBA_UNORM", CompressedPixelFormat::Bc2RGBAUnorm)
        .value("BC2_RGBA_SRGB", CompressedPixelFormat::Bc2RGBASrgb)
        .value("BC3_RGBA_UNORM", CompressedPixelFormat::Bc3RGBAUnorm)
        .value("BC3_RGBA_SRGB", CompressedPixelFormat::Bc3RGBASrgb)
        .value("BC4_R_UNORM", CompressedPixelFormat::Bc4RUnorm)
        .value("BC4_R_SNORM", CompressedPixelFormat::Bc4RSnorm)
        .value("BC5_RG_UNORM", CompressedPixelFormat::Bc5RGUnorm)
        .value("BC5_RG_SNORM", CompressedPixelFormat::Bc5RGSnorm)
        .value("BC6H_RGB_UFLOAT", CompressedPixelFormat::Bc6hRGBUfloat)
        .value("BC6H_RGB_SFLOAT", CompressedPixelFormat::Bc6hRGBSfloat)
        .value("BC7_RGBA_UNORM", CompressedPixelFormat::Bc7RGBAUnorm)
        .value("BC7_RGBA_SRGB", CompressedPixelFormat::Bc7RGBASrgb)
        .value("EAC_R11_UNORM", CompressedPixelFormat::EacR11Unorm)
        .value("EAC_R11_SNORM", CompressedPixelFormat::EacR11Snorm)
        .value("EAC_RG11_UNORM", CompressedPixelFormat::EacRG11Unorm)
        .value("EAC_RG11_SNORM", CompressedPixelFormat::EacRG11Snorm)
        .value("ETC2_RGB8_UNORM", CompressedPixelFormat::Etc2RGB8Unorm)
        .value("ETC2_RGB8_SRGB", CompressedPixelFormat::Etc2RGB8Srgb)
        .value("ETC2_RGB8A1_UNORM", CompressedPixelFormat::Etc2RGB8A1Unorm)
        .value("ETC2_RGB8A1_SRGB", CompressedPixelFormat::Etc2RGB8A1Srgb)
        .value("ETC2_RGBA8_UNORM", CompressedPixelFormat::Etc2RGBA8Unorm)
        .value("ETC2_RGBA8_SRGB", CompressedPixelFormat::Etc2RGBA8Srgb)
        .value("ASTC_4X4_RGBA_UNORM", CompressedPixelFormat::Astc4x4RGBAUnorm)
        .value("ASTC_4X4_RGBA_SRGB", CompressedPixelFormat::Astc4x4RGBASrgb)
        .value("ASTC_4X4_RGBAF", CompressedPixelFormat::Astc4x4RGBAF)
        .value("ASTC_5X4_RGBA_UNORM", CompressedPixelFormat::Astc5x4RGBAUnorm)
        .value("ASTC_5X4_RGBA_SRGB", CompressedPixelFormat::Astc5x4RGBASrgb)
        .value("ASTC_5X4_RGBAF", CompressedPixelFormat::Astc5x4RGBAF)
        .value("ASTC_5X5_RGBA_UNORM", CompressedPixelFormat::Astc5x5RGBAUnorm)
        .value("ASTC_5X5_RGBA_SRGB", CompressedPixelFormat::Astc5x5RGBASrgb)
        .value("ASTC_5X5_RGBAF", CompressedPixelFormat::Astc5x5RGBAF)
        .value("ASTC_6X5_RGBA_UNORM", CompressedPixelFormat::Astc6x5RGBAUnorm)
        .value("ASTC_6X5_RGBA_SRGB", CompressedPixelFormat::Astc6x5RGBASrgb)
        .value("ASTC_6X5_RGBAF", CompressedPixelFormat::Astc6x5RGBAF)
        .value("ASTC_6X6_RGBA_UNORM", CompressedPixelFormat::Astc6x6RGBAUnorm)
        .value("ASTC_6X6_RGBA_SRGB", CompressedPixelFormat::Astc6x6RGBASrgb)
        .value("ASTC_6X6_RGBAF", CompressedPixelFormat::Astc6x6RGBAF)
        .value("ASTC_8X5_RGBA_UNORM", CompressedPixelFormat::Astc8x5RGBAUnorm)
        .value("ASTC_8X5_RGBA_SRGB", CompressedPixelFormat::Astc8x5RGBASrgb)
        .value("ASTC_8X5_RGBAF", CompressedPixelFormat::Astc8x5RGBAF)
        .value("ASTC_8X6_RGBA_UNORM", CompressedPixelFormat::Astc8x6RGBAUnorm)
        .value("ASTC_8X6_RGBA_SRGB", CompressedPixelFormat::Astc8x6RGBASrgb)
        .value("ASTC_8X6_RGBAF", CompressedPixelFormat::Astc8x6RGBAF)
        .value("ASTC_8X8_RGBA_UNORM", CompressedPixelFormat::Astc8x8RGBAUnorm)
        .value("ASTC_8X8_RGBA_SRGB", CompressedPixelFormat::Astc8x8RGBASrgb)
        .value("ASTC_8X8_RGBAF", CompressedPixelFormat::Astc8x8RGBAF)
        .value("ASTC_10X5_RGBA_UNORM", CompressedPixelFormat::Astc10x5RGBAUnorm)
        .value("ASTC_10X5_RGBA_SRGB", CompressedPixelFormat::Astc10x5RGBASrgb)
        .value("ASTC_10X5_RGBAF", CompressedPixelFormat::Astc10x5RGBAF)
        .value("ASTC_10X6_RGBA_UNORM", CompressedPixelFormat::Astc10x6RGBAUnorm)
        .value("ASTC_10X6_RGBA_SRGB", CompressedPixelFormat::Astc10x6RGBASrgb)
        .value("ASTC_10X6_RGBAF", CompressedPixelFormat::Astc10x6RGBAF)
        .value("ASTC_10X8_RGBA_UNORM", CompressedPixelFormat::Astc10x8RGBAUnorm)
        .value("ASTC_10X8_RGBA_SRGB", CompressedPixelFormat::Astc10x8RGBASrgb)
        .value("ASTC_10X8_RGBAF", CompressedPixelFormat::Astc10x8RGBAF)
        .value("ASTC_10X10_RGBA_UNORM", CompressedPixelFormat::Astc10x10RGBAUnorm)
        .value("ASTC_10X10_RGBA_SRGB", CompressedPixelFormat::Astc10x10RGBASrgb)
        .value("ASTC_10X10_RGBAF", CompressedPixelFormat::Astc10x10RGBAF)
        .value("ASTC_12X10_RGBA_UNORM", CompressedPixelFormat::Astc12x10RGBAUnorm)
        .value("ASTC_12X10_RGBA_SRGB", CompressedPixelFormat::Astc12x10RGBASrgb)
        .value("ASTC_12X10_RGBAF", CompressedPixelFormat::Astc12x10RGBAF)
        .value("ASTC_12X12_RGBA_UNORM", CompressedPixelFormat::Astc12x12RGBAUnorm)
        .value("ASTC_12X12_RGBA_SRGB", CompressedPixelFormat::Astc12x12RGBASrgb)
        .value("ASTC_12X12_RGBAF", CompressedPixelFormat::Astc12x12RGBAF)
        .value("ASTC_3X3X3_RGBA_UNORM", CompressedPixelFormat::Astc3x3x3RGBAUnorm)
        .value("ASTC_3X3X3_RGBA_SRGB", CompressedPixelFormat::Astc3x3x3RGBASrgb)
        .value("ASTC_3X3X3_RGBAF", CompressedPixelFormat::Astc3x3x3RGBAF)
        .value("ASTC_4X3X3_RGBA_UNORM", CompressedPixelFormat::Astc4x3x3RGBAUnorm)
        .value("ASTC_4X3X3_RGBA_SRGB", CompressedPixelFormat::Astc4x3x3RGBASrgb)
        .value("ASTC_4X3X3_RGBAF", CompressedPixelFormat::Astc4x3x3RGBAF)
        .value("ASTC_4X4X3_RGBA_UNORM", CompressedPixelFormat::Astc4x4x3RGBAUnorm)
        .value("ASTC_4X4X3_RGBA_SRGB", CompressedPixelFormat::Astc4x4x3RGBASrgb)
        .value("ASTC_4X4X3_RGBAF", CompressedPixelFormat::Astc4x4x3RGBAF)
        .value("ASTC_4X4X4_RGBA_UNORM", CompressedPixelFormat::Astc4x4x4RGBAUnorm)
        .value("ASTC_4X4X4_RGBA_SRGB", CompressedPixelFormat::Astc4x4x4RGBASrgb)
        .value("ASTC_4X4X4_RGBAF", CompressedPixelFormat::Astc4x4x4RGBAF)
        .value("ASTC_5X4X4_RGBA_UNORM", CompressedPixelFormat::Astc5x4x4RGBAUnorm)
        .value("ASTC_5X4X4_RGBA_SRGB", CompressedPixelFormat::Astc5x4x4RGBASrgb)
        .value("ASTC_5X4X4_RGBAF", CompressedPixelFormat::Astc5x4x4RGBAF)
        .value("ASTC_5X5X4_RGBA_UNORM", CompressedPixelFormat::Astc5x5x4RGBAUnorm)
        .value("ASTC_5X5X4_RGBA_SRGB", CompressedPixelFormat::Astc5x5x4RGBASrgb)
        .value("ASTC_5X5X4_RGBAF", CompressedPixelFormat::Astc5x5x4RGBAF)
        .value("ASTC_5X5X5_RGBA_UNORM", CompressedPixelFormat::Astc5x5x5RGBAUnorm)
        .value("ASTC_5X5X5_RGBA_SRGB", CompressedPixelFormat::Astc5x5x5RGBASrgb)
        .value("ASTC_5X5X5_RGBAF", CompressedPixelFormat::Astc5x5x5RGBAF)
        .value("ASTC_6X5X5_RGBA_UNORM", CompressedPixelFormat::Astc6x5x5RGBAUnorm)
        .value("ASTC_6X5X5_RGBA_SRGB", CompressedPixelFormat::Astc6x5x5RGBASrgb)
        .value("ASTC_6X5X5_RGBAF", CompressedPixelFormat::Astc6x5x5RGBAF)
        .value("ASTC_6X6X5_RGBA_UNORM", CompressedPixelFormat::Astc6x6x5RGBAUnorm)
        .value("ASTC_6X6X5_RGBA_SRGB", CompressedPixelFormat::Astc6x6x5RGBASrgb)
        .value("ASTC_6X6X5_RGBAF", CompressedPixelFormat::Astc6x6x5RGBAF)
        .value("ASTC_6X6X6_RGBA_UNORM", CompressedPixelFormat::Astc6x6x6RGBAUnorm)
        .value("ASTC_6X6X6_RGBA_SRGB", CompressedPixelFormat::Astc6x6x6RGBASrgb)
        .value("ASTC_6X6X6_RGBAF", CompressedPixelFormat::Astc6x6x6RGBAF)
        .value("PVRTC_RGB_2PP_UNORM", CompressedPixelFormat::PvrtcRGB2bppUnorm)
        .value("PVRTC_RGB_2PP_SRGB", CompressedPixelFormat::PvrtcRGB2bppSrgb)
        .value("PVRTC_RGBA_2PP_UNORM", CompressedPixelFormat::PvrtcRGBA2bppUnorm)
        .value("PVRTC_RGBA_2PP_SRGB", CompressedPixelFormat::PvrtcRGBA2bppSrgb)
        .value("PVRTC_RGB_4PP_UNORM", CompressedPixelFormat::PvrtcRGB4bppUnorm)
        .value("PVRTC_RGB_4PP_SRGB", CompressedPixelFormat::PvrtcRGB4bppSrgb)
        .value("PVRTC_RGBA_4PP_UNORM", CompressedPixelFormat::PvrtcRGBA4bppUnorm)
        .value("PVRTC_RGBA_4PP_SRGB", CompressedPixelFormat::PvrtcRGBA4bppSrgb)
        .def_property_readonly("block_size", [](CompressedPixelFormat self) {
            if(isCompressedPixelFormatImplementationSpecific(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine size of an implementation-specific format");
                throw py::error_already_set{};
            }
            return compressedPixelFormatBlockSize(self);
        }, "Block size of given compressed pixel format")
        .def_property_readonly("block_data_size", [](CompressedPixelFormat self) {
            if(isCompressedPixelFormatImplementationSpecific(self)) {
                PyErr_SetString(PyExc_AssertionError, "can't determine size of an implementation-specific format");
                throw py::error_already_set{};
            }
            return compressedPixelFormatBlockDataSize(self);
        }, "Block data size of given compressed pixel format")
        .def_property_readonly("is_implementation_specific", [](CompressedPixelFormat self) {
            return isCompressedPixelFormatImplementationSpecific(self);
        }, "Whether given compressed pixel format wraps an implementation-specific identifier")
        /** @todo wrap/unwrap, similarly to custom MeshAttribute etc in Trade */
        ;

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

    py::class_<CompressedImage1D> compressedImage1D{m, "CompressedImage1D", "One-dimensional compressed image"};
    py::class_<CompressedImage2D> compressedImage2D{m, "CompressedImage2D", "Two-dimensional compressed image"};
    py::class_<CompressedImage3D> compressedImage3D{m, "CompressedImage3D", "Three-dimensional compressed image"};
    compressedImage(compressedImage1D);
    compressedImage(compressedImage2D);
    compressedImage(compressedImage3D);

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

    py::class_<CompressedImageView1D, PyImageViewHolder<CompressedImageView1D>> compressedImageView1D{m, "CompressedImageView1D", "One-dimensional compressed image view"};
    py::class_<CompressedImageView2D, PyImageViewHolder<CompressedImageView2D>> compressedImageView2D{m, "CompressedImageView2D", "Two-dimensional compressed image view"};
    py::class_<CompressedImageView3D, PyImageViewHolder<CompressedImageView3D>> compressedImageView3D{m, "CompressedImageView3D", "Three-dimensional compressed image view"};
    py::class_<MutableCompressedImageView1D, PyImageViewHolder<MutableCompressedImageView1D>> mutableCompressedImageView1D{m, "MutableCompressedImageView1D", "One-dimensional mutable compressed image view"};
    py::class_<MutableCompressedImageView2D, PyImageViewHolder<MutableCompressedImageView2D>> mutableCompressedImageView2D{m, "MutableCompressedImageView2D", "Two-dimensional mutable compressed image view"};
    py::class_<MutableCompressedImageView3D, PyImageViewHolder<MutableCompressedImageView3D>> mutableCompressedImageView3D{m, "MutableCompressedImageView3D", "Three-dimensional mutable compressed image view"};

    compressedImageView(compressedImageView1D);
    compressedImageView(compressedImageView2D);
    compressedImageView(compressedImageView3D);
    compressedImageView(mutableCompressedImageView1D);
    compressedImageView(mutableCompressedImageView2D);
    compressedImageView(mutableCompressedImageView3D);

    compressedImageViewFromMutable(compressedImageView1D);
    compressedImageViewFromMutable(compressedImageView2D);
    compressedImageViewFromMutable(compressedImageView3D);

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

    #ifdef Magnum_SceneTools_FOUND
    /* Depends on trade */
    py::module_ scenetools = m.def_submodule("scenetools");
    magnum::scenetools(scenetools);
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
