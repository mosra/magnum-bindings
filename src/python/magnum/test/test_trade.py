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
import platform
import sys
import tempfile
import unittest

from corrade import pluginmanager
from magnum import *
from magnum import primitives, trade
import magnum

class ImageData(unittest.TestCase):
    def test(self):
        # The only way to get an image instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "rgb.png"))

        image = importer.image2d(0)
        image_refcount = sys.getrefcount(image)
        self.assertFalse(image.is_compressed)
        self.assertEqual(image.storage.alignment, 1) # libPNG has 4 tho
        self.assertEqual(image.format, PixelFormat.RGB8_UNORM)
        self.assertEqual(image.pixel_size, 3)
        self.assertEqual(image.size, Vector2i(3, 2))
        # TODO: ugh, report as bytes, not chars
        self.assertEqual(ord(image.pixels[1, 2, 2]), 181)
        self.assertEqual(ord(image.data[9 + 6 + 2]), 181) # libPNG has 12 +

        data = image.data
        self.assertEqual(len(data), 3*3*2)
        self.assertIs(data.owner, image)
        self.assertEqual(sys.getrefcount(image), image_refcount + 1)

        del data
        self.assertEqual(sys.getrefcount(image), image_refcount)

    def test_compressed(self):
        # The only way to get an image instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('DdsImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "rgba_dxt1.dds"))
        image = importer.image2d(0)
        self.assertEqual(len(image.data), 8)
        self.assertTrue(image.is_compressed)
        # TODO: compressed properties

        # No compressed-image-related APIs exposed ATM, so just verifying the
        # uncompressed ones fail properly
        with self.assertRaisesRegex(AttributeError, "image is compressed"):
            image.storage
        with self.assertRaisesRegex(AttributeError, "image is compressed"):
            image.format
        with self.assertRaisesRegex(AttributeError, "image is compressed"):
            image.pixel_size
        with self.assertRaisesRegex(AttributeError, "image is compressed"):
            image.pixels

    def test_convert_view(self):
        # The only way to get an image instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "rgb.png"))
        image = importer.image2d(0)

        view = ImageView2D(image)
        mutable_view = MutableImageView2D(image)

    def test_convert_view_compressed(self):
        # The only way to get an image instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('DdsImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "rgba_dxt1.dds"))
        image = importer.image2d(0)

        # No compressed-image-related APIs exposed ATM, so just verifying the
        # uncompressed ones fail properly
        with self.assertRaisesRegex(RuntimeError, "image is compressed"):
            view = ImageView2D(image)
        with self.assertRaisesRegex(RuntimeError, "image is compressed"):
            mutable_view = MutableImageView2D(image)

