/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022 Vladimír Vondruš <mosra@centrum.cz>

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
#include <pybind11/stl.h> /* for std::vector */
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/ArrayViewStl.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/SceneTools/Hierarchy.h>
#include <Magnum/Trade/SceneData.h>

#include "magnum/bootstrap.h"

namespace magnum {

void scenetools(py::module_& m) {
    m.doc() = "Scene manipulation and optimization tools";

    #ifndef MAGNUM_BUILD_STATIC
    /* These are a part of the same module in the static build, no need to
       import (also can't import because there it's _magnum.*) */
    py::module_::import("magnum.trade");
    #endif

    m
        .def("absolute_field_transformations2d", [](const Trade::SceneData& scene, Trade::SceneField field, const Matrix3& globalTransformation) {
            const Containers::Optional<UnsignedInt> fieldId = scene.findFieldId(field);
            if(!fieldId) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            if(!scene.is2D()) {
                PyErr_SetString(PyExc_AssertionError, "the scene is not 2D");
                throw py::error_already_set{};
            }
            if(!scene.hasField(Trade::SceneField::Parent)) {
                PyErr_SetString(PyExc_AssertionError, "the scene has no hierarchy");
                throw py::error_already_set{};
            }

            /** @todo maybe do a caster for arrays, finally?! */
            std::vector<Matrix3> out(scene.fieldSize(*fieldId));
            SceneTools::absoluteFieldTransformations2DInto(scene, *fieldId, out, globalTransformation);
            return out;
        }, "Calculate absolute 2D transformations for given field", py::arg("scene"), py::arg("field"), py::arg("global_transformation") = Matrix3{})
        .def("absolute_field_transformations2d", [](const Trade::SceneData& scene, UnsignedInt fieldId, const Matrix3& globalTransformation) {
            if(fieldId >= scene.fieldCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            if(!scene.is2D()) {
                PyErr_SetString(PyExc_AssertionError, "the scene is not 2D");
                throw py::error_already_set{};
            }
            if(!scene.hasField(Trade::SceneField::Parent)) {
                PyErr_SetString(PyExc_AssertionError, "the scene has no hierarchy");
                throw py::error_already_set{};
            }

            /** @todo maybe do a caster for arrays, finally?! */
            std::vector<Matrix3> out(scene.fieldSize(fieldId));
            SceneTools::absoluteFieldTransformations2DInto(scene, fieldId, out, globalTransformation);
            return out;
        }, "Calculate absolute 2D transformations for given named field", py::arg("scene"), py::arg("field_id"), py::arg("global_transformation") = Matrix3{})
        .def("absolute_field_transformations3d", [](const Trade::SceneData& scene, Trade::SceneField field, const Matrix4& globalTransformation) {
            const Containers::Optional<UnsignedInt> fieldId = scene.findFieldId(field);
            if(!fieldId) {
                PyErr_SetNone(PyExc_KeyError);
                throw py::error_already_set{};
            }
            if(!scene.is3D()) {
                PyErr_SetString(PyExc_AssertionError, "the scene is not 3D");
                throw py::error_already_set{};
            }
            if(!scene.hasField(Trade::SceneField::Parent)) {
                PyErr_SetString(PyExc_AssertionError, "the scene has no hierarchy");
                throw py::error_already_set{};
            }

            /** @todo maybe do a caster for arrays, finally?! */
            std::vector<Matrix4> out(scene.fieldSize(*fieldId));
            SceneTools::absoluteFieldTransformations3DInto(scene, *fieldId, out, globalTransformation);
            return out;
        }, "Calculate absolute 3D transformations for given field", py::arg("scene"), py::arg("field"), py::arg("global_transformation") = Matrix4{})
        .def("absolute_field_transformations3d", [](const Trade::SceneData& scene, UnsignedInt fieldId, const Matrix4& globalTransformation) {
            if(fieldId >= scene.fieldCount()) {
                PyErr_SetNone(PyExc_IndexError);
                throw py::error_already_set{};
            }
            if(!scene.is3D()) {
                PyErr_SetString(PyExc_AssertionError, "the scene is not 3D");
                throw py::error_already_set{};
            }
            if(!scene.hasField(Trade::SceneField::Parent)) {
                PyErr_SetString(PyExc_AssertionError, "the scene has no hierarchy");
                throw py::error_already_set{};
            }

            /** @todo maybe do a caster for arrays, finally?! */
            std::vector<Matrix4> out(scene.fieldSize(fieldId));
            SceneTools::absoluteFieldTransformations3DInto(scene, fieldId, out, globalTransformation);
            return out;
        }, "Calculate absolute 2D transformations for given named field", py::arg("scene"), py::arg("field_id"), py::arg("global_transformation") = Matrix4{});
}

}

#ifndef MAGNUM_BUILD_STATIC
/* TODO: remove declaration when https://github.com/pybind/pybind11/pull/1863
   is released */
extern "C" PYBIND11_EXPORT PyObject* PyInit_scenetools();
PYBIND11_MODULE(scenetools, m) {
    magnum::scenetools(m);
}
#endif
