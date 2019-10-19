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
#include <Magnum/Math/Color.h>
#include <Magnum/Primitives/Axis.h>
#include <Magnum/Primitives/Capsule.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/Primitives/Cone.h>
#include <Magnum/Primitives/Crosshair.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Cylinder.h>
#include <Magnum/Primitives/Gradient.h>
#include <Magnum/Primitives/Grid.h>
#include <Magnum/Primitives/Icosphere.h>
#include <Magnum/Primitives/Line.h>
#include <Magnum/Primitives/Plane.h>
#include <Magnum/Primitives/Square.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/Trade/MeshData2D.h>
#include <Magnum/Trade/MeshData3D.h>

#include "corrade/EnumOperators.h"
#include "magnum/bootstrap.h"

namespace magnum {

void primitives(py::module& m) {
    m.doc() = "Primitive library";

    #ifndef MAGNUM_BUILD_STATIC
    /* These are a part of the same module in the static build, no need to
       import (also can't import because there it's _magnum.*) */
    py::module::import("magnum.trade");
    #endif

    py::enum_<Primitives::CapsuleTextureCoords>{m, "CapsuleTextureCoords", "Whether to generate capsule texture coordinates"}
        .value("DONT_GENERATE", Primitives::CapsuleTextureCoords::DontGenerate)
        .value("GENERATE", Primitives::CapsuleTextureCoords::Generate);

    py::enum_<Primitives::CircleTextureCoords>{m, "CircleTextureCoords", "Whether to generate circle texture coordinates"}
        .value("DONT_GENERATE", Primitives::CircleTextureCoords::DontGenerate)
        .value("GENERATE", Primitives::CircleTextureCoords::Generate);

    py::enum_<Primitives::ConeFlag> coneFlags{m, "ConeFlags", "Cone flags"};
    coneFlags.value("GENERATE_TEXTURE_COORDS", Primitives::ConeFlag::GenerateTextureCoords)
        .value("CAP_END", Primitives::ConeFlag::CapEnd)
        .value("NONE", Primitives::ConeFlag{});
    corrade::enumOperators(coneFlags);

    py::enum_<Primitives::CylinderFlag> cylinderFlags{m, "CylinderFlags", "Cylinder flags"};
    cylinderFlags.value("GENERATE_TEXTURE_COORDS", Primitives::CylinderFlag::GenerateTextureCoords)
        .value("CAP_ENDS", Primitives::CylinderFlag::CapEnds)
        .value("NONE", Primitives::CylinderFlag{});
    corrade::enumOperators(cylinderFlags);

    py::enum_<Primitives::GridFlag> gridFlags{m, "GridFlags", "Grid flags"};
    gridFlags.value("GENERATE_TEXTURE_COORDS", Primitives::GridFlag::GenerateTextureCoords)
        .value("GENERATE_NORMALS", Primitives::GridFlag::GenerateNormals)
        .value("NONE", Primitives::GridFlag{});
    corrade::enumOperators(gridFlags);

    py::enum_<Primitives::PlaneTextureCoords>{m, "PlaneTextureCoords", "Whether to generate plane texture coordinates"}
        .value("DONT_GENERATE", Primitives::PlaneTextureCoords::DontGenerate)
        .value("GENERATE", Primitives::PlaneTextureCoords::Generate);

    py::enum_<Primitives::SquareTextureCoords>{m, "SquareTextureCoords", "Whether to generate square texture coordinates"}
        .value("DONT_GENERATE", Primitives::SquareTextureCoords::DontGenerate)
        .value("GENERATE", Primitives::SquareTextureCoords::Generate);

    py::enum_<Primitives::UVSphereTextureCoords>{m, "UVSphereTextureCoords", "Whether to generate UV sphere texture coordinates"}
        .value("DONT_GENERATE", Primitives::UVSphereTextureCoords::DontGenerate)
        .value("GENERATE", Primitives::UVSphereTextureCoords::Generate);

    m
        .def("axis2d", Primitives::axis2D, "2D axis")
        .def("axis3d", Primitives::axis3D, "3D axis")

        .def("capsule2d_wireframe", Primitives::capsule2DWireframe, "Wireframe 2D capsule", py::arg("hemisphere_rings"), py::arg("cylinder_rings"), py::arg("half_length"))
        .def("capsule3d_solid", Primitives::capsule3DSolid, "Solid 3D capsule", py::arg("hemisphere_rings"), py::arg("cylinder_rings"), py::arg("segments"), py::arg("half_length"), py::arg("texture_coords") = Primitives::CapsuleTextureCoords::DontGenerate)
        .def("capsule3d_wireframe", Primitives::capsule3DWireframe, "Wireframe 3D capsule", py::arg("hemisphere_rings"), py::arg("cylinder_rings"), py::arg("segments"), py::arg("half_length"))

        .def("circle2d_solid", Primitives::circle2DSolid, "Solid 2D circle", py::arg("segments"), py::arg("texture_coords") = Primitives::CircleTextureCoords::DontGenerate)
        .def("circle2d_wireframe", Primitives::circle2DWireframe, "Wireframe 2D circle", py::arg("segments"))
        .def("circle3d_solid", Primitives::circle3DSolid, "Solid 3D circle", py::arg("segments"), py::arg("texture_coords") = Primitives::CircleTextureCoords::DontGenerate)
        .def("circle3d_wireframe", Primitives::circle3DWireframe, "Wireframe 3D circle", py::arg("segments"))

        .def("cone_solid", [](UnsignedInt rings, UnsignedInt segments, Float halfLength, Primitives::ConeFlag flags) {
            return Primitives::coneSolid(rings, segments, halfLength, flags);
        }, "Solid 3D cone", py::arg("rings"), py::arg("segments"), py::arg("half_length"), py::arg("flags") = Primitives::ConeFlag{})
        .def("cone_wireframe", Primitives::coneWireframe, "Wireframe 3D cone", py::arg("segments"), py::arg("half_length"))

        .def("crosshair2d", Primitives::crosshair2D, "2D crosshair")
        .def("crosshair3d", Primitives::crosshair3D, "3D crosshair")

        .def("cube_solid", Primitives::cubeSolid, "Solid 3D cube")
        .def("cube_solid_strip", Primitives::cubeSolidStrip, "Solid 3D cube as a single strip")
        .def("cube_wireframe", Primitives::cubeWireframe, "Wireframe 3D cube")

        .def("cylinder_solid", [](UnsignedInt rings, UnsignedInt segments, Float halfLength, Primitives::CylinderFlag flags) {
            return Primitives::cylinderSolid(rings, segments, halfLength, flags);
        }, "Solid 3D cylinder", py::arg("rings"), py::arg("segments"), py::arg("half_length"), py::arg("flags") = Primitives::CylinderFlag{})
        .def("cylinder_wireframe", Primitives::cylinderWireframe, "Wireframe 3D cylinder", py::arg("rings"), py::arg("segments"), py::arg("half_length"))

        .def("gradient2d", Primitives::gradient2D, "2D square with a gradient", py::arg("a"), py::arg("color_a"), py::arg("b"), py::arg("color_b"))
        .def("gradient2d_horizontal", Primitives::gradient2DHorizontal, "2D square with a horizontal gradient", py::arg("color_left"), py::arg("color_right"))
        .def("gradient2d_vertical", Primitives::gradient2DVertical, "2D square with a vertical gradient", py::arg("color_bottom"), py::arg("color_top"))
        .def("gradient3d", Primitives::gradient3D, "3D plane with a gradient", py::arg("a"), py::arg("color_a"), py::arg("b"), py::arg("color_b"))
        .def("gradient3d_horizontal", Primitives::gradient3DHorizontal, "3D plane with a horizontal gradient", py::arg("color_left"), py::arg("color_right"))
        .def("gradient3d_vertical", Primitives::gradient3DVertical, "3D plane with a vertical gradient", py::arg("color_bottom"), py::arg("color_top"))

        .def("grid3d_solid", [](const Vector2i& subdivisions, Primitives::GridFlag flags) {
            return Primitives::grid3DSolid(subdivisions, flags);
        }, "Solid 3D grid", py::arg("subdivisions"), py::arg("flags") = Primitives::GridFlag::GenerateNormals)
        .def("grid3d_wireframe", Primitives::grid3DWireframe, "Wireframe 3D grid")

        .def("icosphere_solid", Primitives::icosphereSolid, py::arg("subdivisions"))

        .def("line2d", static_cast<Trade::MeshData2D(*)(const Vector2&, const Vector2&)>(Primitives::line2D), "2D line", py::arg("a"), py::arg("b"))
        .def("line2d", static_cast<Trade::MeshData2D(*)()>(Primitives::line2D), "2D line in an identity transformation")
        .def("line3d", static_cast<Trade::MeshData3D(*)(const Vector3&, const Vector3&)>(Primitives::line3D), "3D line", py::arg("a"), py::arg("b"))
        .def("line3d", static_cast<Trade::MeshData3D(*)()>(Primitives::line3D), "3D line in an identity transformation")

        .def("plane_solid", Primitives::planeSolid, "Solid 3D plane", py::arg("texture_coords") = Primitives::PlaneTextureCoords::DontGenerate)
        .def("plane_wireframe", Primitives::planeWireframe, "Wireframe 3D plane")

        .def("square_solid", Primitives::squareSolid, "Solid 2D square", py::arg("texture_coords") = Primitives::SquareTextureCoords::DontGenerate)
        .def("square_wireframe", Primitives::squareWireframe, "Wireframe 2D square")

        .def("uv_sphere_solid", Primitives::uvSphereSolid, "Solid 3D UV sphere", py::arg("rings"), py::arg("segments"), py::arg("texture_coords") = Primitives::UVSphereTextureCoords::DontGenerate)
        .def("uv_sphere_wireframe", Primitives::uvSphereWireframe, "Wireframe 3D UV sphere", py::arg("rings"), py::arg("segments"));
}

}

#ifndef MAGNUM_BUILD_STATIC
/* TODO: remove declaration when https://github.com/pybind/pybind11/pull/1863
   is released */
extern "C" PYBIND11_EXPORT PyObject* PyInit_primitives();
PYBIND11_MODULE(primitives, m) {
    magnum::primitives(m);
}
#endif
