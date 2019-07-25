#ifndef corrade_PyArrayView_h
#define corrade_PyArrayView_h
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

#include <pybind11/pybind11.h>
#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/StridedArrayView.h>

#include "corrade/bootstrap.h"

namespace corrade {

/* Wrapper for Containers::ArrayView holding a reference to the memory owner */
template<class T> struct PyArrayView: Containers::ArrayView<T> {
    /*implicit*/PyArrayView() noexcept: obj{py::none{}} {}
    explicit PyArrayView(Containers::ArrayView<T> view, py::object obj) noexcept: Containers::ArrayView<T>{view}, obj{std::move(obj)} {}

    /* Turning this into a reference so buffer protocol can point to it instead
       of needing to allocate */
    std::size_t& sizeRef() { return Containers::ArrayView<T>::_size; }

    py::object obj;
};

/* Wrapper for Containers::StridedArrayView holding a reference to the memory owner */
template<unsigned dimensions, class T> struct PyStridedArrayView: Containers::StridedArrayView<dimensions, T> {
    /*implicit*/ PyStridedArrayView() noexcept: obj{py::none{}} {}
    explicit PyStridedArrayView(Containers::StridedArrayView<dimensions, T> view, py::object obj) noexcept: Containers::StridedArrayView<dimensions, T>{view}, obj{std::move(obj)} {}

    /* Turning this into a reference so buffer protocol can point to it instead
       of needing to allocate */
    Containers::StridedDimensions<dimensions, std::size_t>& sizeRef() {
        return Containers::StridedArrayView<dimensions, T>::_size;
    }
    Containers::StridedDimensions<dimensions, std::ptrdiff_t>& strideRef() {
        return Containers::StridedArrayView<dimensions, T>::_stride;
    }

    py::object obj;
};

}

#endif
