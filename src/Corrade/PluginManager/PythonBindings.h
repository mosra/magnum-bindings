#ifndef Corrade_PluginManager_PythonBindings_h
#define Corrade_PluginManager_PythonBindings_h
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

#include <memory> /* :( */
#include <pybind11/pybind11.h>
#include <Corrade/Containers/Pointer.h>
#include <Corrade/Utility/Assert.h>

namespace Corrade { namespace PluginManager {

/* Stores additional stuff needed for proper refcounting of plugin instances.
   Due to obvious reasons we can't subclass plugins so this is the only
   possible way. */
template<class T> struct PyPluginHolder: std::unique_ptr<T> {
    explicit PyPluginHolder(T* object) noexcept: std::unique_ptr<T>{object} {
        /* Plugin instance without an owner can only be without a manager and
           thus without any metadata */
        CORRADE_INTERNAL_ASSERT(!object->metadata());
    }

    explicit PyPluginHolder(T* object, pybind11::object manager) noexcept: std::unique_ptr<T>{object}, manager{std::move(manager)} {}

    PyPluginHolder(PyPluginHolder<T>&&) noexcept = default;
    PyPluginHolder(const PyPluginHolder<T>&) = delete;
    PyPluginHolder<T>& operator=(PyPluginHolder<T>&&) noexcept = default;
    PyPluginHolder<T>& operator=(const PyPluginHolder<T>&) = default;

    ~PyPluginHolder() {
        /* On destruction, first `manager` and then the plugin would be
           destroyed, which would mean it asserts due to the manager being
           destructed while plugins are still around. To flip the order, we
           need to reset the pointer first */
        std::unique_ptr<T>::reset();
    }

    pybind11::object manager;
};

template<class T> PyPluginHolder<T> pyPluginHolder(Containers::Pointer<T>&& plugin, pybind11::object owner) {
    return PyPluginHolder<T>{plugin.release(), std::move(owner)};
}

}}

PYBIND11_DECLARE_HOLDER_TYPE(T, Corrade::PluginManager::PyPluginHolder<T>)

#endif
