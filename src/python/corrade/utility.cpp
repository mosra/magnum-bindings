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
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Assert.h>
#include <Corrade/Utility/Configuration.h>

#include "corrade/bootstrap.h"

#include "Corrade/Containers/StridedArrayViewPythonBindings.h"

namespace corrade {

template<unsigned dimensions> void algorithmsCopy(py::module_& m) {
    m.def("copy", [](const Containers::PyStridedArrayView<dimensions, const char>& src, const Containers::PyStridedArrayView<dimensions, char>& dst) {
        if(src.size() != dst.size()) {
            PyErr_SetString(PyExc_AssertionError, "sizes don't match");
            throw py::error_already_set{};
        }
        if(src.itemsize != dst.itemsize) {
            PyErr_SetString(PyExc_AssertionError, "type sizes don't match");
            throw py::error_already_set{};
        }
        if(Containers::StringView{src.format} != Containers::StringView{dst.format}) {
            PyErr_SetString(PyExc_AssertionError, "types don't match");
            throw py::error_already_set{};
        }

        Utility::copy(
            Containers::arrayCast<dimensions + 1, const char>(Containers::StridedArrayView<dimensions, const void>{src}, src.itemsize),
            Containers::arrayCast<dimensions + 1, char>(Containers::StridedArrayView<dimensions, void>{dst}, dst.itemsize));
    }, "Copy a strided array view to another", py::arg("src"), py::arg("dst"));
}

void utility(py::module_& m) {
    m.doc() = "Utilities";

    #ifndef CORRADE_BUILD_STATIC
    /* Need array views for copy() and others. These are a part of the same
       module in the static build, no need to import (also can't import because
       there it's _corrade.*) */
    py::module_::import("corrade.containers");
    #endif

    algorithmsCopy<1>(m);
    algorithmsCopy<2>(m);
    algorithmsCopy<3>(m);
    algorithmsCopy<4>(m);

    py::class_<Utility::ConfigurationGroup>{m, "ConfigurationGroup", "Group of values in a configuration file"}
        .def_property_readonly("has_groups", &Utility::ConfigurationGroup::hasGroups, "Whether this group has any subgroups")
        .def("group", [](Utility::ConfigurationGroup& self, const std::string& name) {
            Utility::ConfigurationGroup* group = self.group(name);
            if(!group) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            return group;
        }, "Group", py::arg("name"), py::return_value_policy::reference_internal)
        .def("add_group", [](Utility::ConfigurationGroup& self, const std::string& name) {
            Utility::ConfigurationGroup* group = self.addGroup(name);
            CORRADE_INTERNAL_ASSERT(group);
            return group;
        }, "Add a group", py::arg("name"), py::return_value_policy::reference_internal)
        .def_property_readonly("has_values", &Utility::ConfigurationGroup::hasValues, "Whether this group has any values")
        .def("__getitem__", [](Utility::ConfigurationGroup& self, const std::string& key) {
            /** @todo should return an Optional once ConfigurationGroup is
                reworked */
            return self.value(key);
        }, "Value", py::arg("key"))
        .def("__setitem__", [](Utility::ConfigurationGroup& self, const std::string& key, const std::string& value) {
            self.setValue(key, value);
        }, "Set a value", py::arg("key"), py::arg("value"))
        .def("__setitem__", [](Utility::ConfigurationGroup& self, const std::string& key, double value) {
            self.setValue(key, value);
        }, "Set a value", py::arg("key"), py::arg("value"))
        .def("__setitem__", [](Utility::ConfigurationGroup& self, const std::string& key, std::int64_t value) {
            self.setValue(key, value);
        }, "Set a value", py::arg("key"), py::arg("value"))
        .def("__setitem__", [](Utility::ConfigurationGroup& self, const std::string& key, bool value) {
            self.setValue(key, value);
        }, "Set a value", py::arg("key"), py::arg("value"));

    py::class_<Utility::Configuration, Utility::ConfigurationGroup>{m, "Configuration", "Parser and writer for configuration files"}
        .def(py::init(), "Construct an empty configuration")
        .def(py::init([](const std::string& filename) {
            std::unique_ptr<Utility::Configuration> self{new Utility::Configuration{filename}};
            if(!self->isValid()) {
                PyErr_SetNone(PyExc_IOError);
                throw py::error_already_set{};
            }
            return self;
        }), "Parse a configuration file", py::arg("filename"))
        .def("save", [](Utility::Configuration& self) {
            if(!self.save()) {
                PyErr_SetNone(PyExc_IOError);
                throw py::error_already_set{};
            }
        }, "Save the configuration")
        .def("save", [](Utility::Configuration& self, const std::string& filename) {
            if(!self.save(filename)) {
                PyErr_SetNone(PyExc_IOError);
                throw py::error_already_set{};
            }
        }, "Save the configuration to another file", py::arg("filename"));
}

}

#ifndef CORRADE_BUILD_STATIC
/* TODO: remove declaration when https://github.com/pybind/pybind11/pull/1863
   is released */
extern "C" PYBIND11_EXPORT PyObject* PyInit_utility();
PYBIND11_MODULE(utility, m) {
    corrade::utility(m);
}
#endif
