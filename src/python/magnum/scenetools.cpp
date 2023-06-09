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

#include <pybind11/pybind11.h>
#include <pybind11/stl.h> /* for std::vector */
#include <Corrade/Containers/ArrayViewStl.h>
#include <Corrade/Containers/BitArray.h>
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pair.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/SceneTools/Filter.h>
#include <Magnum/SceneTools/Hierarchy.h>
#include <Magnum/Trade/SceneData.h>

#include "Corrade/PythonBindings.h"
#include "Magnum/Trade/PythonBindings.h"

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
        .def("filter_fields", [](const Trade::SceneData& scene, const Containers::BitArrayView fieldsToKeep) {
            if(fieldsToKeep.size() != scene.fieldCount()) {
                PyErr_Format(PyExc_AssertionError, "expected %u bits but got %zu", scene.fieldCount(), fieldsToKeep.size());
                throw py::error_already_set{};
            }

            /* If the scene already has an owner, use that instead to avoid
               long reference chains */
            py::object sceneOwner = pyObjectHolderFor<Trade::PyDataHolder>(scene).owner;
            return Trade::pyDataHolder(SceneTools::filterFields(scene, fieldsToKeep), sceneOwner.is_none() ? py::cast(scene) : std::move(sceneOwner));
        }, "Filter a scene to contain only the selected subset of fields", py::arg("scene"), py::arg("fields_to_keep"))
        .def("filter_only_fields", [](const Trade::SceneData& scene, const std::vector<Trade::SceneField> fields) {
            /* If the scene already has an owner, use that instead to avoid
               long reference chains */
            py::object sceneOwner = pyObjectHolderFor<Trade::PyDataHolder>(scene).owner;
            return Trade::pyDataHolder(SceneTools::filterOnlyFields(scene, fields), sceneOwner.is_none() ? py::cast(scene) : std::move(sceneOwner));
        }, "Filter a scene to contain only the selected subset of named fields", py::arg("scene"), py::arg("fields"))
        .def("filter_except_fields", [](const Trade::SceneData& scene, const std::vector<Trade::SceneField> fields) {
            /* If the scene already has an owner, use that instead to avoid
               long reference chains */
            py::object sceneOwner = pyObjectHolderFor<Trade::PyDataHolder>(scene).owner;
            return Trade::pyDataHolder(SceneTools::filterExceptFields(scene, fields), sceneOwner.is_none() ? py::cast(scene) : std::move(sceneOwner));
        }, "Filter a scene to contain everything the selected subset of named fields", py::arg("scene"), py::arg("fields"))
        /** @todo ew, especially the cast .. i hope they have compatible
            layout, not like std::tuple */
        /* The enum-based overloads NEEDS to be before the integer overload,
           otherwise pybind happily uses the enums as integer values!!! */
        .def("filter_field_entries", [](const Trade::SceneData& scene, const std::vector<std::pair<Trade::SceneField, Containers::BitArrayView>> entriesToKeepStl) {
            const auto entriesToKeep = Containers::arrayCast<const Containers::Pair<Trade::SceneField, Containers::BitArrayView>>(Containers::arrayView(entriesToKeepStl));
            Containers::BitArray usedFields{ValueInit, scene.fieldCount()};
            for(std::size_t i = 0; i != entriesToKeep.size(); ++i) {
                const Containers::Optional<UnsignedInt> fieldId = scene.findFieldId(entriesToKeep[i].first());
                if(!fieldId) {
                    PyErr_Format(PyExc_AssertionError, "field at index %zu not found", i, scene.fieldCount());
                    throw py::error_already_set{};
                }
                if(usedFields[*fieldId]) {
                    PyErr_Format(PyExc_AssertionError, "field at index %zu listed more than once", i);
                    throw py::error_already_set{};
                }
                usedFields.set(*fieldId);
                const Containers::BitArrayView mask = entriesToKeep[i].second();
                if(mask.size() != scene.fieldSize(*fieldId)) {
                    PyErr_Format(PyExc_AssertionError, "expected %zu bits for field at index %zu but got %zu", scene.fieldSize(*fieldId), i, mask.size());
                    throw py::error_already_set{};
                }
                const Trade::SceneFieldType fieldType = scene.fieldType(*fieldId);
                if(Trade::Implementation::isSceneFieldTypeString(fieldType)) {
                    PyErr_SetString(PyExc_NotImplementedError, "filtering string fields is not implemented yet, sorry");
                    throw py::error_already_set{};
                }
                if(fieldType == Trade::SceneFieldType::Bit) {
                    PyErr_SetString(PyExc_NotImplementedError, "filtering bit fields is not implemented yet, sorry");
                    throw py::error_already_set{};
                }
            }
            /** @todo check field sharing as well to avoid an assertion --
                make an internal helper in SceneTools or some such, it makes no
                sense to duplicate the whole logic here */

            return SceneTools::filterFieldEntries(scene, entriesToKeep);
        }, "Filter individual entries of named fields in a scene", py::arg("scene"), py::arg("entries_to_keep"))
        .def("filter_field_entries", [](const Trade::SceneData& scene, const std::vector<std::pair<UnsignedInt, Containers::BitArrayView>> entriesToKeepStl) {
            const auto entriesToKeep = Containers::arrayCast<const Containers::Pair<UnsignedInt, Containers::BitArrayView>>(Containers::arrayView(entriesToKeepStl));
            Containers::BitArray usedFields{ValueInit, scene.fieldCount()};
            for(std::size_t i = 0; i != entriesToKeep.size(); ++i) {
                const UnsignedInt fieldId = entriesToKeep[i].first();
                if(fieldId >= scene.fieldCount()) {
                    PyErr_Format(PyExc_AssertionError, "index %u out of range for %u fields", fieldId, scene.fieldCount());
                    throw py::error_already_set{};
                }
                if(usedFields[fieldId]) {
                    PyErr_Format(PyExc_AssertionError, "field %u listed more than once", fieldId);
                    throw py::error_already_set{};
                }
                usedFields.set(fieldId);
                const Containers::BitArrayView mask = entriesToKeep[i].second();
                if(mask.size() != scene.fieldSize(fieldId)) {
                    PyErr_Format(PyExc_AssertionError, "expected %zu bits for field %u but got %zu", scene.fieldSize(fieldId), fieldId, mask.size());
                    throw py::error_already_set{};
                }
                const Trade::SceneFieldType fieldType = scene.fieldType(fieldId);
                if(Trade::Implementation::isSceneFieldTypeString(fieldType)) {
                    PyErr_SetString(PyExc_NotImplementedError, "filtering string fields is not implemented yet, sorry");
                    throw py::error_already_set{};
                }
                if(fieldType == Trade::SceneFieldType::Bit) {
                    PyErr_SetString(PyExc_NotImplementedError, "filtering bit fields is not implemented yet, sorry");
                    throw py::error_already_set{};
                }
            }
            /** @todo check field sharing as well to avoid an assertion --
                make an internal helper in SceneTools or some such, it makes no
                sense to duplicate the whole logic here */

            return SceneTools::filterFieldEntries(scene, entriesToKeep);
        }, "Filter individual entries of fields in a scene", py::arg("scene"), py::arg("entries_to_keep"))
        .def("filter_objects", [](const Trade::SceneData& scene, const Containers::BitArrayView objectsToKeep) {
            if(objectsToKeep.size() != scene.mappingBound()) {
                PyErr_Format(PyExc_AssertionError, "expected %llu bits but got %zu", scene.mappingBound(), objectsToKeep.size());
                throw py::error_already_set{};
            }
            /** @todo this will blow up if any objects have a bit / string
                field, implement that already so it's not needed to check
                here */

            return SceneTools::filterObjects(scene, objectsToKeep);
        }, "Filter objects in a scene", py::arg("scene"), py::arg("objects_to_keep"))
        .def("parents_breadth_first", [](const Trade::SceneData& scene) {
            const Containers::Optional<UnsignedInt> parentFieldId = scene.findFieldId(Trade::SceneField::Parent);
            if(!parentFieldId) {
                PyErr_SetString(PyExc_AssertionError, "the scene has no hierarchy");
                throw py::error_already_set{};
            }

            /** @todo ugh, add type converters for Corrade arrays already */
            std::vector<std::pair<UnsignedInt, Int>> out{scene.fieldSize(*parentFieldId)};
            SceneTools::parentsBreadthFirstInto(scene,
                Containers::stridedArrayView(out).slice(&decltype(out)::value_type::first),
                Containers::stridedArrayView(out).slice(&decltype(out)::value_type::second));
            return out;
        }, "Retrieve parents in a breadth-first order", py::arg("scene"))
        .def("children_depth_first", [](const Trade::SceneData& scene) {
            const Containers::Optional<UnsignedInt> parentFieldId = scene.findFieldId(Trade::SceneField::Parent);
            if(!parentFieldId) {
                PyErr_SetString(PyExc_AssertionError, "the scene has no hierarchy");
                throw py::error_already_set{};
            }

            /** @todo ugh, add type converters for Corrade arrays already */
            std::vector<std::pair<UnsignedInt, UnsignedInt>> out{scene.fieldSize(*parentFieldId)};
            SceneTools::childrenDepthFirstInto(scene,
                Containers::stridedArrayView(out).slice(&decltype(out)::value_type::first),
                Containers::stridedArrayView(out).slice(&decltype(out)::value_type::second));
            return out;
        }, "Retrieve children in a depth-first order", py::arg("scene"))
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
