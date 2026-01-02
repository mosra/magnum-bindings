#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020, 2021, 2022, 2023, 2024, 2025, 2026
#             Vladimír Vondruš <mosra@centrum.cz>
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#

import array
import os
import sys
import unittest

from corrade import containers
from magnum import *
from magnum import meshtools, primitives, trade

class CompressIndices(unittest.TestCase):
    def test(self):
        mesh = primitives.cube_solid()
        self.assertTrue(mesh.is_indexed)
        self.assertEqual(mesh.index_type, MeshIndexType.UNSIGNED_SHORT)

        compressed = meshtools.compress_indices(mesh, at_least=MeshIndexType.UNSIGNED_BYTE)
        self.assertEqual(compressed.index_type, MeshIndexType.UNSIGNED_BYTE)

    def test_not_indexed(self):
        mesh = primitives.line2d()
        self.assertFalse(mesh.is_indexed)

        with self.assertRaisesRegex(AssertionError, "the mesh is not indexed"):
            meshtools.compress_indices(mesh)

class Concatenate(unittest.TestCase):
    def test(self):
        cube = primitives.cube_solid()
        # It's a TRIANGLE_STRIP
        plane = meshtools.generate_indices(primitives.plane_solid())
        concatenated = meshtools.concatenate([cube, plane])
        self.assertEqual(concatenated.vertex_count, cube.vertex_count + plane.vertex_count)
        self.assertEqual(concatenated.index_count, cube.index_count + plane.index_count)

    def test_empty(self):
        with self.assertRaisesRegex(AssertionError, "expected at least one mesh"):
            meshtools.concatenate([])

    def test_invalid_primitive(self):
        with self.assertRaisesRegex(AssertionError, "MeshPrimitive.TRIANGLE_STRIP is not supported, turn it into a plain indexed mesh first"):
            meshtools.concatenate([primitives.cube_solid(), primitives.plane_solid()])
        # Should check that also for the first argument
        with self.assertRaisesRegex(AssertionError, "MeshPrimitive.TRIANGLE_STRIP is not supported, turn it into a plain indexed mesh first"):
            meshtools.concatenate([primitives.plane_solid()])

    def test_inconsistent_primitive(self):
        with self.assertRaisesRegex(AssertionError, "expected MeshPrimitive.TRIANGLES but got MeshPrimitive.LINES in mesh 1"):
            meshtools.concatenate([primitives.cube_solid(), primitives.line3d()])
        with self.assertRaisesRegex(AssertionError, "expected MeshPrimitive.LINES but got MeshPrimitive.TRIANGLES in mesh 1"):
            meshtools.concatenate([primitives.line3d(), primitives.cube_solid()])

class Duplicate(unittest.TestCase):
    def test(self):
        mesh = primitives.cube_solid()
        self.assertTrue(mesh.is_indexed)

        duplicated = meshtools.duplicate(mesh)
        self.assertFalse(duplicated.is_indexed)

    def test_not_indexed(self):
        mesh = primitives.line2d()
        self.assertFalse(mesh.is_indexed)

        with self.assertRaisesRegex(AssertionError, "the mesh is not indexed"):
            meshtools.duplicate(mesh)

class GenerateIndices(unittest.TestCase):
    def test(self):
        mesh = primitives.cube_solid_strip()
        self.assertFalse(mesh.is_indexed)
        self.assertEqual(mesh.primitive, MeshPrimitive.TRIANGLE_STRIP)

        indexed = meshtools.generate_indices(mesh)
        self.assertTrue(indexed.is_indexed)
        self.assertEqual(indexed.primitive, MeshPrimitive.TRIANGLES)

    def test_invalid_primitive(self):
        mesh = primitives.cube_solid()
        self.assertEqual(mesh.primitive, MeshPrimitive.TRIANGLES)

        with self.assertRaisesRegex(AssertionError, "invalid primitive MeshPrimitive.TRIANGLES"):
            meshtools.generate_indices(mesh)