class MeshData(unittest.TestCase):
    def test_custom_attribute(self):
        # Creating a custom attribute
        a = trade.MeshAttribute.CUSTOM(17)
        self.assertTrue(a.is_custom)
        if hasattr(a, 'value'): # only since pybind11 2.6.2
            self.assertEqual(a.value, 32768 + 17)
        self.assertEqual(a.custom_value, 17)
        self.assertEqual(a.name, "CUSTOM(17)")
        self.assertEqual(str(a), "MeshAttribute.CUSTOM(17)")
        self.assertEqual(repr(a), "<MeshAttribute.CUSTOM(17): 32785>")

        # Lowest possible custom value, test that it's correctly recognized as
        # custom by all APIs
        zero = trade.MeshAttribute.CUSTOM(0)
        self.assertTrue(zero.is_custom)
        if hasattr(zero, 'value'): # only since pybind11 2.6.2
            self.assertEqual(zero.value, 32768)
        self.assertEqual(zero.custom_value, 0)
        self.assertEqual(zero.name, "CUSTOM(0)")
        self.assertEqual(str(zero), "MeshAttribute.CUSTOM(0)")
        self.assertEqual(repr(zero), "<MeshAttribute.CUSTOM(0): 32768>")

        # Largest possible custom value
        largest = trade.MeshAttribute.CUSTOM(32767)
        self.assertTrue(largest.is_custom)
        if hasattr(largest, 'value'): # only since pybind11 2.6.2
            self.assertEqual(largest.value, 65535)
        self.assertEqual(largest.custom_value, 32767)

        # Creating a custom attribute with a value that won't fit
        with self.assertRaisesRegex(ValueError, "custom value too large"):
            trade.MeshAttribute.CUSTOM(32768)

        # Accessing properties on builtin values should still work as expected
        b = trade.MeshAttribute.BITANGENT
        self.assertFalse(b.is_custom)
        if hasattr(b, 'value'): # only since pybind11 2.6.2
            self.assertEqual(b.value, 3)
        with self.assertRaisesRegex(AttributeError, "not a custom value"):
            b.custom_value
        self.assertEqual(b.name, "BITANGENT")
        self.assertEqual(str(b), "MeshAttribute.BITANGENT")
        self.assertEqual(repr(b), "<MeshAttribute.BITANGENT: 3>")

    def test(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)
        self.assertEqual(mesh.primitive, MeshPrimitive.TRIANGLES)
        self.assertEqual(mesh.index_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)
        self.assertEqual(mesh.vertex_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)

        # Index properties
        self.assertTrue(mesh.is_indexed)
        self.assertEqual(mesh.index_count, 3)
        self.assertEqual(mesh.index_type, MeshIndexType.UNSIGNED_SHORT)
        self.assertEqual(mesh.index_offset, 0)
        self.assertEqual(mesh.index_stride, 2)

        self.assertEqual(mesh.vertex_count, 3)

        # TODO once configuration is exposed, disable the JOINTS/WEIGHTS
        # backwards compatibility to avoid this mess
        if magnum.BUILD_DEPRECATED:
            self.assertEqual(mesh.attribute_count(), 11)

            # Attribute properties by ID
            self.assertEqual(mesh.attribute_name(3), trade.MeshAttribute.POSITION)
            # Custom attribute
            self.assertEqual(mesh.attribute_name(8), trade.MeshAttribute.CUSTOM(10))
            self.assertEqual(mesh.attribute_id(3), 0)
            # Attribute 5 is the second TEXTURE_COORDINATES attribute
            self.assertEqual(mesh.attribute_id(5), 1)
            self.assertEqual(mesh.attribute_format(0), VertexFormat.VECTOR3UB_NORMALIZED)
            self.assertEqual(mesh.attribute_format(10), VertexFormat.UNSIGNED_INT)
            self.assertEqual(mesh.attribute_offset(0), 20)
            self.assertEqual(mesh.attribute_offset(3), 0)
            self.assertEqual(mesh.attribute_stride(2), 28)
            self.assertEqual(mesh.attribute_array_size(0), 0)
            # Attribute 1 is JOINT_IDS
            self.assertEqual(mesh.attribute_array_size(1), 4)

            # Attribute properties by name
            self.assertTrue(mesh.has_attribute(trade.MeshAttribute.COLOR))
            self.assertTrue(mesh.has_attribute(trade.MeshAttribute.POSITION))
            self.assertFalse(mesh.has_attribute(trade.MeshAttribute.TANGENT))
            self.assertEqual(mesh.attribute_count(trade.MeshAttribute.POSITION), 1)
            self.assertEqual(mesh.attribute_count(trade.MeshAttribute.TEXTURE_COORDINATES), 2)
            self.assertEqual(mesh.attribute_count(trade.MeshAttribute.TANGENT), 0)
            self.assertEqual(mesh.attribute_id(trade.MeshAttribute.POSITION), 3)
            self.assertEqual(mesh.attribute_id(trade.MeshAttribute.TEXTURE_COORDINATES, 1), 5)
            self.assertEqual(mesh.attribute_format(trade.MeshAttribute.COLOR), VertexFormat.VECTOR3UB_NORMALIZED)
            self.assertEqual(mesh.attribute_format(trade.MeshAttribute.OBJECT_ID), VertexFormat.UNSIGNED_INT)
            self.assertEqual(mesh.attribute_offset(trade.MeshAttribute.COLOR), 20)
            self.assertEqual(mesh.attribute_offset(trade.MeshAttribute.POSITION), 0)
            self.assertEqual(mesh.attribute_stride(trade.MeshAttribute.WEIGHTS), 28)
            self.assertEqual(mesh.attribute_array_size(trade.MeshAttribute.POSITION), 0)
            self.assertEqual(mesh.attribute_array_size(trade.MeshAttribute.WEIGHTS), 4)
        else:
            self.assertEqual(mesh.attribute_count(), 9)

            # Attribute properties by ID
            self.assertEqual(mesh.attribute_name(2), trade.MeshAttribute.POSITION)
            # Custom attribute
            self.assertEqual(mesh.attribute_name(6), trade.MeshAttribute.CUSTOM(8))
            self.assertEqual(mesh.attribute_id(2), 0)
            # Attribute 4 is the second TEXTURE_COORDINATES attribute
            self.assertEqual(mesh.attribute_id(4), 1)
            self.assertEqual(mesh.attribute_format(0), VertexFormat.VECTOR3UB_NORMALIZED)
            self.assertEqual(mesh.attribute_format(8), VertexFormat.UNSIGNED_INT)
            self.assertEqual(mesh.attribute_offset(0), 20)
            self.assertEqual(mesh.attribute_offset(2), 0)
            self.assertEqual(mesh.attribute_stride(3), 28)
            self.assertEqual(mesh.attribute_array_size(0), 0)
            # Attribute 1 is JOINT_IDS
            self.assertEqual(mesh.attribute_array_size(1), 4)

            # Attribute properties by name
            self.assertTrue(mesh.has_attribute(trade.MeshAttribute.COLOR))
            self.assertTrue(mesh.has_attribute(trade.MeshAttribute.POSITION))
            self.assertFalse(mesh.has_attribute(trade.MeshAttribute.TANGENT))
            self.assertEqual(mesh.attribute_count(trade.MeshAttribute.POSITION), 1)
            self.assertEqual(mesh.attribute_count(trade.MeshAttribute.TEXTURE_COORDINATES), 2)
            self.assertEqual(mesh.attribute_count(trade.MeshAttribute.TANGENT), 0)
            self.assertEqual(mesh.attribute_id(trade.MeshAttribute.POSITION), 2)
            self.assertEqual(mesh.attribute_id(trade.MeshAttribute.TEXTURE_COORDINATES, 1), 4)
            self.assertEqual(mesh.attribute_format(trade.MeshAttribute.COLOR), VertexFormat.VECTOR3UB_NORMALIZED)
            self.assertEqual(mesh.attribute_format(trade.MeshAttribute.OBJECT_ID), VertexFormat.UNSIGNED_INT)
            self.assertEqual(mesh.attribute_offset(trade.MeshAttribute.COLOR), 20)
            self.assertEqual(mesh.attribute_offset(trade.MeshAttribute.POSITION), 0)
            self.assertEqual(mesh.attribute_stride(trade.MeshAttribute.WEIGHTS), 28)
            self.assertEqual(mesh.attribute_array_size(trade.MeshAttribute.POSITION), 0)
            self.assertEqual(mesh.attribute_array_size(trade.MeshAttribute.WEIGHTS), 4)

    def test_index_data_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)
        mesh_refcount = sys.getrefcount(mesh)

        index_data = mesh.index_data
        self.assertEqual(len(index_data), 6)
        self.assertIs(index_data.owner, mesh)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del index_data
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

        mutable_index_data = mesh.mutable_index_data
        self.assertEqual(len(mutable_index_data), 6)
        self.assertIs(mutable_index_data.owner, mesh)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del mutable_index_data
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

    def test_vertex_data_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)
        mesh_refcount = sys.getrefcount(mesh)

        vertex_data = mesh.vertex_data
        self.assertEqual(len(vertex_data), 84)
        self.assertIs(vertex_data.owner, mesh)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del vertex_data
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

        mutable_vertex_data = mesh.mutable_vertex_data
        self.assertEqual(len(mutable_vertex_data), 84)
        self.assertIs(mutable_vertex_data.owner, mesh)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del mutable_vertex_data
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

    def test_indices_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)
        mesh_refcount = sys.getrefcount(mesh)

        indices = mesh.indices
        self.assertEqual(indices.size, (3, ))
        self.assertEqual(indices.stride, (2, ))
        self.assertEqual(indices.format, 'H')
        self.assertEqual(list(indices), [0, 2, 1])
        self.assertIs(indices.owner, mesh)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del indices
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

        mutable_indices = mesh.mutable_indices
        self.assertEqual(mutable_indices.size, (3, ))
        self.assertEqual(mutable_indices.stride, (2, ))
        self.assertEqual(mutable_indices.format, 'H')
        self.assertEqual(list(mutable_indices), [0, 2, 1])
        self.assertIs(mutable_indices.owner, mesh)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del mutable_indices
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

    def test_attribute_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)
        mesh_refcount = sys.getrefcount(mesh)
        position_id = mesh.attribute_id(trade.MeshAttribute.POSITION)

        positions = mesh.attribute(position_id)
        self.assertEqual(positions.size, (3, ))
        self.assertEqual(positions.stride, (28, ))
        self.assertEqual(positions.format, '3f')
        self.assertEqual(list(positions), [
            Vector3(-1, -1, 0.25),
            Vector3(0, 1, 0.5),
            Vector3(1, -1, 0.25)
        ])
        self.assertIs(positions.owner, mesh)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del positions
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

        object_ids = mesh.attribute(trade.MeshAttribute.OBJECT_ID)
        self.assertEqual(object_ids.size, (3, ))
        self.assertEqual(object_ids.stride, (28, ))
        self.assertEqual(object_ids.format, 'I')
        self.assertEqual(list(object_ids), [216, 16777235, 2872872013])
        self.assertIs(object_ids.owner, mesh)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del object_ids
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

        mutable_positions = mesh.mutable_attribute(position_id)
        self.assertEqual(mutable_positions.size, (3, ))
        self.assertEqual(mutable_positions.stride, (28, ))
        self.assertEqual(mutable_positions.format, '3f')
        self.assertEqual(list(mutable_positions), [
            Vector3(-1, -1, 0.25),
            Vector3(0, 1, 0.5),
            Vector3(1, -1, 0.25)
        ])
        self.assertIs(mutable_positions.owner, mesh)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del mutable_positions
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

        mutable_object_ids = mesh.mutable_attribute(trade.MeshAttribute.OBJECT_ID)
        self.assertEqual(mutable_object_ids.size, (3, ))
        self.assertEqual(mutable_object_ids.stride, (28, ))
        self.assertEqual(mutable_object_ids.format, 'I')
        self.assertEqual(list(mutable_object_ids), [216, 16777235, 2872872013])
        self.assertIs(mutable_object_ids.owner, mesh)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del mutable_object_ids
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

    def test_mutable_index_data_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)
        self.assertEqual(mesh.index_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)

        index_data = mesh.index_data
        mutable_index_data = mesh.mutable_index_data
        # Second index is 2, it's a 16-bit LE number
        # TODO: ugh, report as bytes, not chars
        self.assertEqual(ord(index_data[2]), 2)
        self.assertEqual(ord(mutable_index_data[2]), 2)

        mutable_index_data[2] = chr(76)
        self.assertEqual(ord(index_data[2]), 76)

    def test_mutable_vertex_data_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)
        self.assertEqual(mesh.vertex_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)

        vertex_data = mesh.vertex_data
        mutable_vertex_data = mesh.mutable_vertex_data
        # The color attribute is at offset 20, G channel is the next byte
        # TODO: ugh, report as bytes, not chars
        self.assertEqual(ord(vertex_data[21]), 51)
        self.assertEqual(ord(mutable_vertex_data[21]), 51)

        mutable_vertex_data[21] = chr(76)
        self.assertEqual(vertex_data[21], chr(76))

    def test_mutable_indices_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)
        self.assertEqual(mesh.index_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)

        indices = mesh.indices
        mutable_indices = mesh.mutable_indices
        self.assertEqual(indices[1], 2)
        self.assertEqual(mutable_indices[1], 2)

        mutable_indices[1] = 76
        self.assertEqual(indices[1], 76)

    def test_mutable_attributes_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)
        self.assertEqual(mesh.index_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)
        position_id = mesh.attribute_id(trade.MeshAttribute.POSITION)

        positions = mesh.attribute(position_id)
        mutable_positions = mesh.mutable_attribute(position_id)
        self.assertEqual(positions[1], Vector3(0, 1, 0.5))
        self.assertEqual(mutable_positions[1], Vector3(0, 1, 0.5))

        mutable_positions[1] *= 2
        self.assertEqual(positions[1], Vector3(0, 2, 1))

        object_ids = mesh.attribute(trade.MeshAttribute.OBJECT_ID)
        mutable_object_ids = mesh.mutable_attribute(trade.MeshAttribute.OBJECT_ID)
        self.assertEqual(object_ids[1], 16777235)
        self.assertEqual(mutable_object_ids[1], 16777235)

        mutable_object_ids[1] //= 1000
        self.assertEqual(object_ids[1], 16777)

    def test_packed_attribute_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)
        self.assertEqual(mesh.index_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)
        packed_attribute = importer.mesh_attribute_for_name("_CUSTOM_PACKED_ATTRIBUTE")
        self.assertEqual(mesh.attribute_format(packed_attribute), VertexFormat.VECTOR3UB)

        packed = mesh.attribute(packed_attribute)
        mutable_packed = mesh.mutable_attribute(packed_attribute)
        self.assertEqual(packed[1], Vector3i(51, 102, 255))
        self.assertEqual(mutable_packed[1], Vector3i(51, 102, 255))

        mutable_packed[1] -= Vector3i(12, 56, 200)
        self.assertEqual(packed[1], Vector3(39, 46, 55))

    def test_normalized_attribute_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)
        self.assertEqual(mesh.index_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)
        self.assertEqual(mesh.attribute_format(trade.MeshAttribute.COLOR), VertexFormat.VECTOR3UB_NORMALIZED)

        normalized = mesh.attribute(trade.MeshAttribute.COLOR)
        mutable_normalized = mesh.mutable_attribute(trade.MeshAttribute.COLOR)
        self.assertEqual(normalized[1], Vector3(0.2, 0.4, 1))
        self.assertEqual(mutable_normalized[1], Vector3(0.2, 0.4, 1))

        mutable_normalized[1] *= 0.5
        # Rounding errors are expected
        self.assertEqual(normalized[1], Vector3(0.101961, 0.2, 0.501961))

    def test_data_access_not_mutable(self):
        mesh = primitives.cube_solid()
        # TODO split this once there's a mesh where only one or the other would
        #   be true (maybe with zero-copy loading of PLYs / STLs?)
        self.assertEqual(mesh.index_data_flags, trade.DataFlags.NONE)
        self.assertEqual(mesh.vertex_data_flags, trade.DataFlags.NONE)

        with self.assertRaisesRegex(AttributeError, "mesh index data is not mutable"):
            mesh.mutable_index_data
        with self.assertRaisesRegex(AttributeError, "mesh index data is not mutable"):
            mesh.mutable_indices
        with self.assertRaisesRegex(AttributeError, "mesh vertex data is not mutable"):
            mesh.mutable_vertex_data
        with self.assertRaisesRegex(AttributeError, "mesh vertex data is not mutable"):
            mesh.mutable_attribute(0)
        with self.assertRaisesRegex(AttributeError, "mesh vertex data is not mutable"):
            mesh.mutable_attribute(trade.MeshAttribute.POSITION)

    def test_nonindexed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(1)
        self.assertFalse(mesh.is_indexed)

        # Accessing the index data should be possible, they're just empty
        self.assertEqual(len(mesh.index_data), 0)

        # Accessing any other index-related info should cause an exception
        with self.assertRaisesRegex(AttributeError, "mesh is not indexed"):
            mesh.index_count
        with self.assertRaisesRegex(AttributeError, "mesh is not indexed"):
            mesh.index_type
        with self.assertRaisesRegex(AttributeError, "mesh is not indexed"):
            mesh.index_offset
        with self.assertRaisesRegex(AttributeError, "mesh is not indexed"):
            mesh.index_stride
        with self.assertRaisesRegex(AttributeError, "mesh is not indexed"):
            mesh.indices
        with self.assertRaisesRegex(AttributeError, "mesh is not indexed"):
            mesh.mutable_indices

    def test_attribute_oob(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)

        # Access by OOB ID
        with self.assertRaises(IndexError):
            mesh.attribute_name(mesh.attribute_count())
        with self.assertRaises(IndexError):
            mesh.attribute_id(mesh.attribute_count())
        with self.assertRaises(IndexError):
            mesh.attribute_format(mesh.attribute_count())
        with self.assertRaises(IndexError):
            mesh.attribute_offset(mesh.attribute_count())
        with self.assertRaises(IndexError):
            mesh.attribute_stride(mesh.attribute_count())
        with self.assertRaises(IndexError):
            mesh.attribute_array_size(mesh.attribute_count())
        with self.assertRaises(IndexError):
            mesh.attribute(mesh.attribute_count())
        with self.assertRaises(IndexError):
            mesh.mutable_attribute(mesh.attribute_count())

        # Access by nonexistent name
        with self.assertRaises(KeyError):
            mesh.attribute_id(trade.MeshAttribute.TANGENT)
        with self.assertRaises(KeyError):
            mesh.attribute_format(trade.MeshAttribute.TANGENT)
        with self.assertRaises(KeyError):
            mesh.attribute_offset(trade.MeshAttribute.TANGENT)
        with self.assertRaises(KeyError):
            mesh.attribute_stride(trade.MeshAttribute.TANGENT)
        with self.assertRaises(KeyError):
            mesh.attribute_array_size(trade.MeshAttribute.TANGENT)
        with self.assertRaises(KeyError):
            mesh.attribute(trade.MeshAttribute.TANGENT)
        with self.assertRaises(KeyError):
            mesh.mutable_attribute(trade.MeshAttribute.TANGENT)

        # Access by existing name + OOB ID
        with self.assertRaises(KeyError):
            mesh.attribute_id(trade.MeshAttribute.TEXTURE_COORDINATES, 2)
        with self.assertRaises(KeyError):
            mesh.attribute_format(trade.MeshAttribute.TEXTURE_COORDINATES, 2)
        with self.assertRaises(KeyError):
            mesh.attribute_offset(trade.MeshAttribute.TEXTURE_COORDINATES, 2)
        with self.assertRaises(KeyError):
            mesh.attribute_stride(trade.MeshAttribute.TEXTURE_COORDINATES, 2)
        with self.assertRaises(KeyError):
            mesh.attribute_array_size(trade.MeshAttribute.TEXTURE_COORDINATES, 2)
        with self.assertRaises(KeyError):
            mesh.attribute(trade.MeshAttribute.TEXTURE_COORDINATES, 2)
        with self.assertRaises(KeyError):
            mesh.mutable_attribute(trade.MeshAttribute.TEXTURE_COORDINATES, 2)

    def test_attribute_access_array(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)
        joint_ids_id = mesh.attribute_id(trade.MeshAttribute.JOINT_IDS)

        with self.assertRaisesRegex(NotImplementedError, "array attributes not implemented yet, sorry"):
            mesh.attribute(joint_ids_id)
        with self.assertRaisesRegex(NotImplementedError, "array attributes not implemented yet, sorry"):
            mesh.mutable_attribute(joint_ids_id)
        with self.assertRaisesRegex(NotImplementedError, "array attributes not implemented yet, sorry"):
            mesh.attribute(trade.MeshAttribute.JOINT_IDS)
        with self.assertRaisesRegex(NotImplementedError, "array attributes not implemented yet, sorry"):
            mesh.mutable_attribute(trade.MeshAttribute.JOINT_IDS)

    def test_attribute_access_unsupported_format(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        custom_attribute = importer.mesh_attribute_for_name("_CUSTOM_MATRIX_ATTRIBUTE");
        self.assertIsNotNone(custom_attribute)

        mesh = importer.mesh(0)
        custom_attribute_id = mesh.attribute_id(custom_attribute)

        with self.assertRaisesRegex(NotImplementedError, "access to this vertex format is not implemented yet, sorry"):
            mesh.attribute(custom_attribute_id)
        with self.assertRaisesRegex(NotImplementedError, "access to this vertex format is not implemented yet, sorry"):
            mesh.mutable_attribute(custom_attribute_id)
        with self.assertRaisesRegex(NotImplementedError, "access to this vertex format is not implemented yet, sorry"):
            mesh.attribute(custom_attribute)
        with self.assertRaisesRegex(NotImplementedError, "access to this vertex format is not implemented yet, sorry"):
            mesh.mutable_attribute(custom_attribute)

class SceneData(unittest.TestCase):
    def test_custom_field(self):
        # Creating a custom attribute
        a = trade.SceneField.CUSTOM(17)
        self.assertTrue(a.is_custom)
        if hasattr(a, 'value'): # only since pybind11 2.6.2
            self.assertEqual(a.value, 0x80000000 + 17)
        self.assertEqual(a.custom_value, 17)
        self.assertEqual(a.name, "CUSTOM(17)")
        self.assertEqual(str(a), "SceneField.CUSTOM(17)")
        self.assertEqual(repr(a), "<SceneField.CUSTOM(17): 2147483665>")

        # Lowest possible custom value, test that it's correctly recognized as
        # custom by all APIs
        zero = trade.SceneField.CUSTOM(0)
        self.assertTrue(zero.is_custom)
        if hasattr(zero, 'value'): # only since pybind11 2.6.2
            self.assertEqual(zero.value, 0x80000000)
        self.assertEqual(zero.custom_value, 0)
        self.assertEqual(zero.name, "CUSTOM(0)")
        self.assertEqual(str(zero), "SceneField.CUSTOM(0)")
        self.assertEqual(repr(zero), "<SceneField.CUSTOM(0): 2147483648>")

        # Largest possible custom value
        largest = trade.SceneField.CUSTOM(0x7fffffff)
        self.assertTrue(largest.is_custom)
        if hasattr(largest, 'value'): # only since pybind11 2.6.2
            self.assertEqual(largest.value, 0xffffffff)
        self.assertEqual(largest.custom_value, 0x7fffffff)

        # Creating a custom attribute with a value that won't fit
        with self.assertRaisesRegex(ValueError, "custom value too large"):
            trade.SceneField.CUSTOM(0x80000000)

        # Accessing properties on builtin values should still work as expected
        b = trade.SceneField.SKIN
        self.assertFalse(b.is_custom)
        if hasattr(b, 'value'): # only since pybind11 2.6.2
            self.assertEqual(b.value, 10)
        with self.assertRaisesRegex(AttributeError, "not a custom value"):
            b.custom_value
        self.assertEqual(b.name, "SKIN")
        self.assertEqual(str(b), "SceneField.SKIN")
        self.assertEqual(repr(b), "<SceneField.SKIN: 10>")

    def test(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        scene = importer.scene(0)
        self.assertEqual(scene.mapping_type, trade.SceneMappingType.UNSIGNED_INT)
        self.assertEqual(scene.mapping_bound, 4)
        self.assertEqual(scene.field_count, 7)
        # TODO add some array extras once supported to have this different from
        #   the mapping bound
        self.assertEqual(scene.field_size_bound, 4)
        self.assertFalse(scene.is_2d)
        self.assertTrue(scene.is_3d)

        # Field properties by ID
        self.assertEqual(scene.field_name(2), trade.SceneField.TRANSFORMATION)
        self.assertEqual(scene.field_name(6), trade.SceneField.CUSTOM(1))
        # TODO some field flags in glTF please?
        self.assertEqual(scene.field_flags(2), trade.SceneFieldFlags.NONE)
        self.assertEqual(scene.field_type(2), trade.SceneFieldType.MATRIX4X4)
        self.assertEqual(scene.field_size(3), 3)
        # TODO add some array extras once supported to have this non-zero for
        #   some fields
        self.assertEqual(scene.field_array_size(2), 0)
        self.assertTrue(scene.has_field_object(2, 3))
        self.assertFalse(scene.has_field_object(4, 1))
        self.assertEqual(scene.field_object_offset(2, 3), 2)
        self.assertEqual(scene.field_object_offset(2, 3, 1), 2)

        # Field properties by name
        self.assertEqual(scene.field_id(trade.SceneField.CUSTOM(0)), 5)
        self.assertTrue(scene.has_field(trade.SceneField.IMPORTER_STATE))
        self.assertFalse(scene.has_field(trade.SceneField.SKIN))
        self.assertTrue(scene.has_field_object(trade.SceneField.TRANSFORMATION, 3))
        self.assertFalse(scene.has_field_object(trade.SceneField.CAMERA, 1))
        self.assertEqual(scene.field_object_offset(trade.SceneField.TRANSFORMATION, 3), 2)
        self.assertEqual(scene.field_object_offset(trade.SceneField.TRANSFORMATION, 3, 1), 2)
        # TODO some field flags in glTF please?
        self.assertEqual(scene.field_flags(trade.SceneField.PARENT), trade.SceneFieldFlags.NONE)
        self.assertEqual(scene.field_type(trade.SceneField.CUSTOM(1)), trade.SceneFieldType.STRING_OFFSET32)
        self.assertEqual(scene.field_size(trade.SceneField.CUSTOM(0)), 1)
        # TODO add some array extras once supported to have this non-zero for
        #   some fields
        self.assertEqual(scene.field_array_size(trade.SceneField.TRANSLATION), 0)

    def test_mapping_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        scene = importer.scene(0)
        scene_refcount = sys.getrefcount(scene)
        translation_id = scene.field_id(trade.SceneField.TRANSLATION)

        translations = scene.mapping(translation_id)
        self.assertEqual(translations.size, (3, ))
        self.assertEqual(translations.stride, (4, ))
        self.assertEqual(translations.format, 'I')
        self.assertEqual(list(translations), [1, 3, 0])
        self.assertIs(translations.owner, scene)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)

        del translations
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

        cameras = scene.mapping(trade.SceneField.CAMERA)
        self.assertEqual(cameras.size, (2, ))
        self.assertEqual(cameras.stride, (4, ))
        self.assertEqual(cameras.format, 'I')
        self.assertEqual(list(cameras), [2, 3])
        self.assertIs(cameras.owner, scene)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)

        del cameras
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

        mutable_translations = scene.mutable_mapping(translation_id)
        self.assertEqual(mutable_translations.size, (3, ))
        self.assertEqual(mutable_translations.stride, (4, ))
        self.assertEqual(mutable_translations.format, 'I')
        self.assertEqual(list(mutable_translations), [1, 3, 0])
        self.assertIs(mutable_translations.owner, scene)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)

        del mutable_translations
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

        mutable_cameras = scene.mutable_mapping(trade.SceneField.CAMERA)
        self.assertEqual(mutable_cameras.size, (2, ))
        self.assertEqual(mutable_cameras.stride, (4, ))
        self.assertEqual(mutable_cameras.format, 'I')
        self.assertEqual(list(mutable_cameras), [2, 3])
        self.assertIs(mutable_cameras.owner, scene)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)

        del mutable_cameras
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

    def test_field_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        scene = importer.scene(0)
        scene_refcount = sys.getrefcount(scene)
        translation_id = scene.field_id(trade.SceneField.TRANSLATION)

        translations = scene.field(translation_id)
        self.assertEqual(translations.size, (3, ))
        self.assertEqual(translations.stride, (12, ))
        self.assertEqual(translations.format, '3f')
        self.assertEqual(list(translations), [
            Vector3(1, 2, 3),
            Vector3(4, 5, 6),
            Vector3(7, 8, 9)
        ])
        self.assertIs(translations.owner, scene)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)

        del translations
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

        cameras = scene.field(trade.SceneField.CAMERA)
        self.assertEqual(cameras.size, (2, ))
        self.assertEqual(cameras.stride, (4, ))
        self.assertEqual(cameras.format, 'I')
        self.assertEqual(list(cameras), [1, 0])
        self.assertIs(cameras.owner, scene)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)

        del cameras
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

        mutable_translations = scene.mutable_field(translation_id)
        self.assertEqual(mutable_translations.size, (3, ))
        self.assertEqual(mutable_translations.stride, (12, ))
        self.assertEqual(mutable_translations.format, '3f')
        self.assertEqual(list(mutable_translations), [
            Vector3(1, 2, 3),
            Vector3(4, 5, 6),
            Vector3(7, 8, 9)
        ])
        self.assertIs(mutable_translations.owner, scene)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)

        del mutable_translations
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

        mutable_cameras = scene.mutable_field(trade.SceneField.CAMERA)
        self.assertEqual(mutable_cameras.size, (2, ))
        self.assertEqual(mutable_cameras.stride, (4, ))
        self.assertEqual(mutable_cameras.format, 'I')
        self.assertEqual(list(mutable_cameras), [1, 0])
        self.assertIs(mutable_cameras.owner, scene)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)

        del mutable_cameras
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

    def test_mutable_mapping_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        scene = importer.scene(0)
        self.assertEqual(scene.data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)
        translation_id = scene.field_id(trade.SceneField.TRANSLATION)

        translations = scene.mapping(translation_id)
        mutable_translations = scene.mutable_mapping(translation_id)
        self.assertEqual(translations[1], 3)
        self.assertEqual(mutable_translations[1], 3)

        mutable_translations[1] = 776
        self.assertEqual(translations[1], 776)

        cameras = scene.mapping(trade.SceneField.CAMERA)
        mutable_cameras = scene.mutable_mapping(trade.SceneField.CAMERA)
        self.assertEqual(cameras[1], 3)
        self.assertEqual(mutable_cameras[1], 3)

        mutable_cameras[1] = 13378
        self.assertEqual(cameras[1], 13378)

    def test_mutable_field_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        scene = importer.scene(0)
        self.assertEqual(scene.data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)
        translation_id = scene.field_id(trade.SceneField.TRANSLATION)

        translations = scene.field(translation_id)
        mutable_translations = scene.mutable_field(translation_id)
        self.assertEqual(translations[1], Vector3(4, 5, 6))
        self.assertEqual(mutable_translations[1], Vector3(4, 5, 6))

        mutable_translations[1] *= 0.5
        self.assertEqual(translations[1], Vector3(2, 2.5, 3))

        cameras = scene.field(trade.SceneField.CAMERA)
        mutable_cameras = scene.mutable_field(trade.SceneField.CAMERA)
        self.assertEqual(cameras[1], 0)
        self.assertEqual(mutable_cameras[1], 0)

        mutable_cameras[1] = 13378
        self.assertEqual(cameras[1], 13378)

    def test_pointer_field_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        scene = importer.scene(0)
        self.assertEqual(scene.data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)

        pointer = scene.field(trade.SceneField.IMPORTER_STATE)
        mutable_pointer = scene.mutable_field(trade.SceneField.IMPORTER_STATE)
        self.assertEqual(pointer.format, 'P')
        self.assertEqual(mutable_pointer.format, 'P')
        self.assertNotEqual(pointer[1], 0x0)
        self.assertEqual(mutable_pointer[1], pointer[1])

        mutable_pointer[1] = 0xdeadbeef
        self.assertEqual(pointer[1], 0xdeadbeef)

    def test_data_access_not_mutable(self):
        pass
        # TODO implement once there's a way to get immutable SceneData, either
        #   by "deserializing" a binary blob or via some SceneTools API

    def test_field_oob(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        scene = importer.scene(0)

        # Access by OOB field ID
        with self.assertRaises(IndexError):
            scene.field_name(scene.field_count)
        with self.assertRaises(IndexError):
            scene.field_flags(scene.field_count)
        with self.assertRaises(IndexError):
            scene.field_type(scene.field_count)
        with self.assertRaises(IndexError):
            scene.field_size(scene.field_count)
        with self.assertRaises(IndexError):
            scene.field_array_size(scene.field_count)
        with self.assertRaisesRegex(IndexError, "field out of range"):
            scene.has_field_object(scene.field_count, 0)
        with self.assertRaisesRegex(IndexError, "field out of range"):
            scene.field_object_offset(scene.field_count, 0)
        with self.assertRaises(IndexError):
            scene.mapping(scene.field_count)
        with self.assertRaises(IndexError):
            scene.mutable_mapping(scene.field_count)
        with self.assertRaises(IndexError):
            scene.field(scene.field_count)
        with self.assertRaises(IndexError):
            scene.mutable_field(scene.field_count)

        # Access by nonexistent field name
        with self.assertRaises(KeyError):
            scene.field_id(trade.SceneField.SCALING)
        with self.assertRaises(KeyError):
            scene.field_flags(trade.SceneField.SCALING)
        with self.assertRaises(KeyError):
            scene.field_type(trade.SceneField.SCALING)
        with self.assertRaises(KeyError):
            scene.field_size(trade.SceneField.SCALING)
        with self.assertRaises(KeyError):
            scene.field_array_size(trade.SceneField.SCALING)
        with self.assertRaises(KeyError):
            scene.has_field_object(trade.SceneField.SCALING, 0)
        with self.assertRaises(KeyError):
            scene.field_object_offset(trade.SceneField.SCALING, 0)
        with self.assertRaises(KeyError):
            scene.mapping(trade.SceneField.SCALING)
        with self.assertRaises(KeyError):
            scene.mutable_mapping(trade.SceneField.SCALING)
        with self.assertRaises(KeyError):
            scene.field(trade.SceneField.SCALING)
        with self.assertRaises(KeyError):
            scene.mutable_field(trade.SceneField.SCALING)

        # OOB object ID
        with self.assertRaisesRegex(IndexError, "object out of range"):
            scene.has_field_object(0, 4) # PARENT
        with self.assertRaisesRegex(IndexError, "object out of range"):
            scene.has_field_object(trade.SceneField.PARENT, 4)
        with self.assertRaisesRegex(IndexError, "object out of range"):
            scene.field_object_offset(0, 4) # PARENT
        with self.assertRaisesRegex(IndexError, "object out of range"):
            scene.field_object_offset(trade.SceneField.PARENT, 4)

        # Lookup error
        with self.assertRaises(LookupError):
            scene.field_object_offset(4, 1) # CAMERA
        with self.assertRaises(LookupError):
            scene.field_object_offset(trade.SceneField.CAMERA, 1)

        # Lookup error due to field offset being at the end
        with self.assertRaises(LookupError):
            scene.field_object_offset(0, 1, scene.field_size(0)) # PARENT
        with self.assertRaises(LookupError):
            scene.field_object_offset(trade.SceneField.PARENT, 1, scene.field_size(trade.SceneField.PARENT))

        # OOB field offset (offset == size is allowed, tested above)
        with self.assertRaisesRegex(IndexError, "offset out of range"):
            scene.field_object_offset(0, 1, scene.field_size(0) + 1) # PARENT
        with self.assertRaisesRegex(IndexError, "offset out of range"):
            scene.field_object_offset(trade.SceneField.PARENT, 1, scene.field_size(trade.SceneField.PARENT) + 1)

    def test_field_access_array(self):
        pass
        # TODO implement once there's some importer that gives back arrays
        #   (gltf? not sure)

    def test_field_access_unsupported_type(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))
        string_field = importer.scene_field_for_name('aString')
        self.assertIsNotNone(string_field)

        scene = importer.scene(0)
        string_field_id = scene.field_id(string_field)

        with self.assertRaisesRegex(NotImplementedError, "access to this scene field type is not implemented yet, sorry"):
            scene.field(string_field_id)
        with self.assertRaisesRegex(NotImplementedError, "access to this scene field type is not implemented yet, sorry"):
            scene.mutable_field(string_field_id)
        with self.assertRaisesRegex(NotImplementedError, "access to this scene field type is not implemented yet, sorry"):
            scene.field(string_field)
        with self.assertRaisesRegex(NotImplementedError, "access to this scene field type is not implemented yet, sorry"):
            scene.mutable_field(string_field)

