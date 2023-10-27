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
#include <Corrade/Containers/BitArrayView.h>
#include <Corrade/Containers/Optional.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Copy.h>
#include <Magnum/MeshTools/Concatenate.h>
#include <Magnum/MeshTools/Duplicate.h>
#include <Magnum/MeshTools/Filter.h>
#include <Magnum/MeshTools/GenerateIndices.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/RemoveDuplicates.h>
#include <Magnum/MeshTools/Transform.h>
#include <Magnum/Trade/MeshData.h>

#include "Corrade/PythonBindings.h"
#include "Magnum/Trade/PythonBindings.h"

#include "corrade/EnumOperators.h"
#include "magnum/bootstrap.h"

namespace magnum {

void meshtools(py::module_& m) {
    m.doc() = "Mesh tools";

    #ifndef MAGNUM_BUILD_STATIC
    /* These are a part of the same module in the static build, no need to
       import (also can't import because there it's _magnum.*) */
    py::module_::import("magnum.gl");
    py::module_::import("magnum.trade");
    #endif

    py::enum_<MeshTools::CompileFlag> compileFlags{m, "CompileFlags", "Mesh compilation flags"};
    compileFlags
        .value("NONE", MeshTools::CompileFlag{})
        .value("GENERATE_FLAT_NORMALS", MeshTools::CompileFlag::GenerateFlatNormals)
        .value("GENERATE_SMOOTH_NORMALS", MeshTools::CompileFlag::GenerateSmoothNormals);
    corrade::enumOperators(compileFlags);

    py::enum_<MeshTools::InterleaveFlag> interleaveFlags{m, "InterleaveFlags", "Interleaving behavior flags"};
    interleaveFlags
        .value("NONE", MeshTools::InterleaveFlag{})
        .value("PRESERVE_INTERLEAVED_ATTRIBUTES", MeshTools::InterleaveFlag::PreserveInterleavedAttributes)
        .value("PRESERVE_STRIDED_INDICES", MeshTools::InterleaveFlag::PreserveStridedIndices);
    corrade::enumOperators(interleaveFlags);

    m
        .def("compile", [](const Trade::MeshData& mesh, MeshTools::CompileFlag flags) {
            return MeshTools::compile(mesh, flags);
        }, "Compile 3D mesh data", py::arg("mesh"), py::arg("flags") = MeshTools::CompileFlag{})
        .def("compress_indices", [](const Trade::MeshData& mesh, MeshIndexType atLeast) {
            if(!mesh.isIndexed()) {
                PyErr_SetString(PyExc_AssertionError, "the mesh is not indexed");
                throw py::error_already_set{};
            }
            /** @todo check that the indices aren't impl-specific once it's
                possible to test */

            return MeshTools::compressIndices(mesh, atLeast);
        }, "Compress mesh data indices", py::arg("mesh"),
            #if PYBIND11_VERSION_MAJOR*100 + PYBIND11_VERSION_MINOR >= 206
            py::kw_only{}, /* new in pybind11 2.6 */
            #endif
            py::arg("at_least") = MeshIndexType::UnsignedShort)
        /** @todo ew, expose Iterable directly */
        .def("concatenate", [](const std::vector<std::reference_wrapper<Trade::MeshData>>& meshes, MeshTools::InterleaveFlag flags) {
            if(meshes.empty()) {
                PyErr_SetString(PyExc_AssertionError, "expected at least one mesh");
                throw py::error_already_set{};
            }
            const MeshPrimitive primitive = meshes[0].get().primitive();
            for(std::size_t i = 0; i != meshes.size(); ++i) {
                const Trade::MeshData& mesh = meshes[i];
                if(mesh.primitive() == MeshPrimitive::LineStrip ||
                   mesh.primitive() == MeshPrimitive::LineLoop ||
                   mesh.primitive() == MeshPrimitive::TriangleStrip ||
                   mesh.primitive() == MeshPrimitive::TriangleFan)
                {
                    PyErr_Format(PyExc_AssertionError, "%S is not supported, turn it into a plain indexed mesh first", py::cast(mesh.primitive()).ptr());
                    throw py::error_already_set{};
                }
                if(mesh.primitive() != primitive) {
                    PyErr_Format(PyExc_AssertionError, "expected %S but got %S in mesh %zu", py::cast(primitive).ptr(), py::cast(mesh.primitive()).ptr(), i);
                    throw py::error_already_set{};
                }
            }
            /** @todo check that the indices/Vertices aren't impl-specific once
                it's possible to test */
            /** @todo there's a lot more assertions re attribute formats, array
                sizes, etc */

            return MeshTools::concatenate(Containers::Iterable<const Trade::MeshData>{meshes.data(), meshes.size(), sizeof(std::reference_wrapper<Trade::MeshData>), [](const void* data) -> const Trade::MeshData& {
                return static_cast<const std::reference_wrapper<Trade::MeshData>*>(data)->get();
            }}, flags);
        }, "Concatenate meshes together", py::arg("meshes"), py::arg("flags") = MeshTools::InterleaveFlag::PreserveInterleavedAttributes)
        .def("duplicate", [](const Trade::MeshData& mesh) {
            if(!mesh.isIndexed()) {
                PyErr_SetString(PyExc_AssertionError, "the mesh is not indexed");
                throw py::error_already_set{};
            }
            /** @todo check that the indices aren't impl-specific once it's
                possible to test */

            return MeshTools::duplicate(mesh);
        }, "Duplicate indexed mesh data", py::arg("mesh"))
        .def("filter_attributes", [](const Trade::MeshData& mesh, const Containers::BitArrayView attributesToKeep) {
            if(attributesToKeep.size() != mesh.attributeCount()) {
                PyErr_Format(PyExc_AssertionError, "expected %u bits but got %zu", mesh.attributeCount(), attributesToKeep.size());
                throw py::error_already_set{};
            }

            /* If the mesh already has an owner, use that instead to avoid
               long reference chains */
            py::object meshOwner = pyObjectHolderFor<Trade::PyDataHolder>(mesh).owner;
            return Trade::pyDataHolder(MeshTools::filterAttributes(mesh, attributesToKeep), meshOwner.is_none() ? py::cast(mesh) : std::move(meshOwner));
        }, "Filter a mesh to contain only the selected subset of attributes", py::arg("mesh"), py::arg("attributes_to_keep"))
        .def("filter_except_attributes", [](const Trade::MeshData& mesh, const std::vector<Trade::MeshAttribute> attributes) {
            /* If the mesh already has an owner, use that instead to avoid
               long reference chains */
            py::object meshOwner = pyObjectHolderFor<Trade::PyDataHolder>(mesh).owner;
            return Trade::pyDataHolder(MeshTools::filterExceptAttributes(mesh, attributes), meshOwner.is_none() ? py::cast(mesh) : std::move(meshOwner));
        }, "Filter a mesh to contain everything except the selected subset of named attributes", py::arg("mesh"), py::arg("attributes"))
        .def("filter_only_attributes", [](const Trade::MeshData& mesh, const std::vector<Trade::MeshAttribute> attributes) {
            /* If the mesh already has an owner, use that instead to avoid
               long reference chains */
            py::object meshOwner = pyObjectHolderFor<Trade::PyDataHolder>(mesh).owner;
            return Trade::pyDataHolder(MeshTools::filterOnlyAttributes(mesh, attributes), meshOwner.is_none() ? py::cast(mesh) : std::move(meshOwner));
        }, "Filter a mesh to contain only the selected subset of named attributes", py::arg("mesh"), py::arg("attributes"))
        .def("generate_indices", [](const Trade::MeshData& mesh) {
            if(mesh.primitive() != MeshPrimitive::LineStrip &&
               mesh.primitive() != MeshPrimitive::LineLoop &&
               mesh.primitive() != MeshPrimitive::TriangleStrip &&
               mesh.primitive() != MeshPrimitive::TriangleFan)
            {
                PyErr_Format(PyExc_AssertionError, "invalid primitive %S", py::cast(mesh.primitive()).ptr());
                throw py::error_already_set{};
            }
            /** @todo check that the indices aren't impl-specific once it's
                possible to test */

            return MeshTools::generateIndices(mesh);
        }, "Convert a mesh to plain indexed lines or triangles", py::arg("mesh"))
        .def("interleave", [](const Trade::MeshData& mesh, MeshTools::InterleaveFlag flags) {
            /** @todo check that the vertices/indices aren't impl-specific if
                the interleaved preservation is disabled, once it's possible to
                test */
            return MeshTools::interleave(mesh, {}, flags);
        }, "Interleave mesh data", py::arg("mesh"), py::arg("flags") = MeshTools::InterleaveFlag::PreserveInterleavedAttributes)
        .def("copy", static_cast<Trade::MeshData(*)(const Trade::MeshData&)>(MeshTools::copy), "Make a owned copy of the mesh", py::arg("mesh"))
        /** @todo check that the indices/vertices aren't impl-specific once
            it's possible to test */
        .def("remove_duplicates", static_cast<Trade::MeshData(*)(const Trade::MeshData&)>(MeshTools::removeDuplicates), "Remove mesh data duplicates", py::arg("mesh"))
        /** @todo check that the indices/vertices aren't impl-specific once
            it's possible to test */
        .def("remove_duplicates_fuzzy", MeshTools::removeDuplicatesFuzzy, "Remove mesh data duplicates", py::arg("mesh"),
            #if PYBIND11_VERSION_MAJOR*100 + PYBIND11_VERSION_MINOR >= 206
            py::kw_only{}, /* new in pybind11 2.6 */
            #endif
            py::arg("float_epsilon") = Math::TypeTraits<Float>::epsilon(),
            py::arg("double_epsilon") = Math::TypeTraits<Double>::epsilon())
        .def("transform2d", [](const Trade::MeshData& mesh, const Matrix3& transformation, UnsignedInt id, Int morphTargetId, MeshTools::InterleaveFlag flags) {
            const Containers::Optional<UnsignedInt> positionAttributeId = mesh.findAttributeId(Trade::MeshAttribute::Position, id, morphTargetId);
            if(!positionAttributeId) {
                if(morphTargetId == -1)
                    PyErr_Format(PyExc_KeyError, "the mesh has no positions with index %u", id);
                else
                    PyErr_Format(PyExc_KeyError, "the mesh has no positions with index %u in morph target %i", id, morphTargetId);
                throw py::error_already_set{};
            }
            if(vertexFormatComponentCount(mesh.attributeFormat(*positionAttributeId)) != 2) {
                PyErr_Format(PyExc_AssertionError, "expected 2D positions but got %S", py::cast(mesh.attributeFormat(*positionAttributeId)).ptr());
                throw py::error_already_set{};
            }
            /** @todo check that the positions aren't impl-specific once
                it's possible to test */

            return MeshTools::transform2D(mesh, transformation, id, morphTargetId, flags);
        }, "Transform 2D positions in a mesh data", py::arg("mesh"), py::arg("transformation"),
            #if PYBIND11_VERSION_MAJOR*100 + PYBIND11_VERSION_MINOR >= 206
            py::kw_only{}, /* new in pybind11 2.6 */
            #endif
            py::arg("id") = 0, py::arg("morph_target_id") = -1, py::arg("flags") = MeshTools::InterleaveFlag::PreserveInterleavedAttributes)
        .def("transform2d_in_place", [](Trade::MeshData& mesh, const Matrix3& transformation, UnsignedInt id, Int morphTargetId) {
            if(!(mesh.vertexDataFlags() & Trade::DataFlag::Mutable)) {
                PyErr_SetString(PyExc_AssertionError, "vertex data not mutable");
                throw py::error_already_set{};
            }

            const Containers::Optional<UnsignedInt> positionAttributeId = mesh.findAttributeId(Trade::MeshAttribute::Position, id, morphTargetId);
            if(!positionAttributeId) {
                if(morphTargetId == -1)
                    PyErr_Format(PyExc_KeyError, "the mesh has no positions with index %u", id);
                else
                    PyErr_Format(PyExc_KeyError, "the mesh has no positions with index %u in morph target %i", id, morphTargetId);
                throw py::error_already_set{};
            }
            if(mesh.attributeFormat(*positionAttributeId) != VertexFormat::Vector2) {
                PyErr_Format(PyExc_AssertionError, "expected %S positions but got %S", py::cast(VertexFormat::Vector2).ptr(), py::cast(mesh.attributeFormat(*positionAttributeId)).ptr());
                throw py::error_already_set{};
            }

            MeshTools::transform2DInPlace(mesh, transformation, id, morphTargetId);
        }, "Transform 2D positions in a mesh data in-place", py::arg("mesh"), py::arg("transformation"),
            #if PYBIND11_VERSION_MAJOR*100 + PYBIND11_VERSION_MINOR >= 206
            py::kw_only{}, /* new in pybind11 2.6 */
            #endif
            py::arg("id") = 0, py::arg("morph_target_id") = -1)
        .def("transform3d", [](const Trade::MeshData& mesh, const Matrix4& transformation, UnsignedInt id, Int morphTargetId, MeshTools::InterleaveFlag flags) {
            const Containers::Optional<UnsignedInt> positionAttributeId = mesh.findAttributeId(Trade::MeshAttribute::Position, id, morphTargetId);
            if(!positionAttributeId) {
                if(morphTargetId == -1)
                    PyErr_Format(PyExc_KeyError, "the mesh has no positions with index %u", id);
                else
                    PyErr_Format(PyExc_KeyError, "the mesh has no positions with index %u in morph target %i", id, morphTargetId);
                throw py::error_already_set{};
            }
            if(vertexFormatComponentCount(mesh.attributeFormat(*positionAttributeId)) != 3) {
                PyErr_Format(PyExc_AssertionError, "expected 3D positions but got %S", py::cast(mesh.attributeFormat(*positionAttributeId)).ptr());
                throw py::error_already_set{};
            }
            /** @todo check that the positions, normals, ... aren't
                impl-specific once it's possible to test */

            return MeshTools::transform3D(mesh, transformation, id, morphTargetId, flags);
        }, "Transform 3D positions, normals, tangents and bitangents in a mesh data", py::arg("mesh"), py::arg("transformation"),
            #if PYBIND11_VERSION_MAJOR*100 + PYBIND11_VERSION_MINOR >= 206
            py::kw_only{}, /* new in pybind11 2.6 */
            #endif
            py::arg("id") = 0, py::arg("morph_target_id") = -1, py::arg("flags") = MeshTools::InterleaveFlag::PreserveInterleavedAttributes)
        .def("transform3d_in_place", [](Trade::MeshData& mesh, const Matrix4& transformation, UnsignedInt id, Int morphTargetId) {
            if(!(mesh.vertexDataFlags() & Trade::DataFlag::Mutable)) {
                PyErr_SetString(PyExc_AssertionError, "vertex data not mutable");
                throw py::error_already_set{};
            }

            const Containers::Optional<UnsignedInt> positionAttributeId = mesh.findAttributeId(Trade::MeshAttribute::Position, id, morphTargetId);
            if(!positionAttributeId) {
                if(morphTargetId == -1)
                    PyErr_Format(PyExc_KeyError, "the mesh has no positions with index %u", id);
                else
                    PyErr_Format(PyExc_KeyError, "the mesh has no positions with index %u in morph target %i", id, morphTargetId);
                throw py::error_already_set{};
            }
            if(mesh.attributeFormat(*positionAttributeId) != VertexFormat::Vector3) {
                PyErr_Format(PyExc_AssertionError, "expected %S positions but got %S", py::cast(VertexFormat::Vector3).ptr(), py::cast(mesh.attributeFormat(*positionAttributeId)).ptr());
                throw py::error_already_set{};
            }

            const Containers::Optional<UnsignedInt> tangentAttributeId = mesh.findAttributeId(Trade::MeshAttribute::Tangent, id, morphTargetId);
            const Containers::Optional<UnsignedInt> bitangentAttributeId = mesh.findAttributeId(Trade::MeshAttribute::Bitangent, id, morphTargetId);
            const Containers::Optional<UnsignedInt> normalAttributeId = mesh.findAttributeId(Trade::MeshAttribute::Normal, id, morphTargetId);
            if(tangentAttributeId &&
               (mesh.attributeFormat(*tangentAttributeId) != VertexFormat::Vector3 &&
                mesh.attributeFormat(*tangentAttributeId) != VertexFormat::Vector4))
            {
                PyErr_Format(PyExc_AssertionError, "expected %S or %S tangents but got %S", py::cast(VertexFormat::Vector3).ptr(), py::cast(VertexFormat::Vector4).ptr(), py::cast(mesh.attributeFormat(*tangentAttributeId)).ptr());
                throw py::error_already_set{};
            }
            if(bitangentAttributeId && mesh.attributeFormat(*bitangentAttributeId) != VertexFormat::Vector3) {
                PyErr_Format(PyExc_AssertionError, "expected %S bitangents but got %S", py::cast(VertexFormat::Vector3).ptr(), py::cast(mesh.attributeFormat(*bitangentAttributeId)).ptr());
                throw py::error_already_set{};
            }
            if(normalAttributeId && mesh.attributeFormat(*normalAttributeId) != VertexFormat::Vector3) {
                PyErr_Format(PyExc_AssertionError, "expected %S normals but got %S", py::cast(VertexFormat::Vector3).ptr(), py::cast(mesh.attributeFormat(*normalAttributeId)).ptr());
                throw py::error_already_set{};
            }

            MeshTools::transform3DInPlace(mesh, transformation, id, morphTargetId);
        }, "Transform 3D position, normals, tangents and bitangents in a mesh data in-place", py::arg("mesh"), py::arg("transformation"),
            #if PYBIND11_VERSION_MAJOR*100 + PYBIND11_VERSION_MINOR >= 206
            py::kw_only{}, /* new in pybind11 2.6 */
            #endif
            py::arg("id") = 0, py::arg("morph_target_id") = -1)
        .def("transform_texture_coordinates2d", [](const Trade::MeshData& mesh, const Matrix3& transformation, UnsignedInt id, Int morphTargetId, MeshTools::InterleaveFlag flags) {
            const Containers::Optional<UnsignedInt> textureCoordinateAttributeId = mesh.findAttributeId(Trade::MeshAttribute::TextureCoordinates, id, morphTargetId);
            if(!textureCoordinateAttributeId) {
                if(morphTargetId == -1)
                    PyErr_Format(PyExc_KeyError, "the mesh has no texture coordinates with index %u", id);
                else
                    PyErr_Format(PyExc_KeyError, "the mesh has no texture coordinates with index %u in morph target %i", id, morphTargetId);
                throw py::error_already_set{};
            }
            /** @todo check that the texture coordinates aren't impl-specific
                once it's possible to test */

            return MeshTools::transformTextureCoordinates2D(mesh, transformation, id, morphTargetId, flags);
        }, "Transform 2D texture coordinates in a mesh data", py::arg("mesh"), py::arg("transformation"),
            #if PYBIND11_VERSION_MAJOR*100 + PYBIND11_VERSION_MINOR >= 206
            py::kw_only{}, /* new in pybind11 2.6 */
            #endif
            py::arg("id") = 0, py::arg("morph_target_id") = -1, py::arg("flags") = MeshTools::InterleaveFlag::PreserveInterleavedAttributes)
        .def("transform_texture_coordinates2d_in_place", [](Trade::MeshData& mesh, const Matrix3& transformation, UnsignedInt id, Int morphTargetId) {
            if(!(mesh.vertexDataFlags() & Trade::DataFlag::Mutable)) {
                PyErr_SetString(PyExc_AssertionError, "vertex data not mutable");
                throw py::error_already_set{};
            }

            const Containers::Optional<UnsignedInt> textureCoordinateAttributeId = mesh.findAttributeId(Trade::MeshAttribute::TextureCoordinates, id, morphTargetId);
            if(!textureCoordinateAttributeId) {
                if(morphTargetId == -1)
                    PyErr_Format(PyExc_KeyError, "the mesh has no texture coordinates with index %u", id);
                else
                    PyErr_Format(PyExc_KeyError, "the mesh has no texture coordinates with index %u in morph target %i", id, morphTargetId);
                throw py::error_already_set{};
            }
            if(mesh.attributeFormat(*textureCoordinateAttributeId) != VertexFormat::Vector2) {
                PyErr_Format(PyExc_AssertionError, "expected %S texture coordinates but got %S", py::cast(VertexFormat::Vector2).ptr(), py::cast(mesh.attributeFormat(*textureCoordinateAttributeId)).ptr());
                throw py::error_already_set{};
            }

            MeshTools::transformTextureCoordinates2DInPlace(mesh, transformation, id, morphTargetId);
        }, "Transform 2D texture coordinates in a mesh data in-place", py::arg("mesh"), py::arg("transformation"),
            #if PYBIND11_VERSION_MAJOR*100 + PYBIND11_VERSION_MINOR >= 206
            py::kw_only{}, /* new in pybind11 2.6 */
            #endif
            py::arg("id") = 0, py::arg("morph_target_id") = -1);
}

}

#ifndef MAGNUM_BUILD_STATIC
/* TODO: remove declaration when https://github.com/pybind/pybind11/pull/1863
   is released */
extern "C" PYBIND11_EXPORT PyObject* PyInit_meshtools();
PYBIND11_MODULE(meshtools, m) {
    magnum::meshtools(m);
}
#endif