class Filter(unittest.TestCase):
    def test(self):
        mesh = primitives.cube_solid()
        mesh_refcount = sys.getrefcount(mesh)
        self.assertEqual(mesh.attribute_count(), 2)
        self.assertTrue(mesh.has_attribute(trade.MeshAttribute.NORMAL))

        attributes_to_keep = containers.BitArray.value_init(mesh.attribute_count())
        attributes_to_keep[mesh.attribute_id(trade.MeshAttribute.NORMAL)] = True

        filtered = meshtools.filter_attributes(mesh, attributes_to_keep)
        filtered_refcount = sys.getrefcount(filtered)
        self.assertEqual(filtered.attribute_count(), 1)
        self.assertTrue(filtered.has_attribute(trade.MeshAttribute.NORMAL))
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)
        self.assertIs(filtered.owner, mesh)

        # Subsequent filtering will still reference the original mesh, not the
        # intermediates
        filtered2 = meshtools.filter_attributes(filtered, containers.BitArray.direct_init(filtered.attribute_count(), True))
        self.assertEqual(filtered2.attribute_count(), 1)
        self.assertTrue(filtered2.has_attribute(trade.MeshAttribute.NORMAL))
        self.assertEqual(sys.getrefcount(filtered), filtered_refcount)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 2)
        self.assertIs(filtered2.owner, mesh)

        del filtered
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del filtered2
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

    def test_invalid_size(self):
        with self.assertRaisesRegex(AssertionError, "expected 2 bits but got 3"):
            meshtools.filter_attributes(primitives.cube_solid(), containers.BitArray.value_init(3))

    def test_only_attributes(self):
        mesh = primitives.cube_solid()
        mesh_refcount = sys.getrefcount(mesh)
        self.assertEqual(mesh.attribute_count(), 2)
        self.assertTrue(mesh.has_attribute(trade.MeshAttribute.NORMAL))

        # Attributes that are not present in the mesh are deliberately ignored
        filtered = meshtools.filter_only_attributes(mesh, [trade.MeshAttribute.TEXTURE_COORDINATES, trade.MeshAttribute.NORMAL])
        filtered_refcount = sys.getrefcount(filtered)
        self.assertEqual(filtered.attribute_count(), 1)
        self.assertTrue(filtered.has_attribute(trade.MeshAttribute.NORMAL))
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)
        self.assertIs(filtered.owner, mesh)

        # Subsequent filtering will still reference the original mesh, not the
        # intermediates
        filtered2 = meshtools.filter_only_attributes(filtered, [trade.MeshAttribute.NORMAL])
        self.assertEqual(filtered.attribute_count(), 1)
        self.assertTrue(filtered.has_attribute(trade.MeshAttribute.NORMAL))
        self.assertEqual(sys.getrefcount(filtered), filtered_refcount)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 2)
        self.assertIs(filtered2.owner, mesh)

        del filtered
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del filtered2
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

    def test_except_attributes(self):
        mesh = primitives.cube_solid()
        mesh_refcount = sys.getrefcount(mesh)
        self.assertEqual(mesh.attribute_count(), 2)
        self.assertTrue(mesh.has_attribute(trade.MeshAttribute.NORMAL))

        # Attributes that are not present in the mesh are deliberately ignored
        filtered = meshtools.filter_except_attributes(mesh, [trade.MeshAttribute.TEXTURE_COORDINATES, trade.MeshAttribute.NORMAL])
        filtered_refcount = sys.getrefcount(filtered)
        self.assertEqual(filtered.attribute_count(), 1)
        self.assertTrue(filtered.has_attribute(trade.MeshAttribute.POSITION))
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)
        self.assertIs(filtered.owner, mesh)

        # Subsequent filtering will still reference the original mesh, not the
        # intermediates
        filtered2 = meshtools.filter_except_attributes(filtered, [trade.MeshAttribute.NORMAL])
        self.assertEqual(filtered.attribute_count(), 1)
        self.assertTrue(filtered.has_attribute(trade.MeshAttribute.POSITION))
        self.assertEqual(sys.getrefcount(filtered), filtered_refcount)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 2)
        self.assertIs(filtered2.owner, mesh)

        del filtered
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del filtered2
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

