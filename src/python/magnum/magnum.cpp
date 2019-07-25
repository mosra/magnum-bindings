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

#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/PixelStorage.h>

#include "Magnum/Python.h"

#include "corrade/PyArrayView.h"
#include "magnum/bootstrap.h"

#ifdef MAGNUM_BUILD_STATIC
#include "magnum/staticconfigure.h"
#endif

namespace py = pybind11;

namespace magnum { namespace {

template<UnsignedInt dimensions, class T> struct PyDimensionTraits;
template<class T> struct PyDimensionTraits<1, T> {
    typedef T VectorType;
    static VectorType from(const Math::Vector<1, T>& vec) { return vec[0]; }
};
template<class T> struct PyDimensionTraits<2, T> {
    typedef Math::Vector2<T> VectorType;
    static VectorType from(const Math::Vector<2, T>& vec) { return vec; }
};
template<class T> struct PyDimensionTraits<3, T> {
    typedef Math::Vector3<T> VectorType;
    static VectorType from(const Math::Vector<3, T>& vec) { return vec; }
};

template<class T> void imageView(py::class_<T>& c) {
    /*
        Missing APIs:

        Type, ErasedType, Dimensions
    */

    c
        /* Constructors */
        .def(py::init([](const PixelStorage& storage, PixelFormat format, const typename PyDimensionTraits<T::Dimensions, Int>::VectorType& size, const corrade::PyArrayView<typename T::Type>& data) {
            return T{ImageView<T::Dimensions, typename T::Type>{storage, format, size, data}, data.obj};
        }), "Constructor")
        .def(py::init([](PixelFormat format, const typename PyDimensionTraits<T::Dimensions, Int>::VectorType& size, const corrade::PyArrayView<typename T::Type>& data) {
            return T{ImageView<T::Dimensions, typename T::Type>{format, size, data}, data.obj};
        }), "Constructor")
        .def(py::init([](const PixelStorage& storage, PixelFormat format, const typename PyDimensionTraits<T::Dimensions, Int>::VectorType& size) {
            return T{ImageView<T::Dimensions, typename T::Type>{storage, format, size}, py::none{}};
        }), "Construct an empty view")
        .def(py::init([](PixelFormat format, const typename PyDimensionTraits<T::Dimensions, Int>::VectorType& size) {
            return T{ImageView<T::Dimensions, typename T::Type>{format, size}, py::none{}};
        }), "Construct an empty view")

        /* Properties */
        .def_property_readonly("storage", &T::storage, "Storage of pixel data")
        .def_property_readonly("format", &T::format, "Format of pixel data")
        .def_property_readonly("pixel_size", &T::pixelSize, "Pixel size (in bytes)")
        .def_property_readonly("size", [](T& self) {
            return PyDimensionTraits<T::Dimensions, Int>::from(self.size());
        }, "Image size")
        .def_property("data", [](T& self) {
            return corrade::PyArrayView<typename T::Type>{{static_cast<typename T::Type*>(self.data().data()), self.data().size()}, self.owner};
        }, [](T& self, const corrade::PyArrayView<typename T::Type>& data) {
            self.setData(data);
            self.owner = data.obj;
        }, "Image data")
        .def_property_readonly("pixels", [](T& self) {
            return corrade::PyStridedArrayView<T::Dimensions+1, typename T::Type>{self.pixels(), self.owner};
        }, "View on pixel data")

        .def_readonly("owner", &T::owner, "Memory owner");
}

template<class T> void imageViewFromMutable(py::class_<T>& c) {
    c
        .def(py::init([](const PyImageView<T::Dimensions, char>& other) {
            return T{ImageView<T::Dimensions, const char>{other}, other.owner};
        }), "Constructor");
}

void magnum(py::module& m) {
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
        .value("R8UNORM", PixelFormat::R8Unorm)
        .value("RG8UNORM", PixelFormat::RG8Unorm)
        .value("RGB8UNORM", PixelFormat::RGB8Unorm)
        .value("RGBA8UNORM", PixelFormat::RGBA8Unorm)
        .value("R8SNORM", PixelFormat::R8Snorm)
        .value("RG8SNORM", PixelFormat::RG8Snorm)
        .value("RGB8SNORM", PixelFormat::RGB8Snorm)
        .value("RGBA8SNORM", PixelFormat::RGBA8Snorm)
        .value("R8UI", PixelFormat::R8UI)
        .value("RG8UI", PixelFormat::RG8UI)
        .value("RGB8UI", PixelFormat::RGB8UI)
        .value("RGBA8UI", PixelFormat::RGBA8UI)
        .value("R8I", PixelFormat::R8I)
        .value("RG8I", PixelFormat::RG8I)
        .value("RGB8I", PixelFormat::RGB8I)
        .value("RGBA8I", PixelFormat::RGBA8I)
        .value("R16UNORM", PixelFormat::R16Unorm)
        .value("RG16UNORM", PixelFormat::RG16Unorm)
        .value("RGB16UNORM", PixelFormat::RGB16Unorm)
        .value("RGBA16UNORM", PixelFormat::RGBA16Unorm)
        .value("R16SNORM", PixelFormat::R16Snorm)
        .value("RG16SNORM", PixelFormat::RG16Snorm)
        .value("RGB16SNORM", PixelFormat::RGB16Snorm)
        .value("RGBA16SNORM", PixelFormat::RGBA16Snorm)
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
        .value("RGBA32F", PixelFormat::RGBA32F);

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

    py::class_<PyImageView<1, const char>> imageView1D{m, "ImageView1D", "One-dimensional image view"};
    py::class_<PyImageView<2, const char>> imageView2D{m, "ImageView2D", "Two-dimensional image view"};
    py::class_<PyImageView<3, const char>> imageView3D{m, "ImageView3D", "Three-dimensional image view"};
    py::class_<PyImageView<1, char>> mutableImageView1D{m, "MutableImageView1D", "One-dimensional mutable image view"};
    py::class_<PyImageView<2, char>> mutableImageView2D{m, "MutableImageView2D", "Two-dimensional mutable image view"};
    py::class_<PyImageView<3, char>> mutableImageView3D{m, "MutableImageView3D", "Three-dimensional mutable image view"};

    imageView(imageView1D);
    imageView(imageView2D);
    imageView(imageView3D);
    imageView(mutableImageView1D);
    imageView(mutableImageView2D);
    imageView(mutableImageView3D);

    imageViewFromMutable(imageView1D);
    imageViewFromMutable(imageView2D);
    imageViewFromMutable(imageView3D);
}

}}

