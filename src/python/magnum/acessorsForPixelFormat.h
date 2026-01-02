#ifndef magnum_accessorsForPixelFormat_h
#define magnum_accessorsForPixelFormat_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
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
#include <Corrade/Containers/Triple.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Half.h>
#include <Magnum/Math/Packing.h>

#include "Magnum/StridedArrayViewPythonBindings.h"

#include "magnum/bootstrap.h"

/* This is used by both the root magnum module (Image, ImageView) and
   magnum.trade.ImageData. There's no easy way to call an exported symbol of
   another module due to Python isolating their namespaces so it's duplicated
   in both modules. */
namespace magnum { namespace {

Containers::Triple<const char*, py::object(*)(const char*), void(*)(char*, py::handle)> accessorsForPixelFormat(const PixelFormat format) {
    switch(format) {
        #define _c(format, type)                                                  \
            case PixelFormat::format: return {                              \
                Containers::Implementation::pythonFormatString<type>(),     \
                [](const char* item) {                                      \
                    return py::cast(*reinterpret_cast<const type*>(item));  \
                },                                                          \
                [](char* item, py::handle object) {                         \
                    *reinterpret_cast<type*>(item) = py::cast<type>(object); \
                }};
        /* Types (such as half-floats) that need to be cast before passed
           from/to pybind that doesn't understand the type directly */
        #define _cc(format, type, castType)                                 \
            case PixelFormat::format: return {                             \
                Containers::Implementation::pythonFormatString<type>(),     \
                [](const char* item) {                                      \
                    return py::cast(castType(*reinterpret_cast<const type*>(item))); \
                },                                                          \
                [](char* item, py::handle object) {                         \
                    *reinterpret_cast<type*>(item) = type(py::cast<castType>(object)); \
                }};
        /* Normalized types that need to be packed/unpacked before passed
           from/to pybind */
        #define _cNormalized(format, type, unpackType)                      \
            case PixelFormat::format: return {                              \
                Containers::Implementation::pythonFormatString<type>(),     \
                [](const char* item) {                                      \
                    return py::cast(Math::unpack<unpackType>(*reinterpret_cast<const type*>(item))); \
                },                                                          \
                [](char* item, py::handle object) {                         \
                    *reinterpret_cast<type*>(item) = Math::pack<type>(py::cast<unpackType>(object)); \
                }};
        /* LCOV_EXCL_START */
        _cNormalized(R8Unorm, UnsignedByte, Float)
        _cNormalized(RG8Unorm, Vector2ub, Vector2)
        _cNormalized(RGB8Unorm, Vector3ub, Vector3)
        _cNormalized(RGBA8Unorm, Vector4ub, Vector4)
        _cNormalized(R8Snorm, Byte, Float)
        _cNormalized(RG8Snorm, Vector2b, Vector2)
        _cNormalized(RGB8Snorm, Vector3b, Vector3)
        _cNormalized(RGBA8Snorm, Vector4b, Vector4)
        /* LCOV_EXCL_STOP */
        case PixelFormat::R8Srgb: return {
            Containers::Implementation::pythonFormatString<UnsignedByte>(),
            /** @todo have an (internal) API to convert just R/RG sRGB channels */
            [](const char* item) {
                return py::cast(Color3::fromSrgb(Vector3ub{*reinterpret_cast<const UnsignedByte*>(item), 0, 0}).r());
            },
            [](char* item, py::handle object) {
                *reinterpret_cast<UnsignedByte*>(item) = Color3{py::cast<Float>(object), 0.0f, 0.0f}.toSrgb<UnsignedByte>().r();
            }};
        case PixelFormat::RG8Srgb: return {
            Containers::Implementation::pythonFormatString<Vector2ub>(),
            /** @todo have an (internal) API to convert just R/RG sRGB channels */
            [](const char* item) {
                return py::cast(Color3::fromSrgb(Vector3ub{*reinterpret_cast<const Vector2ub*>(item), 0}).rg());
            },
            [](char* item, py::handle object) {
                *reinterpret_cast<Vector2ub*>(item) = Color3{py::cast<Vector2>(object), 0.0f}.toSrgb<UnsignedByte>().rg();
            }};
        case PixelFormat::RGB8Srgb: return {
            Containers::Implementation::pythonFormatString<Vector3ub>(),
            [](const char* item) {
                return py::cast(Color3::fromSrgb(*reinterpret_cast<const Vector3ub*>(item)));
            },
            [](char* item, py::handle object) {
                *reinterpret_cast<Vector3ub*>(item) = py::cast<Color3>(object).toSrgb<UnsignedByte>();
            }};
        case PixelFormat::RGBA8Srgb: return {
            Containers::Implementation::pythonFormatString<Vector4ub>(),
            [](const char* item) {
                return py::cast(Color4::fromSrgbAlpha(*reinterpret_cast<const Vector4ub*>(item)));
            },
            [](char* item, py::handle object) {
                *reinterpret_cast<Vector4ub*>(item) = py::cast<Color4>(object).toSrgbAlpha<UnsignedByte>();
            }};
        /* LCOV_EXCL_START */
        _cc(R8UI, UnsignedByte, UnsignedInt)
        _cc(RG8UI, Vector2ub, Vector2ui)
        _cc(RGB8UI, Vector3ub, Vector3ui)
        _cc(RGBA8UI, Vector4ub, Vector4ui)
        _cc(R8I, Byte, Int)
        _cc(RG8I, Vector2b, Vector2i)
        _cc(RGB8I, Vector3b, Vector3i)
        _cc(RGBA8I, Vector4b, Vector4i)
        _cNormalized(R16Unorm, UnsignedShort, Float)
        _cNormalized(RG16Unorm, Vector2us, Vector2)
        _cNormalized(RGB16Unorm, Vector3us, Vector3)
        _cNormalized(RGBA16Unorm, Vector4us, Vector4)
        _cNormalized(R16Snorm, Short, Float)
        _cNormalized(RG16Snorm, Vector2s, Vector2)
        _cNormalized(RGB16Snorm, Vector3s, Vector3)
        _cNormalized(RGBA16Snorm, Vector4s, Vector4)
        _cc(R16UI, UnsignedShort, UnsignedInt)
        _cc(RG16UI, Vector2us, Vector2ui)
        _cc(RGB16UI, Vector3us, Vector3ui)
        _cc(RGBA16UI, Vector4us, Vector4ui)
        _cc(R16I, Short, Int)
        _cc(RG16I, Vector2s, Vector2i)
        _cc(RGB16I, Vector3s, Vector3i)
        _cc(RGBA16I, Vector4s, Vector4i)
        _c(R32UI, UnsignedInt)
        _c(RG32UI, Vector2ui)
        _c(RGB32UI, Vector3ui)
        _c(RGBA32UI, Vector4ui)
        _c(R32I, Int)
        _c(RG32I, Vector2i)
        _c(RGB32I, Vector3i)
        _c(RGBA32I, Vector4i)
        _cc(R16F, Half, Float)
        _cc(RG16F, Vector2h, Vector2)
        _cc(RGB16F, Vector3h, Vector3)
        _cc(RGBA16F, Vector4h, Vector4)
        _c(R32F, Float)
        _c(RG32F, Vector2)
        _c(RGB32F, Vector3)
        _c(RGBA32F, Vector4)
        /* LCOV_EXCL_STOP */
        #undef _c
        #undef _cc
        #undef _cNormalized

        /** @todo handle depth/stencil types (yes, i'm lazy) */
        default:
            return {};
    }
}

template<UnsignedInt dimensions, class T> Containers::StridedArrayView<dimensions - 1, T> flattenPixelView(const Containers::ArrayView<T> data, const Containers::StridedArrayView<dimensions, T>& pixels) {
    /** @todo have some builtin API for this, this is awful (pixels<void>()?
        flatten<dimensions>() that asserts the last dimension is contiguous and
        of a POD type? transpose<3, 0, 1, 2>()[0], differently named to avoid
        clashes, taking dimension order?) */
    Containers::Size<dimensions - 1> size{NoInit};
    Containers::Stride<dimensions - 1> stride{NoInit};
    for(std::size_t i = 0; i != dimensions - 1; ++i) {
        size[i] = pixels.size()[i];
        stride[i] = pixels.stride()[i];
    }
    return Containers::StridedArrayView<dimensions - 1, T>{
        data,
        static_cast<T*>(pixels.data()),
        size,
        stride};
}

}}

#endif
