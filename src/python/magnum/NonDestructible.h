#ifndef magnum_NonDestructible_h
#define magnum_NonDestructible_h
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

#include <memory>
#include <Corrade/Utility/Assert.h>

#include "magnum/bootstrap.h"

namespace pybind11 {
    template<typename, typename ...> class class_;
}

/* This is a variant of https://github.com/pybind/pybind11/issues/1178,
   implemented on the client side instead of patching pybind itself */
namespace magnum {

template<class, bool> struct NonDestructibleBaseDeleter;
template<class T> struct NonDestructibleBaseDeleter<T, false> {
    void operator()(T*) { CORRADE_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */ }
};
template<class T> struct NonDestructibleBaseDeleter<T, true> {
    void operator()(T* ptr) { delete ptr; }
};

template<class T> using NonDestructible = py::class_<T, std::unique_ptr<T, NonDestructibleBaseDeleter<T, std::is_destructible<T>::value>>>;

template<class T, class Base> using NonDestructibleBase = py::class_<T, Base, std::unique_ptr<T, NonDestructibleBaseDeleter<T, std::is_destructible<T>::value>>>;

}

#endif