PYBIND11_MODULE(_magnum, m) {
    /* We need ArrayView for images */
    py::module::import("corrade.containers");

    m.doc() = "Root Magnum module";

    py::module math = m.def_submodule("math");
    magnum::math(m, math);

    /* These need stuff from math, so need to be called after */
    magnum::magnum(m);

    /* In case Magnum is a bunch of static libraries, put everything into a
       single shared lib to make it easier to install (which is the point of
       static builds) and avoid issues with multiply-defined global symbols.

       These need to be defined in the order they depend on. */
    #ifdef MAGNUM_BUILD_STATIC
    #ifdef Magnum_GL_FOUND
    py::module gl = m.def_submodule("gl");
    magnum::gl(gl);
    #endif

    #ifdef Magnum_SceneGraph_FOUND
    py::module scenegraph = m.def_submodule("scenegraph");
    magnum::scenegraph(scenegraph);
    #endif

    #ifdef Magnum_Trade_FOUND
    py::module trade = m.def_submodule("trade");
    magnum::trade(trade);
    #endif

    #ifdef Magnum_MeshTools_FOUND
    /* Depends on trade and gl */
    py::module meshtools = m.def_submodule("meshtools");
    magnum::meshtools(meshtools);
    #endif

    #ifdef Magnum_Primitives_FOUND
    /* Depends on trade */
    py::module primitives = m.def_submodule("primitives");
    magnum::primitives(primitives);
    #endif

    #ifdef Magnum_Shaders_FOUND
    /* Depends on gl */
    py::module shaders = m.def_submodule("shaders");
    magnum::shaders(shaders);
    #endif

    /* Keep the doc in sync with platform/__init__.py */
    py::module platform = m.def_submodule("platform");
    platform.doc() = "Platform-specific application and context creation";

    #ifdef Magnum_GlfwApplication_FOUND
    py::module glfw = platform.def_submodule("glfw");
    magnum::platform::glfw(glfw);
    #endif

    #ifdef Magnum_Sdl2Application_FOUND
    py::module sdl2 = platform.def_submodule("sdl2");
    magnum::platform::sdl2(sdl2);
    #endif

    #ifdef Magnum_WindowlessEglApplication_FOUND
    py::module egl = platform.def_submodule("egl");
    magnum::platform::egl(egl);
    #endif

    #ifdef Magnum_WindowlessGlxApplication_FOUND
    py::module glx = platform.def_submodule("glx");
    magnum::platform::glx(glx);
    #endif
    #endif
}