class Importer(unittest.TestCase):
    def test_manager(self):
        manager = trade.ImporterManager()
        self.assertIn('cz.mosra.magnum.Trade.AbstractImporter', manager.plugin_interface)
        self.assertIn('importers', manager.plugin_directory)
        self.assertIn('StbImageImporter', manager.plugin_list)
        self.assertIn('PngImporter', manager.alias_list)
        self.assertEqual(manager.load_state('StbImageImporter'), pluginmanager.LoadState.NOT_LOADED)

        self.assertTrue(manager.load('StbImageImporter') & pluginmanager.LoadState.LOADED)
        self.assertEqual(manager.unload('StbImageImporter'), pluginmanager.LoadState.NOT_LOADED)

        with self.assertRaisesRegex(RuntimeError, "can't load plugin"):
            manager.load('NonexistentImporter')
        with self.assertRaisesRegex(RuntimeError, "can't unload plugin"):
            manager.unload('NonexistentImporter')

    def test(self):
        manager = trade.ImporterManager()

        self.assertIn('cz.mosra.magnum.Trade.AbstractImporter', trade.AbstractImporter.plugin_interface)
        self.assertIn(manager.plugin_directory, trade.AbstractImporter.plugin_search_paths)
        if platform.system() == 'Windows':
            self.assertEqual(trade.AbstractImporter.plugin_suffix, '.dll')
        else:
            self.assertEqual(trade.AbstractImporter.plugin_suffix, '.so')
        self.assertEqual(trade.AbstractImporter.plugin_metadata_suffix, '.conf')

        importer = manager.load_and_instantiate('StbImageImporter')
        self.assertEqual(importer.plugin, 'StbImageImporter')
        self.assertEqual(importer.features, trade.ImporterFeatures.OPEN_DATA)
        self.assertEqual(importer.flags, trade.ImporterFlags.NONE)

        importer.flags = trade.ImporterFlags.VERBOSE
        self.assertEqual(importer.flags, trade.ImporterFlags.VERBOSE)

    def test_set_plugin_directory(self):
        manager = trade.ImporterManager()

        plugin_directory = manager.plugin_directory
        self.assertIn('PngImporter', manager.alias_list)

        manager.plugin_directory = "/nonexistent"
        self.assertNotIn('PngImporter', manager.alias_list)

        manager.plugin_directory = plugin_directory
        self.assertIn('PngImporter', manager.alias_list)

    def test_set_preferred_plugins(self):
        manager = trade.ImporterManager()
        # TODO test this better once we can verify it gets actually loaded
        manager.set_preferred_plugins('TgaImporter', ['StbImageImporter', 'DevIlImageImporter'])
        self.assertIn('StbImageImporter', manager.alias_list)

    def test_set_preferred_plugins_alias_not_found(self):
        manager = trade.ImporterManager()
        with self.assertRaises(KeyError):
            manager.set_preferred_plugins('ApngImporter', [])

    def test_metadata(self):
        manager = trade.ImporterManager()
        manager.set_preferred_plugins('PngImporter', ['StbImageImporter'])
        manager_refcount = sys.getrefcount(manager)

        metadata = manager.metadata('PngImporter')
        self.assertEqual(sys.getrefcount(manager), manager_refcount + 1)
        self.assertEqual(metadata.name, 'StbImageImporter')
        self.assertEqual(metadata.provides, ['BmpImporter', 'GifImporter', 'HdrImporter', 'JpegImporter', 'PgmImporter', 'PicImporter', 'PngImporter', 'PpmImporter', 'PsdImporter', 'TgaImporter'])

        del metadata
        self.assertEqual(sys.getrefcount(manager), manager_refcount)

        metadata = manager.metadata('GltfImporter')
        self.assertEqual(sys.getrefcount(manager), manager_refcount + 1)
        self.assertEqual(metadata.depends, ['AnyImageImporter'])

        importer = manager.load_and_instantiate('GltfImporter')
        importer_refcount = sys.getrefcount(importer)
        self.assertEqual(sys.getrefcount(manager), manager_refcount + 2)

        metadata = manager.metadata('AnyImageImporter')
        # Replacing the previous metadata instance so it stays the same
        self.assertEqual(sys.getrefcount(manager), manager_refcount + 2)
        self.assertEqual(metadata.used_by, ['GltfImporter'])

        # Retrieving metadata from the plugin instance should be the same
        # instance
        metadata_from_plugin = importer.metadata
        self.assertEqual(sys.getrefcount(importer), importer_refcount + 1)
        self.assertEqual(metadata_from_plugin.depends, ['AnyImageImporter'])

        del metadata_from_plugin
        self.assertEqual(sys.getrefcount(importer), importer_refcount)

        del importer
        del metadata
        self.assertEqual(sys.getrefcount(manager), manager_refcount)

    def test_configuration(self):
        manager = trade.ImporterManager()

        metadata = manager.metadata('StbImageImporter')
        metadata_refcount = sys.getrefcount(metadata)

        # Setting the value from initial configuration should make the plugin
        # inherit that
        initial_configuration = metadata.configuration
        self.assertEqual(sys.getrefcount(metadata), metadata_refcount + 1)
        self.assertEqual(initial_configuration['forceChannelCount'], '0')
        initial_configuration['forceChannelCount'] = '7'

        del initial_configuration
        self.assertEqual(sys.getrefcount(metadata), metadata_refcount)

        importer = manager.load_and_instantiate('StbImageImporter')
        importer_refcount = sys.getrefcount(importer)

        configuration = importer.configuration
        self.assertEqual(sys.getrefcount(importer), importer_refcount + 1)
        self.assertEqual(configuration['forceChannelCount'], '7')

        configuration['forceChannelCount'] = '2'

        del configuration
        self.assertEqual(sys.getrefcount(importer), importer_refcount)

        # Verify the config change is actually used and not being done on some
        # copy that gets thrown away
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))
        image = importer.image2d(0)
        self.assertEqual(image.format, PixelFormat.RG8_UNORM) # not RGB8

    def test_no_file_opened(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        self.assertFalse(importer.is_opened)

        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.default_scene
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.scene_count
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.object_count
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.scene_for_name('')
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.object_for_name('')
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.scene_name(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.object_name(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.scene(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.scene('')

        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.mesh_count
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.mesh_level_count(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.mesh_for_name('')
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.mesh_name(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.mesh(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.mesh('')

        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image1d_count
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image2d_count
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image3d_count
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image1d_level_count(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image2d_level_count(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image3d_level_count(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image1d_for_name('')
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image2d_for_name('')
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image3d_for_name('')
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image1d_name(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image2d_name(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image3d_name(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image1d(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image1d('')
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image2d(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image2d('')
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image3d(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image3d('')

    def test_index_oob(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))

        with self.assertRaises(IndexError):
            importer.scene_name(0)
        with self.assertRaises(IndexError):
            importer.object_name(0)
        with self.assertRaises(IndexError):
            importer.scene(0)

        with self.assertRaises(IndexError):
            importer.mesh_level_count(0)
        with self.assertRaises(IndexError):
            importer.mesh_name(0)
        with self.assertRaisesRegex(IndexError, "ID out of bounds"):
            importer.mesh(0)

        with self.assertRaises(IndexError):
            importer.image1d_level_count(0)
        with self.assertRaises(IndexError):
            importer.image2d_level_count(1)
        with self.assertRaises(IndexError):
            importer.image3d_level_count(0)

        with self.assertRaises(IndexError):
            importer.image1d_name(0)
        with self.assertRaises(IndexError):
            importer.image2d_name(1)
        with self.assertRaises(IndexError):
            importer.image3d_name(0)

        with self.assertRaisesRegex(IndexError, "ID out of bounds"):
            importer.image1d(0)
        with self.assertRaisesRegex(IndexError, "level out of bounds"):
            importer.image2d(0, 1)
        with self.assertRaisesRegex(IndexError, "ID out of bounds"):
            importer.image2d(1)
        with self.assertRaisesRegex(IndexError, "ID out of bounds"):
            importer.image3d(0)

    def test_open_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')

        with self.assertRaisesRegex(RuntimeError, "opening nonexistent.png failed"):
            importer.open_file('nonexistent.png')
        with self.assertRaisesRegex(RuntimeError, "opening data failed"):
            importer.open_data(b'')

    def test_scene(self):
        # importer refcounting tested in image2d
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')

        # Asking for custom scene field names should work even if not opened,
        # returns None
        self.assertIsNone(importer.scene_field_name(trade.SceneField.CUSTOM(1)))
        self.assertIsNone(importer.scene_field_for_name('aString'))

        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))
        self.assertEqual(importer.default_scene, 1)
        self.assertEqual(importer.scene_count, 3)
        self.assertEqual(importer.scene_name(1), "A default scene that's empty")
        self.assertEqual(importer.scene_for_name("A default scene that's empty"), 1)
        self.assertEqual(importer.object_count, 5)
        self.assertEqual(importer.object_name(2), "Camera node")
        self.assertEqual(importer.object_for_name("Camera node"), 2)

        # It should work after opening
        self.assertEqual(importer.scene_field_name(trade.SceneField.CUSTOM(1)), 'aString')
        self.assertEqual(importer.scene_field_for_name('aString'), trade.SceneField.CUSTOM(1))

        scene = importer.scene(0)
        self.assertEqual(scene.field_count, 7)
        self.assertTrue(scene.has_field(importer.scene_field_for_name('aString')))

    def test_scene_by_name(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        scene = importer.scene("A scene")
        self.assertEqual(scene.field_count, 7)

    def test_scene_by_name_not_found(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        with self.assertRaises(KeyError):
            importer.scene('Nonexistent')

    def test_scene_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        with self.assertRaisesRegex(RuntimeError, "import failed"):
            importer.scene(2)
        with self.assertRaisesRegex(RuntimeError, "import failed"):
            importer.scene("A broken scene")

    def test_mesh(self):
        # importer refcounting tested in image2d
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')

        # Asking for custom mesh attribute names should work even if not
        # opened, returns None
        # TODO once configuration is exposed, disable the JOINTS/WEIGHTS
        # backwards compatibility to avoid this mess
        if magnum.BUILD_DEPRECATED:
            self.assertIsNone(importer.mesh_attribute_name(trade.MeshAttribute.CUSTOM(9)))
        else:
            self.assertIsNone(importer.mesh_attribute_name(trade.MeshAttribute.CUSTOM(7)))
        self.assertIsNone(importer.mesh_attribute_for_name("_CUSTOM_ATTRIBUTE"))

        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        self.assertEqual(importer.mesh_count, 3)
        self.assertEqual(importer.mesh_level_count(0), 1)
        self.assertEqual(importer.mesh_name(0), 'Indexed mesh')
        self.assertEqual(importer.mesh_for_name('Indexed mesh'), 0)

        # It should work after opening
        # TODO once configuration is exposed, disable the JOINTS/WEIGHTS
        # backwards compatibility to avoid this mess
        if magnum.BUILD_DEPRECATED:
            self.assertEqual(importer.mesh_attribute_name(trade.MeshAttribute.CUSTOM(10)), "_CUSTOM_MATRIX_ATTRIBUTE")
            self.assertEqual(importer.mesh_attribute_for_name("_CUSTOM_MATRIX_ATTRIBUTE"), trade.MeshAttribute.CUSTOM(10))
        else:
            self.assertEqual(importer.mesh_attribute_name(trade.MeshAttribute.CUSTOM(8)), "_CUSTOM_MATRIX_ATTRIBUTE")
            self.assertEqual(importer.mesh_attribute_for_name("_CUSTOM_MATRIX_ATTRIBUTE"), trade.MeshAttribute.CUSTOM(8))

        mesh = importer.mesh(0)
        self.assertEqual(mesh.primitive, MeshPrimitive.TRIANGLES)
        self.assertTrue(mesh.has_attribute(importer.mesh_attribute_for_name("_CUSTOM_MATRIX_ATTRIBUTE")))

    def test_mesh_level_oob(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        with self.assertRaises(IndexError):
            importer.mesh(0, 1)

    def test_mesh_by_name(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh('Non-indexed mesh')
        self.assertEqual(mesh.primitive, MeshPrimitive.TRIANGLES)

    def test_mesh_by_name_not_found(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        with self.assertRaises(KeyError):
            importer.mesh('Nonexistent')

    def test_mesh_by_name_level_oob(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        with self.assertRaisesRegex(IndexError, "level out of bounds"):
            importer.mesh('Non-indexed mesh', 1)

    def test_mesh_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        with self.assertRaisesRegex(RuntimeError, "import failed"):
            importer.mesh(2)
        with self.assertRaisesRegex(RuntimeError, "import failed"):
            importer.mesh('A broken mesh')

    def test_image2d(self):
        manager = trade.ImporterManager()
        manager_refcount = sys.getrefcount(manager)

        # Importer references the manager to ensure it doesn't get GC'd before
        # the plugin instances
        importer = manager.load_and_instantiate('StbImageImporter')
        self.assertIs(importer.manager, manager)
        self.assertEqual(sys.getrefcount(manager), manager_refcount + 1)

        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))
        self.assertEqual(importer.image2d_count, 1)
        self.assertEqual(importer.image2d_level_count(0), 1)
        self.assertEqual(importer.image2d_name(0), '')
        self.assertEqual(importer.image2d_for_name(''), -1)

        image = importer.image2d(0)
        self.assertEqual(image.size, Vector2i(3, 2))

        # Deleting the importer should decrease manager refcount again
        del importer
        self.assertEqual(sys.getrefcount(manager), manager_refcount)

    def test_image_level_oob(self):
        # importer refcounting tested in image2d
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))

        with self.assertRaisesRegex(IndexError, "level out of bounds"):
            importer.image2d(0, 1)

    def test_image2d_by_name(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'image.gltf'))

        image = importer.image2d('A named image')
        self.assertEqual(image.size, Vector2i(3, 2))

    def test_image2d_by_name_not_found(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'image.gltf'))

        with self.assertRaises(KeyError):
            importer.image2d('Nonexistent')

    def test_image2d_data(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')

        with open(os.path.join(os.path.dirname(__file__), "rgb.png"), 'rb') as f:
            importer.open_data(f.read())

        image = importer.image2d(0)
        self.assertEqual(image.size, Vector2i(3, 2))

    def test_image2d_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'image.gltf'))

        with self.assertRaisesRegex(RuntimeError, "import failed"):
            importer.image2d(0)
        with self.assertRaisesRegex(RuntimeError, "import failed"):
            importer.image2d('A broken image')

class ImageConverter(unittest.TestCase):
    def test(self):
        converter = trade.ImageConverterManager().load_and_instantiate('StbImageConverter')
        self.assertEqual(converter.features, trade.ImageConverterFeatures.CONVERT2D_TO_FILE|trade.ImageConverterFeatures.CONVERT2D_TO_DATA)
        self.assertEqual(converter.flags, trade.ImageConverterFlags.NONE)

        converter.flags = trade.ImageConverterFlags.VERBOSE
        self.assertEqual(converter.flags, trade.ImageConverterFlags.VERBOSE)

    def test_image2d(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))
        image = importer.image2d(0)

        converter = trade.ImageConverterManager().load_and_instantiate('StbImageConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.convert_to_file(image, os.path.join(tmp, "image.png"))
            self.assertTrue(os.path.exists(os.path.join(tmp, "image.png")))

    def test_image2d_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))
        image = importer.image2d(0)

        converter = trade.ImageConverterManager().load_and_instantiate('StbImageConverter')

        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(RuntimeError, "conversion failed"):
                converter.convert_to_file(image, os.path.join(tmp, "image.hdr"))

class SceneConverter(unittest.TestCase):
    def test(self):
        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')
        self.assertEqual(converter.features, trade.SceneConverterFeatures.CONVERT_MESH_TO_FILE|trade.SceneConverterFeatures.CONVERT_MESH_TO_DATA)
        self.assertEqual(converter.flags, trade.SceneConverterFlags.NONE)

        converter.flags = trade.SceneConverterFlags.VERBOSE
        self.assertEqual(converter.flags, trade.SceneConverterFlags.VERBOSE)

    def test_mesh(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        mesh = importer.mesh(1)

        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.convert_to_file(mesh, os.path.join(tmp, "mesh.ply"))
            self.assertTrue(os.path.exists(os.path.join(tmp, "mesh.ply")))

    def test_mesh_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        mesh = importer.mesh(1)

        converter = trade.SceneConverterManager().load_and_instantiate('AnySceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(RuntimeError, "conversion failed"):
                converter.convert_to_file(mesh, os.path.join(tmp, "mesh.obj"))
