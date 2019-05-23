#ifndef magnum_math_vector_h
#define magnum_math_vector_h
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
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Vector4.h>

#include "magnum/math.h"

namespace magnum {

/* Things common for vectors of all sizes and types */
template<class T> void vector(py::module& m, py::class_<T>& c) {
    /*
        Missing APIs:

        from(T*)
        Type
        construction from different types
        VectorNi * VectorN and variants (5)
    */

    m
        .def("dot", [](const T& a, const T& b) { return Math::dot(a, b); },
            "Dot product of two vectors");

    c
        /* Constructors */
        .def_static("zero_init", []() {
            return T{Math::ZeroInit};
        }, "Construct a zero vector")
        .def(py::init(), "Default constructor")
        .def(py::init<typename T::Type>(), "Construct a vector with one value for all components")

        /* Comparison */
        .def(py::self == py::self, "Equality comparison")
        .def(py::self != py::self, "Non-equality comparison")
        .def(py::self < py::self, "Component-wise less than comparison")
        .def(py::self > py::self, "Component-wise greater than comparison")
        .def(py::self <= py::self, "Component-wise less than or equal comparison")
        .def(py::self >= py::self, "Component-wise greater than or equal comparison")

        /* Set / get. Need to throw IndexError in order to allow iteration:
           https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */
        .def("__setitem__", [](T& self, std::size_t i, typename T::Type value) {
            if(i >= T::Size) throw pybind11::index_error{};
            self[i] = value;
        }, "Set a value at given position")
        .def("__getitem__", [](const T& self, std::size_t i) {
            if(i >= T::Size) throw pybind11::index_error{};
            return self[i];
        }, "Value at given position")

        /* Operators */
        .def(-py::self, "Negated vector")
        .def(py::self += py::self, "Add and assign a vector")
        .def(py::self + py::self, "Add a vector")
        .def(py::self -= py::self, "Subtract and assign a vector")
        .def(py::self - py::self, "Subtract a vector")
        .def(py::self *= typename T::Type{}, "Multiply with a scalar and assign")
        .def(py::self * typename T::Type{}, "Multiply with a scalar")
        .def(py::self /= typename T::Type{}, "Divide with a scalar and assign")
        .def(py::self / typename T::Type{}, "Divide with a scalar")
        .def(py::self *= py::self, "Multiply a vector component-wise and assign")
        .def(py::self * py::self, "Multiply a vector component-wise")
        .def(py::self /= py::self, "Divide a vector component-wise and assign")
        .def(py::self / py::self, "Divide a vector component-wise")
        .def(typename T::Type{} * py::self, "Multiply a scalar with a vector")
        .def(typename T::Type{} / py::self, "Divide a vector with a scalar and invert")

        /* Member functions common for floating-point and integer types */
        .def("is_zero", &T::isZero, "Whether the vector is zero")
        .def("dot", static_cast<typename T::Type(T::*)() const>(&T::dot), "Dot product of the vector")
        .def("flipped", &T::flipped, "Flipped vector")
        .def("sum", &T::sum, "Sum of values in the vector")
        .def("product", &T::product, "Product of values in the vector")
        .def("min", &T::min, "Minimal value in the vector")
        .def("max", &T::max, "Maximal value in the vector")
        .def("minmax", &T::minmax, "Minimal and maximal value in the vector")

        .def("__repr__", repr<T>, "Object representation");

    /* Vector length */
    char lenDocstring[] = "Vector size. Returns _.";
    lenDocstring[sizeof(lenDocstring) - 3] = '0' + T::Size;
    c.def_static("__len__", []() { return int(T::Size); }, lenDocstring);
}

template<class T> void vector2(py::class_<Math::Vector2<T>>& c) {
    py::implicitly_convertible<const std::tuple<T, T>&, Math::Vector2<T>>();

    c
        /* Constructors */
        .def(py::init<T, T>(), "Constructor")
        .def(py::init([](const std::tuple<T, T>& value) {
            return Math::Vector2<T>{std::get<0>(value), std::get<1>(value)};
        }), "Construct from a tuple")

        /* Static constructors */
        .def_static("x_axis", &Math::Vector2<T>::xAxis,
            "Vector in a direction of X axis (right)", py::arg("length") = T(1))
        .def_static("y_axis", &Math::Vector2<T>::yAxis,
            "Vector in a direction of Y axis (up)", py::arg("length") = T(1))
        .def_static("x_scale", &Math::Vector2<T>::xScale,
            "Scaling vector in a direction of X axis (width)", py::arg("scale"))
        .def_static("y_scale", &Math::Vector2<T>::yScale,
            "Scaling vector in a direction of Y axis (height)", py::arg("scale"))

        /* Methods */
        .def("perpendicular", &Math::Vector2<T>::perpendicular,
            "Perpendicular vector")

        /* Properties */
        .def_property("x",
            static_cast<T(Math::Vector2<T>::*)() const>(&Math::Vector2<T>::x),
            [](Math::Vector2<T>& self, T value) { self.x() = value; },
            "X component")
        .def_property("y",
            static_cast<T(Math::Vector2<T>::*)() const>(&Math::Vector2<T>::y),
            [](Math::Vector2<T>& self, T value) { self.y() = value; },
            "Y component");
}

template<class T> void vector3(py::class_<Math::Vector3<T>>& c) {
    py::implicitly_convertible<const std::tuple<T, T, T>&, Math::Vector3<T>>();

    c
        /* Constructors */
        .def(py::init<T, T, T>(), "Constructor")
        .def(py::init([](const std::tuple<T, T, T>& value) {
            return Math::Vector3<T>{std::get<0>(value), std::get<1>(value), std::get<2>(value)};
        }), "Construct from a tuple")

        /* Static constructors */
        .def_static("x_axis", &Math::Vector3<T>::xAxis,
            "Vector in a direction of X axis (right)", py::arg("length") = T(1))
        .def_static("y_axis", &Math::Vector3<T>::yAxis,
            "Vector in a direction of Y axis (up)", py::arg("length") = T(1))
        .def_static("z_axis", &Math::Vector3<T>::zAxis,
            "Vector in a direction of Z axis (backward)", py::arg("length") = T(1))
        .def_static("x_scale", &Math::Vector3<T>::xScale,
            "Scaling vector in a direction of X axis (width)", py::arg("scale"))
        .def_static("y_scale", &Math::Vector3<T>::yScale,
            "Scaling vector in a direction of Y axis (height)", py::arg("scale"))
        .def_static("z_scale", &Math::Vector3<T>::zScale,
            "Scaling vector in a direction of Z axis (depth)", py::arg("scale"))

        /* Properties */
        .def_property("x",
            static_cast<T(Math::Vector3<T>::*)() const>(&Math::Vector3<T>::x),
            [](Math::Vector3<T>& self, T value) { self.x() = value; },
            "X component")
        .def_property("y",
            static_cast<T(Math::Vector3<T>::*)() const>(&Math::Vector3<T>::y),
            [](Math::Vector3<T>& self, T value) { self.y() = value; },
            "Y component")
        .def_property("z",
            static_cast<T(Math::Vector3<T>::*)() const>(&Math::Vector3<T>::z),
            [](Math::Vector3<T>& self, T value) { self.z() = value; },
            "Z component")

        .def_property("r",
            static_cast<T(Math::Vector3<T>::*)() const>(&Math::Vector3<T>::r),
            [](Math::Vector3<T>& self, T value) { self.r() = value; },
            "R component")
        .def_property("g",
            static_cast<T(Math::Vector3<T>::*)() const>(&Math::Vector3<T>::g),
            [](Math::Vector3<T>& self, T value) { self.g() = value; },
            "G component")
        .def_property("b",
            static_cast<T(Math::Vector3<T>::*)() const>(&Math::Vector3<T>::b),
            [](Math::Vector3<T>& self, T value) { self.b() = value; },
            "B component")

        .def_property("xy",
            static_cast<const Math::Vector2<T>(Math::Vector3<T>::*)() const>(&Math::Vector3<T>::xy),
            [](Math::Vector3<T>& self, const Math::Vector2<T>& value) { self.xy() = value; },
            "XY part of the vector");
}

template<class T> void vector4(py::class_<Math::Vector4<T>>& c) {
    py::implicitly_convertible<const std::tuple<T, T, T, T>&, Math::Vector4<T>>();

    c
        /* Constructors */
        .def(py::init<T, T, T, T>(), "Constructor")
        .def(py::init([](const std::tuple<T, T, T, T>& value) {
            return Math::Vector4<T>{std::get<0>(value), std::get<1>(value), std::get<2>(value), std::get<3>(value)};
        }), "Construct from a tuple")

        /* Properties */
        .def_property("x",
            static_cast<T(Math::Vector4<T>::*)() const>(&Math::Vector4<T>::x),
            [](Math::Vector4<T>& self, T value) { self.x() = value; },
            "X component")
        .def_property("y",
            static_cast<T(Math::Vector4<T>::*)() const>(&Math::Vector4<T>::y),
            [](Math::Vector4<T>& self, T value) { self.y() = value; },
            "Y component")
        .def_property("z",
            static_cast<T(Math::Vector4<T>::*)() const>(&Math::Vector4<T>::z),
            [](Math::Vector4<T>& self, T value) { self.z() = value; },
            "Z component")
        .def_property("w",
            static_cast<T(Math::Vector4<T>::*)() const>(&Math::Vector4<T>::w),
            [](Math::Vector4<T>& self, T value) { self.w() = value; },
            "W component")

        .def_property("r",
            static_cast<T(Math::Vector4<T>::*)() const>(&Math::Vector4<T>::r),
            [](Math::Vector4<T>& self, T value) { self.r() = value; },
            "R component")
        .def_property("g",
            static_cast<T(Math::Vector4<T>::*)() const>(&Math::Vector4<T>::g),
            [](Math::Vector4<T>& self, T value) { self.g() = value; },
            "G component")
        .def_property("b",
            static_cast<T(Math::Vector4<T>::*)() const>(&Math::Vector4<T>::b),
            [](Math::Vector4<T>& self, T value) { self.b() = value; },
            "B component")
        .def_property("a",
            static_cast<T(Math::Vector4<T>::*)() const>(&Math::Vector4<T>::a),
            [](Math::Vector4<T>& self, T value) { self.a() = value; },
            "A component")

        .def_property("xyz",
            static_cast<const Math::Vector3<T>(Math::Vector4<T>::*)() const>(&Math::Vector4<T>::xyz),
            [](Math::Vector4<T>& self, const Math::Vector3<T>& value) { self.xyz() = value; },
            "XYZ part of the vector")
        .def_property("rgb",
            static_cast<const Math::Vector3<T>(Math::Vector4<T>::*)() const>(&Math::Vector4<T>::rgb),
            [](Math::Vector4<T>& self, const Math::Vector3<T>& value) { self.rgb() = value; },
            "RGB part of the vector")
        .def_property("xy",
            static_cast<const Math::Vector2<T>(Math::Vector4<T>::*)() const>(&Math::Vector4<T>::xy),
            [](Math::Vector4<T>& self, const Math::Vector2<T>& value) { self.xy() = value; },
            "XY part of the vector");
}

template<class U, template<class> class Type, class T, class ...Args> void convertibleImplementation(py::class_<Type<T>, Args...>& c, std::false_type) {
    c.def(py::init<Type<U>>(), "Construct from different underlying type");
}

template<class U, template<class> class Type, class T, class ...Args> void convertibleImplementation(py::class_<Type<T>, Args...>&, std::true_type) {}

template<template<class> class Type, class T, class ...Args> void convertible(py::class_<Type<T>, Args...>& c) {
    convertibleImplementation<UnsignedInt>(c, std::is_same<T, UnsignedInt>{});
    convertibleImplementation<Int>(c, std::is_same<T, Int>{});
    convertibleImplementation<Float>(c, std::is_same<T, Float>{});
    convertibleImplementation<Double>(c, std::is_same<T, Double>{});
}

template<class T, class Base> void color(py::class_<T, Base>& c) {
    c
        .def_static("zero_init", []() {
            return T{Math::ZeroInit};
        }, "Construct a zero color")
        .def(py::init(), "Default constructor");
}

template<class T> void color3(py::class_<Math::Color3<T>, Math::Vector3<T>>& c) {
    py::implicitly_convertible<const std::tuple<T, T, T>&, Math::Color3<T>>();

    c
        /* Constructors */
        .def(py::init<T, T, T>(), "Constructor")
        .def(py::init<T>(), "Construct with one value for all components")
        .def(py::init<Math::Vector3<T>>(), "Construct from a vector")
        .def(py::init([](const std::tuple<T, T, T>& value) {
            return Math::Color3<T>{std::get<0>(value), std::get<1>(value), std::get<2>(value)};
        }), "Construct from a tuple")

        .def_static("from_hsv", [](Degd hue, typename Math::Color3<T>::FloatingPointType saturation, typename Math::Color3<T>::FloatingPointType value) {
            return Math::Color3<T>::fromHsv({Math::Deg<T>(hue), saturation, value});
        }, "Create RGB color from HSV representation", py::arg("hue"), py::arg("saturation"), py::arg("value"))

        /* Accessors */
        .def("to_hsv", [](Math::Color3<T>& self) {
            auto hsv = self.toHsv();
            return std::make_tuple(Degd(hsv.hue), hsv.saturation, hsv.value);
        }, "Convert to HSV representation")
        .def("hue", [](Math::Color3<T>& self) {
            return Degd(self.hue());
        }, "Hue")
        .def("saturation", &Math::Color3<T>::saturation, "Saturation")
        .def("value", &Math::Color3<T>::value, "Value");
}

template<class T> void color4(py::class_<Math::Color4<T>, Math::Vector4<T>>& c) {
    py::implicitly_convertible<const std::tuple<T, T, T>&, Math::Color4<T>>();
    py::implicitly_convertible<const std::tuple<T, T, T, T>&, Math::Color4<T>>();
    py::implicitly_convertible<const Math::Color3<T>&, Math::Color4<T>>();

    c
        /* Constructors */
        .def(py::init<T, T, T, T>(), "Constructor", py::arg("r"), py::arg("g"), py::arg("b"), py::arg("a") = Math::Implementation::fullChannel<T>())
        .def(py::init<T, T>(), "Construct with one value for all components", py::arg("rgb"), py::arg("alpha") = Math::Implementation::fullChannel<T>())
        .def(py::init<Math::Color3<T>, T>(), "Construct from a vector", py::arg("rgb"), py::arg("alpha") = Math::Implementation::fullChannel<T>())
        .def(py::init<Math::Vector4<T>>(), "Construct from a vector")
        .def(py::init([](const std::tuple<T, T, T>& value) {
            return Math::Color4<T>{std::get<0>(value), std::get<1>(value), std::get<2>(value)};
        }), "Construct from a RGB tuple")
        .def(py::init([](const std::tuple<T, T, T, T>& value) {
            return Math::Color4<T>{std::get<0>(value), std::get<1>(value), std::get<2>(value), std::get<3>(value)};
        }), "Construct from a RGBA tuple")

        .def_static("from_hsv", [](Degd hue, typename Math::Color4<T>::FloatingPointType saturation, typename Math::Color4<T>::FloatingPointType value, T alpha) {
            return Math::Color4<T>::fromHsv({Math::Deg<T>(hue), saturation, value}, alpha);
        }, "Create RGB color from HSV representation", py::arg("hue"), py::arg("saturation"), py::arg("value"), py::arg("alpha") = Math::Implementation::fullChannel<T>())

        /* Accessors */
        .def("to_hsv", [](Math::Color4<T>& self) {
            auto hsv = self.toHsv();
            return std::make_tuple(Degd(hsv.hue), hsv.saturation, hsv.value);
        }, "Convert to HSV representation")
        .def("hue", [](Math::Color4<T>& self) {
            return Degd(self.hue());
        }, "Hue")
        .def("saturation", &Math::Color4<T>::saturation, "Saturation")
        .def("value", &Math::Color4<T>::value, "Value");
}

}

#endif