class Interleave(unittest.TestCase):
    def test(self):
        mesh = meshtools.filter_except_attributes(primitives.circle3d_solid(3, primitives.Circle3DFlags.TEXTURE_COORDINATES), [trade.MeshAttribute.NORMAL])
        # Position + gap after normals + texture coordinates
        self.assertEqual(mesh.attribute_count(), 2)
        self.assertEqual(mesh.attribute_stride(trade.MeshAttribute.POSITION), 12 + 12 + 8)

        interleaved = meshtools.interleave(mesh)
        self.assertEqual(interleaved.attribute_count(), 2)
        # Gap after normals not removed
        self.assertEqual(interleaved.attribute_stride(trade.MeshAttribute.POSITION), 12 + 12 + 8)

        interleaved_packed = meshtools.interleave(mesh, flags=meshtools.InterleaveFlags.NONE)
        self.assertEqual(interleaved_packed.attribute_count(), 2)
        # Gap after normals removed
        self.assertEqual(interleaved_packed.attribute_stride(trade.MeshAttribute.POSITION), 12 + 8)

    def test_extra(self):
        interleaved = meshtools.interleave(primitives.plane_solid(), extra=[
            trade.MeshAttributeData(trade.MeshAttribute.OBJECT_ID, VertexFormat.UNSIGNED_SHORT, array.array('H', [3, 176, 2, 14]))
        ])
        self.assertEqual(interleaved.attribute_count(), 3)
        self.assertTrue(interleaved.has_attribute(trade.MeshAttribute.OBJECT_ID))
        self.assertEqual(interleaved.attribute_format(trade.MeshAttribute.OBJECT_ID), VertexFormat.UNSIGNED_SHORT)
        self.assertEqual(list(interleaved.attribute(trade.MeshAttribute.OBJECT_ID)), [3, 176, 2, 14])

    def test_extra_invalid(self):
        with self.assertRaisesRegex(AssertionError, "extra attribute 1 expected to have 4 items but got 5"):
            meshtools.interleave(primitives.plane_solid(), extra=[
                trade.MeshAttributeData(trade.MeshAttribute.CUSTOM(33), VertexFormat.UNSIGNED_BYTE, array.array('B', [3, 176, 2, 12])),
                trade.MeshAttributeData(trade.MeshAttribute.OBJECT_ID, VertexFormat.UNSIGNED_SHORT, array.array('H', [3, 176, 2, 12, 6]))
            ])

class Copy(unittest.TestCase):
    def test(self):
        mesh = primitives.square_solid()
        self.assertEqual(mesh.vertex_data_flags, trade.DataFlags.GLOBAL)

        copy = meshtools.copy(mesh)
        self.assertEqual(copy.vertex_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)

class RemoveDuplicates(unittest.TestCase):
    def test(self):
        mesh = meshtools.duplicate(primitives.cube_solid())
        self.assertFalse(mesh.is_indexed)
        self.assertEqual(mesh.vertex_count, 36)

        deduplicated = meshtools.remove_duplicates(mesh)
        self.assertTrue(deduplicated.is_indexed)
        self.assertEqual(deduplicated.vertex_count, 24)

    def test_fuzzy(self):
        mesh = meshtools.duplicate(primitives.cube_solid())
        self.assertFalse(mesh.is_indexed)
        self.assertEqual(mesh.vertex_count, 36)

        deduplicated = meshtools.remove_duplicates_fuzzy(mesh)
        self.assertTrue(deduplicated.is_indexed)
        self.assertEqual(deduplicated.vertex_count, 24)

        # Haha
        single_point = meshtools.remove_duplicates_fuzzy(mesh, float_epsilon=1e6)
        self.assertEqual(single_point.vertex_count, 1)

