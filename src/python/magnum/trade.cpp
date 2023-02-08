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
#include <Corrade/Containers/StringStl.h> /** @todo drop once we have our string casters */
#include <Corrade/Containers/Triple.h>
#include <Magnum/ImageView.h>
#include <Magnum/Math/Half.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/AbstractImageConverter.h>
#include <Magnum/Trade/AbstractSceneConverter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>

#include "Corrade/Containers/PythonBindings.h"
#include "Corrade/Containers/OptionalPythonBindings.h"
#include "Corrade/Containers/StridedArrayViewPythonBindings.h"
#include "Magnum/PythonBindings.h"

#include "corrade/EnumOperators.h"
#include "corrade/pluginmanager.h"
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
    py::str name = py::detail::enum_name(arg);
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
            py::handle type = py::type::handle_of(arg);
            py::object type_name = type.attr("__name__");
            return py::str("<{}.{}: {}>")
                .format(std::move(type_name), enumWithCustomValuesName<T, baseCustomValue>(arg), py::int_(arg));
            },
        py::name("__repr__"),
        py::is_method(enum_));
    enum_.attr("name") = py::handle(reinterpret_cast<PyObject*>(&PyProperty_Type))(py::cpp_function(&enumWithCustomValuesName<T, baseCustomValue>, py::name("name"), py::is_method(enum_)));
    enum_.attr("__str__") = py::cpp_function(
        [](const py::object& arg) -> py::str {
            py::object type_name = py::type::handle_of(arg).attr("__name__");
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

template<UnsignedInt dimensions> void imageData(py::class_<Trade::ImageData<dimensions>>& c) {
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
        .def_property_readonly("pixels", [](Trade::ImageData<dimensions>& self) {
            if(self.isCompressed()) {
                PyErr_SetString(PyExc_AttributeError, "image is compressed");
                throw py::error_already_set{};
            }

            return Containers::pyArrayViewHolder(Containers::PyStridedArrayView<dimensions + 1, const char>{self.pixels()}, py::cast(self));
        }, "View on pixel data");
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
template<Containers::String(Trade::AbstractImporter::*f)(UnsignedInt), UnsignedInt(Trade::AbstractImporter::*bounds)() const> std::string checkOpenedBoundsReturnsString(Trade::AbstractImporter& self, UnsignedInt id) {
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

template<class R, Containers::Optional<R>(Trade::AbstractImporter::*f)(UnsignedInt, UnsignedInt), UnsignedInt(Trade::AbstractImporter::*bounds)() const, UnsignedInt(Trade::AbstractImporter::*levelBounds)(UnsignedInt)> R checkOpenedBoundsResult(Trade::AbstractImporter& self, UnsignedInt id, UnsignedInt level) {
    if(!self.isOpened()) {
        PyErr_SetString(PyExc_AssertionError, "no file opened");
        throw py::error_already_set{};
    }

    if(id >= (self.*bounds)()) {
        PyErr_SetNone(PyExc_IndexError);
        throw py::error_already_set{};
    }

    if(level >= (self.*levelBounds)(id)) {
        PyErr_SetNone(PyExc_IndexError);
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
        PyErr_SetNone(PyExc_IndexError);
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

/* Can't be named just checkResult() because the AbstractImageConverter
   overload would confuse GCC 4.8 */
/** @todo drop std::string in favor of our own string caster */
template<class T, bool(Trade::AbstractSceneConverter::*f)(const T&, Containers::StringView)> void checkSceneConverterResult(Trade::AbstractSceneConverter& self, const T& mesh, const std::string& filename) {
    /** @todo log redirection -- but we'd need assertions to not be part of
        that so when it dies, the user can still see why */
    bool out = (self.*f)(mesh,
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
        #define _c(type, string)                                            \
            case MeshIndexType::type: return {                              \
                string,                                                     \
                [](const char* item) {                                      \
                    return py::cast(*reinterpret_cast<const type*>(item));  \
                },                                                          \
                [](char* item, py::handle object) {                         \
                    *reinterpret_cast<type*>(item) = py::cast<type>(object); \
                }};
        _c(UnsignedByte, "B")
        _c(UnsignedShort, "H")
        _c(UnsignedInt, "I")
        #undef _c
    }

    return {};
}

Containers::Triple<const char*, py::object(*)(const char*), void(*)(char*, py::handle)> accessorsForVertexFormat(const VertexFormat format) {
    switch(format) {
        #define _c(format, string)                                          \
            case VertexFormat::format: return {                             \
                string,                                                     \
                [](const char* item) {                                      \
                    return py::cast(*reinterpret_cast<const format*>(item)); \
                },                                                          \
                [](char* item, py::handle object) {                         \
                    *reinterpret_cast<format*>(item) = py::cast<format>(object); \
                }};
        /* Types (such as half-floats) that need to be cast before passed
           from/to pybind that doesn't understand the type directly */
        #define _cc(format, castType, string)                               \
            case VertexFormat::format: return {                             \
                string,                                                     \
                [](const char* item) {                                      \
                    return py::cast(format(*reinterpret_cast<const castType*>(item))); \
                },                                                          \
                [](char* item, py::handle object) {                         \
                    *reinterpret_cast<format*>(item) = format(py::cast<castType>(object)); \
                }};
        _c(Float,                   "f")
        _c(Double,                  "d")
        _cc(UnsignedByte, UnsignedInt, "B")
        _cc(Byte, Int,              "b")
        _cc(UnsignedShort, UnsignedInt, "H")
        _cc(Short, Int,             "h")
        _c(UnsignedInt,             "I")
        _c(Int,                     "i")

        _c(Vector2,                 "ff")
        _c(Vector2d,                "dd")
        _cc(Vector2ub, Vector2ui,   "BB")
        _cc(Vector2b, Vector2i,     "bb")
        _cc(Vector2us, Vector2ui,   "HH")
        _cc(Vector2s, Vector2i,     "hh")
        _c(Vector2ui,               "II")
        _c(Vector2i,                "ii")

        _c(Vector3,                 "fff")
        _c(Vector3d,                "ddd")
        _cc(Vector3ub, Vector3ui,   "BBB")
        _cc(Vector3b, Vector3i,     "bbb")
        _cc(Vector3us, Vector3ui,   "HHH")
        _cc(Vector3s, Vector3i,     "hhh")
        _c(Vector3ui,               "III")
        _c(Vector3i,                "iii")

        _c(Vector4,                 "ffff")
        _c(Vector4d,                "dddd")
        _cc(Vector4ub, Vector4ui,   "BBBB")
        _cc(Vector4b, Vector4i,     "bbbb")
        _cc(Vector4us, Vector4ui,   "HHHH")
        _cc(Vector4s, Vector4i,     "hhhh")
        _c(Vector4ui,               "IIII")
        _c(Vector4i,                "iiii")
        #undef _c
        #undef _cc

        /** @todo handle half, normalized and matrix types */
        default:
            return {};
    }

    return {};
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

}

void trade(py::module_& m) {
    m.doc() = "Data format exchange";

    /* AbstractImporter depends on this */
    py::module_::import("corrade.pluginmanager");

    py::enum_<Trade::DataFlag> dataFlag{m, "DataFlag", "Data flag"};
    dataFlag
        .value("OWNED", Trade::DataFlag::Owned)
        .value("EXTERNALLY_OWNED", Trade::DataFlag::ExternallyOwned)
        .value("MUTABLE", Trade::DataFlag::Mutable);
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

    py::class_<Trade::MeshData>{m, "MeshData", "Mesh data"}
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

        /* IMPORTANT: due to yet-uninvestigated pybind11 platform-specific
           behavioral differences the following overloads need to have the
           MeshAttribute overload *before* the UnsignedInt overload, otherwise
           the integer overload gets picked even if an enum is passed from
           Python, causing massive suffering */
        .def("attribute_name", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetString(PyExc_IndexError, "");
                throw py::error_already_set{};
            }
            return self.attributeName(id);
        }, "Attribute name", py::arg("id"))
        .def("attribute_id", [](Trade::MeshData& self, Trade::MeshAttribute name, UnsignedInt id) {
            if(const Containers::Optional<UnsignedInt> found = self.findAttributeId(name, id))
                return *found;
            PyErr_SetString(PyExc_KeyError, "");
            throw py::error_already_set{};
        }, "Absolute ID of a named attribute", py::arg("name"), py::arg("id") = 0)
        .def("attribute_id", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetString(PyExc_IndexError, "");
                throw py::error_already_set{};
            }
            return self.attributeId(id);
        }, "Attribute ID in a set of attributes of the same name", py::arg("id"))
        .def("attribute_format", [](Trade::MeshData& self, Trade::MeshAttribute name, UnsignedInt id) {
            if(const Containers::Optional<UnsignedInt> found = self.findAttributeId(name, id))
                return self.attributeFormat(*found);
            PyErr_SetString(PyExc_KeyError, "");
            throw py::error_already_set{};
        }, "Format of a named attribute", py::arg("name"), py::arg("id") = 0)
        .def("attribute_format", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetString(PyExc_IndexError, "");
                throw py::error_already_set{};
            }
            return self.attributeFormat(id);
        }, "Attribute format", py::arg("id"))
        .def("attribute_offset", [](Trade::MeshData& self, Trade::MeshAttribute name, UnsignedInt id) {
            if(const Containers::Optional<UnsignedInt> found = self.findAttributeId(name, id))
                return self.attributeOffset(*found);
            PyErr_SetString(PyExc_KeyError, "");
            throw py::error_already_set{};
        }, "Offset of a named attribute", py::arg("name"), py::arg("id") = 0)
        .def("attribute_offset", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetString(PyExc_IndexError, "");
                throw py::error_already_set{};
            }
            return self.attributeOffset(id);
        }, "Attribute offset", py::arg("id"))
        .def("attribute_stride", [](Trade::MeshData& self, Trade::MeshAttribute name, UnsignedInt id) {
            if(const Containers::Optional<UnsignedInt> found = self.findAttributeId(name, id))
                return self.attributeStride(*found);
            PyErr_SetString(PyExc_KeyError, "");
            throw py::error_already_set{};
        }, "Stride of a named attribute", py::arg("name"), py::arg("id") = 0)
        .def("attribute_stride", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetString(PyExc_IndexError, "");
                throw py::error_already_set{};
            }
            return self.attributeStride(id);
        }, "Attribute stride", py::arg("id"))
        .def("attribute_array_size", [](Trade::MeshData& self, Trade::MeshAttribute name, UnsignedInt id) {
            if(const Containers::Optional<UnsignedInt> found = self.findAttributeId(name, id))
                return self.attributeArraySize(*found);
            PyErr_SetString(PyExc_KeyError, "");
            throw py::error_already_set{};
        }, "Array size of a named attribute", py::arg("name"), py::arg("id") = 0)
        .def("attribute_array_size", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetString(PyExc_IndexError, "");
                throw py::error_already_set{};
            }
            return self.attributeArraySize(id);
        }, "Attribute array size", py::arg("id"))
        .def("attribute", [](Trade::MeshData& self, Trade::MeshAttribute name, UnsignedInt id) {
            const Containers::Optional<UnsignedInt> found = self.findAttributeId(name, id);
            if(!found) {
                PyErr_SetString(PyExc_KeyError, "");
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
                PyErr_SetString(PyExc_IndexError, "");
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
                PyErr_SetString(PyExc_KeyError, "");
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
        }, "Data for given named attribute", py::arg("name"), py::arg("id") = 0)
        .def("mutable_attribute", [](Trade::MeshData& self, UnsignedInt id) {
            if(id >= self.attributeCount()) {
                PyErr_SetString(PyExc_IndexError, "");
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
        }, "Mutable data for given attribute", py::arg("id"));

    py::class_<Trade::ImageData1D> imageData1D{m, "ImageData1D", "One-dimensional image data"};
    py::class_<Trade::ImageData2D> imageData2D{m, "ImageData2D", "Two-dimensional image data"};
    py::class_<Trade::ImageData3D> imageData3D{m, "ImageData3D", "Three-dimensional image data"};
    imageData(imageData1D);
    imageData(imageData2D);
    imageData(imageData3D);

    /* Importer. Skipping file callbacks and openState as those operate with
       void*. Leaving the name as AbstractImporter (instead of Importer) to
       avoid needless name differences and because in the future there *might*
       be pure Python importers (not now tho). */
    py::class_<Trade::AbstractImporter, PluginManager::PyPluginHolder<Trade::AbstractImporter>> abstractImporter{m, "AbstractImporter", "Interface for importer plugins"};
    corrade::plugin(abstractImporter);
    abstractImporter
        /** @todo features */
        .def_property_readonly("is_opened", &Trade::AbstractImporter::isOpened, "Whether any file is opened")
        .def("open_data", [](Trade::AbstractImporter& self, Containers::ArrayView<const char> data) {
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

        /** @todo all other data types */
        .def_property_readonly("mesh_count", checkOpened<UnsignedInt, &Trade::AbstractImporter::meshCount>, "Mesh count")
        .def("mesh_level_count", checkOpenedBounds<UnsignedInt, &Trade::AbstractImporter::meshLevelCount, &Trade::AbstractImporter::meshCount>, "Mesh level count", py::arg("id"))
        .def("mesh_for_name", checkOpenedString<Int, &Trade::AbstractImporter::meshForName>, "Mesh ID for given name")
        .def("mesh_name", checkOpenedBoundsReturnsString<&Trade::AbstractImporter::meshName, &Trade::AbstractImporter::meshCount>, "Mesh name", py::arg("id"))
        .def("mesh", checkOpenedBoundsResult<Trade::MeshData, &Trade::AbstractImporter::mesh, &Trade::AbstractImporter::meshCount, &Trade::AbstractImporter::meshLevelCount>, "Mesh", py::arg("id"), py::arg("level") = 0)
        .def("mesh", checkOpenedBoundsResultString<Trade::MeshData, &Trade::AbstractImporter::mesh, &Trade::AbstractImporter::meshForName, &Trade::AbstractImporter::meshLevelCount>, "Mesh", py::arg("name"), py::arg("level") = 0)
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

        .def_property_readonly("image1d_count", checkOpened<UnsignedInt, &Trade::AbstractImporter::image1DCount>, "One-dimensional image count")
        .def_property_readonly("image2d_count", checkOpened<UnsignedInt, &Trade::AbstractImporter::image2DCount>, "Two-dimensional image count")
        .def_property_readonly("image3d_count", checkOpened<UnsignedInt, &Trade::AbstractImporter::image3DCount>, "Three-dimensional image count")
        .def("image1d_level_count", checkOpenedBounds<UnsignedInt, &Trade::AbstractImporter::image1DLevelCount, &Trade::AbstractImporter::image1DCount>, "One-dimensional image level count", py::arg("id"))
        .def("image2d_level_count", checkOpenedBounds<UnsignedInt, &Trade::AbstractImporter::image2DLevelCount, &Trade::AbstractImporter::image2DCount>, "Two-dimensional image level count", py::arg("id"))
        .def("image3d_level_count", checkOpenedBounds<UnsignedInt, &Trade::AbstractImporter::image3DLevelCount, &Trade::AbstractImporter::image3DCount>, "Three-dimensional image level count", py::arg("id"))
        .def("image1d_for_name", checkOpenedString<Int, &Trade::AbstractImporter::image1DForName>, "One-dimensional image ID for given name")
        .def("image2d_for_name", checkOpenedString<Int, &Trade::AbstractImporter::image2DForName>, "Two-dimensional image ID for given name")
        .def("image3d_for_name", checkOpenedString<Int, &Trade::AbstractImporter::image3DForName>, "Three-dimensional image ID for given name")
        .def("image1d_name", checkOpenedBoundsReturnsString<&Trade::AbstractImporter::image1DName, &Trade::AbstractImporter::image1DCount>, "One-dimensional image name", py::arg("id"))
        .def("image2d_name", checkOpenedBoundsReturnsString<&Trade::AbstractImporter::image2DName, &Trade::AbstractImporter::image2DCount>, "Two-dimensional image name", py::arg("id"))
        .def("image3d_name", checkOpenedBoundsReturnsString< &Trade::AbstractImporter::image3DName, &Trade::AbstractImporter::image3DCount>, "Three-dimensional image name", py::arg("id"))
        .def("image1d", checkOpenedBoundsResult<Trade::ImageData1D, &Trade::AbstractImporter::image1D, &Trade::AbstractImporter::image1DCount, &Trade::AbstractImporter::image1DLevelCount>, "One-dimensional image", py::arg("id"), py::arg("level") = 0)
        .def("image1d", checkOpenedBoundsResultString<Trade::ImageData1D, &Trade::AbstractImporter::image1D, &Trade::AbstractImporter::image1DForName, &Trade::AbstractImporter::image1DLevelCount>, "One-dimensional image", py::arg("name"), py::arg("level") = 0)
        .def("image2d", checkOpenedBoundsResult<Trade::ImageData2D, &Trade::AbstractImporter::image2D, &Trade::AbstractImporter::image2DCount, &Trade::AbstractImporter::image2DLevelCount>, "Two-dimensional image", py::arg("id"), py::arg("level") = 0)
        .def("image2d", checkOpenedBoundsResultString<Trade::ImageData2D, &Trade::AbstractImporter::image2D, &Trade::AbstractImporter::image2DForName, &Trade::AbstractImporter::image2DLevelCount>, "Two-dimensional image", py::arg("name"), py::arg("level") = 0)
        .def("image3d", checkOpenedBoundsResult<Trade::ImageData3D, &Trade::AbstractImporter::image3D, &Trade::AbstractImporter::image3DCount, &Trade::AbstractImporter::image3DLevelCount>, "Three-dimensional image", py::arg("id"), py::arg("level") = 0)
        .def("image3d", checkOpenedBoundsResultString<Trade::ImageData3D, &Trade::AbstractImporter::image3D, &Trade::AbstractImporter::image3DForName, &Trade::AbstractImporter::image3DLevelCount>, "Threee-dimensional image", py::arg("name"), py::arg("level") = 0);

    py::class_<PluginManager::Manager<Trade::AbstractImporter>, PluginManager::AbstractManager> importerManager{m, "ImporterManager", "Manager for importer plugins"};
    corrade::manager(importerManager);

    /* Image converter */
    py::class_<Trade::AbstractImageConverter, PluginManager::PyPluginHolder<Trade::AbstractImageConverter>> abstractImageConverter{m, "AbstractImageConverter", "Interface for image converter plugins"};
    abstractImageConverter
        /** @todo features */
        .def("convert_to_file", checkImageConverterResult<ImageView1D, &Trade::AbstractImageConverter::convertToFile>, "Convert a 1D image to a file", py::arg("image"), py::arg("filename"))
        .def("convert_to_file", checkImageConverterResult<ImageView2D, &Trade::AbstractImageConverter::convertToFile>, "Convert a 2D image to a file", py::arg("image"), py::arg("filename"))
        .def("convert_to_file", checkImageConverterResult<ImageView3D, &Trade::AbstractImageConverter::convertToFile>, "Convert a 3D image to a file", py::arg("image"), py::arg("filename"));
    corrade::plugin(abstractImageConverter);

    py::class_<PluginManager::Manager<Trade::AbstractImageConverter>, PluginManager::AbstractManager> imageConverterManager{m, "ImageConverterManager", "Manager for image converter plugins"};
    corrade::manager(imageConverterManager);

    /* Scene converter */
    py::class_<Trade::AbstractSceneConverter, PluginManager::PyPluginHolder<Trade::AbstractSceneConverter>> abstractSceneConverter{m, "AbstractSceneConverter", "Interface for scene converter plugins"};
    abstractSceneConverter
        /** @todo features */
        .def("convert_to_file", checkSceneConverterResult<Trade::MeshData, &Trade::AbstractSceneConverter::convertToFile>, "Convert a mesh to a file", py::arg("mesh"), py::arg("filename"));
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
