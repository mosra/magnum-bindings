#ifndef corrade_pluginmanager_h
#define corrade_pluginmanager_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024
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

#include <memory> /* :( */
#include <pybind11/pybind11.h>
#include <pybind11/stl.h> /** @todo remove once I can return Array<String> directly */
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/StringStl.h> /** @todo drop once we have our string casters */
#include <Corrade/PluginManager/Manager.h>

#include "Corrade/PythonBindings.h"
#include "Corrade/PluginManager/PythonBindings.h"

#include "corrade/bootstrap.h"

namespace corrade {

template<class T> void plugin(py::class_<T, PluginManager::PyPluginHolder<T>, PluginManager::AbstractPlugin>& c) {
    c
        .def_property_readonly_static("plugin_interface", [](const py::object&) {
            /** @todo drop std::string in favor of our own string caster */
            return std::string{T::pluginInterface()};
        }, "Plugin interface string")
        .def_property_readonly_static("plugin_search_paths", [](const py::object&) {
            /** @todo drop std::string in favor of our own string caster */
            std::vector<std::string> out;
            for(auto&& i: T::pluginSearchPaths())
                out.push_back(i);
            return out;
        }, "Plugin search paths")
        .def_property_readonly_static("plugin_suffix", [](const py::object&) {
            /** @todo drop std::string in favor of our own string caster */
            return std::string{T::pluginSuffix()};
        }, "Plugin binary suffix")
        .def_property_readonly_static("plugin_metadata_suffix", [](const py::object&) {
            /** @todo drop std::string in favor of our own string caster */
            return std::string{T::pluginMetadataSuffix()};
        }, "Plugin metadata file suffix")
        /** @todo plugin interface string, search paths, suffix, metadata file
            suffix (all are static properties) */
        .def_property_readonly("manager", [](const T& self) {
            return pyObjectHolderFor<PluginManager::PyPluginHolder>(self).manager;
        }, "Manager owning this plugin instance");
}

template<class T> void manager(py::class_<PluginManager::Manager<T>, PluginManager::AbstractManager>& c) {
    c
        .def(py::init<const std::string&>(), py::arg("plugin_directory") = std::string{}, "Constructor")
        .def("instantiate", [](PluginManager::Manager<T>& self, const std::string& plugin) {
            /* This causes a double lookup, but well... better than dying */
            if(!(self.loadState(plugin) & PluginManager::LoadState::Loaded)) {
                PyErr_Format(PyExc_RuntimeError, "plugin %s is not loaded", plugin.data());
                throw py::error_already_set{};
            }

            auto loaded = self.instantiate(plugin);
            if(!loaded) {
                PyErr_Format(PyExc_RuntimeError, "can't instantiate plugin %s", plugin.data());
                throw py::error_already_set{};
            }

            return PluginManager::pyPluginHolder(std::move(loaded), py::cast(self));
        }, "Instantiate a plugin")
        .def("load_and_instantiate", [](PluginManager::Manager<T>& self, const std::string& plugin) {
            auto loaded = self.loadAndInstantiate(plugin);
            if(!loaded) {
                PyErr_Format(PyExc_RuntimeError, "can't load and instantiate plugin %s", plugin.data());
                throw py::error_already_set{};
            }

            return PluginManager::pyPluginHolder(std::move(loaded), py::cast(self));
        }, "Load and instantiate plugin");
}

}

#endif