class Transform(unittest.TestCase):
    # TODO test everything with explicit morph target once there's support in
    #   some importer

    def test_2d(self):
        mesh = primitives.line2d()
        self.assertEqual(mesh.attribute(trade.MeshAttribute.POSITION)[0], (0.0, 0.0))

        transformed = meshtools.transform2d(mesh, Matrix3.translation(Vector2.x_axis(100.0)))
        self.assertEqual(transformed.attribute(trade.MeshAttribute.POSITION)[0], (100.0, 0.0))

    def test_2d_in_place(self):
        mesh = primitives.line2d()
        self.assertEqual(mesh.attribute(trade.MeshAttribute.POSITION)[0], (0.0, 0.0))

        meshtools.transform2d_in_place(mesh, Matrix3.translation(Vector2.x_axis(100.0)))
        self.assertEqual(mesh.attribute(trade.MeshAttribute.POSITION)[0], (100.0, 0.0))

    def test_3d(self):
        mesh = primitives.line3d()
        self.assertEqual(mesh.attribute(trade.MeshAttribute.POSITION)[0], (0.0, 0.0, 0.0))

        transformed = meshtools.transform3d(mesh, Matrix4.translation(Vector3.x_axis(100.0)))
        self.assertEqual(transformed.attribute(trade.MeshAttribute.POSITION)[0], (100.0, 0.0, 0.0))

    def test_3d_in_place(self):
        mesh = primitives.line3d()
        self.assertEqual(mesh.attribute(trade.MeshAttribute.POSITION)[0], (0.0, 0.0, 0.0))

        meshtools.transform3d_in_place(mesh, Matrix4.translation(Vector3.x_axis(100.0)))
        self.assertEqual(mesh.attribute(trade.MeshAttribute.POSITION)[0], (100.0, 0.0, 0.0))

    def test_texture_coordinates2d(self):
        mesh = primitives.square_solid(primitives.SquareFlags.TEXTURE_COORDINATES)
        self.assertEqual(mesh.attribute(trade.MeshAttribute.TEXTURE_COORDINATES)[0], (1.0, 0.0))

        transformed = meshtools.transform_texture_coordinates2d(mesh, Matrix3.translation(Vector2.x_axis(100.0)))
        self.assertEqual(transformed.attribute(trade.MeshAttribute.TEXTURE_COORDINATES)[0], (101.0, 0.0))

    def test_texture_coordinates2d_in_place(self):
        mesh = meshtools.copy(primitives.square_solid(primitives.SquareFlags.TEXTURE_COORDINATES))
        self.assertEqual(mesh.attribute(trade.MeshAttribute.TEXTURE_COORDINATES)[0], (1.0, 0.0))

        meshtools.transform_texture_coordinates2d_in_place(mesh, Matrix3.translation(Vector2.x_axis(100.0)))
        self.assertEqual(mesh.attribute(trade.MeshAttribute.TEXTURE_COORDINATES)[0], (101.0, 0.0))

    def test_no_attribute(self):
        mesh = meshtools.copy(primitives.square_solid(primitives.SquareFlags.TEXTURE_COORDINATES))

        # ID not found
        with self.assertRaisesRegex(KeyError, "the mesh has no positions with index 1"):
            meshtools.transform2d(mesh, Matrix3(), id=1)
        with self.assertRaisesRegex(KeyError, "the mesh has no positions with index 1"):
            meshtools.transform2d_in_place(mesh, Matrix3(), id=1)
        with self.assertRaisesRegex(KeyError, "the mesh has no positions with index 1"):
            meshtools.transform3d(mesh, Matrix4(), id=1)
        with self.assertRaisesRegex(KeyError, "the mesh has no positions with index 1"):
            meshtools.transform3d_in_place(mesh, Matrix4(), id=1)
        with self.assertRaisesRegex(KeyError, "the mesh has no texture coordinates with index 1"):
            meshtools.transform_texture_coordinates2d(mesh, Matrix3(), id=1)
        with self.assertRaisesRegex(KeyError, "the mesh has no texture coordinates with index 1"):
            meshtools.transform_texture_coordinates2d_in_place(mesh, Matrix3(), id=1)

        # Morph target not found
        with self.assertRaisesRegex(KeyError, "the mesh has no positions with index 0 in morph target 37"):
            meshtools.transform2d(mesh, Matrix3(), morph_target_id=37)
        with self.assertRaisesRegex(KeyError, "the mesh has no positions with index 0 in morph target 37"):
            meshtools.transform2d_in_place(mesh, Matrix3(), morph_target_id=37)
        with self.assertRaisesRegex(KeyError, "the mesh has no positions with index 0 in morph target 37"):
            meshtools.transform3d(mesh, Matrix4(), morph_target_id=37)
        with self.assertRaisesRegex(KeyError, "the mesh has no positions with index 0 in morph target 37"):
            meshtools.transform3d_in_place(mesh, Matrix4(), morph_target_id=37)
        with self.assertRaisesRegex(KeyError, "the mesh has no texture coordinates with index 0 in morph target 37"):
            meshtools.transform_texture_coordinates2d(mesh, Matrix3(), morph_target_id=37)
        with self.assertRaisesRegex(KeyError, "the mesh has no texture coordinates with index 0 in morph target 37"):
            meshtools.transform_texture_coordinates2d_in_place(mesh, Matrix3(), morph_target_id=37)

    def test_not_2d_not_3d(self):
        mesh2d = primitives.line2d()
        mesh3d = primitives.line3d()

        with self.assertRaisesRegex(AssertionError, "expected 2D positions but got VertexFormat.VECTOR3"):
            meshtools.transform2d(mesh3d, Matrix3())
        with self.assertRaisesRegex(AssertionError, "expected VertexFormat.VECTOR2 positions but got VertexFormat.VECTOR3"):
            meshtools.transform2d_in_place(mesh3d, Matrix3())
        with self.assertRaisesRegex(AssertionError, "expected 3D positions but got VertexFormat.VECTOR2"):
            meshtools.transform3d(mesh2d, Matrix4())
        with self.assertRaisesRegex(AssertionError, "expected VertexFormat.VECTOR3 positions but got VertexFormat.VECTOR2"):
            meshtools.transform3d_in_place(mesh2d, Matrix4())

    def test_not_float(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh-packed.gltf'))

        # Non-in-place should work
        # TODO test 2D positions
        # TODO test bitangents and three-component tangents once there's a tool
        #   to convert from/to four-component (don't want to use Assimp)
        packed_positions = meshtools.transform3d(importer.mesh('packed positions'), Matrix4.rotation_x(Deg(90.0)))
        packed_normals = meshtools.transform3d(importer.mesh('packed normals'), Matrix4.rotation_x(Deg(90.0)))
        packed_tangents = meshtools.transform3d(importer.mesh('packed tangents'), Matrix4.rotation_x(Deg(90.0)))
        packed_texcoords = meshtools.transform_texture_coordinates2d(importer.mesh('packed texcoords'), Matrix3.rotation(Deg(90.0)))
        self.assertEqual(packed_positions.attribute(trade.MeshAttribute.POSITION)[1], (4.0, -6.0, 5.0))
        self.assertEqual(packed_normals.attribute(trade.MeshAttribute.NORMAL)[1], (0.0, 0.0, 1.0))
        self.assertEqual(packed_tangents.attribute(trade.MeshAttribute.TANGENT)[1], (0.0, 0.0, 1.0, 0.0))
        self.assertEqual(packed_texcoords.attribute(trade.MeshAttribute.TEXTURE_COORDINATES)[1], (-0.5, 0.0))

        # TODO test 2D position with something that's actually 2D
        with self.assertRaisesRegex(AssertionError, "expected VertexFormat.VECTOR2 positions but got VertexFormat.VECTOR3US"):
            meshtools.transform2d_in_place(importer.mesh('packed positions'), Matrix3())
        with self.assertRaisesRegex(AssertionError, "expected VertexFormat.VECTOR3 positions but got VertexFormat.VECTOR3US"):
            meshtools.transform3d_in_place(importer.mesh('packed positions'), Matrix4())
        # TODO test also with an explicit ID and morph target ID to verify it's
        #   correctly propagated
        with self.assertRaisesRegex(AssertionError, "expected VertexFormat.VECTOR3 normals but got VertexFormat.VECTOR3S_NORMALIZED"):
            meshtools.transform3d_in_place(importer.mesh('packed normals'), Matrix4())
        with self.assertRaisesRegex(AssertionError, "expected VertexFormat.VECTOR3 or VertexFormat.VECTOR4 tangents but got VertexFormat.VECTOR4S_NORMALIZED"):
            meshtools.transform3d_in_place(importer.mesh('packed tangents'), Matrix4())
        with self.assertRaisesRegex(AssertionError, "expected VertexFormat.VECTOR2 texture coordinates but got VertexFormat.VECTOR2US_NORMALIZED"):
            meshtools.transform_texture_coordinates2d_in_place(importer.mesh('packed texcoords'), Matrix3())

    def test_in_place_not_mutable(self):
        mesh = primitives.square_solid(primitives.SquareFlags.TEXTURE_COORDINATES)

        with self.assertRaisesRegex(AssertionError, "vertex data not mutable"):
            meshtools.transform2d_in_place(mesh, Matrix3())
        with self.assertRaisesRegex(AssertionError, "vertex data not mutable"):
            meshtools.transform3d_in_place(mesh, Matrix4())
        with self.assertRaisesRegex(AssertionError, "vertex data not mutable"):
            meshtools.transform_texture_coordinates2d_in_place(mesh, Matrix3())
