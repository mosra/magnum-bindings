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
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/BitArray.h>
#include <Corrade/Containers/StridedBitArrayView.h>
#include <Corrade/Containers/Pair.h>
#include <Corrade/Containers/ScopeGuard.h>

#include "Corrade/Containers/PythonBindings.h"
#include "Corrade/Containers/StridedArrayViewPythonBindings.h"

#include "corrade/bootstrap.h"
#include "corrade/PyBuffer.h"

namespace corrade {

using namespace Containers::Literals;

namespace {

struct Slice {
    std::size_t start;
    std::size_t stop;
    bool flip;
    std::size_t step;
};

Slice calculateSlice(const py::slice& slice, std::size_t containerSize) {
    std::size_t size;
    std::ptrdiff_t start, stop, step;

    /* Happens for example when passing a tuple as a slice or a zero step */
    if(!slice.compute(containerSize, reinterpret_cast<std::size_t*>(&start), reinterpret_cast<std::size_t*>(&stop), reinterpret_cast<std::size_t*>(&step), &size))
        throw py::error_already_set{};

    /* If step is negative, start > stop and we have to recalculate */
    /** @todo this doesn't seem to be a guarantee, the assert fires with bad
        input as well */
    CORRADE_INTERNAL_ASSERT((start <= stop) == (step > 0));
    bool flip = false;
    if(step < 0) {
        std::swap(start, stop);
        start += 1;
        stop += 1;
        step = -step;
        flip = true;
    }

    return Slice{std::size_t(start), std::size_t(stop), flip, std::size_t(step)};
}

template<class T> Containers::PyArrayViewHolder<T> arrayViewStridedSlice(const T& self, const Slice& calculated, py::object owner) {
    auto sliced = self.slice(calculated.start, calculated.stop);
    /* every() currently accepts negative numbers in StridedArrayView, but in
       the future it will not, flipped() is the better API. StridedBitArrayView
       accepts just an unsigned type. */
    if(calculated.flip)
        sliced = sliced.template flipped<0>();
    sliced = sliced.every(calculated.step);
    return Containers::pyArrayViewHolder(sliced, calculated.start == calculated.stop ? py::none{} : std::move(owner));
}

template<class T> bool arrayViewBufferProtocol(T& self, Py_buffer& buffer, int flags) {
    if((flags & PyBUF_WRITABLE) == PyBUF_WRITABLE && !std::is_const<typename T::Type>::value) {
        PyErr_SetString(PyExc_BufferError, "array view is not writable");
        return false;
    }

    /* I hate the const_casts but I assume this is to make editing easier, NOT
       to make it possible for users to stomp on these values. */
    buffer.ndim = 1;
    buffer.itemsize = sizeof(typename T::Type);
    buffer.len = sizeof(typename T::Type)*self.size();
    buffer.buf = const_cast<typename std::decay<typename T::Type>::type*>(self.data());
    buffer.readonly = std::is_const<typename T::Type>::value;
    if((flags & PyBUF_FORMAT) == PyBUF_FORMAT)
        buffer.format = const_cast<char*>(Containers::Implementation::pythonFormatString<typename std::decay<typename T::Type>::type>());
    if(flags != PyBUF_SIMPLE) {
        /* The view is immutable (can't change its size after it has been
           constructed), so referencing the size directly is okay */
        buffer.shape = reinterpret_cast<Py_ssize_t*>(&Containers::Implementation::sizeRef(self));
        if((flags & PyBUF_STRIDES) == PyBUF_STRIDES)
            buffer.strides = &buffer.itemsize;
    }

    return true;
}

template<class T> void arrayView(py::class_<Containers::ArrayView<T>, Containers::PyArrayViewHolder<Containers::ArrayView<T>>>& c) {
    /* __getitem__ and __setitem__ relies on this, StridedArrayView has the
       same check */
    static_assert(std::is_same<const T, const char>::value, "only the (const) char ArrayView is meant to be exposed");

    /* Implicitly convertible from a buffer */
    py::implicitly_convertible<py::buffer, Containers::ArrayView<T>>();

    c
        /* Constructor */
        .def(py::init(), "Default constructor")

        /* Buffer protocol */
        .def(py::init([](const py::buffer& other) {
            /* GCC 4.8 otherwise loudly complains about missing initializers */
            Py_buffer buffer{nullptr, nullptr, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr, nullptr};
            if(PyObject_GetBuffer(other.ptr(), &buffer, (std::is_const<T>::value ? 0 : PyBUF_WRITABLE)) != 0)
                throw py::error_already_set{};

            Containers::ScopeGuard e{&buffer, PyBuffer_Release};

            /* I would test for dimensions here but np.array() sometimes gives
               0 for an one-dimensional array so ¯\_(ツ)_/¯ */

            if(buffer.strides && buffer.strides[0] != buffer.itemsize) {
                PyErr_Format(PyExc_BufferError, "expected stride of %zi but got %zi", buffer.itemsize, buffer.strides[0]);
                throw py::error_already_set{};
            }

            /* reinterpret_borrow converts PyObject* to an (automatically
               refcounted) py::object. We take the underlying object instead of
               the buffer because we no longer care about the buffer
               descriptor -- that could allow the GC to haul away a bit more
               garbage */
            return Containers::pyArrayViewHolder(Containers::ArrayView<T>{static_cast<T*>(buffer.buf), std::size_t(buffer.len)}, buffer.len ? py::reinterpret_borrow<py::object>(buffer.obj) : py::none{});
        }), "Construct from a buffer")

        /* Length and memory owning object */
        .def("__len__", &Containers::ArrayView<T>::size, "View size")
        .def_property_readonly("owner", [](const Containers::ArrayView<T>& self) {
            return pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner;
        }, "Memory owner object")

        /* Conversion to bytes */
        .def("__bytes__", [](const Containers::ArrayView<T>& self) {
            return py::bytes(self.data(), self.size());
        }, "Convert to bytes")

        /* Single item retrieval. Need to raise IndexError in order to allow
           iteration: https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */
        .def("__getitem__", [](const Containers::ArrayView<T>& self, std::size_t i) {
            if(i >= self.size()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            /* Treating bytes as unsigned 8-bit integers and not as chars for
               consistency with bytes/bytearray, where you have to use
               a[0] = ord('A') to set a character value. Same done for
               __getitem__ and StridedArrayView. */
            return std::uint8_t(self[i]);
        }, "Value at given position", py::arg("i"))

        /* Slicing */
        .def("__getitem__", [](const Containers::ArrayView<T>& self, py::slice slice) -> py::object {
            const Slice calculated = calculateSlice(slice, self.size());

            /* Non-trivial stride, return a different type */
            /** @todo this always assumes bytes for now -- remember the format
                and provide a checked typed conversion API */
            if(calculated.step != 1 || calculated.flip) {
                return pyCastButNotShitty(arrayViewStridedSlice(Containers::PyStridedArrayView<1, T>{Containers::stridedArrayView(self)}, calculated, pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner));
            }

            /* Usual business */
            auto sliced = self.slice(calculated.start, calculated.stop);
            return pyCastButNotShitty(Containers::pyArrayViewHolder(sliced, sliced.size() ? pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner : py::none{}));
        }, "Slice the view", py::arg("slice"));

    enableBetterBufferProtocol<Containers::ArrayView<T>, arrayViewBufferProtocol>(c);
}

template<class T> void bitArrayView(py::class_<Containers::BasicBitArrayView<T>, Containers::PyArrayViewHolder<Containers::BasicBitArrayView<T>>>& c) {
    /* Implicitly convertible from a BitArray */
    py::implicitly_convertible<Containers::BitArray, Containers::BasicBitArrayView<T>>();

    c
        /* Constructor */
        .def(py::init(), "Default constructor")
        .def(py::init([](Containers::BitArray& other) {
            return pyArrayViewHolder(Containers::BasicBitArrayView<T>{other}, other.size() ? py::cast(other) : py::none{});
        }), "Construct a view on a bit array", py::arg("array"))

        /* Offset, size and memory owning object */
        .def_property_readonly("offset", &Containers::BasicBitArrayView<T>::offset, "Bit offset")
        .def("__len__", &Containers::BasicBitArrayView<T>::size, "Size in bits")
        .def_property_readonly("owner", [](const Containers::BasicBitArrayView<T>& self) {
            return pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner;
        }, "Memory owner object")

        /* Single item retrieval. Need to raise IndexError in order to allow
           iteration: https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */
        .def("__getitem__", [](const Containers::BasicBitArrayView<T>& self, std::size_t i) {
            if(i >= self.size()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self[i];
        }, "Bit at given position", py::arg("i"))

        /* Slicing */
        .def("__getitem__", [](const Containers::BasicBitArrayView<T>& self, py::slice slice) -> py::object {
            const Slice calculated = calculateSlice(slice, self.size());

            /* Non-trivial stride, return a different type */
            if(calculated.step != 1 || calculated.flip) {
                return pyCastButNotShitty(arrayViewStridedSlice(Containers::BasicStridedBitArrayView<1, T>{self}, calculated, pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner));
            }

            /* Usual business */
            auto sliced = self.slice(calculated.start, calculated.stop);
            return pyCastButNotShitty(Containers::pyArrayViewHolder(sliced, sliced.size() ? pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner : py::none{}));
        }, "Slice the view", py::arg("slice"));
}

/* Tuple for given dimension */
template<unsigned dimensions, class T> struct DimensionsTuple;
template<class T> struct DimensionsTuple<1, T> { typedef std::tuple<T> Type; };
template<class T> struct DimensionsTuple<2, T> { typedef std::tuple<T, T> Type; };
template<class T> struct DimensionsTuple<3, T> { typedef std::tuple<T, T, T> Type; };
template<class T> struct DimensionsTuple<4, T> { typedef std::tuple<T, T, T, T> Type; };

/* Size tuple for given dimension */
template<unsigned dimensions> typename DimensionsTuple<dimensions, std::size_t>::Type size(const Containers::Size<dimensions>&);
template<> std::tuple<std::size_t> size(const Containers::Size1D& size) {
    return std::make_tuple(size[0]);
}
template<> std::tuple<std::size_t, std::size_t> size(const Containers::Size2D& size) {
    return std::make_tuple(size[0], size[1]);
}
template<> std::tuple<std::size_t, std::size_t, std::size_t> size(const Containers::Size3D& size) {
    return std::make_tuple(size[0], size[1], size[2]);
}
template<> std::tuple<std::size_t, std::size_t, std::size_t, std::size_t> size(const Containers::Size4D& size) {
    return std::make_tuple(size[0], size[1], size[2], size[3]);
}

/* Stride tuple for given dimension */
template<unsigned dimensions> typename DimensionsTuple<dimensions, std::ptrdiff_t>::Type stride(const Containers::Stride<dimensions>&);
template<> std::tuple<std::ptrdiff_t> stride(const Containers::Stride1D& stride) {
    return std::make_tuple(stride[0]);
}
template<> std::tuple<std::ptrdiff_t, std::ptrdiff_t> stride(const Containers::Stride2D& stride) {
    return std::make_tuple(stride[0], stride[1]);
}
template<> std::tuple<std::ptrdiff_t, std::ptrdiff_t, std::ptrdiff_t> stride(const Containers::Stride3D& stride) {
    return std::make_tuple(stride[0], stride[1], stride[2]);
}
template<> std::tuple<std::ptrdiff_t, std::ptrdiff_t, std::ptrdiff_t, std::ptrdiff_t> stride(const Containers::Stride4D& stride) {
    return std::make_tuple(stride[0], stride[1], stride[2], stride[3]);
}

/* Byte conversion for given dimension */
template<unsigned dimensions> Containers::Array<char> bytes(Containers::StridedArrayView<dimensions, const char>);
template<> Containers::Array<char> bytes(Containers::StridedArrayView1D<const char> view) {
    Containers::Array<char> out{view.size()};
    std::size_t pos = 0;
    for(const char i: view) out[pos++] = i;
    return out;
}
template<> Containers::Array<char> bytes(Containers::StridedArrayView2D<const char> view) {
    Containers::Array<char> out{view.size()[0]*view.size()[1]};
    std::size_t pos = 0;
    for(Containers::StridedArrayView1D<const char> i: view)
        for(const char j: i) out[pos++] = j;
    return out;
}
template<> Containers::Array<char> bytes(Containers::StridedArrayView3D<const char> view) {
    Containers::Array<char> out{view.size()[0]*view.size()[1]*view.size()[2]};
    std::size_t pos = 0;
    for(Containers::StridedArrayView2D<const char> i: view)
        for(Containers::StridedArrayView1D<const char> j: i)
            for(const char k: j) out[pos++] = k;
    return out;
}
template<> Containers::Array<char> bytes(Containers::StridedArrayView<4, const char> view) {
    Containers::Array<char> out{view.size()[0]*view.size()[1]*view.size()[2]*view.size()[3]};
    std::size_t pos = 0;
    for(Containers::StridedArrayView3D<const char> i: view)
        for(Containers::StridedArrayView2D<const char> j: i)
            for(Containers::StridedArrayView1D<const char> k: j)
                for(const char l: k) out[pos++] = l;
    return out;
}

/* Getting a runtime tuple index. Ugh. */
template<class T> const T& dimensionsTupleGet(const typename DimensionsTuple<1, T>::Type& tuple, std::size_t i) {
    if(i == 0) return std::get<0>(tuple);
    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}
template<class T> const T& dimensionsTupleGet(const typename DimensionsTuple<2, T>::Type& tuple, std::size_t i) {
    if(i == 0) return std::get<0>(tuple);
    if(i == 1) return std::get<1>(tuple);
    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}
template<class T> const T& dimensionsTupleGet(const typename DimensionsTuple<3, T>::Type& tuple, std::size_t i) {
    if(i == 0) return std::get<0>(tuple);
    if(i == 1) return std::get<1>(tuple);
    if(i == 2) return std::get<2>(tuple);
    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}
template<class T> const T& dimensionsTupleGet(const typename DimensionsTuple<4, T>::Type& tuple, std::size_t i) {
    if(i == 0) return std::get<0>(tuple);
    if(i == 1) return std::get<1>(tuple);
    if(i == 2) return std::get<2>(tuple);
    if(i == 3) return std::get<3>(tuple);
    CORRADE_INTERNAL_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}

template<class T> bool stridedArrayViewBufferProtocol(T& self, Py_buffer& buffer, int flags) {
    if((flags & PyBUF_STRIDES) != PyBUF_STRIDES) {
        /* TODO: allow this if the array actually *is* contiguous? */
        PyErr_SetString(PyExc_BufferError, "array view is not contiguous");
        return false;
    }

    if((flags & PyBUF_WRITABLE) == PyBUF_WRITABLE && !std::is_const<typename T::Type>::value) {
        PyErr_SetString(PyExc_BufferError, "array view is not writable");
        return false;
    }

    /* I hate the const_casts but I assume this is to make editing easier, NOT
       to make it possible for users to stomp on these values. */
    buffer.ndim = T::Dimensions;
    buffer.itemsize = self.itemsize;
    buffer.len = self.itemsize;
    for(std::size_t i = 0; i != T::Dimensions; ++i)
        buffer.len *= Containers::Implementation::sizeRef(self)[i];
    buffer.buf = const_cast<typename std::decay<typename T::ErasedType>::type*>(self.data());
    buffer.readonly = std::is_const<typename T::Type>::value;
    if((flags & PyBUF_FORMAT) == PyBUF_FORMAT)
        /* A valid format shouldn't be an empty string. If it is, it's because
           it was null originally. Pass null in that case again. */
        buffer.format = self.format ? self.format.data() : nullptr;
    /* The view is immutable (can't change its size after it has been
       constructed), so referencing the size/stride directly is okay */
    buffer.shape = const_cast<Py_ssize_t*>(reinterpret_cast<const Py_ssize_t*>(Containers::Implementation::sizeRef(self).begin()));
    buffer.strides = const_cast<Py_ssize_t*>(reinterpret_cast<const Py_ssize_t*>(Containers::Implementation::strideRef(self).begin()));

    return true;
}

inline std::size_t largerStride(std::size_t a, std::size_t b) {
    return a < b ? b : a; /* max(), but named like this to avoid clashes */
}

template<unsigned> struct StridedOperation;
template<> struct StridedOperation<1> {
    template<template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions, T> flipped(const View<dimensions, T>& view, unsigned dimension) {
        if(dimension == 0)
            return view.template flipped<0>();
        PyErr_Format(PyExc_IndexError, "dimension %u out of range for a %iD view", dimension, dimensions);
        throw py::error_already_set{};
    }
    template<template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions, T> broadcasted(const View<dimensions, T>& view, unsigned dimension, std::size_t size) {
        if(dimension == 0) {
            if(Containers::Size<dimensions>{view.size()}[0] != 1) {
                PyErr_Format(PyExc_ValueError, "can't broadcast dimension %u with %zu elements", dimension, Containers::Size<dimensions>{view.size()}[0]);
                throw py::error_already_set{};
            }
            return view.template broadcasted<0>(size);
        }
        PyErr_Format(PyExc_IndexError, "dimension %u out of range for a %iD view", dimension, dimensions);
        throw py::error_already_set{};
    }
    template<unsigned count, template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions + count - 1, T> expanded(const View<dimensions, T>& view, unsigned dimension, const typename DimensionsTuple<count, std::size_t>::Type& size) {
        if(dimension == 0) {
            Containers::Size<count> iSize{NoInit};
            std::size_t totalSize = 1;
            for(std::size_t i = 0; i != count; ++i) {
                iSize[i] = dimensionsTupleGet<std::size_t>(size, i);
                totalSize *= iSize[i];
            }
            if(totalSize != Containers::Size<dimensions>{view.size()}[0]) {
                PyErr_Format(PyExc_ValueError, "total size %zu doesn't match dimension %u with %zu elements", totalSize, dimension, Containers::Size<dimensions>{view.size()}[0]);
                throw py::error_already_set{};
            }
            return view.template expanded<0>(iSize);
        }
        PyErr_Format(PyExc_IndexError, "dimension %u out of range for a %iD view", dimension, dimensions);
        throw py::error_already_set{};
    }
};
template<> struct StridedOperation<2> {
    template<template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions, T> transposed(const View<dimensions, T>& view, unsigned a, unsigned b) {
        if((a == 0 && b == 1) ||
           (a == 1 && b == 0))
            return view.template transposed<0, 1>();
        PyErr_Format(PyExc_IndexError, "dimensions %u, %u can't be transposed in a %iD view", a, b, dimensions);
        throw py::error_already_set{};
    }
    template<template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions, T> flipped(const View<dimensions, T>& view, unsigned dimension) {
        if(dimension == 1)
            return view.template flipped<1>();
        return StridedOperation<1>::flipped(view, dimension);
    }
    template<template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions, T> broadcasted(const View<dimensions, T>& view, unsigned dimension, std::size_t size) {
        if(dimension == 1) {
            if(view.size()[1] != 1) {
                PyErr_Format(PyExc_ValueError, "can't broadcast dimension %u with %zu elements", dimension, view.size()[1]);
                throw py::error_already_set{};
            }
            return view.template broadcasted<1>(size);
        }
        return StridedOperation<1>::broadcasted(view, dimension, size);
    }
    template<unsigned count, template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions + count - 1, T> expanded(const View<dimensions, T>& view, unsigned dimension, const typename DimensionsTuple<count, std::size_t>::Type& size) {
        if(dimension == 1) {
            Containers::Size<count> iSize{NoInit};
            std::size_t totalSize = 1;
            for(std::size_t i = 0; i != count; ++i) {
                iSize[i] = dimensionsTupleGet<std::size_t>(size, i);
                totalSize *= iSize[i];
            }
            if(totalSize != view.size()[1]) {
                PyErr_Format(PyExc_ValueError, "total size %zu doesn't match dimension %u with %zu elements", totalSize, dimension, view.size()[1]);
                throw py::error_already_set{};
            }
            return view.template expanded<1>(iSize);
        }
        return StridedOperation<1>::expanded<count>(view, dimension, size);
    }
};
template<> struct StridedOperation<3> {
    template<template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions, T> transposed(const View<dimensions, T>& view, unsigned a, unsigned b) {
        if((a == 0 && b == 2) ||
           (a == 2 && b == 0))
            return view.template transposed<0, 2>();
        if((a == 1 && b == 2) ||
           (a == 2 && b == 1))
            return view.template transposed<1, 2>();
        return StridedOperation<2>::transposed(view, a, b);
    }
    template<template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions, T> flipped(const View<dimensions, T>& view, unsigned dimension) {
        if(dimension == 2)
            return view.template flipped<2>();
        return StridedOperation<2>::flipped(view, dimension);
    }
    template<template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions, T> broadcasted(const View<dimensions, T>& view, unsigned dimension, std::size_t size) {
        if(dimension == 2) {
            if(view.size()[2] != 1) {
                PyErr_Format(PyExc_ValueError, "can't broadcast dimension %u with %zu elements", dimension, view.size()[2]);
                throw py::error_already_set{};
            }
            return view.template broadcasted<2>(size);
        }
        return StridedOperation<2>::broadcasted(view, dimension, size);
    }
    template<unsigned count, template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions + count - 1, T> expanded(const View<dimensions, T>& view, unsigned dimension, const typename DimensionsTuple<count, std::size_t>::Type& size) {
        if(dimension == 2) {
            Containers::Size<count> iSize{NoInit};
            std::size_t totalSize = 1;
            for(std::size_t i = 0; i != count; ++i) {
                iSize[i] = dimensionsTupleGet<std::size_t>(size, i);
                totalSize *= iSize[i];
            }
            if(totalSize != view.size()[2]) {
                PyErr_Format(PyExc_ValueError, "total size %zu doesn't match dimension %u with %zu elements", totalSize, dimension, view.size()[2]);
                throw py::error_already_set{};
            }
            return view.template expanded<2>(iSize);
        }
        return StridedOperation<2>::expanded<count>(view, dimension, size);
    }
};
template<> struct StridedOperation<4> {
    template<template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions, T> transposed(const View<dimensions, T>& view, unsigned a, unsigned b) {
        if((a == 0 && b == 3) ||
           (a == 3 && b == 0))
            return view.template transposed<0, 3>();
        if((a == 1 && b == 3) ||
           (a == 3 && b == 1))
            return view.template transposed<1, 3>();
        if((a == 2 && b == 3) ||
           (a == 3 && b == 2))
            return view.template transposed<2, 3>();
        return StridedOperation<3>::transposed(view, a, b);
    }
    template<template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions, T> flipped(const View<dimensions, T>& view, unsigned dimension) {
        if(dimension == 3)
            return view.template flipped<3>();
        return StridedOperation<3>::flipped(view, dimension);
    }
    template<template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions, T> broadcasted(const View<dimensions, T>& view, unsigned dimension, std::size_t size) {
        if(dimension == 3) {
            if(view.size()[3] != 1) {
                PyErr_Format(PyExc_ValueError, "can't broadcast dimension %u with %zu elements", dimension, view.size()[3]);
                throw py::error_already_set{};
            }
            return view.template broadcasted<3>(size);
        }
        return StridedOperation<3>::broadcasted(view, dimension, size);
    }
    template<unsigned count, template<unsigned, class> class View, unsigned dimensions, class T> static View<dimensions + count - 1, T> expanded(const View<dimensions, T>& view, unsigned dimension, const typename DimensionsTuple<count, std::size_t>::Type& size) {
        if(dimension == 3) {
            Containers::Size<count> iSize{NoInit};
            std::size_t totalSize = 1;
            for(std::size_t i = 0; i != count; ++i) {
                iSize[i] = dimensionsTupleGet<std::size_t>(size, i);
                totalSize *= iSize[i];
            }
            if(totalSize != view.size()[3]) {
                PyErr_Format(PyExc_ValueError, "total size %zu doesn't match dimension %u with %zu elements", totalSize, dimension, view.size()[3]);
                throw py::error_already_set{};
            }
            return view.template expanded<3>(iSize);
        }
        return StridedOperation<3>::expanded<count>(view, dimension, size);
    }
};

template<unsigned dimensions, template<unsigned> class Steps, class T> Containers::PyArrayViewHolder<T> stridedArrayViewSlice(const T& self, const typename DimensionsTuple<dimensions, py::slice>::Type& slice, py::object owner) {
    Containers::Size<dimensions> starts;
    Containers::Size<dimensions> stops;
    Containers::StridedDimensions<dimensions, bool> flips;
    Steps<dimensions> steps;

    bool empty = false;
    for(std::size_t i = 0; i != dimensions; ++i) {
        const Slice calculated = calculateSlice(dimensionsTupleGet<py::slice>(slice, i), self.size()[i]);
        starts[i] = calculated.start;
        stops[i] = calculated.stop;
        steps[i] = calculated.step;
        flips[i] = calculated.flip;
        steps[i] = calculated.step;

        if(calculated.start == calculated.stop)
            empty = true;
    }

    auto sliced = self.slice(starts, stops);
    /* every() currently accepts negative numbers in StridedArrayView, but in
       the future it will not, flipped() is the better API. StridedBitArrayView
       accepts just an unsigned type. */
    for(std::size_t i = 0; i != dimensions; ++i)
        if(flips[i]) sliced = StridedOperation<dimensions>::flipped(sliced, i);
    sliced = sliced.every(steps);
    return Containers::pyArrayViewHolder(sliced, empty ? py::none{} : std::move(owner));
}

Containers::Pair<py::object(*)(const char*), void(*)(char*, py::handle)> accessorsForFormat(const char* const format) {
    /* The format string can be null, in which case B should be assumed:
       https://docs.python.org/3/c-api/buffer.html#c.Py_buffer.format */
    const Containers::StringView formatString = format ? format : "B"_s;

    /* Matching the entries (and order) in StridedArrayViewPythonBindings.h */
    #define _c(string, type)                                                \
        if(formatString == #string ## _s)  return {                         \
            [](const char* item) {                                          \
                return py::cast(*reinterpret_cast<const type*>(item));      \
            },                                                              \
            [](char* item, py::handle object) {                             \
                *reinterpret_cast<type*>(item) = py::cast<type>(object);    \
            }};
    _c(b, std::int8_t)
    _c(B, std::uint8_t)
    _c(h, std::int16_t)
    _c(H, std::uint16_t)
    _c(i, std::int32_t)
    _c(I, std::uint32_t)
    /** @todo numpy's np.int64 is a `l` even though struct says it's 4 bytes,
        what to do?! */
    _c(q, std::int64_t)
    _c(Q, std::uint64_t)
    _c(f, float)
    _c(d, double)

    return {
        [](const char*) -> py::object {
            PyErr_SetString(PyExc_NotImplementedError, "access to this data format is not implemented, sorry");
            throw py::error_already_set{};
        },
        [](char*, py::handle) {
            PyErr_SetString(PyExc_NotImplementedError, "access to this data format is not implemented, sorry");
            throw py::error_already_set{};
        }};
}

template<unsigned dimensions, class T> void stridedArrayView(py::class_<Containers::PyStridedArrayView<dimensions, T>, Containers::PyArrayViewHolder<Containers::PyStridedArrayView<dimensions, T>>>& c) {
    /* Implicitly convertible from a buffer */
    py::implicitly_convertible<py::buffer, Containers::PyStridedArrayView<dimensions, T>>();

    c
        /* Constructor */
        .def(py::init(), "Default constructor")

        /* Buffer protocol */
        .def(py::init([](const py::buffer& other) {
            /* GCC 4.8 otherwise loudly complains about missing initializers */
            Py_buffer buffer{nullptr, nullptr, 0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr, nullptr};
            if(PyObject_GetBuffer(other.ptr(), &buffer, PyBUF_STRIDES|PyBUF_FORMAT|(std::is_const<T>::value ? 0 : PyBUF_WRITABLE)) != 0)
                throw py::error_already_set{};

            Containers::ScopeGuard e{&buffer, PyBuffer_Release};

            if(buffer.ndim != dimensions) {
                PyErr_Format(PyExc_BufferError, "expected %u dimensions but got %i", dimensions, buffer.ndim);
                throw py::error_already_set{};
            }

            const Containers::Pair<py::object(*)(const char*), void(*)(char*, py::handle)> accessors = accessorsForFormat(buffer.format);

            /* Calculate total memory size that spans the whole view. Mainly to
               make the constructor assert happy, not used otherwise */
            std::size_t size = 0;
            for(std::size_t i = 0; i != dimensions; ++i)
                size = largerStride(buffer.shape[i]*(buffer.strides[i] < 0 ? -buffer.strides[i] : buffer.strides[i]), size);

            /* reinterpret_borrow converts PyObject* to an (automatically
               refcounted) py::object. We take the underlying object instead of
               the buffer because we no longer care about the buffer
               descriptor -- that could allow the GC to haul away a bit more
               garbage */
            return Containers::pyArrayViewHolder(Containers::PyStridedArrayView<dimensions, T>{
                Containers::StridedArrayView<dimensions, T>{
                    {static_cast<T*>(buffer.buf), size},
                    Containers::StaticArrayView<dimensions, const std::size_t>{reinterpret_cast<std::size_t*>(buffer.shape)},
                    Containers::StaticArrayView<dimensions, const std::ptrdiff_t>{reinterpret_cast<std::ptrdiff_t*>(buffer.strides)}},
                buffer.format,
                std::size_t(buffer.itemsize),
                accessors.first(),
                accessors.second()},
                buffer.len ? py::reinterpret_borrow<py::object>(buffer.obj) : py::none{});
        }), "Construct from a buffer")

        /* Length, size/stride tuple, dimension count and memory owning object */
        .def("__len__", [](const Containers::PyStridedArrayView<dimensions, T>& self) {
            return Containers::Size<dimensions>(self.size())[0];
        }, "View size in the top-level dimension")
        .def_property_readonly("size", [](const Containers::PyStridedArrayView<dimensions, T>& self) {
            return size<dimensions>(self.size());
        }, "View size in each dimension")
        .def_property_readonly("stride", [](const Containers::PyStridedArrayView<dimensions, T>& self) {
            return stride<dimensions>(self.stride());
        }, "View stride in each dimension")
        .def_property_readonly("dimensions", [](const Containers::PyStridedArrayView<dimensions, T>&) { return dimensions; }, "Dimension count")
        .def_property_readonly("format", [](const Containers::PyStridedArrayView<dimensions, T>& self) {
            /* A valid format shouldn't be an empty string. If it is, it's
               because it was null originally. Return null in that case to
               turn this into a None, consistently with how Python's
               memoryview etc expects the format strings to be. */
            return self.format ? self.format.data() : nullptr;
        }, "Format of each item")
        .def_property_readonly("owner", [](const Containers::PyStridedArrayView<dimensions, T>& self) {
            return pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner;
        }, "Memory owner object")

        /* Conversion to bytes */
        .def("__bytes__", [](const Containers::PyStridedArrayView<dimensions, T>& self) {
            /* TODO: use _PyBytes_Resize() to avoid the double copy */
            const Containers::Array<char> out = bytes(Containers::arrayCast<const char>(self));
            return py::bytes(out.data(), out.size());
        }, "Convert to bytes")

        /* Slicing of the top dimension */
        .def("__getitem__", [](const Containers::PyStridedArrayView<dimensions, T>& self, py::slice slice) {
            const Slice calculated = calculateSlice(slice, Containers::Size<dimensions>{self.size()}[0]);
            return arrayViewStridedSlice(self, calculated, pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Slice the view", py::arg("slice"))

        /* Fancy operations */
        .def("slice_bit", [](const Containers::PyStridedArrayView<dimensions, T>& self, std::size_t index) {
            if(index >= self.itemsize*8) {
                PyErr_Format(PyExc_IndexError, "index %zu out of range for %zu bits", index, self.itemsize*8);
                throw py::error_already_set{};
            }
            return Containers::pyArrayViewHolder(self.sliceBit(index), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Slice to a bit", py::arg("index"))
        .def("flipped", [](const Containers::PyStridedArrayView<dimensions, T>& self, const std::size_t dimension) {
            return Containers::pyArrayViewHolder(StridedOperation<dimensions>::flipped(self, dimension), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Flip a dimension", py::arg("dimension"))
        .def("broadcasted", [](const Containers::PyStridedArrayView<dimensions, T>& self, const std::size_t dimension, std::size_t size) {
            return Containers::pyArrayViewHolder(StridedOperation<dimensions>::broadcasted(self, dimension, size), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Broadcast a dimension", py::arg("dimension"), py::arg("size"));

    enableBetterBufferProtocol<Containers::PyStridedArrayView<dimensions, T>, stridedArrayViewBufferProtocol>(c);
}

template<class T> void stridedArrayView1D(py::class_<Containers::PyStridedArrayView<1, T>, Containers::PyArrayViewHolder<Containers::PyStridedArrayView<1, T>>>& c) {
    c
        /* Single item retrieval. Need to raise IndexError in order to allow
           iteration: https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */
        .def("__getitem__", [](const Containers::PyStridedArrayView<1, T>& self, std::size_t i) {
            if(i >= self.size()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self.getitem(&self[i]);
        }, "Value at given position", py::arg("i"))

        /* Fancy operations */
        .def("expanded", [](const Containers::PyStridedArrayView<1, T>& self, unsigned dimension, const std::tuple<std::size_t, std::size_t>& size) {
            return Containers::pyArrayViewHolder(StridedOperation<1>::expanded<2>(self, dimension, size), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Expand a dimension", py::arg("dimension"), py::arg("size"))
        .def("expanded", [](const Containers::PyStridedArrayView<1, T>& self, unsigned dimension, const std::tuple<std::size_t, std::size_t, std::size_t>& size) {
            return Containers::pyArrayViewHolder(StridedOperation<1>::expanded<3>(self, dimension, size), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Expand a dimension", py::arg("dimension"), py::arg("size"))
        .def("expanded", [](const Containers::PyStridedArrayView<1, T>& self, unsigned dimension, const std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>& size) {
            return Containers::pyArrayViewHolder(StridedOperation<1>::expanded<4>(self, dimension, size), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Expand a dimension", py::arg("dimension"), py::arg("size"));
}

template<class T> void stridedArrayView2D(py::class_<Containers::PyStridedArrayView<2, T>, Containers::PyArrayViewHolder<Containers::PyStridedArrayView<2, T>>>& c) {
    c
        /* Fancy operations */
        .def("expanded", [](const Containers::PyStridedArrayView<2, T>& self, unsigned dimension, const std::tuple<std::size_t, std::size_t>& size) {
            return Containers::pyArrayViewHolder(StridedOperation<2>::expanded<2>(self, dimension, size), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Expand a dimension", py::arg("dimension"), py::arg("size"))
        .def("expanded", [](const Containers::PyStridedArrayView<2, T>& self, unsigned dimension, const std::tuple<std::size_t, std::size_t, std::size_t>& size) {
            return Containers::pyArrayViewHolder(StridedOperation<2>::expanded<3>(self, dimension, size), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Expand a dimension", py::arg("dimension"), py::arg("size"));
}

template<class T> void stridedArrayView3D(py::class_<Containers::PyStridedArrayView<3, T>, Containers::PyArrayViewHolder<Containers::PyStridedArrayView<3, T>>>& c) {
    c
        /* Fancy operations */
        .def("expanded", [](const Containers::PyStridedArrayView<3, T>& self, unsigned dimension, const std::tuple<std::size_t, std::size_t>& size) {
            return Containers::pyArrayViewHolder(StridedOperation<3>::expanded<2>(self, dimension, size), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Expand a dimension", py::arg("dimension"), py::arg("size"));
}

template<unsigned dimensions, class T> void stridedArrayViewND(py::class_<Containers::PyStridedArrayView<dimensions, T>, Containers::PyArrayViewHolder<Containers::PyStridedArrayView<dimensions, T>>>& c) {
    c
        /* Sub-view and single item retrieval. Need to raise IndexError in
           order to allow iteration: https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */
        .def("__getitem__", [](const Containers::PyStridedArrayView<dimensions, T>& self, std::size_t i) {
            if(i >= Containers::Size<dimensions>{self.size()}[0]) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return Containers::pyArrayViewHolder(self[i], pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Sub-view at given position", py::arg("i"))
        .def("__getitem__", [](const Containers::PyStridedArrayView<dimensions, T>& self, const typename DimensionsTuple<dimensions, std::size_t>::Type& iTuple) {
            Containers::Size<dimensions> iSize{NoInit};
            for(std::size_t j = 0; j != dimensions; ++j) {
                const std::size_t i = dimensionsTupleGet<std::size_t>(iTuple, j);
                if(i >= self.size()[j]) {
                    PyErr_SetNone(PyExc_IndexError);
                    throw py::error_already_set{};
                }
                iSize[j] = i;
            }
            return self.getitem(&self[iSize]);
        }, "Value at given position", py::arg("i"))

        /* Multi-dimensional slicing */
        .def("__getitem__", [](const Containers::PyStridedArrayView<dimensions, T>& self, const typename DimensionsTuple<dimensions, py::slice>::Type& slice) {
            return stridedArrayViewSlice<dimensions, Containers::Stride>(self, slice, pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Slice the view", py::arg("slice"))

        /* Fancy operations */
        .def("transposed", [](const Containers::PyStridedArrayView<dimensions, T>& self, const std::size_t a, std::size_t b) {
            return Containers::pyArrayViewHolder(StridedOperation<dimensions>::transposed(self, a, b), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Transpose two dimensions", py::arg("a"), py::arg("b"));
}

template<unsigned dimensions> void mutableStridedArrayViewND(py::class_<Containers::PyStridedArrayView<dimensions, char>, Containers::PyArrayViewHolder<Containers::PyStridedArrayView<dimensions, char>>>& c) {
    c
        .def("__setitem__", [](const Containers::PyStridedArrayView<dimensions, char>& self, const typename DimensionsTuple<dimensions, std::size_t>::Type& iTuple, py::handle value) {
            Containers::Size<dimensions> iSize{NoInit};
            for(std::size_t j = 0; j != dimensions; ++j) {
                const std::size_t i = dimensionsTupleGet<std::size_t>(iTuple, j);
                if(i >= self.size()[j]) {
                    PyErr_SetNone(PyExc_IndexError);
                    throw py::error_already_set{};
                }
                iSize[j] = i;
            }
            self.setitem(&self[iSize], value);
        }, "Set a value at given position", py::arg("i"), py::arg("value"));
}

template<unsigned dimensions, class T> void stridedBitArrayView(py::class_<Containers::BasicStridedBitArrayView<dimensions, T>, Containers::PyArrayViewHolder<Containers::BasicStridedBitArrayView<dimensions, T>>>& c) {
    c
        /* Constructor */
        .def(py::init(), "Default constructor")

        /* Length, size/stride tuple, dimension count and memory owning object */
        .def_property_readonly("offset", &Containers::BasicStridedBitArrayView<dimensions, T>::offset, "Bit offset")
        .def("__len__", [](const Containers::BasicStridedBitArrayView<dimensions, T>& self) {
            return Containers::Size<dimensions>(self.size())[0];
        }, "View size in the top-level dimension")
        .def_property_readonly("size", [](const Containers::BasicStridedBitArrayView<dimensions, T>& self) {
            return size<dimensions>(self.size());
        }, "View size in each dimension")
        .def_property_readonly("stride", [](const Containers::BasicStridedBitArrayView<dimensions, T>& self) {
            return stride<dimensions>(self.stride());
        }, "View stride in each dimension")
        .def_property_readonly("dimensions", [](const Containers::BasicStridedBitArrayView<dimensions, T>&) { return dimensions; }, "Dimension count")
        .def_property_readonly("owner", [](const Containers::BasicStridedBitArrayView<dimensions, T>& self) {
            return pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner;
        }, "Memory owner object")

        /* Slicing of the top dimension */
        .def("__getitem__", [](const Containers::BasicStridedBitArrayView<dimensions, T>& self, py::slice slice) {
            const Slice calculated = calculateSlice(slice, Containers::Size<dimensions>{self.size()}[0]);
            return arrayViewStridedSlice(self, calculated, pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Slice the view", py::arg("slice"))

        /* Fancy operations */
        .def("flipped", [](const Containers::BasicStridedBitArrayView<dimensions, T>& self, const std::size_t dimension) {
            return Containers::pyArrayViewHolder(StridedOperation<dimensions>::flipped(self, dimension), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Flip a dimension", py::arg("dimension"))
        .def("broadcasted", [](const Containers::BasicStridedBitArrayView<dimensions, T>& self, const std::size_t dimension, std::size_t size) {
            return Containers::pyArrayViewHolder(StridedOperation<dimensions>::broadcasted(self, dimension, size), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Broadcast a dimension", py::arg("dimension"), py::arg("size"));
}

template<class T> void stridedBitArrayView1D(py::class_<Containers::BasicStridedBitArrayView<1, T>, Containers::PyArrayViewHolder<Containers::BasicStridedBitArrayView<1, T>>>& c) {
    /* Implicitly convertible from a BitArray */
    py::implicitly_convertible<Containers::BitArray, Containers::BasicStridedBitArrayView<1, T>>();

    c
        .def(py::init([](Containers::BasicBitArrayView<T>& other) {
            return pyArrayViewHolder(Containers::BasicStridedBitArrayView<1, T>{other}, pyObjectHolderFor<Containers::PyArrayViewHolder>(other).owner);
        }), "Construct from a bit array view", py::arg("view"))
        /* Construction of a const view from a mutable is done directly in
           containers() at the end */

        /* Single item retrieval. Need to raise IndexError in order to allow
           iteration: https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */
        .def("__getitem__", [](const Containers::BasicStridedBitArrayView<1, T>& self, std::size_t i) {
            if(i >= self.size()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self[i];
        }, "Bit at given position", py::arg("i"))

        /* Fancy operations */
        .def("expanded", [](const Containers::BasicStridedBitArrayView<1, T>& self, unsigned dimension, const std::tuple<std::size_t, std::size_t>& size) {
            return Containers::pyArrayViewHolder(StridedOperation<1>::expanded<2>(self, dimension, size), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Expand a dimension", py::arg("dimension"), py::arg("size"))
        .def("expanded", [](const Containers::BasicStridedBitArrayView<1, T>& self, unsigned dimension, const std::tuple<std::size_t, std::size_t, std::size_t>& size) {
            return Containers::pyArrayViewHolder(StridedOperation<1>::expanded<3>(self, dimension, size), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Expand a dimension", py::arg("dimension"), py::arg("size"))
        .def("expanded", [](const Containers::BasicStridedBitArrayView<1, T>& self, unsigned dimension, const std::tuple<std::size_t, std::size_t, std::size_t, std::size_t>& size) {
            return Containers::pyArrayViewHolder(StridedOperation<1>::expanded<4>(self, dimension, size), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Expand a dimension", py::arg("dimension"), py::arg("size"));
}

template<class T> void stridedBitArrayView2D(py::class_<Containers::BasicStridedBitArrayView<2, T>, Containers::PyArrayViewHolder<Containers::BasicStridedBitArrayView<2, T>>>& c) {
    c
        /* Fancy operations */
        .def("expanded", [](const Containers::BasicStridedBitArrayView<2, T>& self, unsigned dimension, const std::tuple<std::size_t, std::size_t>& size) {
            return Containers::pyArrayViewHolder(StridedOperation<2>::expanded<2>(self, dimension, size), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Expand a dimension", py::arg("dimension"), py::arg("size"))
        .def("expanded", [](const Containers::BasicStridedBitArrayView<2, T>& self, unsigned dimension, const std::tuple<std::size_t, std::size_t, std::size_t>& size) {
            return Containers::pyArrayViewHolder(StridedOperation<2>::expanded<3>(self, dimension, size), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Expand a dimension", py::arg("dimension"), py::arg("size"));
}

template<class T> void stridedBitArrayView3D(py::class_<Containers::BasicStridedBitArrayView<3, T>, Containers::PyArrayViewHolder<Containers::BasicStridedBitArrayView<3, T>>>& c) {
    c
        /* Fancy operations */
        .def("expanded", [](const Containers::BasicStridedBitArrayView<3, T>& self, unsigned dimension, const std::tuple<std::size_t, std::size_t>& size) {
            return Containers::pyArrayViewHolder(StridedOperation<3>::expanded<2>(self, dimension, size), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Expand a dimension", py::arg("dimension"), py::arg("size"));
}

template<unsigned dimensions, class T> void stridedBitArrayViewND(py::class_<Containers::BasicStridedBitArrayView<dimensions, T>, Containers::PyArrayViewHolder<Containers::BasicStridedBitArrayView<dimensions, T>>>& c) {
    c
        /* Sub-view and single bit retrieval. Need to raise IndexError in order
           to allow iteration: https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */
        .def("__getitem__", [](const Containers::BasicStridedBitArrayView<dimensions, T>& self, std::size_t i) {
            if(i >= Containers::Size<dimensions>{self.size()}[0]) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return Containers::pyArrayViewHolder(self[i], pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Sub-view at given position", py::arg("i"))
        .def("__getitem__", [](const Containers::BasicStridedBitArrayView<dimensions, T>& self, const typename DimensionsTuple<dimensions, std::size_t>::Type& iTuple) {
            Containers::Size<dimensions> iSize{NoInit};
            for(std::size_t j = 0; j != dimensions; ++j) {
                const std::size_t i = dimensionsTupleGet<std::size_t>(iTuple, j);
                if(i >= self.size()[j]) {
                    PyErr_SetNone(PyExc_IndexError);
                    throw py::error_already_set{};
                }
                iSize[j] = i;
            }
            return self[iSize];
        }, "Bit at given position", py::arg("i"))

        /* Multi-dimensional slicing */
        .def("__getitem__", [](const Containers::BasicStridedBitArrayView<dimensions, T>& self, const typename DimensionsTuple<dimensions, py::slice>::Type& slice) {
            return stridedArrayViewSlice<dimensions, Containers::Size>(self, slice, pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Slice the view", py::arg("slice"))

        /* Fancy operations */
        .def("transposed", [](const Containers::BasicStridedBitArrayView<dimensions, T>& self, const std::size_t a, std::size_t b) {
            return Containers::pyArrayViewHolder(StridedOperation<dimensions>::transposed(self, a, b), pyObjectHolderFor<Containers::PyArrayViewHolder>(self).owner);
        }, "Transpose two dimensions", py::arg("a"), py::arg("b"));
}

template<unsigned dimensions> void mutableStridedBitArrayViewND(py::class_<Containers::BasicStridedBitArrayView<dimensions, char>, Containers::PyArrayViewHolder<Containers::BasicStridedBitArrayView<dimensions, char>>>& c) {
    c
        .def("__setitem__", [](const Containers::BasicStridedBitArrayView<dimensions, char>& self, const typename DimensionsTuple<dimensions, std::size_t>::Type& iTuple, bool value) {
            Containers::Size<dimensions> iSize{NoInit};
            for(std::size_t j = 0; j != dimensions; ++j) {
                const std::size_t i = dimensionsTupleGet<std::size_t>(iTuple, j);
                if(i >= self.size()[j]) {
                    PyErr_SetNone(PyExc_IndexError);
                    throw py::error_already_set{};
                }
                iSize[j] = i;
            }
            self.set(iSize, value);
        }, "Set a bit at given position", py::arg("i"), py::arg("value"));
}

}

void containers(py::module_& m) {
    m.doc() = "Container implementations";

    py::class_<Containers::BitArray>{m, "BitArray", "Bit array"}
        /* Constructors */
        .def_static("value_init", [](std::size_t size) {
            return Containers::BitArray{ValueInit, size};
        }, "Construct a zero-initialized array", py::arg("size"))
        .def_static("no_init", [](std::size_t size) {
            return Containers::BitArray{NoInit, size};
        }, "Construct an array without initializing its contents", py::arg("size"))
        .def_static("direct_init", [](std::size_t size, bool value) {
            return Containers::BitArray{DirectInit, size, value};
        }, "Construct an array initialized to a particular bit value", py::arg("size"), py::arg("value"))
        .def(py::init(), "Default constructor")

        /* Offset, size and memory owning object */
        .def_property_readonly("offset", &Containers::BitArray::offset, "Bit offset")
        .def("__len__", &Containers::BitArray::size, "Size in bits")

        /* Single bit access. Need to raise IndexError in order to allow
           iteration: https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */
        .def("__getitem__", [](const Containers::BitArray& self, std::size_t i) {
            if(i >= self.size()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            return self[i];
        }, "Bit at given position", py::arg("i"))
        .def("__setitem__", [](Containers::BitArray& self, std::size_t i, const bool value) {
            if(i >= self.size()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            self.set(i, value);
        }, "Set a bit at given position", py::arg("i"), py::arg("value"))

        /* Slicing */
        .def("__getitem__", [](Containers::BitArray& self, py::slice slice) -> py::object {
            const Slice calculated = calculateSlice(slice, self.size());

            /* Non-trivial stride, return a different type */
            if(calculated.step != 1 || calculated.flip) {
                /** @todo what's up with the cast?! */
                return pyCastButNotShitty(arrayViewStridedSlice(Containers::MutableStridedBitArrayView1D{Containers::MutableBitArrayView{self}}, calculated, py::cast(self)));
            }

            /* Usual business */
            const auto sliced = self.slice(calculated.start, calculated.stop);
            return pyCastButNotShitty(Containers::pyArrayViewHolder(sliced, sliced.size() ? py::cast(self) : py::none{}));
        }, "Slice the view", py::arg("slice"));

    py::class_<Containers::ArrayView<const char>, Containers::PyArrayViewHolder<Containers::ArrayView<const char>>> arrayView_{m,
        "ArrayView", "Array view", py::buffer_protocol{}};
    arrayView(arrayView_);

    py::class_<Containers::ArrayView<char>, Containers::PyArrayViewHolder<Containers::ArrayView<char>>> mutableArrayView_{m,
        "MutableArrayView", "Mutable array view", py::buffer_protocol{}};
    arrayView(mutableArrayView_);
    mutableArrayView_
        /* Treating bytes as unsigned 8-bit integers and not as chars for
           consistency with bytes/bytearray, where you have to use
           a[0] = ord('A') to set a character value. Same done for __getitem__
           and StridedArrayView. */
        .def("__setitem__", [](const Containers::ArrayView<char>& self, std::size_t i, std::uint8_t value) {
            if(i >= self.size()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            self[i] = value;
        }, "Set a value at given position", py::arg("i"), py::arg("value"));

    py::class_<Containers::BitArrayView, Containers::PyArrayViewHolder<Containers::BitArrayView>> bitArrayView_{m,
        "BitArrayView", "Bit array view"};
    bitArrayView(bitArrayView_);

    py::class_<Containers::MutableBitArrayView, Containers::PyArrayViewHolder<Containers::MutableBitArrayView>> mutableBitArrayView_{m,
        "MutableBitArrayView", "Mutable bit array view"};
    bitArrayView(mutableBitArrayView_);
    mutableBitArrayView_
        .def("__setitem__", [](const Containers::MutableBitArrayView& self, std::size_t i, bool value) {
            if(i >= self.size()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            self.set(i, value);
        }, "Set a bit at given position", py::arg("i"), py::arg("value"));

    /* These have to be defined before StridedArrayView types in order to have
       them ready for the return type of sliceBit() */
    py::class_<Containers::StridedBitArrayView1D, Containers::PyArrayViewHolder<Containers::StridedBitArrayView1D>> stridedBitArrayView1D_{m,
        "StridedBitArrayView1D", "One-dimensional bit array view with stride information"};
    py::class_<Containers::StridedBitArrayView2D, Containers::PyArrayViewHolder<Containers::StridedBitArrayView2D>> stridedBitArrayView2D_{m,
        "StridedBitArrayView2D", "Two-dimensional bit array view with stride information"};
    py::class_<Containers::StridedBitArrayView3D, Containers::PyArrayViewHolder<Containers::StridedBitArrayView3D>> stridedBitArrayView3D_{m,
        "StridedBitArrayView3D", "Three-dimensional bit array view with stride information"};
    py::class_<Containers::StridedBitArrayView4D, Containers::PyArrayViewHolder<Containers::StridedBitArrayView4D>> stridedBitArrayView4D_{m,
        "StridedBitArrayView4D", "Four-dimensional bit array view with stride information"};
    stridedBitArrayView(stridedBitArrayView1D_);
    stridedBitArrayView1D(stridedBitArrayView1D_);
    stridedBitArrayView1D_
        .def(py::init([](Containers::MutableBitArrayView& other) {
            return pyArrayViewHolder(Containers::StridedBitArrayView1D{other}, pyObjectHolderFor<Containers::PyArrayViewHolder>(other).owner);
        }), "Construct from a bit array view", py::arg("view"));
    stridedBitArrayView(stridedBitArrayView2D_);
    stridedBitArrayView2D(stridedBitArrayView2D_);
    stridedBitArrayViewND(stridedBitArrayView2D_);
    stridedBitArrayView(stridedBitArrayView3D_);
    stridedBitArrayView3D(stridedBitArrayView3D_);
    stridedBitArrayViewND(stridedBitArrayView3D_);
    stridedBitArrayView(stridedBitArrayView4D_);
    stridedBitArrayViewND(stridedBitArrayView4D_);

    py::class_<Containers::MutableStridedBitArrayView1D, Containers::PyArrayViewHolder<Containers::MutableStridedBitArrayView1D>> mutableStridedBitArrayView1D_{m,
        "MutableStridedBitArrayView1D", "Mutable one-dimensional bit array view with stride information", py::buffer_protocol{}};
    py::class_<Containers::MutableStridedBitArrayView2D, Containers::PyArrayViewHolder<Containers::MutableStridedBitArrayView2D>> mutableStridedBitArrayView2D_{m,
        "MutableStridedBitArrayView2D", "Mutable two-dimensional bit array view with stride information", py::buffer_protocol{}};
    py::class_<Containers::MutableStridedBitArrayView3D, Containers::PyArrayViewHolder<Containers::MutableStridedBitArrayView3D>> mutableStridedBitArrayView3D_{m,
        "MutableStridedBitArrayView3D", "Mutable three-dimensional bit array view with stride information", py::buffer_protocol{}};
    py::class_<Containers::MutableStridedBitArrayView4D, Containers::PyArrayViewHolder<Containers::MutableStridedBitArrayView4D>> mutableStridedBitArrayView4D_{m,
        "MutableStridedBitArrayView4D", "Mutable four-dimensional bit array view with stride information", py::buffer_protocol{}};
    stridedBitArrayView(mutableStridedBitArrayView1D_);
    stridedBitArrayView1D(mutableStridedBitArrayView1D_);
    stridedBitArrayView(mutableStridedBitArrayView2D_);
    stridedBitArrayView2D(mutableStridedBitArrayView2D_);
    stridedBitArrayViewND(mutableStridedBitArrayView2D_);
    stridedBitArrayView(mutableStridedBitArrayView3D_);
    stridedBitArrayView3D(mutableStridedBitArrayView3D_);
    stridedBitArrayViewND(mutableStridedBitArrayView3D_);
    stridedBitArrayView(mutableStridedBitArrayView4D_);
    stridedBitArrayViewND(mutableStridedBitArrayView4D_);
    mutableStridedBitArrayView1D_
        .def("__setitem__", [](const Containers::BasicStridedBitArrayView<1, char>& self, const std::size_t i, bool value) {
            if(i >= self.size()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            self.set(i, value);
        }, "Set a bit at given position", py::arg("i"), py::arg("value"));
    mutableStridedBitArrayViewND(mutableStridedBitArrayView2D_);
    mutableStridedBitArrayViewND(mutableStridedBitArrayView3D_);
    mutableStridedBitArrayViewND(mutableStridedBitArrayView4D_);

    py::class_<Containers::PyStridedArrayView<1, const char>, Containers::PyArrayViewHolder<Containers::PyStridedArrayView<1, const char>>> stridedArrayView1D_{m,
        "StridedArrayView1D", "One-dimensional array view with stride information", py::buffer_protocol{}};
    py::class_<Containers::PyStridedArrayView<2, const char>, Containers::PyArrayViewHolder<Containers::PyStridedArrayView<2, const char>>> stridedArrayView2D_{m,
        "StridedArrayView2D", "Two-dimensional array view with stride information", py::buffer_protocol{}};
    py::class_<Containers::PyStridedArrayView<3, const char>, Containers::PyArrayViewHolder<Containers::PyStridedArrayView<3, const char>>> stridedArrayView3D_{m,
        "StridedArrayView3D", "Three-dimensional array view with stride information", py::buffer_protocol{}};
    py::class_<Containers::PyStridedArrayView<4, const char>, Containers::PyArrayViewHolder<Containers::PyStridedArrayView<4, const char>>> stridedArrayView4D_{m,
        "StridedArrayView4D", "Four-dimensional array view with stride information", py::buffer_protocol{}};
    stridedArrayView(stridedArrayView1D_);
    stridedArrayView1D(stridedArrayView1D_);
    stridedArrayView(stridedArrayView2D_);
    stridedArrayView2D(stridedArrayView2D_);
    stridedArrayViewND(stridedArrayView2D_);
    stridedArrayView(stridedArrayView3D_);
    stridedArrayView3D(stridedArrayView3D_);
    stridedArrayViewND(stridedArrayView3D_);
    stridedArrayView(stridedArrayView4D_);
    stridedArrayViewND(stridedArrayView4D_);

    py::class_<Containers::PyStridedArrayView<1, char>, Containers::PyArrayViewHolder<Containers::PyStridedArrayView<1, char>>> mutableStridedArrayView1D_{m,
        "MutableStridedArrayView1D", "Mutable one-dimensional array view with stride information", py::buffer_protocol{}};
    py::class_<Containers::PyStridedArrayView<2, char>, Containers::PyArrayViewHolder<Containers::PyStridedArrayView<2, char>>> mutableStridedArrayView2D_{m,
        "MutableStridedArrayView2D", "Mutable two-dimensional array view with stride information", py::buffer_protocol{}};
    py::class_<Containers::PyStridedArrayView<3, char>, Containers::PyArrayViewHolder<Containers::PyStridedArrayView<3, char>>> mutableStridedArrayView3D_{m,
        "MutableStridedArrayView3D", "Mutable three-dimensional array view with stride information", py::buffer_protocol{}};
    py::class_<Containers::PyStridedArrayView<4, char>, Containers::PyArrayViewHolder<Containers::PyStridedArrayView<4, char>>> mutableStridedArrayView4D_{m,
        "MutableStridedArrayView4D", "Mutable four-dimensional array view with stride information", py::buffer_protocol{}};
    stridedArrayView(mutableStridedArrayView1D_);
    stridedArrayView1D(mutableStridedArrayView1D_);
    stridedArrayView(mutableStridedArrayView2D_);
    stridedArrayView2D(mutableStridedArrayView2D_);
    stridedArrayViewND(mutableStridedArrayView2D_);
    stridedArrayView(mutableStridedArrayView3D_);
    stridedArrayView3D(mutableStridedArrayView3D_);
    stridedArrayViewND(mutableStridedArrayView3D_);
    stridedArrayView(mutableStridedArrayView4D_);
    stridedArrayViewND(mutableStridedArrayView4D_);
    mutableStridedArrayView1D_
        .def("__setitem__", [](const Containers::PyStridedArrayView<1, char>& self, const std::size_t i, py::handle value) {
            if(i >= self.size()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            self.setitem(&self[i], value);
        }, "Set a value at given position", py::arg("i"), py::arg("value"));
    mutableStridedArrayViewND(mutableStridedArrayView2D_);
    mutableStridedArrayViewND(mutableStridedArrayView3D_);
    mutableStridedArrayViewND(mutableStridedArrayView4D_);
}

}

#ifndef CORRADE_BUILD_STATIC
/* TODO: remove declaration when https://github.com/pybind/pybind11/pull/1863
   is released */
extern "C" PYBIND11_EXPORT PyObject* PyInit_containers();
PYBIND11_MODULE(containers, m) {
    corrade::containers(m);
}
#endif
