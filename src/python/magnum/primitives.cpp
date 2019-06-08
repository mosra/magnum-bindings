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
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Square.h>
#include <Magnum/Trade/MeshData2D.h>
#include <Magnum/Trade/MeshData3D.h>

#include "magnum/bootstrap.h"

namespace magnum { namespace {

void primitives(py::module& m) {
    py::module::import("magnum.trade");

    py::enum_<Primitives::SquareTextureCoords>{m, "SquareTextureCoords", "Whether to generate square texture coordinates"}
        .value("DONT_GENERATE", Primitives::SquareTextureCoords::DontGenerate)
        .value("GENERATE", Primitives::SquareTextureCoords::Generate);

    m
        .def("cube_solid", Primitives::cubeSolid, "Solid 3D cube")
        .def("cube_solid_strip", Primitives::cubeSolidStrip, "Solid 3D cube as a single strip")
        .def("cube_wireframe", Primitives::cubeWireframe, "Wireframe 3D cube")

        .def("square_solid", Primitives::squareSolid, "Solid 2D square")
        .def("square_wireframe", Primitives::squareWireframe, "Wireframe 2D square");
}

}}

PYBIND11_MODULE(primitives, m) {
    m.doc() = "Primitive library";

    magnum::primitives(m);
}
