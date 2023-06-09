#ifndef Magnum_Trade_PythonBindings_h
#define Magnum_Trade_PythonBindings_h
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

#include "Magnum/Trade/Data.h"
#include "Magnum/Trade/MeshData.h" /* :( */

namespace Magnum { namespace Trade {

namespace Implementation {

/* For assertions only */
template<class T> inline bool pyDataFlagsNeedOwner(const T& data) {
    return !(data.dataFlags() & (DataFlag::Owned|DataFlag::Global));
}
inline bool pyDataFlagsNeedOwner(const Trade::MeshData& data) {
    return
        !(data.indexDataFlags() & (DataFlag::Owned|DataFlag::Global)) ||
        !(data.vertexDataFlags() & (DataFlag::Owned|DataFlag::Global));
}

}

/* Stores additional stuff needed for proper refcounting of non-owning FooData.
   Better than subclassing each FooData class because then we would need to
   wrap it every time it's exposed to Python, making 3rd party bindings
   unnecessarily complex */
template<class T> struct PyDataHolder: std::unique_ptr<T> {
    explicit PyDataHolder(T* object): PyDataHolder{object, pybind11::none{}} {
        /* Data without an owner can only be self-owned or global */
        CORRADE_INTERNAL_ASSERT(!Implementation::pyDataFlagsNeedOwner(*object));
    }

    explicit PyDataHolder(T* object, pybind11::object owner): std::unique_ptr<T>{object}, owner{std::move(owner)} {}

    pybind11::object owner;
};

template<class T> PyDataHolder<T> pyDataHolder(T&& data, pybind11::object owner) {
    return PyDataHolder<T>{new T{std::move(data)}, std::move(owner)};
}

}}

PYBIND11_DECLARE_HOLDER_TYPE(T, Magnum::Trade::PyDataHolder<T>)

#endif
