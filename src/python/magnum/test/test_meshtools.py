#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020, 2021, 2022 Vladimír Vondruš <mosra@centrum.cz>
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

import os
import unittest

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

        with self.assertRaisesRegex(AssertionError, "invalid mesh primitive"):
            meshtools.generate_indices(mesh)

class FilterAttributes(unittest.TestCase):
    def test_only(self):
        mesh = primitives.cube_solid()
        self.assertEqual(mesh.attribute_count(), 2)
        self.assertTrue(mesh.has_attribute(trade.MeshAttribute.NORMAL))

        # Currently it doesn't blow up if unknown attributes are listed
        filtered = meshtools.filter_only_attributes(mesh, [trade.MeshAttribute.TEXTURE_COORDINATES, trade.MeshAttribute.NORMAL])
        self.assertEqual(filtered.attribute_count(), 1)
        self.assertTrue(filtered.has_attribute(trade.MeshAttribute.NORMAL))

    def test_except(self):
        mesh = primitives.cube_solid()
        self.assertEqual(mesh.attribute_count(), 2)
        self.assertTrue(mesh.has_attribute(trade.MeshAttribute.NORMAL))

        # Currently it doesn't blow up if unknown attributes are listed
        filtered = meshtools.filter_except_attributes(mesh, [trade.MeshAttribute.TEXTURE_COORDINATES, trade.MeshAttribute.NORMAL])
        self.assertEqual(filtered.attribute_count(), 1)
        self.assertFalse(filtered.has_attribute(trade.MeshAttribute.NORMAL))

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

        interleaved_packed = meshtools.interleave(mesh, meshtools.InterleaveFlags.NONE)
        self.assertEqual(interleaved_packed.attribute_count(), 2)
        # Gap after normals removed
        self.assertEqual(interleaved_packed.attribute_stride(trade.MeshAttribute.POSITION), 12 + 8)

class Owned(unittest.TestCase):
    def test(self):
        mesh = primitives.square_solid()
        self.assertEqual(mesh.vertex_data_flags, trade.DataFlags.NONE)

        owned = meshtools.owned(mesh)
        self.assertEqual(owned.vertex_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)

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
        mesh = meshtools.owned(primitives.square_solid(primitives.SquareFlags.TEXTURE_COORDINATES))
        self.assertEqual(mesh.attribute(trade.MeshAttribute.TEXTURE_COORDINATES)[0], (1.0, 0.0))

        meshtools.transform_texture_coordinates2d_in_place(mesh, Matrix3.translation(Vector2.x_axis(100.0)))
        self.assertEqual(mesh.attribute(trade.MeshAttribute.TEXTURE_COORDINATES)[0], (101.0, 0.0))

    def test_no_attribute(self):
        mesh = meshtools.owned(primitives.square_solid(primitives.SquareFlags.TEXTURE_COORDINATES))

        with self.assertRaisesRegex(KeyError, "position attribute not found"):
            meshtools.transform2d(mesh, Matrix3(), 1)
        with self.assertRaisesRegex(KeyError, "position attribute not found"):
            meshtools.transform2d_in_place(mesh, Matrix3(), 1)
        with self.assertRaisesRegex(KeyError, "position attribute not found"):
            meshtools.transform3d(mesh, Matrix4(), 1)
        with self.assertRaisesRegex(KeyError, "position attribute not found"):
            meshtools.transform3d_in_place(mesh, Matrix4(), 1)
        with self.assertRaisesRegex(KeyError, "texture coordinates attribute not found"):
            meshtools.transform_texture_coordinates2d(mesh, Matrix3(), 1)
        with self.assertRaisesRegex(KeyError, "texture coordinates attribute not found"):
            meshtools.transform_texture_coordinates2d_in_place(mesh, Matrix3(), 1)

    def test_not_2d_not_3d(self):
        mesh2d = primitives.line2d()
        mesh3d = primitives.line3d()

        with self.assertRaisesRegex(AssertionError, "positions are not 2D"):
            meshtools.transform2d(mesh3d, Matrix3())
        with self.assertRaisesRegex(AssertionError, "positions are not VECTOR2"):
            meshtools.transform2d_in_place(mesh3d, Matrix3())
        with self.assertRaisesRegex(AssertionError, "positions are not 3D"):
            meshtools.transform3d(mesh2d, Matrix4())
        with self.assertRaisesRegex(AssertionError, "positions are not VECTOR3"):
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
        with self.assertRaisesRegex(AssertionError, "positions are not VECTOR2"):
            meshtools.transform2d_in_place(importer.mesh('packed positions'), Matrix3())
        with self.assertRaisesRegex(AssertionError, "positions are not VECTOR3"):
            meshtools.transform3d_in_place(importer.mesh('packed positions'), Matrix4())
        with self.assertRaisesRegex(AssertionError, "normals are not VECTOR3"):
            meshtools.transform3d_in_place(importer.mesh('packed normals'), Matrix4())
        with self.assertRaisesRegex(AssertionError, "tangents are not VECTOR3 or VECTOR4"):
            meshtools.transform3d_in_place(importer.mesh('packed tangents'), Matrix4())
        with self.assertRaisesRegex(AssertionError, "texture coordinates are not VECTOR2"):
            meshtools.transform_texture_coordinates2d_in_place(importer.mesh('packed texcoords'), Matrix3())

    def test_in_place_not_mutable(self):
        mesh = primitives.square_solid(primitives.SquareFlags.TEXTURE_COORDINATES)

        with self.assertRaisesRegex(AssertionError, "vertex data not mutable"):
            meshtools.transform2d_in_place(mesh, Matrix3())
        with self.assertRaisesRegex(AssertionError, "vertex data not mutable"):
            meshtools.transform3d_in_place(mesh, Matrix4())
        with self.assertRaisesRegex(AssertionError, "vertex data not mutable"):
            meshtools.transform_texture_coordinates2d_in_place(mesh, Matrix3())
