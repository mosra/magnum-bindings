/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
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

#include <sstream>
#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include <Corrade/Containers/PairStl.h> /** @todo drop once Containers::Pair is exposed directly */
#include <Magnum/Magnum.h>
#include <Magnum/Math/Angle.h>
#include <Magnum/Math/BitVector.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/Math/Vector4.h>

#include "magnum/bootstrap.h"
#include "magnum/math.h"

namespace magnum {

/* Keep in sync with math.h */

const char* const FormatStrings[]{
    /* 0. Representing bytes as unsigned. Not using 'c' because then it behaves
       differently from bytes/bytearray, where you can do `a[0] = ord('A')`. */
    "B",

    "b", /* 1 -- std::int8_t */
    "B", /* 2 -- std::uint8_t */
    "i", /* 3 -- std::int32_t */
    "I", /* 4 -- std::uint32_t */
    "f", /* 5 -- float */
    "d"  /* 6 -- double */
};

/* Flipped as numpy expects row-major */
const Py_ssize_t MatrixShapes[][2]{
    {2, 2}, /* 0 -- 2 cols, 2 rows */
    {3, 2}, /* 1 -- 2 cols, 3 rows */
    {4, 2}, /* 2 -- 2 cols, 4 rows */
    {2, 3}, /* 3 -- 3 cols, 2 rows */
    {3, 3}, /* 4 -- 3 cols, 3 rows */
    {4, 3}, /* 5 -- 3 cols, 4 rows */
    {2, 4}, /* 6 -- 4 cols, 2 rows */
    {3, 4}, /* 7 -- 4 cols, 3 rows */
    {4, 4}  /* 8 -- 4 cols, 4 rows */
};
const Py_ssize_t MatrixStridesFloat[][2]{
    {4, 4*2}, /* 0 -- 2 cols, 2 rows */
    {4, 4*3}, /* 1 -- 2 cols, 3 rows */
    {4, 4*4}, /* 2 -- 2 cols, 4 rows */
    {4, 4*2}, /* 3 -- 3 cols, 2 rows */
    {4, 4*3}, /* 4 -- 3 cols, 3 rows */
    {4, 4*4}, /* 5 -- 3 cols, 4 rows */
    {4, 4*2}, /* 6 -- 4 cols, 2 rows */
    {4, 4*3}, /* 7 -- 4 cols, 3 rows */
    {4, 4*4}  /* 8 -- 4 cols, 4 rows */
};
const Py_ssize_t MatrixStridesDouble[][2]{
    {8, 8*2}, /* 0 -- 2 cols, 2 rows */
    {8, 8*3}, /* 1 -- 2 cols, 3 rows */
    {8, 8*4}, /* 2 -- 2 cols, 4 rows */
    {8, 8*2}, /* 3 -- 3 cols, 2 rows */
    {8, 8*3}, /* 4 -- 3 cols, 3 rows */
    {8, 8*4}, /* 5 -- 3 cols, 4 rows */
    {8, 8*2}, /* 6 -- 4 cols, 2 rows */
    {8, 8*3}, /* 7 -- 4 cols, 3 rows */
    {8, 8*4}  /* 8 -- 4 cols, 4 rows */
};

namespace {

template<class T> void angle(py::module_& m, py::class_<T>& c) {
    /*
        Missing APIs:

        Type
    */

    c
        /* Constructors */
        .def_static("zero_init", []() {
            return T{Math::ZeroInit};
        }, "Construct a zero value")
        .def(py::init(), "Default constructor")
        .def(py::init<typename T::Type>(), "Explicit conversion from a unitless type")

        /* Explicit conversion to an underlying type */
        .def("__float__", &T::operator typename T::Type, "Conversion to underlying type")

        /* Comparison */
        .def(py::self == py::self, "Equality comparison")
        .def(py::self != py::self, "Non-equality comparison")
        .def(py::self < py::self, "Less than comparison")
        .def(py::self > py::self, "Greater than comparison")
        .def(py::self <= py::self, "Less than or equal comparison")
        .def(py::self >= py::self, "Greater than or equal comparison")

        /* Pickling */
        .def(py::pickle(
            [](const T& self) {
                return typename T::Type(self);
            },
            [](typename T::Type data) {
                return T(data);
            }
        ))

        /* Arithmetic ops. Need to use lambdas because the C++ functions return
           the Unit base class :( */
        .def("__neg__", [](const T& self) -> T {
                return -self;
            }, "Negated value")
        .def("__iadd__", [](T& self, const T& other) -> T& {
                self += other;
                return self;
            }, "Add and assign a value")
        .def("__add__", [](const T& self, const T& other) -> T {
                return self + other;
            }, "Add a value")
        .def("__isub__", [](T& self, const T& other) -> T& {
                self -= other;
                return self;
            }, "Subtract and assign a value")
        .def("__sub__", [](const T& self, const T& other) -> T {
                return self - other;
            }, "Subtract a value")
        .def("__imul__", [](T& self, typename T::Type other) -> T& {
                self *= other;
                return self;
            }, "Multiply with a number and assign")
        .def("__mul__", [](const T& self, typename T::Type other) -> T {
                return self * other;
            }, "Multiply with a number")
        .def("__itruediv__", [](T& self, typename T::Type other) -> T& {
                self /= other;
                return self;
            }, "Divide with a number and assign")
        .def("__truediv__", [](const T& self, typename T::Type other) -> T {
                return self / other;
            }, "Divide with a number")
        .def("__truediv__", [](const T& self, const T& other) -> typename T::Type {
                return self / other;
            }, "Ratio of two values")

        .def("__repr__", repr<T>, "Object representation");

    /* Overloads of scalar functions */
    m
        .def("isinf", static_cast<bool(*)(T)>(Math::isInf), "If given number is a positive or negative infinity")
        .def("isnan", static_cast<bool(*)(T)>(Math::isNan), "If given number is a NaN")
        .def("min", static_cast<T(*)(T, T)>(Math::min), "Minimum", py::arg("value"), py::arg("min"))
        .def("max", static_cast<T(*)(T, T)>(Math::max), "Maximum", py::arg("value"), py::arg("min"))
        .def("minmax", [](T a, T b) {
            /** @todo bind Containers::Pair directly */
            return std::pair<T, T>(Math::minmax(a, b));
        }, "Minimum and maximum of two values")
        .def("clamp", static_cast<T(*)(T, T, T)>(Math::clamp), "Clamp value", py::arg("value"), py::arg("min"), py::arg("max"))
        .def("sign", Math::sign<T>, "Sign")
        .def("abs", static_cast<T(*)(T)>(Math::abs), "Absolute value")
        .def("floor", static_cast<T(*)(T)>(Math::floor), "Nearest not larger integer")
        .def("round", static_cast<T(*)(T)>(Math::round), "Round value to nearest integer")
        .def("ceil", static_cast<T(*)(T)>(Math::ceil), "Nearest not smaller integer")
        .def("fmod", static_cast<T(*)(T, T)>(Math::fmod), "Floating point division remainder")
        .def("lerp", static_cast<T(*)(const T&, const T&, Double)>(Math::lerp), "Linear interpolation of two values", py::arg("a"), py::arg("b"), py::arg("t"))
        .def("lerp", static_cast<T(*)(const T&, const T&, bool)>(Math::lerp), "Linear interpolation of two values", py::arg("a"), py::arg("b"), py::arg("t"))
        .def("lerp_inverted", static_cast<Double(*)(T, T, T)>(Math::lerpInverted), "Inverse linear interpolation of two values", py::arg("a"), py::arg("b"), py::arg("lerp"))
        .def("select", static_cast<T(*)(const T&, const T&, Double)>(Math::select), "Constant interpolation of two values", py::arg("a"), py::arg("b"), py::arg("t"));
}

template<class T> void bitVector(py::module_& m, py::class_<T>& c) {
    c
        /* Constructors */
        .def_static("zero_init", []() {
            return T{Math::ZeroInit};
        }, "Construct a zero-filled boolean vector")
        .def(py::init(), "Default constructor")
        .def(py::init<bool>(), "Construct a boolean vector with one value for all fields")
        .def(py::init<UnsignedByte>(), "Construct a boolean vector from segment values")

        /* Explicit conversion to bool */
        .def("__bool__", &T::operator bool, "Boolean conversion")

        /* Comparison */
        .def(py::self == py::self, "Equality comparison")
        .def(py::self != py::self, "Non-equality comparison")

        /* Pickling */
        .def(py::pickle(
            [](const T& self) {
                return py::bytes(reinterpret_cast<const char*>(self.data()), sizeof(T));
            },
            [](const py::bytes& data) {
                const std::size_t size = PyBytes_GET_SIZE(data.ptr());
                if(size != sizeof(T)) {
                    PyErr_Format(PyExc_ValueError, "expected %zu bytes but got %zi", sizeof(T), size);
                    throw py::error_already_set{};
                }
                T out;
                /** @todo gah is there really no other way to access contents? */
                std::memcpy(out.data(), PyBytes_AS_STRING(data.ptr()), sizeof(T));
                return out;
            }
        ))

        /* Member functions */
        .def("all", &T::all, "Whether all bits are set")
        .def("none", &T::none, "Whether no bits are set")
        .def("any", &T::any, "Whether any bit is set")

        /* Set / get. Need to raise IndexError in order to allow iteration:
           https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */
        .def("__setitem__",[](T& self, std::size_t i, bool value) {
            if(i >= T::Size) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            self.set(i, value);
        }, "Set a bit at given position")
        .def("__getitem__", [](const T& self, std::size_t i) {
            if(i >= T::Size) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self[i];
        }, "Bit at given position")

        /* Operators */
        .def(~py::self, "Bitwise inversion")
        #ifdef __clang__
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wself-assign-overloaded"
        #endif
        .def(py::self &= py::self, "Bitwise AND and assign")
        #ifdef __clang__
        #pragma GCC diagnostic pop
        #endif
        .def(py::self & py::self, "Bitwise AND")
        #ifdef __clang__
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wself-assign-overloaded"
        #endif
        .def(py::self |= py::self, "Bitwise OR and assign")
        #ifdef __clang__
        #pragma GCC diagnostic pop
        #endif
        .def(py::self | py::self, "Bitwise OR")
        #ifdef __clang__
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wself-assign-overloaded"
        #endif
        .def(py::self ^= py::self, "Bitwise XOR and assign")
        #ifdef __clang__
        #pragma GCC diagnostic pop
        #endif
        .def(py::self ^ py::self, "Bitwise XOR")

        .def("__repr__", repr<T>, "Object representation");

    /* Vector length */
    char lenDocstring[] = "Vector size. Returns _.";
    lenDocstring[sizeof(lenDocstring) - 3] = '0' + T::Size;
    c.def_static("__len__", []() { return int(T::Size); }, lenDocstring);

    m
        .def("lerp", Math::lerp<T::Size>, "Linear interpolation of two values", py::arg("a"), py::arg("b"), py::arg("t"));
}

template<class U, class T, class ...Args> void convertible(py::class_<T, Args...>& c) {
    c.def(py::init<U>(), "Construct from different underlying type");
}

template<class T> void quaternion(py::module_& m, py::class_<T>& c) {
    /*
        Missing APIs:

        Type
        construction from different types
    */

    m
        .def("dot", static_cast<typename T::Type(*)(const T&, const T&)>(&Math::dot),
            "Dot product between two quaternions")
        .def("half_angle", [](const T& normalizedA, const T& normalizedB) {
            if(!normalizedA.isNormalized() || !normalizedB.isNormalized()) {
                PyErr_Format(PyExc_ValueError, "quaternions %S and %S are not normalized", py::cast(normalizedA).ptr(), py::cast(normalizedB).ptr());
                throw py::error_already_set{};
            }
            /** @todo switch back to angle() once it's reintroduced with the
                correct output again */
            return Radd(Math::halfAngle(normalizedA, normalizedB));
        }, "Angle between normalized quaternions", py::arg("normalized_a"), py::arg("normalized_b"))
        .def("lerp", [](const T& normalizedA, const T& normalizedB, typename T::Type t) {
            if(!normalizedA.isNormalized() || !normalizedB.isNormalized()) {
                PyErr_Format(PyExc_ValueError, "quaternions %S and %S are not normalized", py::cast(normalizedA).ptr(), py::cast(normalizedB).ptr());
                throw py::error_already_set{};
            }
            return Math::lerp(normalizedA, normalizedB, t);
        }, "Linear interpolation of two quaternions", py::arg("normalized_a"), py::arg("normalized_b"), py::arg("t"))
        .def("lerp_shortest_path", [](const T& normalizedA, const T& normalizedB, typename T::Type t) {
            if(!normalizedA.isNormalized() || !normalizedB.isNormalized()) {
                PyErr_Format(PyExc_ValueError, "quaternions %S and %S are not normalized", py::cast(normalizedA).ptr(), py::cast(normalizedB).ptr());
                throw py::error_already_set{};
            }
            return Math::lerpShortestPath(normalizedA, normalizedB, t);
        }, "Linear shortest-path interpolation of two quaternions", py::arg("normalized_a"), py::arg("normalized_b"), py::arg("t"))
        .def("slerp", [](const T& normalizedA, const T& normalizedB, typename T::Type t) {
            if(!normalizedA.isNormalized() || !normalizedB.isNormalized()) {
                PyErr_Format(PyExc_ValueError, "quaternions %S and %S are not normalized", py::cast(normalizedA).ptr(), py::cast(normalizedB).ptr());
                throw py::error_already_set{};
            }
            return Math::slerp(normalizedA, normalizedB, t);
        }, "Spherical linear interpolation of two quaternions", py::arg("normalized_a"), py::arg("normalized_b"), py::arg("t"))
        .def("slerp_shortest_path", [](const T& normalizedA, const T& normalizedB, typename T::Type t) {
            if(!normalizedA.isNormalized() || !normalizedB.isNormalized()) {
                PyErr_Format(PyExc_ValueError, "quaternions %S and %S are not normalized", py::cast(normalizedA).ptr(), py::cast(normalizedB).ptr());
                throw py::error_already_set{};
            }
            return Math::slerpShortestPath(normalizedA, normalizedB, t);
        }, "Spherical linear shortest-path interpolation of two quaternions", py::arg("normalized_a"), py::arg("normalized_b"), py::arg("t"));

    c
        /* Constructors */
        .def_static("rotation", [](Radd angle, const Math::Vector3<typename T::Type>& normalizedAxis) {
            if(!normalizedAxis.isNormalized()) {
                PyErr_Format(PyExc_ValueError, "axis %S is not normalized", py::cast(normalizedAxis).ptr());
                throw py::error_already_set{};
            }
            return T::rotation(Math::Rad<typename T::Type>(angle), normalizedAxis);
        }, "Rotation quaternion", py::arg("angle"), py::arg("normalized_axis"))
        .def_static("rotation", [](const Math::Vector3<typename T::Type>& normalizedFrom, const Math::Vector3<typename T::Type>& normalizedTo) {
            if(!normalizedFrom.isNormalized() || !normalizedTo.isNormalized()) {
                PyErr_Format(PyExc_ValueError, "vectors %S and %S are not normalized", py::cast(normalizedFrom).ptr(), py::cast(normalizedTo).ptr());
                throw py::error_already_set{};
            }
            return T::rotation(normalizedFrom, normalizedTo);
        }, "Quaternion rotating from a vector to another", py::arg("normalized_from"), py::arg("normalized_to"))
        .def_static("reflection", [](const Math::Vector3<typename T::Type>& normal) {
            if(!normal.isNormalized()) {
                PyErr_Format(PyExc_ValueError, "normal %S is not normalized", py::cast(normal).ptr());
                throw py::error_already_set{};
            }
            return T::reflection(normal);
        }, "Reflection quaternion", py::arg("normal"))
        .def_static("from_matrix", [](const Math::Matrix3x3<typename T::Type>& matrix) {
            /* Same as the check in fromMatrix() */
            if(std::abs(matrix.determinant() - typename T::Type(1)) >= typename T::Type(3)*Math::TypeTraits<typename T::Type>::epsilon()) {
                PyErr_Format(PyExc_ValueError, "the matrix is not a rotation:\n%S", py::cast(matrix).ptr());
                throw py::error_already_set{};
            }
            return T::fromMatrix(matrix);
        }, "Create a quaternion from rotation matrix", py::arg("matrix"))
        .def_static("zero_init", []() {
            return T{Math::ZeroInit};
        }, "Construct a zero-initialized quaternion")
        .def_static("identity_init", []() {
            return T{Math::IdentityInit};
        }, "Construct an identity quaternion")
        .def(py::init(), "Default constructor")
        .def(py::init<const Math::Vector3<typename T::Type>&, typename T::Type>(),
            "Construct from a vector and a scalar")
        .def(py::init([](const std::pair<std::tuple<typename T::Type, typename T::Type, typename T::Type>, typename T::Type>& value) {
            return T{{std::get<0>(value.first), std::get<1>(value.first), std::get<2>(value.first)}, value.second};
        }), "Construct from a tuple")
        .def(py::init<const Math::Vector3<typename T::Type>&>(),
            "Construct from a vector")

        /* Comparison */
        .def(py::self == py::self, "Equality comparison")
        .def(py::self != py::self, "Non-equality comparison")

        /* Pickling */
        .def(py::pickle(
            [](const T& self) {
                return py::bytes(reinterpret_cast<const char*>(self.data()), sizeof(T));
            },
            [](const py::bytes& data) {
                const std::size_t size = PyBytes_GET_SIZE(data.ptr());
                if(size != sizeof(T)) {
                    PyErr_Format(PyExc_ValueError, "expected %zu bytes but got %zi", sizeof(T), size);
                    throw py::error_already_set{};
                }
                T out;
                /** @todo gah is there really no other way to access contents? */
                std::memcpy(out.data(), PyBytes_AS_STRING(data.ptr()), sizeof(T));
                return out;
            }
        ))

        /* Operators */
        .def(-py::self, "Negated quaternion")
        .def(py::self += py::self, "Add and assign a quaternion")
        .def(py::self + py::self, "Add a quaternion")
        #ifdef __clang__
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wself-assign-overloaded"
        #endif
        .def(py::self -= py::self, "Subtract and assign a quaternion")
        #ifdef __clang__
        #pragma GCC diagnostic pop
        #endif
        .def(py::self - py::self, "Subtract a quaternion")
        .def(py::self *= typename T::Type{}, "Multiply with a scalar and assign")
        .def(py::self * typename T::Type{}, "Multiply with a scalar")
        .def(py::self /= typename T::Type{}, "Divide with a scalar and assign")
        .def(py::self / typename T::Type{}, "Divide with a scalar")
        .def(py::self * py::self, "Multiply with a quaternion")
        .def(typename T::Type{} * py::self, "Multiply a scalar with a quaternion")
        .def(typename T::Type{} / py::self, "Divide a quaternion with a scalar and invert")

        /* Member functions */
        .def("is_normalized", &T::isNormalized,
            "Whether the quaternion is normalized")
        .def("angle", [](const T& self) {
            if(!self.isNormalized()) {
                PyErr_Format(PyExc_ValueError, "%S is not normalized", py::cast(self).ptr());
                throw py::error_already_set{};
            }
            return Radd(self.angle());
        }, "Rotation angle of a unit quaternion")
        .def("axis", [](const T& self) {
            if(!self.isNormalized()) {
                PyErr_Format(PyExc_ValueError, "%S is not normalized", py::cast(self).ptr());
                throw py::error_already_set{};
            }
            return self.axis();
        }, "Rotation axis of a unit quaternion")
        .def("to_matrix", &T::toMatrix,
            "Convert to a rotation matrix")
        .def("dot", &T::dot,
            "Dot product of the quaternion")
        .def("length", &T::length,
            "Quaternion length")
        .def("normalized", &T::normalized,
            "Normalized quaternion (of unit length)")
        .def("conjugated", &T::conjugated,
            "Conjugated quaternion")
        .def("inverted", &T::inverted,
            "Inverted quaternion")
        .def("inverted_normalized", [](const T& self) {
            if(!self.isNormalized()) {
                PyErr_Format(PyExc_ValueError, "%S is not normalized", py::cast(self).ptr());
                throw py::error_already_set{};
            }
            return self.invertedNormalized();
        }, "Inverted normalized quaternion")
        .def("transform_vector", &T::transformVector,
            "Rotate a vector with a quaternion", py::arg("vector"))
        .def("transform_vector_normalized", [](const T& self, const Math::Vector3<typename T::Type>& vector) {
            if(!self.isNormalized()) {
                PyErr_Format(PyExc_ValueError, "%S is not normalized", py::cast(self).ptr());
                throw py::error_already_set{};
            }
            return self.transformVectorNormalized(vector);
        }, "Rotate a vector with a normalized quaternion", py::arg("vector"))
        .def("reflect_vector", &T::reflectVector,
            "Reflect a vector with a reflection quaternion", py::arg("vector"))

        /* Properties */
        .def_property("vector",
            static_cast<const Math::Vector3<typename T::Type>(T::*)() const>(&T::vector),
            [](T& self, const Math::Vector3<typename T::Type>& value) { self.vector() = value; },
            "Vector part")
        .def_property("scalar",
            static_cast<typename T::Type(T::*)() const>(&T::scalar),
            [](T& self, typename T::Type value) { self.scalar() = value; },
            "Scalar part")
        .def_property_readonly("xyzw", &T::xyzw, "Quaternion components in a XYZW order")
        .def_property_readonly("wxyz", &T::wxyz, "Quaternion components in a WXYZ order")

        .def("__repr__", repr<T>, "Object representation");
}

/* Behaves exactly like Py_Type_Type.tp_getattro but redirects access to the
   translation attribute to _stranslation in order to make it behave like a
   function when called on an object */
PyObject* transformationMatrixGettattro(PyObject* const obj, PyObject* const name) {
    if(PyUnicode_Check(name) && PyUnicode_CompareWithASCIIString(name, "translation") == 0) {
        /* TODO: this means one allocation per every attribute access, any
            chance we could minimize that? Storing a global reference to this
            is crappy :/ Maybe allocate and store this inside
            transformationMatrixMetaclass? But who would be responsible for
            Py_DECREF then? Pybind's module destructors are kinda overdone:
            https://pybind11.readthedocs.io/en/stable/advanced/misc.html#module-destructors */
        PyObject* const _stranslation = PyUnicode_FromString("_stranslation");
        PyObject* const ret = PyType_Type.tp_getattro(obj, _stranslation);
        Py_DECREF(_stranslation);
        return ret;
    }

    return PyType_Type.tp_getattro(obj, name);
}

/* Based off pybind11:detail::make_default_metaclass(), but with Python < 3.3
   support and unneeded pybind specifics removed. In particular, we don't need
   any static attribute access modifications from pybind's own metaclass, as
   Matrix[34] doesn't need to support assignment to static attributes. */
PyTypeObject* transformationMatrixMetaclass() {
    constexpr auto *name = "TransformationMatrixType";
    auto name_obj = py::reinterpret_steal<py::object>(PyUnicode_FromString(name));

    /* Danger zone: from now (and until PyType_Ready), make sure to
       issue no Python C API calls which could potentially invoke the
       garbage collector (the GC will call type_traverse(), which will in
       turn find the newly constructed type in an invalid state) */
    auto heap_type = reinterpret_cast<PyHeapTypeObject*>(PyType_Type.tp_alloc(&PyType_Type, 0));
    if(!heap_type)
        py::pybind11_fail("magnum::transformationMatrixMetaclass(): error allocating metaclass!");

    heap_type->ht_name = name_obj.inc_ref().ptr();
    heap_type->ht_qualname = name_obj.inc_ref().ptr();

    auto type = &heap_type->ht_type;
    type->tp_name = name;
    type->tp_base = py::detail::type_incref(&PyType_Type);
    type->tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE|Py_TPFLAGS_HEAPTYPE;

    type->tp_setattro = PyType_Type.tp_setattro;
    /* In order to create reasonable docs for this, we can't override the
       translation attribute at that time --- the _stranslation will be then
       used for documentation. */
    if(std::getenv("MCSS_GENERATING_OUTPUT"))
        type->tp_getattro = PyType_Type.tp_getattro;
    else
        type->tp_getattro = transformationMatrixGettattro;

    if(PyType_Ready(type) < 0)
        py::pybind11_fail("magnum::transformationMatrixMetaclass(): failure in PyType_Ready()!");

    py::setattr(reinterpret_cast<PyObject*>(type), "__module__", py::str("magnum_builtins"));

    return type;
}

}

void math(py::module_& root, py::module_& m) {
    m.doc() = "Math library";

    /* Deg, Rad, Degd, Radd */
    py::class_<Degd> deg{root, "Deg", "Degrees"};
    py::class_<Radd> rad{root, "Rad", "Radians"};
    deg.def(py::init<Radd>(), "Conversion from radians");
    rad.def(py::init<Degd>(), "Conversion from degrees");
    angle(m, deg);
    angle(m, rad);

    /* Cyclic convertibility, so can't do that in angle() */
    py::implicitly_convertible<Radd, Degd>();
    py::implicitly_convertible<Degd, Radd>();

    /* BitVector */
    py::class_<Math::BitVector<2>> boolVector2{root, "BitVector2", "Two-component vector of bits"};
    py::class_<Math::BitVector<3>> boolVector3{root, "BitVector3", "Three-component vector of bits"};
    py::class_<Math::BitVector<4>> boolVector4{root, "BitVector4", "Four-component vector of bits"};
    bitVector(m, boolVector2);
    bitVector(m, boolVector3);
    bitVector(m, boolVector4);

    /* Constants. Putting them into math like Python does and as doubles, since
       Python doesn't really differentiate between 32bit and 64bit floats */
    m.attr("pi") = Constantsd::pi();
    m.attr("pi_half") = Constantsd::piHalf();
    m.attr("pi_quarter") = Constantsd::piQuarter();
    m.attr("tau") = Constantsd::tau();
    m.attr("e") = Constantsd::e();
    m.attr("sqrt2") = Constantsd::sqrt2();
    m.attr("sqrt3") = Constantsd::sqrt3();
    m.attr("sqrt_half") = Constantsd::sqrtHalf();
    m.attr("nan") = Constantsd::nan();
    m.attr("inf") = Constantsd::inf();

    /* Functions */
    m
        .def("div", [](Long x, Long y) {
            /** @todo bind Containers::Pair directly */
            return std::pair<Long, Long>(Math::div(x, y));
        }, "Integer division with remainder", py::arg("x"), py::arg("y"))
        /** @todo binomialCoefficient(), asserts are hard to replicate (have an
            internal variant returning an Optional?) */
        .def("popcount", static_cast<UnsignedInt(*)(UnsignedLong)>(Math::popcount), "Count of bits set in a number")

        /* Trigonometry */
        .def("sin", [](Radd angle) { return Math::sin(angle); }, "Sine")
        .def("cos", [](Radd angle) { return Math::cos(angle); }, "Cosine")
        .def("sincos", [](Radd angle) {
            /** @todo bind Containers::Pair directly */
            return std::pair<Double, Double>(Math::sincos(angle));
        }, "Sine and cosine")
        .def("tan", [](Radd angle) { return Math::tan(angle); }, "Tangent")
        .def("asin", [](Double angle) { return Math::asin(angle); }, "Arc sine")
        .def("acos", [](Double angle) { return Math::acos(angle); }, "Arc cosine")
        .def("atan", [](Double angle) { return Math::atan(angle); }, "Arc tangent")

        /* Scalar/vector functions, scalar versions. Vector versions defined
           for each vector variant below; angle versions defined above. */
        .def("isinf", static_cast<bool(*)(Double)>(Math::isInf), "If given number is a positive or negative infinity")
        .def("isnan", static_cast<bool(*)(Double)>(Math::isNan), "If given number is a NaN")
        .def("min", static_cast<Long(*)(Long, Long)>(Math::min), "Minimum", py::arg("value"), py::arg("min"))
        .def("min", static_cast<Double(*)(Double, Double)>(Math::min), "Minimum", py::arg("value"), py::arg("min"))
        .def("max", static_cast<Long(*)(Long, Long)>(Math::max), "Maximum", py::arg("value"), py::arg("min"))
        .def("max", static_cast<Double(*)(Double, Double)>(Math::max), "Maximum", py::arg("value"), py::arg("min"))
        .def("minmax", [](Long a, Long b) {
            /** @todo bind Containers::Pair directly */
            return std::pair<Long, Long>(Math::minmax(a, b));
        }, "Minimum and maximum of two values")
        .def("minmax", [](Double a, Double b) {
            /** @todo bind Containers::Pair directly */
            return std::pair<Double, Double>(Math::minmax(a, b));
        }, "Minimum and maximum of two values")
        .def("clamp", static_cast<Long(*)(Long, Long, Long)>(Math::clamp), "Clamp value", py::arg("value"), py::arg("min"), py::arg("max"))
        .def("clamp", static_cast<Double(*)(Double, Double, Double)>(Math::clamp), "Clamp value", py::arg("value"), py::arg("min"), py::arg("max"))
        .def("sign", Math::sign<Long>, "Sign")
        .def("sign", Math::sign<Double>, "Sign")
        .def("abs", static_cast<Long(*)(Long)>(Math::abs), "Absolute value")
        .def("abs", static_cast<Double(*)(Double)>(Math::abs), "Absolute value")
        .def("floor", static_cast<Double(*)(Double)>(Math::floor), "Nearest not larger integer")
        .def("round", static_cast<Double(*)(Double)>(Math::round), "Round value to nearest integer")
        .def("ceil", static_cast<Double(*)(Double)>(Math::ceil), "Nearest not smaller integer")
        .def("fmod", static_cast<Double(*)(Double, Double)>(Math::fmod), "Floating point division remainder")
        .def("lerp", static_cast<Long(*)(const Long&, const Long&, Double)>(Math::lerp), "Linear interpolation of two values", py::arg("a"), py::arg("b"), py::arg("t"))
        .def("lerp", static_cast<Double(*)(const Double&, const Double&, Double)>(Math::lerp), "Linear interpolation of two values", py::arg("a"), py::arg("b"), py::arg("t"))
        .def("lerp", static_cast<Long(*)(const Long&, const Long&, bool)>(Math::lerp), "Linear interpolation of two values", py::arg("a"), py::arg("b"), py::arg("t"))
        .def("lerp", static_cast<Double(*)(const Double&, const Double&, bool)>(Math::lerp), "Linear interpolation of two values", py::arg("a"), py::arg("b"), py::arg("t"))
        .def("lerp_inverted", static_cast<Double(*)(Double, Double, Double)>(Math::lerpInverted), "Inverse linear interpolation of two values", py::arg("a"), py::arg("b"), py::arg("lerp"))
        .def("select", static_cast<Long(*)(const Long&, const Long&, Double)>(Math::select), "Constant interpolation of two values", py::arg("a"), py::arg("b"), py::arg("t"))
        .def("select", static_cast<Double(*)(const Double&, const Double&, Double)>(Math::select), "Constant interpolation of two values", py::arg("a"), py::arg("b"), py::arg("t"))
        .def("fma", static_cast<Double(*)(Double, Double, Double)>(Math::fma), "Fused multiply-add")

        /* Exponential and power. These are not defined for angles as they
           require the type to be unitless. */
        .def("log", static_cast<UnsignedInt(*)(UnsignedInt, UnsignedInt)>(Math::log), "Integral algorithm", py::arg("base"), py::arg("number"))
        .def("log2", static_cast<UnsignedInt(*)(UnsignedInt)>(Math::log2), "Base-2 integral algorithm")
        .def("log", static_cast<Double(*)(Double)>(Math::log), "Natural algorithm")
        .def("exp", static_cast<Double(*)(Double)>(Math::exp), "Natural exponential")
        .def("pow", static_cast<Double(*)(Double, Double)>(Math::pow), "Power")
        .def("sqrt", static_cast<Double(*)(Double)>(Math::sqrt), "Square root")
        .def("sqrt_inverted", static_cast<Double(*)(Double)>(Math::sqrtInverted), "Square root");

    /* These are needed for the quaternion, so register them before. Double
       versions are called from inside these. */
    magnum::mathVectorFloat(root, m);
    /* Matrices need a metaclass in order to support the magic translation
       attribute, so allocate it here, just once. TODO: I'm not sure who's
       responsible for deleting the object, actually -- however neither pybind
       seems to be destructing the metaclasses in any way, so in the worst case
       it's being done wrong in a consistent way.  */
    magnum::mathMatrixFloat(root, transformationMatrixMetaclass());

    /* Quaternion */
    py::class_<Quaternion> quaternion_(root, "Quaternion", "Float quaternion");
    py::class_<Quaterniond> quaterniond(root, "Quaterniond", "Double quaternion");
    quaternion(m, quaternion_);
    quaternion(m, quaterniond);
    convertible<Quaterniond>(quaternion_);
    convertible<Quaternion>(quaterniond);

    /* Range */
    magnum::mathRange(root, m);
}

}
