/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024
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
#include <pybind11/stl.h> /* for std::vector */
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/ArrayViewStl.h>
#include <Magnum/MaterialTools/Copy.h>
#include <Magnum/MaterialTools/Filter.h>
#include <Magnum/MaterialTools/Merge.h>
#include <Magnum/MaterialTools/PhongToPbrMetallicRoughness.h>
#include <Magnum/MaterialTools/RemoveDuplicates.h>
#include <Magnum/Trade/MaterialData.h>

#include "Magnum/Trade/PythonBindings.h"
#include "corrade/EnumOperators.h"
#include "magnum/bootstrap.h"

namespace magnum {

void materialtools(py::module_& m) {
    m.doc() = "Material tools";

    #ifndef MAGNUM_BUILD_STATIC
    /* These are a part of the same module in the static build, no need to
       import (also can't import because there it's _magnum.*) */
    py::module_::import("magnum.trade");
    #endif

    py::enum_<MaterialTools::MergeConflicts>{m, "MergeConflicts", "Material merge conflict resolution"}
        .value("FAIL", MaterialTools::MergeConflicts::Fail)
        .value("KEEP_FIRST_IF_SAME_TYPE", MaterialTools::MergeConflicts::KeepFirstIfSameType)
        .value("KEEP_FIRST_IGNORE_TYPE", MaterialTools::MergeConflicts::KeepFirstIgnoreType);

    py::enum_<MaterialTools::PhongToPbrMetallicRoughnessFlag> phongToPbrMetallicRoughnessFlag{m, "PhongToPbrMetallicRoughnessFlags", "Phong to PBR conversion flags"};
    phongToPbrMetallicRoughnessFlag
        .value("KEEP_ORIGINAL_ATTRIBUTES", MaterialTools::PhongToPbrMetallicRoughnessFlag::KeepOriginalAttributes)
        .value("DROP_UNCOVERTIBLE_ATTRIBUTES", MaterialTools::PhongToPbrMetallicRoughnessFlag::DropUnconvertibleAttributes)
        .value("FAIL_ON_UNCONVERTIBLE_ATTRIBUTES", MaterialTools::PhongToPbrMetallicRoughnessFlag::FailOnUnconvertibleAttributes)
        .value("NONE", MaterialTools::PhongToPbrMetallicRoughnessFlag{})
        .value("ALL", MaterialTools::PhongToPbrMetallicRoughnessFlag(Containers::enumCastUnderlyingType(~MaterialTools::PhongToPbrMetallicRoughnessFlags{})));
    corrade::enumOperators(phongToPbrMetallicRoughnessFlag);

    m
        .def("copy", static_cast<Trade::MaterialData(*)(const Trade::MaterialData&)>(MaterialTools::copy), "Make an owned copy of the material", py::arg("material"))
        .def("filter_attributes", [](const Trade::MaterialData& material, const Containers::BitArrayView attributesToKeep, Trade::MaterialType typesToKeep) {
            if(attributesToKeep.size() != material.attributeData().size()) {
                PyErr_Format(PyExc_AssertionError, "expected %u bits but got %zu", material.attributeData().size(), attributesToKeep.size());
                throw py::error_already_set{};
            }

            return MaterialTools::filterAttributes(material, attributesToKeep, typesToKeep);
        }, "Filter material attributes", py::arg("material"), py::arg("attributes_to_keep"), py::arg("types_to_keep") = Trade::MaterialType(Containers::enumCastUnderlyingType(~Trade::MaterialTypes{})))
        .def("filter_layers", [](const Trade::MaterialData& material, const Containers::BitArrayView layersToKeep, Trade::MaterialType typesToKeep) {
            if(layersToKeep.size() != material.layerCount()) {
                PyErr_Format(PyExc_AssertionError, "expected %u bits but got %zu", material.layerCount(), layersToKeep.size());
                throw py::error_already_set{};
            }

            return MaterialTools::filterLayers(material, layersToKeep, typesToKeep);
        }, "Filter material layers", py::arg("material"), py::arg("layers_to_keep"), py::arg("types_to_keep") = Trade::MaterialType(Containers::enumCastUnderlyingType(~Trade::MaterialTypes{})))
        .def("filter_attributes_layers", [](const Trade::MaterialData& material, const Containers::BitArrayView attributesToKeep, const Containers::BitArrayView layersToKeep, Trade::MaterialType typesToKeep) {
            if(attributesToKeep.size() != material.attributeData().size()) {
                PyErr_Format(PyExc_AssertionError, "expected %u attribute bits but got %zu", material.attributeData().size(), attributesToKeep.size());
                throw py::error_already_set{};
            }
            if(layersToKeep.size() != material.layerCount()) {
                PyErr_Format(PyExc_AssertionError, "expected %u layer bits but got %zu", material.layerCount(), layersToKeep.size());
                throw py::error_already_set{};
            }

            return MaterialTools::filterAttributesLayers(material, attributesToKeep, layersToKeep, typesToKeep);
        }, "Filter material attributes and layers", py::arg("material"), py::arg("attributes_to_keep"), py::arg("layers_to_keep"), py::arg("types_to_keep") = Trade::MaterialType(Containers::enumCastUnderlyingType(~Trade::MaterialTypes{})))
        .def("merge", [](const Trade::MaterialData& first, const Trade::MaterialData& second, MaterialTools::MergeConflicts conflicts) {
            Containers::Optional<Trade::MaterialData> out = MaterialTools::merge(first, second, conflicts);
            if(!out) {
                PyErr_SetString(PyExc_RuntimeError, "material merge failed");
                throw py::error_already_set{};
            }

            return *Utility::move(out);
        }, "Merge two materials", py::arg("first"), py::arg("second"), py::arg("conflicts") = MaterialTools::MergeConflicts::Fail)
        .def("phong_to_pbr_metallic_roughness", [](const Trade::MaterialData& material, MaterialTools::PhongToPbrMetallicRoughnessFlag flags) {
            Containers::Optional<Trade::MaterialData> out = MaterialTools::phongToPbrMetallicRoughness(material, flags);
            if(!out) {
                PyErr_SetString(PyExc_RuntimeError, "material conversion failed");
                throw py::error_already_set{};
            }

            return *Utility::move(out);
        }, "Convert a Phong material to PBR metallic/roughness", py::arg("material"), py::arg("flags") = MaterialTools::PhongToPbrMetallicRoughnessFlag{})
        .def("remove_duplicates", [](std::vector<std::reference_wrapper<const Trade::MaterialData>> materials) {
            std::vector<UnsignedInt> indices;
            indices.resize(materials.size());
            const std::size_t count = MaterialTools::removeDuplicatesInto(Containers::Iterable<const Trade::MaterialData>{materials.data(), materials.size(), sizeof(std::reference_wrapper<Trade::MaterialData>), [](const void* data) -> const Trade::MaterialData& {
                return static_cast<const std::reference_wrapper<const Trade::MaterialData>*>(data)->get();
            }}, Containers::arrayView(indices));

            return std::make_pair(std::move(indices), count);
        }, "Remove duplicate materials from a list", py::arg("materials"));
}

}

#ifndef MAGNUM_BUILD_STATIC
/* TODO: remove declaration when https://github.com/pybind/pybind11/pull/1863
   is released */
extern "C" PYBIND11_EXPORT PyObject* PyInit_materialtools();
PYBIND11_MODULE(materialtools, m) {
    magnum::materialtools(m);
}
#endif
