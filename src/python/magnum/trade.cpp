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

#include <pybind11/pybind11.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StridedArrayView.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/StringStl.h> /** @todo drop once we have our string casters */
#include <Corrade/Containers/Triple.h>
#include <Magnum/ImageView.h>
#include <Magnum/Math/Complex.h>
#include <Magnum/Math/DualComplex.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Packing.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/Math/DualQuaternion.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <Magnum/Trade/AbstractSceneConverter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/TextureData.h>

#include "Corrade/Containers/PythonBindings.h"
#include "Corrade/Containers/OptionalPythonBindings.h"
#include "Magnum/PythonBindings.h"
#include "Magnum/StridedArrayViewPythonBindings.h"
#include "Magnum/Trade/PythonBindings.h"

#include "corrade/EnumOperators.h"
#include "corrade/pluginmanager.h"
#include "magnum/acessorsForPixelFormat.h"
#include "magnum/bootstrap.h"

#ifdef CORRADE_TARGET_WINDOWS
/* To allow people to conveniently use Python's os.path, we need to convert
   backslashes to forward slashes as all Corrade and Magnum APIs expect
   forward */
#include <Corrade/Utility/Path.h>
#endif

namespace magnum {

namespace {

/* Adapted from pybind11's base_enum internals -- if enum_name returns ???,
   replace it with CUSTOM(id) */
template<class T, typename std::underlying_type<T>::type baseCustomValue> inline py::str enumWithCustomValuesName(const py::object& arg) {
    /* The enum_name helper is only since pybind11 2.6, before it's inline:
        https://github.com/pybind/pybind11/commit/5e6ec496522b313e34af3de91f6c0565f68e3552 */
    /** @todo remove once support for < 2.6 is dropped */
    #if PYBIND11_VERSION_MAJOR*100 + PYBIND11_VERSION_MINOR >= 206
    py::str name = py::detail::enum_name(arg);
    #else
    py::str name = "???";
    py::dict entries = arg.get_type().attr("__entries");
    for(auto kv: entries) {
        if(py::handle(kv.second[py::int_(0)]).equal(arg)) {
            name = py::str(kv.first);
            break;
        }
    }
    #endif
    /* Haha what the hell is this comparison */
    if(std::string{name} == "???")
        return py::str("CUSTOM({})").format(typename std::underlying_type<T>::type(py::int_(arg)) - baseCustomValue);
    return name;
}

/* Not using the meshAttributeCustom() etc helpers as it would be too painful
   to pass them all, and I'd need to make my own handling of the OOB cases
   anyway */
template<class T, typename std::underlying_type<T>::type baseCustomValue> void enumWithCustomValues(py::enum_<T>& enum_) {
    static_assert(!typename std::underlying_type<T>::type(baseCustomValue << 1),
        "base custom value expected to be a single highest bit");

    enum_
        .def("CUSTOM", [](typename std::underlying_type<T>::type value) {
            /* Assuming the base custom value is a single highest bit, the
               custom value should not have the same bit set (or, in other
               words, should be smaller) */
            if(baseCustomValue & value) {
                PyErr_SetString(PyExc_ValueError, "custom value too large");
                throw py::error_already_set{};
            }
            return T(baseCustomValue + value);
        })
        .def_property_readonly("is_custom", [](T value) {
            return typename std::underlying_type<T>::type(value) >= baseCustomValue;
        })
        .def_property_readonly("custom_value", [](T value) {
            if(typename std::underlying_type<T>::type(value) < baseCustomValue) {
                PyErr_SetString(PyExc_AttributeError, "not a custom value");
                throw py::error_already_set{};
            }
            return typename std::underlying_type<T>::type(value) - baseCustomValue;
        });

    /* Adapted from pybind11's base_enum internals, just calling our
       customEnumName instead of py::detail::enum_name */
    enum_.attr("__repr__") = py::cpp_function(
        [](const py::object& arg) -> py::str {
            py::handle type =
                /* handle_of(arg) is only since pybind11 2.6, before it's
                   arg.get_type():
                    https://github.com/pybind/pybind11/commit/41aa92601ebce548290f6a9efcd66e64216bf972 */
                /** @todo remove once support for < 2.6 is dropped */
                #if PYBIND11_VERSION_MAJOR*100 + PYBIND11_VERSION_MINOR >= 206
                py::type::handle_of(arg)
                #else
                arg.get_type()
                #endif
                ;
            py::object type_name = type.attr("__name__");
            return py::str("<{}.{}: {}>")
                .format(std::move(type_name), enumWithCustomValuesName<T, baseCustomValue>(arg), py::int_(arg));
            },
        py::name("__repr__"),
        py::is_method(enum_));
    enum_.attr("name") = py::handle(reinterpret_cast<PyObject*>(&PyProperty_Type))(py::cpp_function(&enumWithCustomValuesName<T, baseCustomValue>, py::name("name"), py::is_method(enum_)));
    enum_.attr("__str__") = py::cpp_function(
        [](const py::object& arg) -> py::str {
            py::object type_name =
                /* handle_of(arg) is only since pybind11 2.6, before it's
                   arg.get_type():
                    https://github.com/pybind/pybind11/commit/41aa92601ebce548290f6a9efcd66e64216bf972 */
                /** @todo remove once support for < 2.6 is dropped */
                #if PYBIND11_VERSION_MAJOR*100 + PYBIND11_VERSION_MINOR >= 206
                py::type::handle_of(arg)
                #else
                arg.get_type()
                #endif
                .attr("__name__");
            return pybind11::str("{}.{}").format(std::move(type_name), enumWithCustomValuesName<T, baseCustomValue>(arg));
        },
        py::name("name"),
        py::is_method(enum_));
}

template<UnsignedInt dimensions, class T> PyObject* implicitlyConvertibleToImageView(PyObject* obj, PyTypeObject*) {
    py::detail::make_caster<Trade::ImageData<dimensions>> caster;
    if(!caster.load(obj, false)) {
        return nullptr;
    }

    Trade::ImageData<dimensions>& data = caster;
    if(data.isCompressed()) {
        PyErr_SetString(PyExc_RuntimeError, "image is compressed");
        throw py::error_already_set{};
    }

    auto r = pyCastButNotShitty(pyImageViewHolder(ImageView<dimensions, T>(data), py::reinterpret_borrow<py::object>(obj))).release().ptr();
    return r;
}

template<UnsignedInt dimensions, class T> Containers::PyArrayViewHolder<Containers::PyStridedArrayView<dimensions, T>> imagePixelsView(Trade::ImageData<dimensions>& image, const Containers::ArrayView<T> data, const Containers::StridedArrayView<dimensions + 1, T>& pixels) {
    const PixelFormat format = image.format();
    const std::size_t itemsize = pixelFormatSize(format);
    const Containers::Triple<const char*, py::object(*)(const char*), void(*)(char*, py::handle)> formatStringGetitemSetitem = accessorsForPixelFormat(format);
    if(!formatStringGetitemSetitem.first()) {
        PyErr_SetString(PyExc_NotImplementedError, "access to this pixel format is not implemented yet, sorry");
        throw py::error_already_set{};
    }
    return Containers::pyArrayViewHolder(Containers::PyStridedArrayView<dimensions, T>{flattenPixelView(data, pixels), formatStringGetitemSetitem.first(), itemsize, formatStringGetitemSetitem.second(), formatStringGetitemSetitem.third()}, py::cast(image));
}

template<UnsignedInt dimensions> void imageData(py::class_<Trade::ImageData<dimensions>, Trade::PyDataHolder<Trade::ImageData<dimensions>>>& c) {
    /*
        Missing APIs:

        Dimensions
    */

    /* These two are quite hacky attempts to bring the ImageData -> ImageView
       conversion operator functionality here. Using py::implicitly_convertible
       alone doesn't work as it only calls conversion constructors exposed to
       Python, and we can't expose such a thing to Python because ImageView is
       defined in the `magnum` module while this is `magnum.trade`, and that
       would mean a cyclic dependency.

       Instead, I took the guts of py::implicitly_convertible and instead of
       calling into Python I'm calling the C++ conversion operator directly
       myself. That alone is not enough, as this implicit conversion is only
       chosen if the target type has a Python-exposed constructor that takes a
       type that's implicitly convertible from the source type. Ugh.

       If this ever breaks with a pybind update, I'm probably going to
       reimplement this in a pure duck-typed fashion. I hope not tho. */
    {
        auto tinfo = py::detail::get_type_info(typeid(ImageView<dimensions, char>));
        CORRADE_INTERNAL_ASSERT(tinfo);
        tinfo->implicit_conversions.push_back(implicitlyConvertibleToImageView<dimensions, char>);
    } {
        auto tinfo = py::detail::get_type_info(typeid(ImageView<dimensions, const char>));
        CORRADE_INTERNAL_ASSERT(tinfo);
        tinfo->implicit_conversions.push_back(implicitlyConvertibleToImageView<dimensions, const char>);
    }

    c
        /* There are no constructors at the moment --- expecting those types
           get only created by importers. (It would also need the Array type
           and movability figured out, postponing that to later.) */
        .def_property_readonly("data_flags", [](Trade::ImageData<dimensions>& self) {
            return Trade::DataFlag(Containers::enumCastUnderlyingType(self.dataFlags()));
        }, "Data flags")

        /* Properties */
        .def_property_readonly("is_compressed", &Trade::ImageData<dimensions>::isCompressed, "Whether the image is compressed")
        .def_property_readonly("storage", [](Trade::ImageData<dimensions>& self) {
            if(self.isCompressed()) {
                PyErr_SetString(PyExc_AttributeError, "image is compressed");
                throw py::error_already_set{};
            }

            return self.storage();
        }, "Storage of pixel data")
        .def_property_readonly("format", [](Trade::ImageData<dimensions>& self) {
            if(self.isCompressed()) {
                PyErr_SetString(PyExc_AttributeError, "image is compressed");
                throw py::error_already_set{};
            }

            return self.format();
        }, "Format of pixel data")
        .def_property_readonly("pixel_size", [](Trade::ImageData<dimensions>& self) {
            if(self.isCompressed()) {
                PyErr_SetString(PyExc_AttributeError, "image is compressed");
                throw py::error_already_set{};
            }

            return self.pixelSize();
        }, "Pixel size (in bytes)")
        .def_property_readonly("size", [](Trade::ImageData<dimensions>& self) {
            return PyDimensionTraits<dimensions, Int>::from(self.size());
        }, "Image size")
        .def_property_readonly("data", [](Trade::ImageData<dimensions>& self) {
            return Containers::pyArrayViewHolder(self.data(), py::cast(self));
        }, "Raw image data")
        .def_property_readonly("mutable_data", [](Trade::ImageData<dimensions>& self) {
            if(!(self.dataFlags() & Trade::DataFlag::Mutable)) {
                PyErr_SetString(PyExc_AttributeError, "image data is not mutable");
                throw py::error_already_set{};
            }
            return Containers::pyArrayViewHolder(self.mutableData(), py::cast(self));
        }, "Mutable raw image data")
        .def_property_readonly("pixels", [](Trade::ImageData<dimensions>& self) {
            if(self.isCompressed()) {
                PyErr_SetString(PyExc_AttributeError, "image is compressed");
                throw py::error_already_set{};
            }
            return imagePixelsView(self, self.data(), self.pixels());
        }, "Pixel data")
        .def_property_readonly("mutable_pixels", [](Trade::ImageData<dimensions>& self) {
            if(self.isCompressed()) {
                PyErr_SetString(PyExc_AttributeError, "image is compressed");
                throw py::error_already_set{};
            }
            if(!(self.dataFlags() & Trade::DataFlag::Mutable)) {
                PyErr_SetString(PyExc_AttributeError, "image data is not mutable");
                throw py::error_already_set{};
            }
            return imagePixelsView(self, self.mutableData(), self.mutablePixels());
        }, "Mutable pixel data")

        .def_property_readonly("owner", [](Trade::ImageData<dimensions>& self) {
            return pyObjectHolderFor<Trade::PyDataHolder>(self).owner;
        }, "Memory owner");
}

/* For some reason having ...Args as the second (and not last) template
   argument does not work. So I'm listing all variants here ... which are
   exactly two, in fact. */
template<class R, R(Trade::AbstractImporter::*f)() const> R checkOpened(Trade::AbstractImporter& self) {
    if(!self.isOpened()) {
        PyErr_SetString(PyExc_AssertionError, "no file opened");
        throw py::error_already_set{};
    }
    return (self.*f)();
}
template<class R, class Arg1, R(Trade::AbstractImporter::*f)(Arg1)> R checkOpened(Trade::AbstractImporter& self, Arg1 arg1) {
    if(!self.isOpened()) {
        PyErr_SetString(PyExc_AssertionError, "no file opened");
        throw py::error_already_set{};
    }
    return (self.*f)(arg1);
}
/** @todo drop this in favor of our own string caster */
template<class R, R(Trade::AbstractImporter::*f)(Containers::StringView)> R checkOpenedString(Trade::AbstractImporter& self, const std::string& arg1) {
    if(!self.isOpened()) {
        PyErr_SetString(PyExc_AssertionError, "no file opened");
        throw py::error_already_set{};
    }
    return (self.*f)(arg1);
}

template<class R, R(Trade::AbstractImporter::*f)(UnsignedInt), UnsignedInt(Trade::AbstractImporter::*bounds)() const> R checkOpenedBounds(Trade::AbstractImporter& self, UnsignedInt id) {
    if(!self.isOpened()) {
        PyErr_SetString(PyExc_AssertionError, "no file opened");
        throw py::error_already_set{};
    }

    if(id >= (self.*bounds)()) {
        PyErr_SetNone(PyExc_IndexError);
        throw py::error_already_set{};
    }

    return (self.*f)(id);
}
/** @todo drop this in favor of our own string caster */
template<class R, Containers::String(Trade::AbstractImporter::*f)(R), R(Trade::AbstractImporter::*bounds)() const> std::string checkOpenedBoundsReturnsString(Trade::AbstractImporter& self, R id) {
    if(!self.isOpened()) {
        PyErr_SetString(PyExc_AssertionError, "no file opened");
        throw py::error_already_set{};
    }

    if(id >= (self.*bounds)()) {
        PyErr_SetNone(PyExc_IndexError);
        throw py::error_already_set{};
    }

    return (self.*f)(id);
}

template<class R, Containers::Optional<R>(Trade::AbstractImporter::*f)(UnsignedInt), UnsignedInt(Trade::AbstractImporter::*bounds)() const> R checkOpenedBoundsResult(Trade::AbstractImporter& self, UnsignedInt id) {
    if(!self.isOpened()) {
        PyErr_SetString(PyExc_AssertionError, "no file opened");
        throw py::error_already_set{};
    }

    if(id >= (self.*bounds)()) {
        PyErr_SetNone(PyExc_IndexError);
        throw py::error_already_set{};
    }

    /** @todo log redirection -- but we'd need assertions to not be part of
        that so when it dies, the user can still see why */
    Containers::Optional<R> out = (self.*f)(id);
    if(!out) {
        PyErr_SetString(PyExc_RuntimeError, "import failed");
        throw py::error_already_set{};
    }

    return *std::move(out);
}
/** @todo drop std::string in favor of our own string caster */
template<class R, Containers::Optional<R>(Trade::AbstractImporter::*f)(UnsignedInt), Int(Trade::AbstractImporter::*indexForName)(Containers::StringView)> R checkOpenedBoundsResultString(Trade::AbstractImporter& self, const std::string& name) {
    if(!self.isOpened()) {
        PyErr_SetString(PyExc_AssertionError, "no file opened");
        throw py::error_already_set{};
    }

    const Int id = (self.*indexForName)(name);
    if(id == -1) {
        PyErr_SetNone(PyExc_KeyError);
        throw py::error_already_set{};
    }

    /** @todo log redirection -- but we'd need assertions to not be part of
        that so when it dies, the user can still see why */
    Containers::Optional<R> out = (self.*f)(id);
    if(!out) {
        PyErr_SetString(PyExc_RuntimeError, "import failed");
        throw py::error_already_set{};
    }

    return *std::move(out);
}

template<class R, Containers::Optional<R>(Trade::AbstractImporter::*f)(UnsignedInt, UnsignedInt), UnsignedInt(Trade::AbstractImporter::*bounds)() const, UnsignedInt(Trade::AbstractImporter::*levelBounds)(UnsignedInt)> R checkOpenedBoundsResult(Trade::AbstractImporter& self, UnsignedInt id, UnsignedInt level) {
    if(!self.isOpened()) {
        PyErr_SetString(PyExc_AssertionError, "no file opened");
        throw py::error_already_set{};
    }

    if(id >= (self.*bounds)()) {
        PyErr_SetString(PyExc_IndexError, "ID out of bounds");
        throw py::error_already_set{};
    }

    if(level >= (self.*levelBounds)(id)) {
        PyErr_SetString(PyExc_IndexError, "level out of bounds");
        throw py::error_already_set{};
    }

    /** @todo log redirection -- but we'd need assertions to not be part of
        that so when it dies, the user can still see why */
    Containers::Optional<R> out = (self.*f)(id, level);
    if(!out) {
        PyErr_SetString(PyExc_RuntimeError, "import failed");
        throw py::error_already_set{};
    }

    return *std::move(out);
}
/** @todo drop std::string in favor of our own string caster */
template<class R, Containers::Optional<R>(Trade::AbstractImporter::*f)(UnsignedInt, UnsignedInt), Int(Trade::AbstractImporter::*indexForName)(Containers::StringView), UnsignedInt(Trade::AbstractImporter::*levelBounds)(UnsignedInt)> R checkOpenedBoundsResultString(Trade::AbstractImporter& self, const std::string& name, UnsignedInt level) {
    if(!self.isOpened()) {
        PyErr_SetString(PyExc_AssertionError, "no file opened");
        throw py::error_already_set{};
    }

    const Int id = (self.*indexForName)(name);
    if(id == -1) {
        PyErr_SetNone(PyExc_KeyError);
        throw py::error_already_set{};
    }

    if(level >= (self.*levelBounds)(id)) {
        PyErr_SetString(PyExc_IndexError, "level out of bounds");
        throw py::error_already_set{};
    }

    /** @todo log redirection -- but we'd need assertions to not be part of
        that so when it dies, the user can still see why */
    Containers::Optional<R> out = (self.*f)(id, level);
    if(!out) {
        PyErr_SetString(PyExc_RuntimeError, "import failed");
        throw py::error_already_set{};
    }

    return *std::move(out);
}

/* Can't be named just checkResult() because the AbstractSceneConverter
   overload would confuse GCC 4.8 */
/** @todo drop std::string in favor of our own string caster */
template<class T, bool(Trade::AbstractImageConverter::*f)(const T&, Containers::StringView)> void checkImageConverterResult(Trade::AbstractImageConverter& self, const T& image, const std::string& filename) {
    /** @todo log redirection -- but we'd need assertions to not be part of
        that so when it dies, the user can still see why */
    bool out = (self.*f)(image,
        #ifdef CORRADE_TARGET_WINDOWS
        /* To allow people to conveniently use Python's os.path, we need to
           convert backslashes to forward slashes as all Corrade and Magnum
           APIs expect forward */
        Utility::Path::fromNativeSeparators(filename)
        #else
        filename
        #endif
    );
    if(!out) {
        PyErr_SetString(PyExc_RuntimeError, "conversion failed");
        throw py::error_already_set{};
    }
}

Containers::Triple<const char*, py::object(*)(const char*), void(*)(char*, py::handle)> accessorsForMeshIndexType(const MeshIndexType type) {
    switch(type) {
        #define _c(type)                                                    \
            case MeshIndexType::type: return {                              \
                Containers::Implementation::pythonFormatString<type>(),     \
                [](const char* item) {                                      \
                    return py::cast(*reinterpret_cast<const type*>(item));  \
                },                                                          \
                [](char* item, py::handle object) {                         \
                    *reinterpret_cast<type*>(item) = py::cast<type>(object); \
                }};
        /* LCOV_EXCL_START */
        _c(UnsignedByte)
        _c(UnsignedShort)
        _c(UnsignedInt)
        /* LCOV_EXCL_STOP */
        #undef _c
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

Containers::Triple<const char*, py::object(*)(const char*), void(*)(char*, py::handle)> accessorsForVertexFormat(const VertexFormat format) {
    switch(format) {
        #define _c(format)                                                  \
            case VertexFormat::format: return {                             \
                Containers::Implementation::pythonFormatString<format>(),   \
                [](const char* item) {                                      \
                    return py::cast(*reinterpret_cast<const format*>(item)); \
                },                                                          \
                [](char* item, py::handle object) {                         \
                    *reinterpret_cast<format*>(item) = py::cast<format>(object); \
                }};
        /* Types (such as half-floats) that need to be cast before passed
           from/to pybind that doesn't understand the type directly */
        #define _cc(format, castType)                                       \
            case VertexFormat::format: return {                             \
                Containers::Implementation::pythonFormatString<format>(),   \
                [](const char* item) {                                      \
                    return py::cast(castType(*reinterpret_cast<const format*>(item))); \
                },                                                          \
                [](char* item, py::handle object) {                         \
                    *reinterpret_cast<format*>(item) = format(py::cast<castType>(object)); \
                }};
        /* Normalized types that need to be packed/unpacked before passed
           from/to pybind */
        #define _cNormalized(format, unpackType)                            \
            case VertexFormat::format ## Normalized: return {               \
                Containers::Implementation::pythonFormatString<format>(),   \
                [](const char* item) {                                      \
                    return py::cast(Math::unpack<unpackType>(*reinterpret_cast<const format*>(item))); \
                },                                                          \
                [](char* item, py::handle object) {                         \
                    *reinterpret_cast<format*>(item) = Math::pack<format>(py::cast<unpackType>(object)); \
                }};
        /* LCOV_EXCL_START */
        _c(Float)
        _c(Double)
        _c(UnsignedByte)
        _cNormalized(UnsignedByte, Float)
        _c(Byte)
        _cNormalized(Byte, Float)
        _c(UnsignedShort)
        _cNormalized(UnsignedShort, Float)
        _c(Short)
        _cNormalized(Short, Float)
        _c(UnsignedInt)
        _c(Int)

        _c(Vector2)
        _c(Vector2d)
        _cc(Vector2ub, Vector2ui)
        _cNormalized(Vector2ub, Vector2)
        _cc(Vector2b, Vector2i)
        _cNormalized(Vector2b, Vector2)
        _cc(Vector2us, Vector2ui)
        _cNormalized(Vector2us, Vector2)
        _cc(Vector2s, Vector2i)
        _cNormalized(Vector2s, Vector2)
        _c(Vector2ui)
        _c(Vector2i)

        _c(Vector3)
        _c(Vector3d)
        _cc(Vector3ub, Vector3ui)
        _cNormalized(Vector3ub, Vector3)
        _cc(Vector3b, Vector3i)
        _cNormalized(Vector3b, Vector3)
        _cc(Vector3us, Vector3ui)
        _cNormalized(Vector3us, Vector3)
        _cc(Vector3s, Vector3i)
        _cNormalized(Vector3s, Vector3)
        _c(Vector3ui)
        _c(Vector3i)

        _c(Vector4)
        _c(Vector4d)
        _cc(Vector4ub, Vector4ui)
        _cNormalized(Vector4ub, Vector4)
        _cc(Vector4b, Vector4i)
        _cNormalized(Vector4b, Vector4)
        _cc(Vector4us, Vector4ui)
        _cNormalized(Vector4us, Vector4)
        _cc(Vector4s, Vector4i)
        _cNormalized(Vector4s, Vector4)
        _c(Vector4ui)
        _c(Vector4i)
        /* LCOV_EXCL_STOP */
        #undef _c
        #undef _cc
        #undef _cNormalized

        /** @todo handle half and matrix types */
        default:
            return {};
    }
}

template<class T> Containers::PyArrayViewHolder<Containers::PyStridedArrayView<1, T>> meshIndicesView(Trade::MeshData& mesh, const Containers::StridedArrayView2D<T>& data) {
    const MeshIndexType type = mesh.indexType();
    const std::size_t itemsize = meshIndexTypeSize(type);
    const Containers::Triple<const char*, py::object(*)(const char*), void(*)(char*, py::handle)> formatStringGetitemSetitem = accessorsForMeshIndexType(type);
    /** @todo update this once there are plugins that can give back custom
        index types */
    CORRADE_INTERNAL_ASSERT(formatStringGetitemSetitem.first());
    return Containers::pyArrayViewHolder(Containers::PyStridedArrayView<1, T>{data.template transposed<0, 1>()[0], formatStringGetitemSetitem.first(), itemsize, formatStringGetitemSetitem.second(), formatStringGetitemSetitem.third()}, py::cast(mesh));
}

template<class T> Containers::PyArrayViewHolder<Containers::PyStridedArrayView<1, T>> meshAttributeView(Trade::MeshData& mesh, const UnsignedInt id, const Containers::StridedArrayView2D<T>& data) {
    const VertexFormat format = mesh.attributeFormat(id);
    const std::size_t itemsize = vertexFormatSize(format);
    const Containers::Triple<const char*, py::object(*)(const char*), void(*)(char*, py::handle)> formatStringGetitemSetitem = accessorsForVertexFormat(format);
    if(!formatStringGetitemSetitem.first()) {
        PyErr_SetString(PyExc_NotImplementedError, "access to this vertex format is not implemented yet, sorry");
        throw py::error_already_set{};
    }
    return Containers::pyArrayViewHolder(Containers::PyStridedArrayView<1, T>{data.template transposed<0, 1>()[0], formatStringGetitemSetitem.first(), itemsize, formatStringGetitemSetitem.second(), formatStringGetitemSetitem.third()}, py::cast(mesh));
}

Containers::Triple<const char*, py::object(*)(const char*), void(*)(char*, py::handle)> accessorsForSceneMappingType(const Trade::SceneMappingType type) {
    switch(type) {
        #define _c(type)                                                    \
            case Trade::SceneMappingType::type: return {                    \
                Containers::Implementation::pythonFormatString<type>(),     \
                [](const char* item) {                                      \
                    return py::cast(*reinterpret_cast<const type*>(item));  \
                },                                                          \
                [](char* item, py::handle object) {                         \
                    *reinterpret_cast<type*>(item) = py::cast<type>(object); \
                }};
        /* LCOV_EXCL_START */
        _c(UnsignedByte)
        _c(UnsignedShort)
        _c(UnsignedInt)
        _c(UnsignedLong)
        /* LCOV_EXCL_STOP */
        #undef _c
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

Containers::Triple<const char*, py::object(*)(const char*), void(*)(char*, py::handle)> accessorsForSceneFieldType(const Trade::SceneFieldType type) {
    switch(type) {
        #define _ctf(typeEnum, type, format)                                \
            case Trade::SceneFieldType::typeEnum: return {                  \
                format,                                                     \
                [](const char* item) {                                      \
                    return py::cast(*reinterpret_cast<type const*>(item));  \
                },                                                          \
                [](char* item, py::handle object) {                         \
                    *reinterpret_cast<type*>(item) = py::cast<type>(object); \
                }};
        #define _c(type) _ctf(type, type, Containers::Implementation::pythonFormatString<type>())
        /* Types (such as half-floats) that need to be cast before passed
           from/to pybind that doesn't understand the type directly */
        #define _cc(type, castType)                                         \
            case Trade::SceneFieldType::type: return {                      \
                Containers::Implementation::pythonFormatString<type>(),     \
                [](const char* item) {                                      \
                    return py::cast(castType(*reinterpret_cast<const type*>(item))); \
                },                                                          \
                [](char* item, py::handle object) {                         \
                    *reinterpret_cast<type*>(item) = type(py::cast<castType>(object)); \
                }};
        /* LCOV_EXCL_START */
        _c(Float)
        _c(Double)
        _c(UnsignedByte)
        _c(Byte)
        _c(UnsignedShort)
        _c(Short)
        _c(UnsignedInt)
        _c(Int)
        _c(UnsignedLong)
        _c(Long)

        _c(Vector2)
        _c(Vector2d)
        _cc(Vector2ub, Vector2ui)
        _cc(Vector2b, Vector2i)
        _cc(Vector2us, Vector2ui)
        _cc(Vector2s, Vector2i)
        _c(Vector2ui)
        _c(Vector2i)
        _c(Vector3)
        _c(Vector3d)
        _cc(Vector3ub, Vector3ui)
        _cc(Vector3b, Vector3i)
        _cc(Vector3us, Vector3ui)
        _cc(Vector3s, Vector3i)
        _c(Vector3ui)
        _c(Vector3i)
        _c(Vector4)
        _c(Vector4d)
        _cc(Vector4ub, Vector4ui)
        _cc(Vector4b, Vector4i)
        _cc(Vector4us, Vector4ui)
        _cc(Vector4s, Vector4i)
        _c(Vector4ui)
        _c(Vector4i)

        _c(Matrix2x2)
        _c(Matrix2x2d)
        _c(Matrix2x3)
        _c(Matrix2x3d)
        _c(Matrix2x4)
        _c(Matrix2x4d)
        _c(Matrix3x2)
        _c(Matrix3x2d)
        _c(Matrix3x3)
        _c(Matrix3x3d)
        _c(Matrix3x4)
        _c(Matrix3x4d)
        _c(Matrix4x2)
        _c(Matrix4x2d)
        _c(Matrix4x3)
        _c(Matrix4x3d)
        _c(Matrix4x4)
        _c(Matrix4x4d)

        _c(Range1D)
        _c(Range1Dd)
        _c(Range1Di)
        _c(Range2D)
        _c(Range2Dd)
        _c(Range2Di)
        _c(Range3D)
        _c(Range3Dd)
        _c(Range3Di)

        _c(Complex)
        _c(Complexd)
        _c(DualComplex)
        _c(DualComplexd)
        _c(Quaternion)
        _c(Quaterniond)
        _c(DualQuaternion)
        _c(DualQuaterniond)

        _c(Deg)
        _c(Degd)
        _c(Rad)
        _c(Radd)

        /* I see very little reason for accessing these from Python but
           nevertheless, one never knows when it will be useful */
        /** @todo passing them through as void* makes them a capsule object in
            Python, is that useful for anything? and the P type is useless for
            numpy ("'P' is not a valid PEP 3118 buffer format string") */
        _ctf(Pointer, std::size_t, "P")
        _ctf(MutablePointer, std::size_t, "P")
        /* LCOV_EXCL_STOP */
        #undef _c
        #undef _cc

        /** @todo handle this once StridedBitArrayView is exposed */
        case Trade::SceneFieldType::Bit:
        /** @todo handle these once there's something to test with */
        case Trade::SceneFieldType::Half:
        case Trade::SceneFieldType::Vector2h:
        case Trade::SceneFieldType::Vector3h:
        case Trade::SceneFieldType::Vector4h:
        case Trade::SceneFieldType::Matrix2x2h:
        case Trade::SceneFieldType::Matrix2x3h:
        case Trade::SceneFieldType::Matrix2x4h:
        case Trade::SceneFieldType::Matrix3x2h:
        case Trade::SceneFieldType::Matrix3x3h:
        case Trade::SceneFieldType::Matrix3x4h:
        case Trade::SceneFieldType::Matrix4x2h:
        case Trade::SceneFieldType::Matrix4x3h:
        case Trade::SceneFieldType::Matrix4x4h:
        case Trade::SceneFieldType::Range1Dh:
        case Trade::SceneFieldType::Range2Dh:
        case Trade::SceneFieldType::Range3Dh:
        case Trade::SceneFieldType::Degh:
        case Trade::SceneFieldType::Radh:
        /** @todo handle these once StringIterable is exposed */
        case Trade::SceneFieldType::StringOffset8:
        case Trade::SceneFieldType::StringOffset16:
        case Trade::SceneFieldType::StringOffset32:
        case Trade::SceneFieldType::StringOffset64:
        case Trade::SceneFieldType::StringRange8:
        case Trade::SceneFieldType::StringRange16:
        case Trade::SceneFieldType::StringRange32:
        case Trade::SceneFieldType::StringRange64:
        case Trade::SceneFieldType::StringRangeNullTerminated8:
        case Trade::SceneFieldType::StringRangeNullTerminated16:
        case Trade::SceneFieldType::StringRangeNullTerminated32:
        case Trade::SceneFieldType::StringRangeNullTerminated64:
            return {};
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

template<class T> Containers::PyArrayViewHolder<Containers::PyStridedArrayView<1, T>> sceneMappingView(Trade::SceneData& scene, const Containers::StridedArrayView2D<T>& data) {
    const Trade::SceneMappingType type = scene.mappingType();
    const std::size_t itemsize = Trade::sceneMappingTypeSize(type);
    const Containers::Triple<const char*, py::object(*)(const char*), void(*)(char*, py::handle)> formatStringGetitemSetitem = accessorsForSceneMappingType(type);
    /* We support all mapping types */
    CORRADE_INTERNAL_ASSERT(formatStringGetitemSetitem.first());
    return Containers::pyArrayViewHolder(Containers::PyStridedArrayView<1, T>{data.template transposed<0, 1>()[0], formatStringGetitemSetitem.first(), itemsize, formatStringGetitemSetitem.second(), formatStringGetitemSetitem.third()}, py::cast(scene));
}

template<class T> Containers::PyArrayViewHolder<Containers::PyStridedArrayView<1, T>> sceneFieldView(Trade::SceneData& scene, const UnsignedInt id, const Containers::StridedArrayView2D<T>& data) {
    const Trade::SceneFieldType type = scene.fieldType(id);
    const std::size_t itemsize = Trade::sceneFieldTypeSize(type);
    const Containers::Triple<const char*, py::object(*)(const char*), void(*)(char*, py::handle)> formatStringGetitemSetitem = accessorsForSceneFieldType(type);
    if(!formatStringGetitemSetitem.first()) {
        PyErr_SetString(PyExc_NotImplementedError, "access to this scene field type is not implemented yet, sorry");
        throw py::error_already_set{};
    }
    return Containers::pyArrayViewHolder(Containers::PyStridedArrayView<1, T>{data.template transposed<0, 1>()[0], formatStringGetitemSetitem.first(), itemsize, formatStringGetitemSetitem.second(), formatStringGetitemSetitem.third()}, py::cast(scene));
}

}

void trade(py::module_& m) {
    m.doc() = "Data format exchange";

    /* AbstractImporter depends on this */
    py::module_::import("corrade.pluginmanager");

    py::enum_<Trade::DataFlag> dataFlag{m, "DataFlags", "Data flags"};
    dataFlag
        .value("OWNED", Trade::DataFlag::Owned)
        .value("EXTERNALLY_OWNED", Trade::DataFlag::ExternallyOwned)
        .value("GLOBAL", Trade::DataFlag::Global)
        .value("MUTABLE", Trade::DataFlag::Mutable)
        .value("NONE", Trade::DataFlag{});
    corrade::enumOperators(dataFlag);

    py::enum_<Trade::MeshAttribute> meshAttribute{m, "MeshAttribute", "Mesh attribute name"};
    meshAttribute
        .value("POSITION", Trade::MeshAttribute::Position)
        .value("TANGENT", Trade::MeshAttribute::Tangent)
        .value("BITANGENT", Trade::MeshAttribute::Bitangent)
        .value("NORMAL", Trade::MeshAttribute::Normal)
        .value("TEXTURE_COORDINATES", Trade::MeshAttribute::TextureCoordinates)
        .value("COLOR", Trade::MeshAttribute::Color)
        .value("JOINT_IDS", Trade::MeshAttribute::JointIds)
        .value("WEIGHTS", Trade::MeshAttribute::Weights)
        .value("OBJECT_ID", Trade::MeshAttribute::ObjectId);
    enumWithCustomValues<Trade::MeshAttribute, Trade::Implementation::MeshAttributeCustom>(meshAttribute);

    py::class_<Trade::MeshData, Trade::PyDataHolder<Trade::MeshData>>{m, "MeshData", "Mesh data"}
        .def_property_readonly("primitive", &Trade::MeshData::primitive, "Primitive")
        .def_property_readonly("index_data_flags", [](Trade::MeshData& self) {
            return Trade::DataFlag(Containers::enumCastUnderlyingType(self.indexDataFlags()));
        }, "Index data flags")
        .def_property_readonly("vertex_data_flags", [](Trade::MeshData& self) {
            return Trade::DataFlag(Containers::enumCastUnderlyingType(self.vertexDataFlags()));
        }, "Vertex data flags")
        .def_property_readonly("index_data", [](Trade::MeshData& self) {
            return Containers::pyArrayViewHolder(self.indexData(), py::cast(self));
        }, "Raw index data")
        .def_property_readonly("mutable_index_data", [](Trade::MeshData& self) {
            if(!(self.indexDataFlags() & Trade::DataFlag::Mutable)) {
                PyErr_SetString(PyExc_AttributeError, "mesh index data is not mutable");
                throw py::error_already_set{};
            }
            return Containers::pyArrayViewHolder(self.mutableIndexData(), py::cast(self));
        }, "Mutable raw index data")
        /** @todo direct access to MeshAttributeData, once making custom
            MeshData is desired */
        .def_property_readonly("vertex_data", [](Trade::MeshData& self) {
            return Containers::pyArrayViewHolder(self.vertexData(), py::cast(self));
        }, "Raw vertex data")
        .def_property_readonly("mutable_vertex_data", [](Trade::MeshData& self) {
            if(!(self.vertexDataFlags() & Trade::DataFlag::Mutable)) {
                PyErr_SetString(PyExc_AttributeError, "mesh vertex data is not mutable");
                throw py::error_already_set{};
            }
            return Containers::pyArrayViewHolder(self.mutableVertexData(), py::cast(self));
        }, "Mutable raw vertex data")
        .def_property_readonly("is_indexed", &Trade::MeshData::isIndexed, "Whether the mesh is indexed")
        .def_property_readonly("index_count", [](Trade::MeshData& self) {
            if(!self.isIndexed()) {
                PyErr_SetString(PyExc_AttributeError, "mesh is not indexed");
                throw py::error_already_set{};
            }
            return self.indexCount();
        }, "Index count")
        .def_property_readonly("index_type", [](Trade::MeshData& self) {
            if(!self.isIndexed()) {
                PyErr_SetString(PyExc_AttributeError, "mesh is not indexed");
                throw py::error_already_set{};
            }
            return self.indexType();
        }, "Index type")
        .def_property_readonly("index_offset", [](Trade::MeshData& self) {
            if(!self.isIndexed()) {
                PyErr_SetString(PyExc_AttributeError, "mesh is not indexed");
                throw py::error_already_set{};
            }
            return self.indexOffset();
        }, "Index offset")
        .def_property_readonly("index_stride", [](Trade::MeshData& self) {
            if(!self.isIndexed()) {
                PyErr_SetString(PyExc_AttributeError, "mesh is not indexed");
                throw py::error_already_set{};
            }
            return self.indexStride();
        }, "Index stride")
        .def_property_readonly("indices", [](Trade::MeshData& self) {
            if(!self.isIndexed()) {
                PyErr_SetString(PyExc_AttributeError, "mesh is not indexed");
                throw py::error_already_set{};
            }
            return meshIndicesView(self, self.indices());
        }, "Indices")
        .def_property_readonly("mutable_indices", [](Trade::MeshData& self) {
            if(!self.isIndexed()) {
                PyErr_SetString(PyExc_AttributeError, "mesh is not indexed");
                throw py::error_already_set{};
            }
            if(!(self.indexDataFlags() & Trade::DataFlag::Mutable)) {
                PyErr_SetString(PyExc_AttributeError, "mesh index data is not mutable");
                throw py::error_already_set{};
            }
            return meshIndicesView(self, self.mutableIndices());
        }, "Mutable indices")
        .def_property_readonly("vertex_count", &Trade::MeshData::vertexCount, "Vertex count")
        /* Has to be a function instead of a property because there's an
           overload taking a name */
        .def("attribute_count", static_cast<UnsignedInt(Trade::MeshData::*)() const>(&Trade::MeshData::attributeCount), "Attribute array count")
        /** @todo direct access to MeshAttributeData, once making custom
            MeshData is desired */
        .def("has_attribute", &Trade::MeshData::hasAttribute, "Whether the mesh has given attribute", py::arg("name"))
        .def("attribute_count", static_cast<UnsignedInt(Trade::MeshData::*)(Trade::MeshAttribute) const>(&Trade::MeshData::attributeCount), "Count of given named attribute", py::arg("name"))

        /* IMPORTANT: due to pybind11 behavioral differences on (already EOL'd)
           Python 3.7 the following overloads need to have the MeshAttribute
           overload *before* the UnsignedInt overload, otherwise the integer
           overload gets picked even if an enum is passed from Python, causing
           massive suffering */
        .def("attribute_name", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self.attributeName(id);
        }, "Attribute name", py::arg("id"))
        .def("attribute_id", [](Trade::MeshData& self, Trade::MeshAttribute name, UnsignedInt id) {
            if(const Containers::Optional<UnsignedInt> found = self.findAttributeId(name, id))
                return *found;
            PyErr_SetNone(PyExc_KeyError);
            throw py::error_already_set{};
        }, "Absolute ID of a named attribute", py::arg("name"), py::arg("id") = 0)
        .def("attribute_id", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self.attributeId(id);
        }, "Attribute ID in a set of attributes of the same name", py::arg("id"))
        .def("attribute_format", [](Trade::MeshData& self, Trade::MeshAttribute name, UnsignedInt id) {
            if(const Containers::Optional<UnsignedInt> found = self.findAttributeId(name, id))
                return self.attributeFormat(*found);
            PyErr_SetNone(PyExc_KeyError);
            throw py::error_already_set{};
        }, "Format of a named attribute", py::arg("name"), py::arg("id") = 0)
        .def("attribute_format", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self.attributeFormat(id);
        }, "Attribute format", py::arg("id"))
        .def("attribute_offset", [](Trade::MeshData& self, Trade::MeshAttribute name, UnsignedInt id) {
            if(const Containers::Optional<UnsignedInt> found = self.findAttributeId(name, id))
                return self.attributeOffset(*found);
            PyErr_SetNone(PyExc_KeyError);
            throw py::error_already_set{};
        }, "Offset of a named attribute", py::arg("name"), py::arg("id") = 0)
        .def("attribute_offset", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self.attributeOffset(id);
        }, "Attribute offset", py::arg("id"))
        .def("attribute_stride", [](Trade::MeshData& self, Trade::MeshAttribute name, UnsignedInt id) {
            if(const Containers::Optional<UnsignedInt> found = self.findAttributeId(name, id))
                return self.attributeStride(*found);
            PyErr_SetNone(PyExc_KeyError);
            throw py::error_already_set{};
        }, "Stride of a named attribute", py::arg("name"), py::arg("id") = 0)
        .def("attribute_stride", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self.attributeStride(id);
        }, "Attribute stride", py::arg("id"))
        .def("attribute_array_size", [](Trade::MeshData& self, Trade::MeshAttribute name, UnsignedInt id) {
            if(const Containers::Optional<UnsignedInt> found = self.findAttributeId(name, id))
                return self.attributeArraySize(*found);
            PyErr_SetNone(PyExc_KeyError);
            throw py::error_already_set{};
        }, "Array size of a named attribute", py::arg("name"), py::arg("id") = 0)
        .def("attribute_array_size", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self.attributeArraySize(id);
        }, "Attribute array size", py::arg("id"))
        .def("attribute", [](Trade::MeshData& self, Trade::MeshAttribute name, UnsignedInt id) {
            const Containers::Optional<UnsignedInt> found = self.findAttributeId(name, id);
            if(!found) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            /** @todo handle arrays (return a 2D view, and especially annotate
                the return type properly in the docs) */
            if(self.attributeArraySize(*found) != 0) {
                PyErr_SetString(PyExc_NotImplementedError, "array attributes not implemented yet, sorry");
                throw py::error_already_set{};
            }
            return meshAttributeView(self, *found, self.attribute(*found));
        }, "Data for given named attribute", py::arg("name"), py::arg("id") = 0)
        .def("attribute", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            /** @todo handle arrays (return a 2D view, and especially annotate
                the return type properly in the docs) */
            if(self.attributeArraySize(id) != 0) {
                PyErr_SetString(PyExc_NotImplementedError, "array attributes not implemented yet, sorry");
                throw py::error_already_set{};
            }
            return meshAttributeView(self, id, self.attribute(id));
        }, "Data for given attribute", py::arg("id"))
        .def("mutable_attribute", [](Trade::MeshData& self, Trade::MeshAttribute name, UnsignedInt id) {
            const Containers::Optional<UnsignedInt> found = self.findAttributeId(name, id);
            if(!found) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            if(!(self.vertexDataFlags() & Trade::DataFlag::Mutable)) {
                PyErr_SetString(PyExc_AttributeError, "mesh vertex data is not mutable");
                throw py::error_already_set{};
            }
            /** @todo handle arrays (return a 2D view, and especially annotate
                the return type properly in the docs) */
            if(self.attributeArraySize(*found) != 0) {
                PyErr_SetString(PyExc_NotImplementedError, "array attributes not implemented yet, sorry");
                throw py::error_already_set{};
            }
            return meshAttributeView(self, *found, self.mutableAttribute(*found));
        }, "Mutable data for given named attribute", py::arg("name"), py::arg("id") = 0)
        .def("mutable_attribute", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            if(!(self.vertexDataFlags() & Trade::DataFlag::Mutable)) {
                PyErr_SetString(PyExc_AttributeError, "mesh vertex data is not mutable");
                throw py::error_already_set{};
            }
            /** @todo handle arrays (return a 2D view, and especially annotate
                the return type properly in the docs) */
            if(self.attributeArraySize(id) != 0) {
                PyErr_SetString(PyExc_NotImplementedError, "array attributes not implemented yet, sorry");
                throw py::error_already_set{};
            }
            return meshAttributeView(self, id, self.mutableAttribute(id));
        }, "Mutable data for given attribute", py::arg("id"))

        .def_property_readonly("owner", [](Trade::MeshData& self) {
            return pyObjectHolderFor<Trade::PyDataHolder>(self).owner;
        }, "Memory owner");

    py::class_<Trade::ImageData1D, Trade::PyDataHolder<Trade::ImageData1D>> imageData1D{m, "ImageData1D", "One-dimensional image data"};
    py::class_<Trade::ImageData2D, Trade::PyDataHolder<Trade::ImageData2D>> imageData2D{m, "ImageData2D", "Two-dimensional image data"};
    py::class_<Trade::ImageData3D, Trade::PyDataHolder<Trade::ImageData3D>> imageData3D{m, "ImageData3D", "Three-dimensional image data"};
    imageData(imageData1D);
    imageData(imageData2D);
    imageData(imageData3D);

    py::enum_<Trade::SceneMappingType>{m, "SceneMappingType", "Scene object mapping type"}
        .value("UNSIGNED_BYTE", Trade::SceneMappingType::UnsignedByte)
        .value("UNSIGNED_SHORT", Trade::SceneMappingType::UnsignedShort)
        .value("UNSIGNED_INT", Trade::SceneMappingType::UnsignedInt)
        .value("UNSIGNED_LONG", Trade::SceneMappingType::UnsignedLong);

    py::enum_<Trade::SceneField> sceneField{m, "SceneField", "Scene field name"};
    sceneField
        .value("PARENT", Trade::SceneField::Parent)
        .value("TRANSFORMATION", Trade::SceneField::Transformation)
        .value("TRANSLATION", Trade::SceneField::Translation)
        .value("ROTATION", Trade::SceneField::Rotation)
        .value("SCALING", Trade::SceneField::Scaling)
        .value("MESH", Trade::SceneField::Mesh)
        .value("MESH_MATERIAL", Trade::SceneField::MeshMaterial)
        .value("LIGHT", Trade::SceneField::Light)
        .value("CAMERA", Trade::SceneField::Camera)
        .value("SKIN", Trade::SceneField::Skin)
        .value("IMPORTER_STATE", Trade::SceneField::ImporterState);
    enumWithCustomValues<Trade::SceneField, Trade::Implementation::SceneFieldCustom>(sceneField);

    py::enum_<Trade::SceneFieldType>{m, "SceneFieldType", "Scene field type"}
        .value("BIT", Trade::SceneFieldType::Bit)
        .value("FLOAT", Trade::SceneFieldType::Float)
        .value("HALF", Trade::SceneFieldType::Half)
        .value("DOUBLE", Trade::SceneFieldType::Double)
        .value("UNSIGNED_BYTE", Trade::SceneFieldType::UnsignedByte)
        .value("BYTE", Trade::SceneFieldType::Byte)
        .value("UNSIGNED_SHORT", Trade::SceneFieldType::UnsignedShort)
        .value("SHORT", Trade::SceneFieldType::Short)
        .value("UNSIGNED_INT", Trade::SceneFieldType::UnsignedInt)
        .value("INT", Trade::SceneFieldType::Int)
        .value("UNSIGNED_LONG", Trade::SceneFieldType::UnsignedLong)
        .value("LONG", Trade::SceneFieldType::Long)
        .value("VECTOR2", Trade::SceneFieldType::Vector2)
        .value("VECTOR2H", Trade::SceneFieldType::Vector2h)
        .value("VECTOR2D", Trade::SceneFieldType::Vector2d)
        .value("VECTOR2UB", Trade::SceneFieldType::Vector2ub)
        .value("VECTOR2B", Trade::SceneFieldType::Vector2b)
        .value("VECTOR2US", Trade::SceneFieldType::Vector2us)
        .value("VECTOR2S", Trade::SceneFieldType::Vector2s)
        .value("VECTOR2UI", Trade::SceneFieldType::Vector2ui)
        .value("VECTOR2I", Trade::SceneFieldType::Vector2i)
        .value("VECTOR3", Trade::SceneFieldType::Vector3)
        .value("VECTOR3H", Trade::SceneFieldType::Vector3h)
        .value("VECTOR3D", Trade::SceneFieldType::Vector3d)
        .value("VECTOR3UB", Trade::SceneFieldType::Vector3ub)
        .value("VECTOR3B", Trade::SceneFieldType::Vector3b)
        .value("VECTOR3US", Trade::SceneFieldType::Vector3us)
        .value("VECTOR3S", Trade::SceneFieldType::Vector3s)
        .value("VECTOR3UI", Trade::SceneFieldType::Vector3ui)
        .value("VECTOR3I", Trade::SceneFieldType::Vector3i)
        .value("VECTOR4", Trade::SceneFieldType::Vector4)
        .value("VECTOR4H", Trade::SceneFieldType::Vector4h)
        .value("VECTOR4D", Trade::SceneFieldType::Vector4d)
        .value("VECTOR4UB", Trade::SceneFieldType::Vector4ub)
        .value("VECTOR4B", Trade::SceneFieldType::Vector4b)
        .value("VECTOR4US", Trade::SceneFieldType::Vector4us)
        .value("VECTOR4S", Trade::SceneFieldType::Vector4s)
        .value("VECTOR4UI", Trade::SceneFieldType::Vector4ui)
        .value("VECTOR4I", Trade::SceneFieldType::Vector4i)
        .value("MATRIX2X2", Trade::SceneFieldType::Matrix2x2)
        .value("MATRIX2X2H", Trade::SceneFieldType::Matrix2x2h)
        .value("MATRIX2X2D", Trade::SceneFieldType::Matrix2x2d)
        .value("MATRIX2X3", Trade::SceneFieldType::Matrix2x3)
        .value("MATRIX2X3H", Trade::SceneFieldType::Matrix2x3h)
        .value("MATRIX2X3D", Trade::SceneFieldType::Matrix2x3d)
        .value("MATRIX2X4", Trade::SceneFieldType::Matrix2x4)
        .value("MATRIX2X4H", Trade::SceneFieldType::Matrix2x4h)
        .value("MATRIX2X4D", Trade::SceneFieldType::Matrix2x4d)
        .value("MATRIX3X2", Trade::SceneFieldType::Matrix3x2)
        .value("MATRIX3X2H", Trade::SceneFieldType::Matrix3x2h)
        .value("MATRIX3X2D", Trade::SceneFieldType::Matrix3x2d)
        .value("MATRIX3X3", Trade::SceneFieldType::Matrix3x3)
        .value("MATRIX3X3H", Trade::SceneFieldType::Matrix3x3h)
        .value("MATRIX3X3D", Trade::SceneFieldType::Matrix3x3d)
        .value("MATRIX3X4", Trade::SceneFieldType::Matrix3x4)
        .value("MATRIX3X4H", Trade::SceneFieldType::Matrix3x4h)
        .value("MATRIX3X4D", Trade::SceneFieldType::Matrix3x4d)
        .value("MATRIX4X2", Trade::SceneFieldType::Matrix4x2)
        .value("MATRIX4X2H", Trade::SceneFieldType::Matrix4x2h)
        .value("MATRIX4X2D", Trade::SceneFieldType::Matrix4x2d)
        .value("MATRIX4X3", Trade::SceneFieldType::Matrix4x3)
        .value("MATRIX4X3H", Trade::SceneFieldType::Matrix4x3h)
        .value("MATRIX4X3D", Trade::SceneFieldType::Matrix4x3d)
        .value("MATRIX4X4", Trade::SceneFieldType::Matrix4x4)
        .value("MATRIX4X4H", Trade::SceneFieldType::Matrix4x4h)
        .value("MATRIX4X4D", Trade::SceneFieldType::Matrix4x4d)
        .value("RANGE1D", Trade::SceneFieldType::Range1D)
        .value("RANGE1DH", Trade::SceneFieldType::Range1Dh)
        .value("RANGE1DD", Trade::SceneFieldType::Range1Dd)
        .value("RANGE1DI", Trade::SceneFieldType::Range1Di)
        .value("RANGE2D", Trade::SceneFieldType::Range2D)
        .value("RANGE2DH", Trade::SceneFieldType::Range2Dh)
        .value("RANGE2DD", Trade::SceneFieldType::Range2Dd)
        .value("RANGE2DI", Trade::SceneFieldType::Range2Di)
        .value("RANGE3D", Trade::SceneFieldType::Range3D)
        .value("RANGE3DH", Trade::SceneFieldType::Range3Dh)
        .value("RANGE3DD", Trade::SceneFieldType::Range3Dd)
        .value("RANGE3DI", Trade::SceneFieldType::Range3Di)
        .value("COMPLEX", Trade::SceneFieldType::Complex)
        .value("COMPLEXD", Trade::SceneFieldType::Complexd)
        .value("DUAL_COMPLEX", Trade::SceneFieldType::DualComplex)
        .value("DUAL_COMPLEXD", Trade::SceneFieldType::DualComplexd)
        .value("QUATERNION", Trade::SceneFieldType::Quaternion)
        .value("QUATERNIOND", Trade::SceneFieldType::Quaterniond)
        .value("DUAL_QUATERNION", Trade::SceneFieldType::DualQuaternion)
        .value("DUAL_QUATERNIOND", Trade::SceneFieldType::DualQuaterniond)
        .value("DEG", Trade::SceneFieldType::Deg)
        .value("DEGH", Trade::SceneFieldType::Degh)
        .value("DEGD", Trade::SceneFieldType::Degd)
        .value("RAD", Trade::SceneFieldType::Rad)
        .value("RADH", Trade::SceneFieldType::Radh)
        .value("RADD", Trade::SceneFieldType::Radd)
        .value("POINTER", Trade::SceneFieldType::Pointer)
        .value("MUTABLE_POINTER", Trade::SceneFieldType::MutablePointer)
        .value("STRING_OFFSET32", Trade::SceneFieldType::StringOffset32)
        .value("STRING_OFFSET8", Trade::SceneFieldType::StringOffset8)
        .value("STRING_OFFSET16", Trade::SceneFieldType::StringOffset16)
        .value("STRING_OFFSET64", Trade::SceneFieldType::StringOffset64)
        .value("STRING_RANGE32", Trade::SceneFieldType::StringRange32)
        .value("STRING_RANGE8", Trade::SceneFieldType::StringRange8)
        .value("STRING_RANGE16", Trade::SceneFieldType::StringRange16)
        .value("STRING_RANGE64", Trade::SceneFieldType::StringRange64)
        .value("STRING_RANGE_NULL_TERMINATED32", Trade::SceneFieldType::StringRangeNullTerminated32)
        .value("STRING_RANGE_NULL_TERMINATED8", Trade::SceneFieldType::StringRangeNullTerminated8)
        .value("STRING_RANGE_NULL_TERMINATED16", Trade::SceneFieldType::StringRangeNullTerminated16)
        .value("STRING_RANGE_NULL_TERMINATED64", Trade::SceneFieldType::StringRangeNullTerminated64);

    py::enum_<Trade::SceneFieldFlag> sceneFieldFlag{m, "SceneFieldFlags", "Scene field flags"};
    sceneFieldFlag
        .value("OFFSET_ONLY", Trade::SceneFieldFlag::OffsetOnly)
        .value("ORDERED_MAPPING", Trade::SceneFieldFlag::OrderedMapping)
        .value("IMPLICIT_MAPPING", Trade::SceneFieldFlag::ImplicitMapping)
        .value("NULL_TERMINATED_STRING", Trade::SceneFieldFlag::NullTerminatedString)
        .value("NONE", Trade::SceneFieldFlag{});
    corrade::enumOperators(sceneFieldFlag);

    py::class_<Trade::SceneData, Trade::PyDataHolder<Trade::SceneData>>{m, "SceneData", "Scene data"}
        .def_property_readonly("data_flags", [](Trade::SceneData& self) {
            return Trade::DataFlag(Containers::enumCastUnderlyingType(self.dataFlags()));
        }, "Data flags")
        /** @todo expose raw data at all? compared to meshes there's no use
            case like being able to pass raw memory to the GPU ... yet */
        .def_property_readonly("mapping_type", &Trade::SceneData::mappingType, "Type used for object mapping")
        .def_property_readonly("mapping_bound", &Trade::SceneData::mappingBound, "Object mapping bound")
        .def_property_readonly("field_count", &Trade::SceneData::fieldCount, "Field count")
        .def_property_readonly("field_size_bound", &Trade::SceneData::fieldSizeBound, "Field size bound")
        .def_property_readonly("is_2d", &Trade::SceneData::is2D, "Whether the scene is two-dimensional")
        .def_property_readonly("is_3d", &Trade::SceneData::is3D, "Whether the scene is three-dimensional")

        /* IMPORTANT: due to pybind11 behavioral differences on (already EOL'd)
           Python 3.7 the following overloads need to have the SceneField
           overload *before* the UnsignedInt overload, otherwise the integer
           overload gets picked even if an enum is passed from Python, causing
           massive suffering */
        .def("field_name", [](Trade::SceneData& self, UnsignedInt id) {
            if(id >= self.fieldCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self.fieldName(id);
        }, "Field name", py::arg("id"))
        .def("field_flags", [](Trade::SceneData& self, Trade::SceneField fieldName) {
            const Containers::Optional<UnsignedInt> foundField = self.findFieldId(fieldName);
            if(!foundField) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            return Trade::SceneFieldFlag(Containers::enumCastUnderlyingType(self.fieldFlags(*foundField)));
        }, "Flags of a named field", py::arg("name"))
        .def("field_flags", [](Trade::SceneData& self, UnsignedInt id) {
            if(id >= self.fieldCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return Trade::SceneFieldFlag(Containers::enumCastUnderlyingType(self.fieldFlags(id)));
        }, "Field flags", py::arg("id"))
        .def("field_type", [](Trade::SceneData& self, Trade::SceneField fieldName) {
            const Containers::Optional<UnsignedInt> foundField = self.findFieldId(fieldName);
            if(!foundField) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            return self.fieldType(*foundField);
        }, "Type of a named field", py::arg("name"))
        .def("field_type", [](Trade::SceneData& self, UnsignedInt id) {
            if(id >= self.fieldCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self.fieldType(id);
        }, "Field type", py::arg("id"))
        .def("field_size", [](Trade::SceneData& self, Trade::SceneField fieldName) {
            const Containers::Optional<UnsignedInt> foundField = self.findFieldId(fieldName);
            if(!foundField) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            return self.fieldSize(*foundField);
        }, "Number of entries in a named field", py::arg("name"))
        .def("field_size", [](Trade::SceneData& self, UnsignedInt id) {
            if(id >= self.fieldCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self.fieldSize(id);
        }, "Number of entries in a field", py::arg("id"))
        .def("field_array_size", [](Trade::SceneData& self, Trade::SceneField fieldName) {
            const Containers::Optional<UnsignedInt> foundField = self.findFieldId(fieldName);
            if(!foundField) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            return self.fieldArraySize(*foundField);
        }, "Array size of a named field", py::arg("name"))
        .def("field_array_size", [](Trade::SceneData& self, UnsignedInt id) {
            if(id >= self.fieldCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self.fieldArraySize(id);
        }, "Field array size", py::arg("id"))
        .def("field_id", [](Trade::SceneData& self, Trade::SceneField name) {
            if(const Containers::Optional<UnsignedInt> found = self.findFieldId(name))
                return *found;
            PyErr_SetNone(PyExc_KeyError);
            throw py::error_already_set{};
        }, "Absolute ID of a named field", py::arg("name"))
        .def("has_field", &Trade::SceneData::hasField, "Whether the scene has given field")
        .def("field_object_offset", [](Trade::SceneData& self, Trade::SceneField fieldName, UnsignedLong object, std::size_t offset) {
            const Containers::Optional<UnsignedInt> foundField = self.findFieldId(fieldName);
            if(!foundField) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            if(object >= self.mappingBound()) {
                PyErr_SetString(PyExc_IndexError, "object out of range");
                throw py::error_already_set{};
            }
            if(offset >= self.fieldSize(*foundField)) {
                PyErr_SetString(PyExc_IndexError, "offset out of range");
                throw py::error_already_set{};
            }
            const Containers::Optional<std::size_t> found = self.findFieldObjectOffset(*foundField, object, offset);
            if(!found) {
                PyErr_SetNone(PyExc_LookupError);
                throw py::error_already_set{};
            }
            return *found;
        }, "Offset of an object in given name field", py::arg("field_name"), py::arg("object"), py::arg("offset") = 0)
        .def("field_object_offset", [](Trade::SceneData& self, UnsignedInt fieldId, UnsignedLong object, std::size_t offset) {
            if(fieldId >= self.fieldCount()) {
                PyErr_SetString(PyExc_IndexError, "field out of range");
                throw py::error_already_set{};
            }
            if(object >= self.mappingBound()) {
                PyErr_SetString(PyExc_IndexError, "object out of range");
                throw py::error_already_set{};
            }
            if(offset >= self.fieldSize(fieldId)) {
                PyErr_SetString(PyExc_IndexError, "offset out of range");
                throw py::error_already_set{};
            }
            const Containers::Optional<std::size_t> found = self.findFieldObjectOffset(fieldId, object, offset);
            if(!found) {
                PyErr_SetNone(PyExc_LookupError);
                throw py::error_already_set{};
            }
            return *found;
        }, "Offset of an object in given field", py::arg("field_id"), py::arg("object"), py::arg("offset") = 0)
        .def("has_field_object", [](Trade::SceneData& self, Trade::SceneField fieldName, UnsignedLong object) {
            const Containers::Optional<UnsignedInt> foundField = self.findFieldId(fieldName);
            if(!foundField) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            if(object >= self.mappingBound()) {
                PyErr_SetString(PyExc_IndexError, "object out of range");
                throw py::error_already_set{};
            }
            return self.hasFieldObject(*foundField, object);
        }, "Whether a scene field has given object", py::arg("field_name"), py::arg("object"))
        .def("has_field_object", [](Trade::SceneData& self, UnsignedInt fieldId, UnsignedLong object) {
            if(fieldId >= self.fieldCount()) {
                PyErr_SetString(PyExc_IndexError, "field out of range");
                throw py::error_already_set{};
            }
            if(object >= self.mappingBound()) {
                PyErr_SetString(PyExc_IndexError, "object out of range");
                throw py::error_already_set{};
            }
            return self.hasFieldObject(fieldId, object);
        }, "Whether a scene field has given object", py::arg("field_id"), py::arg("object"))
        .def("mapping", [](Trade::SceneData& self, Trade::SceneField name) {
            const Containers::Optional<UnsignedInt> found = self.findFieldId(name);
            if(!found) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            return sceneMappingView(self, self.mapping(*found));
        }, "Object mapping data for given named field", py::arg("name"))
        .def("mapping", [](Trade::SceneData& self, UnsignedInt id) {
            if(id >= self.fieldCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return sceneMappingView(self, self.mapping(id));
        }, "Object mapping data for given field", py::arg("name"))
        .def("mutable_mapping", [](Trade::SceneData& self, Trade::SceneField name) {
            const Containers::Optional<UnsignedInt> found = self.findFieldId(name);
            if(!found) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            if(!(self.dataFlags() & Trade::DataFlag::Mutable)) {
                PyErr_SetString(PyExc_AttributeError, "scene data is not mutable");
                throw py::error_already_set{};
            }
            return sceneMappingView(self, self.mutableMapping(*found));
        }, "Mutable object mapping data for given named field", py::arg("name"))
        .def("mutable_mapping", [](Trade::SceneData& self, UnsignedInt id) {
            if(id >= self.fieldCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            if(!(self.dataFlags() & Trade::DataFlag::Mutable)) {
                PyErr_SetString(PyExc_AttributeError, "scene data is not mutable");
                throw py::error_already_set{};
            }
            return sceneMappingView(self, self.mutableMapping(id));
        }, "Mutable object mapping data for given field", py::arg("name"))
        .def("field", [](Trade::SceneData& self, Trade::SceneField name) {
            const Containers::Optional<UnsignedInt> found = self.findFieldId(name);
            if(!found) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            /** @todo handle arrays (return a 2D (bit) view) */
            if(self.fieldArraySize(*found) != 0) {
                PyErr_SetString(PyExc_NotImplementedError, "array fields not implemented yet, sorry");
                throw py::error_already_set{};
            }
            /** @todo annotate the return type properly in the docs */
            if(self.fieldType(*found) == Trade::SceneFieldType::Bit)
                return pyCastButNotShitty(Containers::pyArrayViewHolder(self.fieldBits(*found), py::cast(self)));
            return pyCastButNotShitty(sceneFieldView(self, *found, self.field(*found)));
        }, "Data for given named field", py::arg("name"))
        .def("field", [](Trade::SceneData& self, UnsignedInt id) {
            if(id >= self.fieldCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            /** @todo handle arrays (return a 2D (bit) view) */
            if(self.fieldArraySize(id) != 0) {
                PyErr_SetString(PyExc_NotImplementedError, "array fields not implemented yet, sorry");
                throw py::error_already_set{};
            }
            /** @todo annotate the return type properly in the docs */
            if(self.fieldType(id) == Trade::SceneFieldType::Bit)
                return pyCastButNotShitty(Containers::pyArrayViewHolder(self.fieldBits(id), py::cast(self)));
            return pyCastButNotShitty(sceneFieldView(self, id, self.field(id)));
        }, "Data for given field", py::arg("name"))
        .def("mutable_field", [](Trade::SceneData& self, Trade::SceneField name) {
            const Containers::Optional<UnsignedInt> found = self.findFieldId(name);
            if(!found) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            if(!(self.dataFlags() & Trade::DataFlag::Mutable)) {
                PyErr_SetString(PyExc_AttributeError, "scene data is not mutable");
                throw py::error_already_set{};
            }
            /** @todo handle arrays (return a 2D (bit) view) */
            if(self.fieldArraySize(*found) != 0) {
                PyErr_SetString(PyExc_NotImplementedError, "array fields not implemented yet, sorry");
                throw py::error_already_set{};
            }
            /** @todo annotate the return type properly in the docs */
            if(self.fieldType(*found) == Trade::SceneFieldType::Bit)
                return pyCastButNotShitty(Containers::pyArrayViewHolder(self.mutableFieldBits(*found), py::cast(self)));
            return pyCastButNotShitty(sceneFieldView(self, *found, self.mutableField(*found)));
        }, "Mutable data for given named field", py::arg("name"))
        .def("mutable_field", [](Trade::SceneData& self, UnsignedInt id) {
            if(id >= self.fieldCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            if(!(self.dataFlags() & Trade::DataFlag::Mutable)) {
                PyErr_SetString(PyExc_AttributeError, "scene data is not mutable");
                throw py::error_already_set{};
            }
            /** @todo handle arrays (return a 2D (bit) view) */
            if(self.fieldArraySize(id) != 0) {
                PyErr_SetString(PyExc_NotImplementedError, "array fields not implemented yet, sorry");
                throw py::error_already_set{};
            }
            /** @todo annotate the return type properly in the docs */
            if(self.fieldType(id) == Trade::SceneFieldType::Bit)
                return pyCastButNotShitty(Containers::pyArrayViewHolder(self.mutableFieldBits(id), py::cast(self)));
            return pyCastButNotShitty(sceneFieldView(self, id, self.mutableField(id)));
        }, "Mutable data for given field", py::arg("name"))

        .def_property_readonly("owner", [](Trade::SceneData& self) {
            return pyObjectHolderFor<Trade::PyDataHolder>(self).owner;
        }, "Memory owner");

    py::enum_<Trade::TextureType>{m, "TextureType", "Texture type"}
        .value("TEXTURE1D", Trade::TextureType::Texture1D)
        .value("TEXTURE1D_ARRAY", Trade::TextureType::Texture1DArray)
        .value("TEXTURE2D", Trade::TextureType::Texture2D)
        .value("TEXTURE2D_ARRAY", Trade::TextureType::Texture2DArray)
        .value("TEXTURE3D", Trade::TextureType::Texture3D)
        .value("CUBE_MAP", Trade::TextureType::CubeMap)
        .value("CUBE_MAP_ARRAY", Trade::TextureType::CubeMapArray);

    py::class_<Trade::TextureData>{m, "TextureData", "Texture data"}
        .def_property_readonly("type", &Trade::TextureData::type, "Texture type")
        .def_property_readonly("minification_filter", &Trade::TextureData::minificationFilter, "Minification filter")
        .def_property_readonly("magnification_filter", &Trade::TextureData::magnificationFilter, "Magnification filter")
        .def_property_readonly("mipmap_filter", &Trade::TextureData::mipmapFilter, "Mipmap filter")
        .def_property_readonly("wrapping", [](Trade::TextureData& self) {
            return std::make_tuple(
                self.wrapping()[0],
                self.wrapping()[1],
                self.wrapping()[2]
            );
        }, "Wrapping")
        .def_property_readonly("image", &Trade::TextureData::image, "Image ID");

    /* Importer. Skipping file callbacks and openState as those operate with
       void*. Leaving the name as AbstractImporter (instead of Importer) to
       avoid needless name differences and because in the future there *might*
       be pure Python importers (not now tho). */
    py::enum_<Trade::ImporterFeature> importerFeatures{m, "ImporterFeatures", "Features supported by an image converter"};
    importerFeatures
        .value("OPEN_DATA", Trade::ImporterFeature::OpenData)
        .value("OPEN_STATE", Trade::ImporterFeature::OpenState)
        .value("FILE_CALLBACK", Trade::ImporterFeature::FileCallback)
        .value("NONE", Trade::ImporterFeature{});
    corrade::enumOperators(importerFeatures);

    py::enum_<Trade::ImporterFlag> importerFlags{m, "ImporterFlags", "Importer flags"};
    importerFlags
        .value("QUIET", Trade::ImporterFlag::Quiet)
        .value("VERBOSE", Trade::ImporterFlag::Verbose)
        .value("NONE", Trade::ImporterFlag{});
    corrade::enumOperators(importerFlags);

    py::class_<Trade::AbstractImporter, PluginManager::PyPluginHolder<Trade::AbstractImporter>, PluginManager::AbstractPlugin> abstractImporter{m, "AbstractImporter", "Interface for importer plugins"};
    corrade::plugin(abstractImporter);
    abstractImporter
        .def_property_readonly("features", [](Trade::AbstractImporter& self) {
            return Trade::ImporterFeature(Containers::enumCastUnderlyingType(self.features()));
        }, "Features supported by this importer")
        .def_property("flags", [](Trade::AbstractImporter& self) {
            return Trade::ImporterFlag(Containers::enumCastUnderlyingType(self.flags()));
        }, [](Trade::AbstractImporter& self, Trade::ImporterFlag flags) {
            self.setFlags(flags);
        }, "Importer flags")
        .def_property_readonly("is_opened", &Trade::AbstractImporter::isOpened, "Whether any file is opened")
        .def("open_data", [](Trade::AbstractImporter& self, Containers::ArrayView<const char> data) {
            if(!(self.features() >= Trade::ImporterFeature::OpenData)) {
                PyErr_SetString(PyExc_AssertionError, "feature not supported");
                throw py::error_already_set{};
            }

            /** @todo log redirection -- but we'd need assertions to not be
                part of that so when it dies, the user can still see why */
            if(self.openData(data)) return;

            PyErr_SetString(PyExc_RuntimeError, "opening data failed");
            throw py::error_already_set{};
        }, "Open raw data", py::arg("data"))
        /** @todo drop std::string in favor of our own string caster */
        .def("open_file", [](Trade::AbstractImporter& self, const std::string& filename) {
            /** @todo log redirection -- but we'd need assertions to not be
                part of that so when it dies, the user can still see why */
            if(self.openFile(
                #ifdef CORRADE_TARGET_WINDOWS
                /* To allow people to conveniently use Python's os.path, we
                   need to convert backslashes to forward slashes as all
                   Corrade and Magnum APIs expect forward */
                Utility::Path::fromNativeSeparators(filename)
                #else
                filename
                #endif
            )) return;

            PyErr_Format(PyExc_RuntimeError, "opening %s failed", filename.data());
            throw py::error_already_set{};
        }, "Open a file", py::arg("filename"))
        .def("close", &Trade::AbstractImporter::close, "Close currently opened file")

        .def_property_readonly("default_scene", checkOpened<Int, &Trade::AbstractImporter::defaultScene>, "Default scene")
        .def_property_readonly("scene_count", checkOpened<UnsignedInt, &Trade::AbstractImporter::sceneCount>, "Scene count")
        .def_property_readonly("object_count", checkOpened<UnsignedLong, &Trade::AbstractImporter::objectCount>, "Object count")
        .def("scene_for_name", checkOpenedString<Int, &Trade::AbstractImporter::sceneForName>, "Scene ID for given name", py::arg("name"))
        .def("object_for_name", checkOpenedString<Long, &Trade::AbstractImporter::objectForName>, "Object ID for given name", py::arg("name"))
        .def("scene_name", checkOpenedBoundsReturnsString<UnsignedInt, &Trade::AbstractImporter::sceneName, &Trade::AbstractImporter::sceneCount>, "Scene name", py::arg("id"))
        .def("object_name", checkOpenedBoundsReturnsString<UnsignedLong, &Trade::AbstractImporter::objectName, &Trade::AbstractImporter::objectCount>, "Scene name", py::arg("id"))
        .def("scene", checkOpenedBoundsResult<Trade::SceneData, &Trade::AbstractImporter::scene, &Trade::AbstractImporter::sceneCount>, "Scene", py::arg("id"))
        .def("scene", checkOpenedBoundsResultString<Trade::SceneData, &Trade::AbstractImporter::scene, &Trade::AbstractImporter::sceneForName>, "Scene for given name", py::arg("name"))
        /** @todo drop std::string in favor of our own string caster */
        .def("scene_field_for_name", [](Trade::AbstractImporter& self, const std::string& name) -> Containers::Optional<Trade::SceneField> {
            const Trade::SceneField field = self.sceneFieldForName(name);
            if(field == Trade::SceneField{})
                return {};
            return field;
        }, "Scene field for given name", py::arg("name"))
        /** @todo drop std::string in favor of our own string caster */
        .def("scene_field_name", [](Trade::AbstractImporter& self, Trade::SceneField name) -> Containers::Optional<std::string> {
            if(const Containers::String field = self.sceneFieldName(name))
                return std::string{field};
            return {};
        }, "String name for given custom scene field", py::arg("name"))

        /** @todo all other data types */
        .def_property_readonly("mesh_count", checkOpened<UnsignedInt, &Trade::AbstractImporter::meshCount>, "Mesh count")
        .def("mesh_level_count", checkOpenedBounds<UnsignedInt, &Trade::AbstractImporter::meshLevelCount, &Trade::AbstractImporter::meshCount>, "Mesh level count", py::arg("id"))
        .def("mesh_for_name", checkOpenedString<Int, &Trade::AbstractImporter::meshForName>, "Mesh ID for given name", py::arg("name"))
        .def("mesh_name", checkOpenedBoundsReturnsString<UnsignedInt, &Trade::AbstractImporter::meshName, &Trade::AbstractImporter::meshCount>, "Mesh name", py::arg("id"))
        .def("mesh", checkOpenedBoundsResult<Trade::MeshData, &Trade::AbstractImporter::mesh, &Trade::AbstractImporter::meshCount, &Trade::AbstractImporter::meshLevelCount>, "Mesh", py::arg("id"), py::arg("level") = 0)
        .def("mesh", checkOpenedBoundsResultString<Trade::MeshData, &Trade::AbstractImporter::mesh, &Trade::AbstractImporter::meshForName, &Trade::AbstractImporter::meshLevelCount>, "Mesh for given name", py::arg("name"), py::arg("level") = 0)
        /** @todo drop std::string in favor of our own string caster */
        .def("mesh_attribute_for_name", [](Trade::AbstractImporter& self, const std::string& name) -> Containers::Optional<Trade::MeshAttribute> {
            const Trade::MeshAttribute attribute = self.meshAttributeForName(name);
            if(attribute == Trade::MeshAttribute{})
                return {};
            return attribute;
        }, "Mesh attribute for given name", py::arg("name"))
        /** @todo drop std::string in favor of our own string caster */
        .def("mesh_attribute_name", [](Trade::AbstractImporter& self, Trade::MeshAttribute name) -> Containers::Optional<std::string> {
            if(const Containers::String attribute = self.meshAttributeName(name))
                return std::string{attribute};
            return {};
        }, "String name for given custom mesh attribute", py::arg("name"))

        .def_property_readonly("texture_count", checkOpened<UnsignedInt, &Trade::AbstractImporter::textureCount>, "Texture count")
        .def("texture_for_name", checkOpenedString<Int, &Trade::AbstractImporter::textureForName>, "Texture ID for given name", py::arg("name"))
        .def("texture_name", checkOpenedBoundsReturnsString<UnsignedInt, &Trade::AbstractImporter::textureName, &Trade::AbstractImporter::textureCount>, "Texture name", py::arg("id"))
        .def("texture", checkOpenedBoundsResult<Trade::TextureData, &Trade::AbstractImporter::texture, &Trade::AbstractImporter::textureCount>, "Texture", py::arg("id"))
        .def("texture", checkOpenedBoundsResultString<Trade::TextureData, &Trade::AbstractImporter::texture, &Trade::AbstractImporter::textureForName>, "Texture for given name", py::arg("name"))

        .def_property_readonly("image1d_count", checkOpened<UnsignedInt, &Trade::AbstractImporter::image1DCount>, "One-dimensional image count")
        .def_property_readonly("image2d_count", checkOpened<UnsignedInt, &Trade::AbstractImporter::image2DCount>, "Two-dimensional image count")
        .def_property_readonly("image3d_count", checkOpened<UnsignedInt, &Trade::AbstractImporter::image3DCount>, "Three-dimensional image count")
        .def("image1d_level_count", checkOpenedBounds<UnsignedInt, &Trade::AbstractImporter::image1DLevelCount, &Trade::AbstractImporter::image1DCount>, "One-dimensional image level count", py::arg("id"))
        .def("image2d_level_count", checkOpenedBounds<UnsignedInt, &Trade::AbstractImporter::image2DLevelCount, &Trade::AbstractImporter::image2DCount>, "Two-dimensional image level count", py::arg("id"))
        .def("image3d_level_count", checkOpenedBounds<UnsignedInt, &Trade::AbstractImporter::image3DLevelCount, &Trade::AbstractImporter::image3DCount>, "Three-dimensional image level count", py::arg("id"))
        .def("image1d_for_name", checkOpenedString<Int, &Trade::AbstractImporter::image1DForName>, "One-dimensional image ID for given name", py::arg("name"))
        .def("image2d_for_name", checkOpenedString<Int, &Trade::AbstractImporter::image2DForName>, "Two-dimensional image ID for given name", py::arg("name"))
        .def("image3d_for_name", checkOpenedString<Int, &Trade::AbstractImporter::image3DForName>, "Three-dimensional image ID for given name", py::arg("name"))
        .def("image1d_name", checkOpenedBoundsReturnsString<UnsignedInt, &Trade::AbstractImporter::image1DName, &Trade::AbstractImporter::image1DCount>, "One-dimensional image name", py::arg("id"))
        .def("image2d_name", checkOpenedBoundsReturnsString<UnsignedInt, &Trade::AbstractImporter::image2DName, &Trade::AbstractImporter::image2DCount>, "Two-dimensional image name", py::arg("id"))
        .def("image3d_name", checkOpenedBoundsReturnsString<UnsignedInt, &Trade::AbstractImporter::image3DName, &Trade::AbstractImporter::image3DCount>, "Three-dimensional image name", py::arg("id"))
        .def("image1d", checkOpenedBoundsResult<Trade::ImageData1D, &Trade::AbstractImporter::image1D, &Trade::AbstractImporter::image1DCount, &Trade::AbstractImporter::image1DLevelCount>, "One-dimensional image", py::arg("id"), py::arg("level") = 0)
        .def("image1d", checkOpenedBoundsResultString<Trade::ImageData1D, &Trade::AbstractImporter::image1D, &Trade::AbstractImporter::image1DForName, &Trade::AbstractImporter::image1DLevelCount>, "One-dimensional image for given name", py::arg("name"), py::arg("level") = 0)
        .def("image2d", checkOpenedBoundsResult<Trade::ImageData2D, &Trade::AbstractImporter::image2D, &Trade::AbstractImporter::image2DCount, &Trade::AbstractImporter::image2DLevelCount>, "Two-dimensional image", py::arg("id"), py::arg("level") = 0)
        .def("image2d", checkOpenedBoundsResultString<Trade::ImageData2D, &Trade::AbstractImporter::image2D, &Trade::AbstractImporter::image2DForName, &Trade::AbstractImporter::image2DLevelCount>, "Two-dimensional image for given name", py::arg("name"), py::arg("level") = 0)
        .def("image3d", checkOpenedBoundsResult<Trade::ImageData3D, &Trade::AbstractImporter::image3D, &Trade::AbstractImporter::image3DCount, &Trade::AbstractImporter::image3DLevelCount>, "Three-dimensional image", py::arg("id"), py::arg("level") = 0)
        .def("image3d", checkOpenedBoundsResultString<Trade::ImageData3D, &Trade::AbstractImporter::image3D, &Trade::AbstractImporter::image3DForName, &Trade::AbstractImporter::image3DLevelCount>, "Threee-dimensional image for given name", py::arg("name"), py::arg("level") = 0);

    py::class_<PluginManager::Manager<Trade::AbstractImporter>, PluginManager::AbstractManager> importerManager{m, "ImporterManager", "Manager for importer plugins"};
    corrade::manager(importerManager);

    /* Image converter */
    py::enum_<Trade::ImageConverterFeature> imageConverterFeatures{m, "ImageConverterFeatures", "Features supported by an image converter"};
    imageConverterFeatures
        .value("CONVERT1D", Trade::ImageConverterFeature::Convert1D)
        .value("CONVERT2D", Trade::ImageConverterFeature::Convert2D)
        .value("CONVERT3D", Trade::ImageConverterFeature::Convert3D)
        .value("CONVERT_COMPRESSED1D", Trade::ImageConverterFeature::ConvertCompressed1D)
        .value("CONVERT_COMPRESSED2D", Trade::ImageConverterFeature::ConvertCompressed2D)
        .value("CONVERT_COMPRESSED3D", Trade::ImageConverterFeature::ConvertCompressed3D)
        .value("CONVERT1D_TO_FILE", Trade::ImageConverterFeature::Convert1DToFile)
        .value("CONVERT2D_TO_FILE", Trade::ImageConverterFeature::Convert2DToFile)
        .value("CONVERT3D_TO_FILE", Trade::ImageConverterFeature::Convert3DToFile)
        .value("CONVERT_COMPRESSED1D_TO_FILE", Trade::ImageConverterFeature::ConvertCompressed1DToFile)
        .value("CONVERT_COMPRESSED2D_TO_FILE", Trade::ImageConverterFeature::ConvertCompressed2DToFile)
        .value("CONVERT_COMPRESSED3D_TO_FILE", Trade::ImageConverterFeature::ConvertCompressed3DToFile)
        .value("CONVERT1D_TO_DATA", Trade::ImageConverterFeature::Convert1DToData)
        .value("CONVERT2D_TO_DATA", Trade::ImageConverterFeature::Convert2DToData)
        .value("CONVERT3D_TO_DATA", Trade::ImageConverterFeature::Convert3DToData)
        .value("CONVERT_COMPRESSED1D_TO_DATA", Trade::ImageConverterFeature::ConvertCompressed1DToData)
        .value("CONVERT_COMPRESSED2D_TO_DATA", Trade::ImageConverterFeature::ConvertCompressed2DToData)
        .value("CONVERT_COMPRESSED3D_TO_DATA", Trade::ImageConverterFeature::ConvertCompressed3DToData)
        .value("LEVELS", Trade::ImageConverterFeature::Levels)
        .value("NONE", Trade::ImageConverterFeature{});
    corrade::enumOperators(imageConverterFeatures);

    py::enum_<Trade::ImageConverterFlag> imageConverterFlags{m, "ImageConverterFlags", "Image converter flags"};
    imageConverterFlags
        .value("QUIET", Trade::ImageConverterFlag::Quiet)
        .value("VERBOSE", Trade::ImageConverterFlag::Verbose)
        .value("NONE", Trade::ImageConverterFlag{});
    corrade::enumOperators(imageConverterFlags);

    py::class_<Trade::AbstractImageConverter, PluginManager::PyPluginHolder<Trade::AbstractImageConverter>, PluginManager::AbstractPlugin> abstractImageConverter{m, "AbstractImageConverter", "Interface for image converter plugins"};
    abstractImageConverter
        .def_property_readonly("features", [](Trade::AbstractImageConverter& self) {
            return Trade::ImageConverterFeature(Containers::enumCastUnderlyingType(self.features()));
        }, "Features supported by this converter")
        .def_property("flags", [](Trade::AbstractImageConverter& self) {
            return Trade::ImageConverterFlag(Containers::enumCastUnderlyingType(self.flags()));
        }, [](Trade::AbstractImageConverter& self, Trade::ImageConverterFlag flags) {
            self.setFlags(flags);
        }, "Converter flags")
        .def("convert_to_file", checkImageConverterResult<ImageView1D, &Trade::AbstractImageConverter::convertToFile>, "Convert a 1D image to a file", py::arg("image"), py::arg("filename"))
        .def("convert_to_file", checkImageConverterResult<ImageView2D, &Trade::AbstractImageConverter::convertToFile>, "Convert a 2D image to a file", py::arg("image"), py::arg("filename"))
        .def("convert_to_file", checkImageConverterResult<ImageView3D, &Trade::AbstractImageConverter::convertToFile>, "Convert a 3D image to a file", py::arg("image"), py::arg("filename"));
    corrade::plugin(abstractImageConverter);

    py::class_<PluginManager::Manager<Trade::AbstractImageConverter>, PluginManager::AbstractManager> imageConverterManager{m, "ImageConverterManager", "Manager for image converter plugins"};
    corrade::manager(imageConverterManager);

    /* Scene converter */
    py::enum_<Trade::SceneConverterFeature> sceneConverterFeatures{m, "SceneConverterFeatures", "Features supported by a scene converter"};
    sceneConverterFeatures
        .value("CONVERT_MESH", Trade::SceneConverterFeature::ConvertMesh)
        .value("CONVERT_MESH_IN_PLACE", Trade::SceneConverterFeature::ConvertMeshInPlace)
        .value("CONVERT_MESH_TO_FILE", Trade::SceneConverterFeature::ConvertMeshToFile)
        .value("CONVERT_MESH_TO_DATA", Trade::SceneConverterFeature::ConvertMeshToData)
        .value("CONVERT_MULTIPLE", Trade::SceneConverterFeature::ConvertMultiple)
        .value("CONVERT_MULTIPLE_TO_FILE", Trade::SceneConverterFeature::ConvertMultipleToFile)
        .value("CONVERT_MULTIPLE_TO_DATA", Trade::SceneConverterFeature::ConvertMultipleToData)
        .value("ADD_SCENES", Trade::SceneConverterFeature::AddScenes)
        .value("ADD_ANIMATIONS", Trade::SceneConverterFeature::AddAnimations)
        .value("ADD_LIGHTS", Trade::SceneConverterFeature::AddLights)
        .value("ADD_CAMERAS", Trade::SceneConverterFeature::AddCameras)
        .value("ADD_SKINS2D", Trade::SceneConverterFeature::AddSkins2D)
        .value("ADD_SKINS3D", Trade::SceneConverterFeature::AddSkins3D)
        .value("ADD_MESHES", Trade::SceneConverterFeature::AddMeshes)
        .value("ADD_MATERIALS", Trade::SceneConverterFeature::AddMaterials)
        .value("ADD_TEXTURES", Trade::SceneConverterFeature::AddTextures)
        .value("ADD_IMAGES1D", Trade::SceneConverterFeature::AddImages1D)
        .value("ADD_IMAGES2D", Trade::SceneConverterFeature::AddImages2D)
        .value("ADD_IMAGES3D", Trade::SceneConverterFeature::AddImages3D)
        .value("ADD_COMPRESSED_IMAGES1D", Trade::SceneConverterFeature::AddCompressedImages1D)
        .value("ADD_COMPRESSED_IMAGES2D", Trade::SceneConverterFeature::AddCompressedImages2D)
        .value("ADD_COMPRESSED_IMAGES3D", Trade::SceneConverterFeature::AddCompressedImages3D)
        .value("MESH_LEVELS", Trade::SceneConverterFeature::MeshLevels)
        .value("IMAGE_LEVELS", Trade::SceneConverterFeature::ImageLevels)
        .value("NONE", Trade::SceneConverterFeature{});
    corrade::enumOperators(sceneConverterFeatures);

    py::enum_<Trade::SceneConverterFlag> sceneConverterFlags{m, "SceneConverterFlags", "Scene converter flags"};
    sceneConverterFlags
        .value("QUIET", Trade::SceneConverterFlag::Quiet)
        .value("VERBOSE", Trade::SceneConverterFlag::Verbose)
        .value("NONE", Trade::SceneConverterFlag{});
    corrade::enumOperators(sceneConverterFlags);

    py::enum_<Trade::SceneContent> sceneContents{m, "SceneContents", "Scene contents"};
    sceneContents
        .value("SCENES", Trade::SceneContent::Scenes)
        .value("ANIMATIONS", Trade::SceneContent::Animations)
        .value("LIGHTS", Trade::SceneContent::Lights)
        .value("CAMERAS", Trade::SceneContent::Cameras)
        .value("SKINS2D", Trade::SceneContent::Skins2D)
        .value("SKINS3D", Trade::SceneContent::Skins3D)
        .value("MESHES", Trade::SceneContent::Meshes)
        .value("MATERIALS", Trade::SceneContent::Materials)
        .value("TEXTURES", Trade::SceneContent::Textures)
        .value("IMAGES1D", Trade::SceneContent::Images1D)
        .value("IMAGES2D", Trade::SceneContent::Images2D)
        .value("IMAGES3D", Trade::SceneContent::Images3D)
        .value("MESH_LEVELS", Trade::SceneContent::MeshLevels)
        .value("IMAGE_LEVELS", Trade::SceneContent::ImageLevels)
        .value("NAMES", Trade::SceneContent::Names)
        .value("ALL", Trade::SceneContent(Containers::enumCastUnderlyingType(~Trade::SceneContent{})))
        .def("FOR", [](Trade::AbstractImporter& importer) {
            if(!importer.isOpened()) {
                PyErr_SetString(PyExc_AssertionError, "no file opened");
                throw py::error_already_set{};
            }
            return Trade::SceneContent(Containers::enumCastUnderlyingType(Trade::sceneContentsFor(importer)));
        })
        .def("FOR", [](Trade::AbstractSceneConverter& converter) {
            return Trade::SceneContent(Containers::enumCastUnderlyingType(Trade::sceneContentsFor(converter)));
        });
    corrade::enumOperators(sceneContents);

    py::class_<Trade::AbstractSceneConverter, PluginManager::PyPluginHolder<Trade::AbstractSceneConverter>, PluginManager::AbstractPlugin> abstractSceneConverter{m, "AbstractSceneConverter", "Interface for scene converter plugins"};
    abstractSceneConverter
        .def_property_readonly("features", [](Trade::AbstractSceneConverter& self) {
            return Trade::SceneConverterFeature(Containers::enumCastUnderlyingType(self.features()));
        }, "Features supported by this converter")
        .def_property("flags", [](Trade::AbstractSceneConverter& self) {
            return Trade::SceneConverterFlag(Containers::enumCastUnderlyingType(self.flags()));
        }, [](Trade::AbstractSceneConverter& self, Trade::SceneConverterFlag flags) {
            self.setFlags(flags);
        }, "Converter flags")
        .def("convert", [](Trade::AbstractSceneConverter& self, const Trade::MeshData& mesh) {
            if(!(self.features() >= Trade::SceneConverterFeature::ConvertMesh)) {
                PyErr_SetString(PyExc_AssertionError, "mesh conversion not supported");
                throw py::error_already_set{};
            }

            /** @todo log redirection -- but we'd need assertions to not be
                part of that so when it dies, the user can still see why */
            Containers::Optional<Trade::MeshData> out = self.convert(mesh);
            if(!out) {
                PyErr_SetString(PyExc_RuntimeError, "conversion failed");
                throw py::error_already_set{};
            }
            return out;
        }, "Convert a mesh", py::arg("mesh"))
        .def("convert_in_place", [](Trade::AbstractSceneConverter& self, Trade::MeshData& mesh) {
            if(!(self.features() >= Trade::SceneConverterFeature::ConvertMeshInPlace)) {
                PyErr_SetString(PyExc_AssertionError, "mesh conversion not supported");
                throw py::error_already_set{};
            }

            /** @todo log redirection -- but we'd need assertions to not be
                part of that so when it dies, the user can still see why */
            if(!self.convertInPlace(mesh)) {
                PyErr_SetString(PyExc_RuntimeError, "conversion failed");
                throw py::error_already_set{};
            }
        }, "Convert a mesh in-place", py::arg("mesh"))
        /** @todo conversion to data */
        /** @todo drop std::string in favor of our own string caster */
        .def("convert_to_file", [](Trade::AbstractSceneConverter& self, const Trade::MeshData& mesh, const std::string& filename) {
            if(!(self.features() >= (Trade::SceneConverterFeature::ConvertMeshToFile)) &&
               !(self.features() >= (Trade::SceneConverterFeature::ConvertMultipleToFile|Trade::SceneConverterFeature::AddMeshes))) {
                PyErr_SetString(PyExc_AssertionError, "mesh conversion not supported");
                throw py::error_already_set{};
            }

            /** @todo log redirection -- but we'd need assertions to not be
                part of that so when it dies, the user can still see why */
            bool out = self.convertToFile(mesh,
                #ifdef CORRADE_TARGET_WINDOWS
                /* To allow people to conveniently use Python's os.path, we
                   need to convert backslashes to forward slashes as all
                   Corrade and Magnum APIs expect forward */
                Utility::Path::fromNativeSeparators(filename)
                #else
                filename
                #endif
            );
            if(!out) {
                PyErr_SetString(PyExc_RuntimeError, "conversion failed");
                throw py::error_already_set{};
            }
        }, "Convert a mesh to a file", py::arg("mesh"), py::arg("filename"))
        .def_property_readonly("is_converting", &Trade::AbstractSceneConverter::isConverting, "Whether any conversion is in progress")
        .def("abort", &Trade::AbstractSceneConverter::abort, "Abort any in-progress conversion")
        /** @todo begin/end (MeshOptimizer), begin/end data */
        /** @todo drop std::string in favor of our own string caster */
        .def("begin_file", [](Trade::AbstractSceneConverter& self, const std::string& filename) {
            if(!(self.features() >= Trade::SceneConverterFeature::ConvertMultipleToFile) &&
               !(self.features() >=  Trade::SceneConverterFeature::ConvertMeshToFile)) {
                PyErr_SetString(PyExc_AssertionError, "feature not supported");
                throw py::error_already_set{};
            }

            if(!self.beginFile(
                #ifdef CORRADE_TARGET_WINDOWS
                /* To allow people to conveniently use Python's os.path, we
                   need to convert backslashes to forward slashes as all
                   Corrade and Magnum APIs expect forward */
                Utility::Path::fromNativeSeparators(filename)
                #else
                filename
                #endif
            )) {
                PyErr_SetString(PyExc_RuntimeError, "beginning the conversion failed");
                throw py::error_already_set{};
            }
        }, "Begin converting a scene to a file", py::arg("filename"))
        .def("end_file", [](Trade::AbstractSceneConverter& self) {
            /** @todo this doesn't catch a mismatch (e.g., when beginData() was
                called instead */
            if(!self.isConverting()) {
                PyErr_SetString(PyExc_AssertionError, "no conversion in progress");
                throw py::error_already_set{};
            }
            if(!self.endFile()) {
                PyErr_SetString(PyExc_RuntimeError, "ending the conversion failed");
                throw py::error_already_set{};
            }
        }, "End converting a scene to a file")
        .def("set_default_scene", [](Trade::AbstractSceneConverter& self, const UnsignedInt id) {
            if(!(self.features() >= Trade::SceneConverterFeature::AddScenes)) {
                PyErr_SetString(PyExc_AssertionError, "feature not supported");
                throw py::error_already_set{};
            }
            if(!self.isConverting()) {
                PyErr_SetString(PyExc_AssertionError, "no conversion in progress");
                throw py::error_already_set{};
            }
            if(id >= self.sceneCount()) {
                PyErr_Format(PyExc_AssertionError, "index %u out of range for %u scenes", id, self.sceneCount());
                throw py::error_already_set{};
            }

            self.setDefaultScene(id);
        }, "Set default scene", py::arg("id"))
        .def_property_readonly("scene_count", [](Trade::AbstractSceneConverter& self) {
            if(!self.isConverting()) {
                PyErr_SetString(PyExc_AssertionError, "no conversion in progress");
                throw py::error_already_set{};
            }
            return self.sceneCount();
        }, "Count of added scenes")
        /** @todo drop std::string in favor of our own string caster */
        .def("add", [](Trade::AbstractSceneConverter& self, const Trade::SceneData& scene, const std::string& name) {
            if(!(self.features() >= Trade::SceneConverterFeature::AddScenes)) {
                PyErr_SetString(PyExc_AssertionError, "scene conversion not supported");
                throw py::error_already_set{};
            }
            if(!self.isConverting()) {
                PyErr_SetString(PyExc_AssertionError, "no conversion in progress");
                throw py::error_already_set{};
            }
            if(const Containers::Optional<UnsignedInt> out = self.add(scene, name))
                return *out;

            PyErr_SetString(PyExc_RuntimeError, "adding the scene failed");
            throw py::error_already_set{};
        }, "Add a scene", py::arg("scene"), py::arg("name") = std::string{})
        .def("set_scene_field_name", [](Trade::AbstractSceneConverter& self, const Trade::SceneField field, const std::string& name) {
            if(!(self.features() >= Trade::SceneConverterFeature::AddScenes)) {
                PyErr_SetString(PyExc_AssertionError, "feature not supported");
                throw py::error_already_set{};
            }
            if(!self.isConverting()) {
                PyErr_SetString(PyExc_AssertionError, "no conversion in progress");
                throw py::error_already_set{};
            }
            if(!Trade::isSceneFieldCustom(field)) {
                PyErr_SetString(PyExc_AssertionError, "not a custom field");
                throw py::error_already_set{};
            }
            self.setSceneFieldName(field, name);
        }, "Set name of a custom scene field", py::arg("field"), py::arg("name"))
        .def_property_readonly("mesh_count", [](Trade::AbstractSceneConverter& self) {
            if(!self.isConverting()) {
                PyErr_SetString(PyExc_AssertionError, "no conversion in progress");
                throw py::error_already_set{};
            }
            return self.meshCount();
        }, "Count of added meshes")
        /** @todo drop std::string in favor of our own string caster */
        .def("add", [](Trade::AbstractSceneConverter& self, const Trade::MeshData& mesh, const std::string& name) {
            if(!(self.features() >= Trade::SceneConverterFeature::AddMeshes) &&
               !(self.features() & (Trade::SceneConverterFeature::ConvertMesh|
                                    Trade::SceneConverterFeature::ConvertMeshToData|
                                    Trade::SceneConverterFeature::ConvertMeshToFile))) {
                PyErr_SetString(PyExc_AssertionError, "mesh conversion not supported");
                throw py::error_already_set{};
            }
            if(!self.isConverting()) {
                PyErr_SetString(PyExc_AssertionError, "no conversion in progress");
                throw py::error_already_set{};
            }
            if(const Containers::Optional<UnsignedInt> out = self.add(mesh, name))
                return *out;

            PyErr_SetString(PyExc_RuntimeError, "adding the mesh failed");
            throw py::error_already_set{};
        }, "Add a mesh", py::arg("mesh"), py::arg("name") = std::string{})
        /** @todo mesh levels */
        .def("set_mesh_attribute_name", [](Trade::AbstractSceneConverter& self, const Trade::MeshAttribute attribute, const std::string& name) {
            if(!(self.features() & (Trade::SceneConverterFeature::AddMeshes|
                                    Trade::SceneConverterFeature::ConvertMesh|
                                    Trade::SceneConverterFeature::ConvertMeshInPlace|
                                    Trade::SceneConverterFeature::ConvertMeshToData|
                                    Trade::SceneConverterFeature::ConvertMeshToFile))) {
                PyErr_SetString(PyExc_AssertionError, "feature not supported");
                throw py::error_already_set{};
            }
            /* Unless single mesh conversion is supported, allow this function
               to be called only if begin*() was called before */
            if(!(self.features() & (Trade::SceneConverterFeature::ConvertMesh|
                                    Trade::SceneConverterFeature::ConvertMeshInPlace|
                                    Trade::SceneConverterFeature::ConvertMeshToData|
                                    Trade::SceneConverterFeature::ConvertMeshToFile)) && !self.isConverting()) {
                PyErr_SetString(PyExc_AssertionError, "no conversion in progress");
                throw py::error_already_set{};
            }
            if(!Trade::isMeshAttributeCustom(attribute)) {
                PyErr_SetString(PyExc_AssertionError, "not a custom attribute");
                throw py::error_already_set{};
            }
            self.setMeshAttributeName(attribute, name);
        }, "Set name of a custom mesh attribute", py::arg("attribute"), py::arg("name"))
        .def("add_importer_contents", [](Trade::AbstractSceneConverter& self, Trade::AbstractImporter& importer, Trade::SceneContent contents) {
            if(!self.isConverting()) {
                PyErr_SetString(PyExc_AssertionError, "no conversion in progress");
                throw py::error_already_set{};
            }
            if(!importer.isOpened()) {
                PyErr_SetString(PyExc_AssertionError, "the importer is not opened");
                throw py::error_already_set{};
            }
            /** @todo check if contents present in the file are supported? or
                make that a runtime failure in Magnum for easier use? */
            if(!self.addImporterContents(importer, contents)) {
                PyErr_SetString(PyExc_RuntimeError, "adding importer contents failed");
                throw py::error_already_set{};
            }
        }, "Add importer contents", py::arg("importer"), py::arg("contents") = Trade::SceneContent(Containers::enumCastUnderlyingType(~Trade::SceneContent{})))
        .def("add_supported_importer_contents", [](Trade::AbstractSceneConverter& self, Trade::AbstractImporter& importer, Trade::SceneContent contents) {
            if(!self.isConverting()) {
                PyErr_SetString(PyExc_AssertionError, "no conversion in progress");
                throw py::error_already_set{};
            }
            if(!importer.isOpened()) {
                PyErr_SetString(PyExc_AssertionError, "the importer is not opened");
                throw py::error_already_set{};
            }
            /** @todo check if contents present in the file are supported? or
                make that a runtime failure in Magnum for easier use? */
            if(!self.addSupportedImporterContents(importer, contents)) {
                PyErr_SetString(PyExc_RuntimeError, "adding importer contents failed");
                throw py::error_already_set{};
            }
        }, "Add supported importer contents", py::arg("importer"), py::arg("contents") = Trade::SceneContent(Containers::enumCastUnderlyingType(~Trade::SceneContent{})));
    corrade::plugin(abstractSceneConverter);

    py::class_<PluginManager::Manager<Trade::AbstractSceneConverter>, PluginManager::AbstractManager> sceneConverterManager{m, "SceneConverterManager", "Manager for scene converter plugins"};
    corrade::manager(sceneConverterManager);
}

}

#ifndef MAGNUM_BUILD_STATIC
/* TODO: remove declaration when https://github.com/pybind/pybind11/pull/1863
   is released */
extern "C" PYBIND11_EXPORT PyObject* PyInit_trade();
PYBIND11_MODULE(trade, m) {
    magnum::trade(m);
}
#endif
