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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "../bootstrap.h" /* for module / _module alias */

#include "Corrade/Containers/PythonBindings.h"
#include "Corrade/Containers/StridedArrayViewPythonBindings.h"

namespace Corrade { namespace Containers { namespace Implementation {
    template<> constexpr const char* pythonFormatString<std::array<double, 3>>() {
        return "ddd";
    }
    template<> constexpr const char* pythonFormatString<std::pair<std::uint64_t, float>>() {
        return "Qf";
    }
}}}

using namespace Corrade;
namespace py = pybind11;

template<class T> struct Container {
    Container(T a = {}, T b = {}, T c = {}): data{a, b, c, a, b, c} {}

    Containers::StridedArrayView2D<T> view() {
        return {Containers::arrayView(data), {2, 3}};
    }

    std::vector<typename std::remove_const<T>::type> list() const {
        return {data, data + 6};
    }

    T data[3*2]{};
};

struct MutableContainerDynamicType {
    explicit MutableContainerDynamicType(const std::string& format): format{format} {}

    Containers::StridedArrayView2D<void> view() {
        return {Containers::arrayView(data), {2, 3}, {12, 4}};
    }

    std::string format;
    char data[3*2*4]{};
};

template<class T> void container(py::class_<Container<T>>& c) {
    c
        .def(py::init())
        .def_property_readonly("view", [](Container<T>& self) {
            return Containers::pyArrayViewHolder(Containers::PyStridedArrayView<2, typename std::conditional<std::is_const<T>::value, const char, char>::type>{self.view()}, py::cast(self));
        })
        .def_property_readonly("list", &Container<T>::list);
}

/* TODO: remove declaration when https://github.com/pybind/pybind11/pull/1863
   is released */
extern "C" PYBIND11_EXPORT PyObject* PyInit_test_stridedarrayview();
PYBIND11_MODULE(test_stridedarrayview, m) {
    py::module_::import("corrade.containers");

    py::class_<Container<const std::int16_t>> containers{m, "Containers"};
    py::class_<Container<int>> mutableContaineri{m, "MutableContaineri"};
    py::class_<Container<std::array<double, 3>>> mutableContainer3d{m, "MutableContainer3d"};
    py::class_<Container<std::pair<std::uint64_t, float>>> mutableContainerlf{m, "MutableContainerlf"};
    container(containers);
    container(mutableContaineri);
    container(mutableContainer3d);
    container(mutableContainerlf);

    py::class_<MutableContainerDynamicType>{m, "MutableContainerDynamicType"}
        .def(py::init<const std::string&>())
        .def_property_readonly("view", [](MutableContainerDynamicType & self) {
            std::size_t itemsize;
            py::object(*getitem)(const char*);
            void(*setitem)(char*, py::handle);
            if(self.format == "f") {
                itemsize = 4;
                getitem = [](const char* item) {
                    return py::cast(*reinterpret_cast<const float*>(item));
                };
                setitem = [](char* item, py::handle object) {
                    *reinterpret_cast<float*>(item) = py::cast<float>(object);
                };
            } else if(self.format == "i") {
                itemsize = 4;
                getitem = [](const char* item) {
                    return py::cast(*reinterpret_cast<const int*>(item));
                };
                setitem = [](char* item, py::handle object) {
                    *reinterpret_cast<int*>(item) = py::cast<int>(object);
                };
            } else if(self.format == "hh") {
                itemsize = 4;
                getitem = [](const char* item) {
                    return py::cast(*reinterpret_cast<const std::pair<short, short>*>(item));
                };
                setitem = [](char* item, py::handle object) {
                    *reinterpret_cast<std::pair<short, short>*>(item) = py::cast<std::pair<short, short>>(object);
                };
            } else {
                /* py::attribute_error is only since 2.8.1, we're currently
                   testing all the way back to 2.3 */
                PyErr_SetString(PyExc_AttributeError, "");
                throw py::error_already_set{};
            }

            return Containers::pyArrayViewHolder(Containers::PyStridedArrayView<2, char>{Containers::arrayCast<char>(self.view()), self.format.data(), itemsize, getitem, setitem}, py::cast(self));
        });

    m.def("get_containers", []() {
        return Container<const std::int16_t>{3, -17565, 5};
    });
}
