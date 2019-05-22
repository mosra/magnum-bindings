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
#include <Magnum/Trade/MeshData2D.h>
#include <Magnum/Trade/MeshData3D.h>

#include "magnum/bootstrap.h"

namespace magnum { namespace {

template<class T> void meshData(py::class_<T>& c) {
    c
        .def_property_readonly("primitive", &T::primitive, "Primitive")
        .def("is_indexed", &T::isIndexed, "Whether the mesh is indexed");
}

void trade(py::module& m) {
    py::class_<Trade::MeshData2D> meshData2D{m, "MeshData2D", "Two-dimensional mesh data"};
    py::class_<Trade::MeshData3D> meshData3D{m, "MeshData3D", "Three-dimensional mesh data"};
    meshData(meshData2D);
    meshData(meshData3D);
}

}}

PYBIND11_MODULE(trade, m) {
    m.doc() = "Data format exchange";

    magnum::trade(m);
}
