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
#include <pybind11/numpy.h> /* so ArrayView is convertible from python array */
#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/ScopeGuard.h>
#include <Corrade/Utility/FormatStl.h>

#include "corrade/bootstrap.h"
#include "corrade/PyArrayView.h"
#include "corrade/PyBuffer.h"

namespace corrade {

namespace {

const char* const FormatStrings[]{
    /* 0. Representing bytes as unsigned. Not using 'c' because then it behaves
       differently from bytes/bytearray, where you can do `a[0] = ord('A')`. */
    "B",
};
template<class> constexpr std::size_t formatIndex();
template<> constexpr std::size_t formatIndex<char>() { return 0; }

struct Slice {
    std::size_t start;
    std::size_t stop;
    std::ptrdiff_t step;
};

Slice calculateSlice(py::slice slice, std::size_t containerSize) {
    std::size_t size;
    std::ptrdiff_t start, stop, step;

    /* Happens for example when passing a tuple as a slice or a zero step */
    if(!slice.compute(containerSize, reinterpret_cast<std::size_t*>(&start), reinterpret_cast<std::size_t*>(&stop), reinterpret_cast<std::size_t*>(&step), &size))
        throw py::error_already_set{};

    /* If step is negative, start > stop and we have to recalculate */
    CORRADE_INTERNAL_ASSERT((start <= stop) == (step > 0));
    if(step < 0) {
        std::swap(start, stop);
        start += 1;
        stop += 1;
    }

    return Slice{std::size_t(start), std::size_t(stop), step};
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
        buffer.format = const_cast<char*>(FormatStrings[formatIndex<typename std::decay<typename T::Type>::type>()]);
    if(flags != PyBUF_SIMPLE) {
        /* The view is immutable (can't change its size after it has been
           constructed), so referencing the size directly is okay */
        buffer.shape = reinterpret_cast<Py_ssize_t*>(&self.sizeRef());
        if((flags & PyBUF_STRIDES) == PyBUF_STRIDES)
            buffer.strides = &buffer.itemsize;
    }

    return true;
}

template<class T> void arrayView(py::class_<PyArrayView<T>>& c) {
    /* Implicitly convertible from a buffer */
    py::implicitly_convertible<py::buffer, PyArrayView<T>>();

    c
        /* Constructor */
        .def(py::init(), "Default constructor")

        /* Buffer protocol */
        .def(py::init([](py::buffer other) {
            Py_buffer buffer{};
            if(PyObject_GetBuffer(other.ptr(), &buffer, (std::is_const<T>::value ? 0 : PyBUF_WRITABLE)) != 0)
                throw py::error_already_set{};

            Containers::ScopeGuard e{&buffer, PyBuffer_Release};

            if(buffer.ndim != 1)
                throw py::buffer_error{Utility::formatString("expected one dimension but got {}", buffer.ndim)};
            if(buffer.strides && buffer.strides[0] != buffer.itemsize)
                throw py::buffer_error{Utility::formatString("expected stride of {} but got {}", buffer.itemsize, buffer.strides[0])};

            /* reinterpret_borrow converts PyObject* to an (automatically
               refcounted) py::object. We take the underlying object instead of
               the buffer because we no longer care about the buffer
               descriptor -- that could allow the GC to haul away a bit more
               garbage */
            return PyArrayView<T>{{static_cast<T*>(buffer.buf), std::size_t(buffer.len)}, py::reinterpret_borrow<py::object>(buffer.obj)};
        }), "Construct from a buffer")

        /* Length and memory owning object */
        .def("__len__", &PyArrayView<T>::size, "View size")
        .def_readonly("obj", &PyArrayView<T>::obj, "Memory owner object")

        /* Conversion to bytes */
        .def("__bytes__", [](const PyArrayView<T>& self) {
            return py::bytes(self.data(), self.size());
        }, "Convert to bytes")

        /* Single item retrieval. Need to throw IndexError in order to allow
           iteration: https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */
        .def("__getitem__", [](const PyArrayView<T>& self, std::size_t i) {
            if(i >= self.size()) throw pybind11::index_error{};
            return self[i];
        }, "Value at given position")

        /* Slicing */
        .def("__getitem__", [](const PyArrayView<T>& self, py::slice slice) -> py::object {
            const Slice calculated = calculateSlice(slice, self.size());

            /* Non-trivial stride, return a different type */
            if(calculated.step != 1) {
                return py::cast(PyStridedArrayView<1, T>(
                Containers::stridedArrayView(self).slice(calculated.start, calculated.stop).every(calculated.step), self.obj));
            }

            /* Usual business */
            return py::cast(PyArrayView<T>{self.slice(calculated.start, calculated.stop), self.obj});
        }, "Slice the view");

    enableBetterBufferProtocol<PyArrayView<T>, arrayViewBufferProtocol>(c);
}

template<class T> void mutableArrayView(py::class_<PyArrayView<T>>& c) {
    c
        .def("__setitem__", [](const PyArrayView<T>& self, std::size_t i, const T& value) {
            if(i >= self.size()) throw pybind11::index_error{};
            self[i] = value;
        }, "Set a value at given position");
}

/* Tuple for given dimension */
template<unsigned dimensions, class T> struct DimensionsTuple;
template<class T> struct DimensionsTuple<1, T> { typedef std::tuple<T> Type; };
template<class T> struct DimensionsTuple<2, T> { typedef std::tuple<T, T> Type; };
template<class T> struct DimensionsTuple<3, T> { typedef std::tuple<T, T, T> Type; };

/* Size tuple for given dimension */
template<unsigned dimensions> typename DimensionsTuple<dimensions, std::size_t>::Type size(Containers::StridedDimensions<dimensions, std::size_t>);
template<> std::tuple<std::size_t> size(Containers::StridedDimensions<1, std::size_t> size) {
    return std::make_tuple(size[0]);
}
template<> std::tuple<std::size_t, std::size_t> size(Containers::StridedDimensions<2, std::size_t> size) {
    return std::make_tuple(size[0], size[1]);
}
template<> std::tuple<std::size_t, std::size_t, std::size_t> size(Containers::StridedDimensions<3, std::size_t> size) {
    return std::make_tuple(size[0], size[1], size[2]);
}

/* Stride tuple for given dimension */
template<unsigned dimensions> typename DimensionsTuple<dimensions, std::ptrdiff_t>::Type stride(Containers::StridedDimensions<dimensions, std::ptrdiff_t>);
template<> std::tuple<std::ptrdiff_t> stride(Containers::StridedDimensions<1, std::ptrdiff_t> stride) {
    return std::make_tuple(stride[0]);
}
template<> std::tuple<std::ptrdiff_t, std::ptrdiff_t> stride(Containers::StridedDimensions<2, std::ptrdiff_t> stride) {
    return std::make_tuple(stride[0], stride[1]);
}
template<> std::tuple<std::ptrdiff_t, std::ptrdiff_t, std::ptrdiff_t> stride(Containers::StridedDimensions<3, std::ptrdiff_t> stride) {
    return std::make_tuple(stride[0], stride[1], stride[2]);
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

/* Getting a runtime tuple index. Ugh. */
template<class T> const T& dimensionsTupleGet(const typename DimensionsTuple<1, T>::Type& tuple, std::size_t i) {
    if(i == 0) return std::get<0>(tuple);
    CORRADE_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}
template<class T> const T& dimensionsTupleGet(const typename DimensionsTuple<2, T>::Type& tuple, std::size_t i) {
    if(i == 0) return std::get<0>(tuple);
    if(i == 1) return std::get<1>(tuple);
    CORRADE_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
}
template<class T> const T& dimensionsTupleGet(const typename DimensionsTuple<3, T>::Type& tuple, std::size_t i) {
    if(i == 0) return std::get<0>(tuple);
    if(i == 1) return std::get<1>(tuple);
    if(i == 2) return std::get<2>(tuple);
    CORRADE_ASSERT_UNREACHABLE(); /* LCOV_EXCL_LINE */
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
    buffer.itemsize = sizeof(typename T::Type);
    buffer.len = sizeof(typename T::Type);
    for(std::size_t i = 0; i != T::Dimensions; ++i)
        buffer.len *= self.sizeRef()[i];
    buffer.buf = const_cast<typename std::decay<typename T::ErasedType>::type*>(self.data());
    buffer.readonly = std::is_const<typename T::Type>::value;
    if((flags & PyBUF_FORMAT) == PyBUF_FORMAT)
        buffer.format = const_cast<char*>(FormatStrings[formatIndex<typename std::decay<typename T::Type>::type>()]);
    /* The view is immutable (can't change its size after it has been
       constructed), so referencing the size/stride directly is okay */
    buffer.shape = const_cast<Py_ssize_t*>(reinterpret_cast<const Py_ssize_t*>(self.sizeRef().begin()));
    buffer.strides = const_cast<Py_ssize_t*>(reinterpret_cast<const Py_ssize_t*>(self.strideRef().begin()));

    return true;
}

inline std::size_t largerStride(std::size_t a, std::size_t b) {
    return a < b ? b : a; /* max(), but named like this to avoid clashes */
}

template<unsigned dimensions, class T> void stridedArrayView(py::class_<PyStridedArrayView<dimensions, T>>& c) {
    c
        /* Constructor */
        .def(py::init(), "Default constructor")

        /* Buffer protocol */
        .def(py::init([](py::buffer other) {
            Py_buffer buffer{};
            if(PyObject_GetBuffer(other.ptr(), &buffer, PyBUF_STRIDES|(std::is_const<T>::value ? 0 : PyBUF_WRITABLE)) != 0)
                throw py::error_already_set{};

            Containers::ScopeGuard e{&buffer, PyBuffer_Release};

            if(buffer.ndim != dimensions)
                throw py::buffer_error{Utility::formatString("expected {} dimensions but got {}", dimensions, buffer.ndim)};

            Containers::StaticArrayView<dimensions, const std::size_t> sizes{reinterpret_cast<std::size_t*>(buffer.shape)};
            Containers::StaticArrayView<dimensions, const std::ptrdiff_t> strides{reinterpret_cast<std::ptrdiff_t*>(buffer.strides)};
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
            return PyStridedArrayView<dimensions, T>{{
                {static_cast<T*>(buffer.buf), size},
                Containers::StaticArrayView<dimensions, const std::size_t>{reinterpret_cast<std::size_t*>(buffer.shape)},
                Containers::StaticArrayView<dimensions, const std::ptrdiff_t>{reinterpret_cast<std::ptrdiff_t*>(buffer.strides)}},
                py::reinterpret_borrow<py::object>(buffer.obj)};
        }), "Construct from a buffer")

        /* Length, size/stride tuple, dimension count and memory owning object */
        .def("__len__", [](const PyStridedArrayView<dimensions, T>& self) {
            return Containers::StridedDimensions<dimensions, std::size_t>(self.size())[0];
        }, "View size in the top-level dimension")
        .def_property_readonly("size", [](const PyStridedArrayView<dimensions, T>& self) {
            return size<dimensions>(self.size());
        }, "View size in each dimension")
        .def_property_readonly("stride", [](const PyStridedArrayView<dimensions, T>& self) {
            return stride<dimensions>(self.stride());
        }, "View stride in each dimension")
        .def_property_readonly("dimensions", [](const PyStridedArrayView<dimensions, T>&) { return dimensions; }, "Dimension count")
        .def_readonly("obj", &PyStridedArrayView<dimensions, T>::obj, "Memory owner object")

        /* Conversion to bytes */
        .def("__bytes__", [](const PyStridedArrayView<dimensions, T>& self) {
            /* TODO: use _PyBytes_Resize() to avoid the double copy */
            const Containers::Array<char> out = bytes(Containers::arrayCast<const char>(self));
            return py::bytes(out.data(), out.size());
        }, "Convert to bytes")

        /* Slicing of the top dimension */
        .def("__getitem__", [](const PyStridedArrayView<dimensions, T>& self, py::slice slice) -> py::object {
            const Slice calculated = calculateSlice(slice, Containers::StridedDimensions<dimensions, const std::size_t>{self.size()}[0]);
            return py::cast(PyStridedArrayView<dimensions, T>(self.slice(calculated.start, calculated.stop).every(calculated.step), self.obj));
        }, "Slice the view");

    enableBetterBufferProtocol<PyStridedArrayView<dimensions, T>, stridedArrayViewBufferProtocol>(c);
}

template<class T> void stridedArrayView1D(py::class_<PyStridedArrayView<1, T>>& c) {
    c
        /* Single item retrieval. Need to throw IndexError in order to allow
           iteration: https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */
        .def("__getitem__", [](const PyStridedArrayView<1, T>& self, std::size_t i) {
            if(i >= self.size()) throw pybind11::index_error{};
            return self[i];
        }, "Value at given position");
}

template<unsigned dimensions, class T> void stridedArrayViewND(py::class_<PyStridedArrayView<dimensions, T>>& c) {
    c
        /* Sub-view retrieval. Need to throw IndexError in order to allow
           iteration: https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */
        .def("__getitem__", [](const PyStridedArrayView<dimensions, T>& self, std::size_t i) {
            if(i >= Containers::StridedDimensions<dimensions, const std::size_t>{self.size()}[0]) throw pybind11::index_error{};
            return PyStridedArrayView<dimensions - 1, T>{self[i], self.obj};
        }, "Sub-view at given position")

        /* Single-item retrieval. Need to throw IndexError in order to allow
           iteration: https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */

        /* Multi-dimensional slicing */
        .def("__getitem__", [](const PyStridedArrayView<dimensions, T>& self, const typename DimensionsTuple<dimensions, py::slice>::Type& slice) -> py::object {
            Containers::StridedDimensions<dimensions, std::size_t> starts;
            Containers::StridedDimensions<dimensions, std::size_t> stops;
            Containers::StridedDimensions<dimensions, std::ptrdiff_t> steps;

            for(std::size_t i = 0; i != dimensions; ++i) {
                const Slice calculated = calculateSlice(dimensionsTupleGet<py::slice>(slice, i), self.size()[i]);
                starts[i] = calculated.start;
                stops[i] = calculated.stop;
                steps[i] = calculated.step;
            }

            return py::cast(PyStridedArrayView<dimensions, T>(self.slice(starts, stops).every(steps), self.obj));
        }, "Slice the view");
}

template<class T> void stridedArrayView2D(py::class_<PyStridedArrayView<2, T>>& c) {
    c
        .def("__getitem__", [](const PyStridedArrayView<2, T>& self, const std::tuple<std::size_t, std::size_t>& i) {
            if(std::get<0>(i) >= self.size()[0] ||
               std::get<1>(i) >= self.size()[1]) throw py::index_error{};
            return self[std::get<0>(i)][std::get<1>(i)];
        }, "Value at given position")
        .def("transposed", [](const PyStridedArrayView<2, T>& self, const std::size_t a, std::size_t b) {
            if((a == 0 && b == 1) ||
               (a == 1 && b == 0))
                return PyStridedArrayView<2, T>{self.template transposed<0, 1>(), self.obj};
            throw py::value_error{Utility::formatString("dimensions {}, {} can't be transposed in a {}D view", a, b, 2)};
        }, "Transpose two dimensions")
        .def("flipped", [](const PyStridedArrayView<2, T>& self, const std::size_t dimension) {
            if(dimension == 0)
                return PyStridedArrayView<2, T>{self.template flipped<0>(), self.obj};
            if(dimension == 1)
                return PyStridedArrayView<2, T>{self.template flipped<1>(), self.obj};
            throw py::value_error{Utility::formatString("dimension {} out of range for a {}D view", dimension, 2)};
        }, "Flip a dimension")
        .def("broadcasted", [](const PyStridedArrayView<2, T>& self, const std::size_t dimension, std::size_t size) {
            if(dimension == 0)
                return PyStridedArrayView<2, T>{self.template broadcasted<0>(size), self.obj};
            if(dimension == 1)
                return PyStridedArrayView<2, T>{self.template broadcasted<1>(size), self.obj};
            throw py::value_error{Utility::formatString("dimension {} out of range for a {}D view", dimension, 2)};
        }, "Broadcast a dimension");
}

template<class T> void stridedArrayView3D(py::class_<PyStridedArrayView<3, T>>& c) {
    c
        .def("__getitem__", [](const PyStridedArrayView<3, T>& self, const std::tuple<std::size_t, std::size_t, std::size_t>& i) {
            if(std::get<0>(i) >= self.size()[0] ||
               std::get<1>(i) >= self.size()[1] ||
               std::get<2>(i) >= self.size()[2]) throw pybind11::index_error{};
            return self[std::get<0>(i)][std::get<1>(i)][std::get<2>(i)];
        }, "Value at given position")
        .def("transposed", [](const PyStridedArrayView<3, T>& self, const std::size_t a, std::size_t b) {
            if((a == 0 && b == 1) ||
               (a == 1 && b == 0))
                return PyStridedArrayView<3, T>{self.template transposed<0, 1>(), self.obj};
            if((a == 0 && b == 2) ||
               (a == 2 && b == 0))
                return PyStridedArrayView<3, T>{self.template transposed<0, 2>(), self.obj};
            if((a == 1 && b == 2) ||
               (a == 2 && b == 1))
                return PyStridedArrayView<3, T>{self.template transposed<1, 2>(), self.obj};
            throw py::value_error{Utility::formatString("dimensions {}, {} can't be transposed in a {}D view", a, b, 3)};
        }, "Transpose two dimensions")
        .def("flipped", [](const PyStridedArrayView<3, T>& self, const std::size_t dimension) {
            if(dimension == 0)
                return PyStridedArrayView<3, T>{self.template flipped<0>(), self.obj};
            if(dimension == 1)
                return PyStridedArrayView<3, T>{self.template flipped<1>(), self.obj};
            if(dimension == 2)
                return PyStridedArrayView<3, T>{self.template flipped<2>(), self.obj};
            throw py::value_error{Utility::formatString("dimension {} out of range for a {}D view", dimension, 3)};
        }, "Flip a dimension")
        .def("broadcasted", [](const PyStridedArrayView<3, T>& self, const std::size_t dimension, std::size_t size) {
            if(dimension == 0)
                return PyStridedArrayView<3, T>{self.template broadcasted<0>(size), self.obj};
            if(dimension == 1)
                return PyStridedArrayView<3, T>{self.template broadcasted<1>(size), self.obj};
            if(dimension == 2)
                return PyStridedArrayView<3, T>{self.template broadcasted<2>(size), self.obj};
            throw py::value_error{Utility::formatString("dimension {} out of range for a {}D view", dimension, 3)};
        }, "Broadcast a dimension");
}

template<class T> void mutableStridedArrayView1D(py::class_<PyStridedArrayView<1, T>>& c) {
    c
        .def("__setitem__", [](const PyStridedArrayView<1, T>& self, const std::size_t i, const T& value) {
            if(i >= self.size()) throw pybind11::index_error{};
            self[i] = value;
        }, "Set a value at given position");
}

template<class T> void mutableStridedArrayView2D(py::class_<PyStridedArrayView<2, T>>& c) {
    c
        .def("__setitem__", [](const PyStridedArrayView<2, T>& self, const std::tuple<std::size_t, std::size_t>& i, const T& value) {
            if(std::get<0>(i) >= self.size()[0] ||
               std::get<1>(i) >= self.size()[1]) throw pybind11::index_error{};
            self[std::get<0>(i)][std::get<1>(i)] = value;
        }, "Set a value at given position");
}

template<class T> void mutableStridedArrayView3D(py::class_<PyStridedArrayView<3, T>>& c) {
    c
        .def("__setitem__", [](const PyStridedArrayView<3, T>& self, const std::tuple<std::size_t, std::size_t, std::size_t>& i, const T& value) {
            if(std::get<0>(i) >= self.size()[0] ||
               std::get<1>(i) >= self.size()[1] ||
               std::get<2>(i) >= self.size()[2]) throw pybind11::index_error{};
            self[std::get<0>(i)][std::get<1>(i)][std::get<2>(i)] = value;
        }, "Set a value at given position");
}

}

void containers(py::module& m) {
    m.doc() = "Corrade containers module";

    py::class_<PyArrayView<const char>> arrayView_{m,
        "ArrayView", "Array view", py::buffer_protocol{}};
    arrayView(arrayView_);

    py::class_<PyArrayView<char>> mutableArrayView_{m,
        "MutableArrayView", "Mutable array view", py::buffer_protocol{}};
    arrayView(mutableArrayView_);
    mutableArrayView(mutableArrayView_);

    py::class_<PyStridedArrayView<1, const char>> stridedArrayView1D_{m,
        "StridedArrayView1D", "One-dimensional array view with stride information", py::buffer_protocol{}};
    py::class_<PyStridedArrayView<2, const char>> stridedArrayView2D_{m,
        "StridedArrayView2D", "Two-dimensional array view with stride information", py::buffer_protocol{}};
    py::class_<PyStridedArrayView<3, const char>> stridedArrayView3D_{m,
        "StridedArrayView3D", "Three-dimensional array view with stride information", py::buffer_protocol{}};
    stridedArrayView(stridedArrayView1D_);
    stridedArrayView1D(stridedArrayView1D_);
    stridedArrayView(stridedArrayView2D_);
    stridedArrayViewND(stridedArrayView2D_);
    stridedArrayView2D(stridedArrayView2D_);
    stridedArrayView(stridedArrayView3D_);
    stridedArrayViewND(stridedArrayView3D_);
    stridedArrayView3D(stridedArrayView3D_);

    py::class_<PyStridedArrayView<1, char>> mutableStridedArrayView1D_{m,
        "MutableStridedArrayView1D", "Mutable one-dimensional array view with stride information", py::buffer_protocol{}};
    py::class_<PyStridedArrayView<2, char>> mutableStridedArrayView2D_{m,
        "MutableStridedArrayView2D", "Mutable two-dimensional array view with stride information", py::buffer_protocol{}};
    py::class_<PyStridedArrayView<3, char>> mutableStridedArrayView3D_{m,
        "MutableStridedArrayView3D", "Mutable three-dimensional array view with stride information", py::buffer_protocol{}};
    stridedArrayView(mutableStridedArrayView1D_);
    stridedArrayView1D(mutableStridedArrayView1D_);
    stridedArrayView(mutableStridedArrayView2D_);
    stridedArrayViewND(mutableStridedArrayView2D_);
    stridedArrayView2D(mutableStridedArrayView2D_);
    stridedArrayView(mutableStridedArrayView3D_);
    stridedArrayViewND(mutableStridedArrayView3D_);
    stridedArrayView3D(mutableStridedArrayView3D_);
    mutableStridedArrayView1D(mutableStridedArrayView1D_);
    mutableStridedArrayView2D(mutableStridedArrayView2D_);
    mutableStridedArrayView3D(mutableStridedArrayView3D_);
}

}

#ifndef CORRADE_BUILD_STATIC
PYBIND11_MODULE(containers, m) {
    corrade::containers(m);
}
#endif
