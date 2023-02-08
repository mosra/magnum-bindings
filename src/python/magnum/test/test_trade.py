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
        self.assertEqual(mesh.index_data_flags, trade.DataFlag.OWNED|trade.DataFlag.MUTABLE)
        self.assertEqual(mesh.vertex_data_flags, trade.DataFlag.OWNED|trade.DataFlag.MUTABLE)

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
            self.assertEqual(mesh.attribute_count(), 10)

            # Attribute properties by ID
            self.assertEqual(mesh.attribute_name(3), trade.MeshAttribute.POSITION)
            # Custom attribute
            self.assertEqual(mesh.attribute_name(8), trade.MeshAttribute.CUSTOM(9))
            self.assertEqual(mesh.attribute_id(3), 0)
            # Attribute 5 is the second TEXTURE_COORDINATES attribute
            self.assertEqual(mesh.attribute_id(5), 1)
            self.assertEqual(mesh.attribute_format(0), VertexFormat.VECTOR3UB_NORMALIZED)
            self.assertEqual(mesh.attribute_format(9), VertexFormat.UNSIGNED_INT)
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
            self.assertEqual(mesh.attribute_count(), 8)

            # Attribute properties by ID
            self.assertEqual(mesh.attribute_name(2), trade.MeshAttribute.POSITION)
            # Custom attribute
            self.assertEqual(mesh.attribute_name(6), trade.MeshAttribute.CUSTOM(7))
            self.assertEqual(mesh.attribute_id(2), 0)
            # Attribute 4 is the second TEXTURE_COORDINATES attribute
            self.assertEqual(mesh.attribute_id(4), 1)
            self.assertEqual(mesh.attribute_format(0), VertexFormat.VECTOR3UB_NORMALIZED)
            self.assertEqual(mesh.attribute_format(7), VertexFormat.UNSIGNED_INT)
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
        self.assertEqual(positions.format, 'fff')
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
        self.assertEqual(mutable_positions.format, 'fff')
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
        self.assertEqual(mesh.index_data_flags, trade.DataFlag.OWNED|trade.DataFlag.MUTABLE)

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
        self.assertEqual(mesh.vertex_data_flags, trade.DataFlag.OWNED|trade.DataFlag.MUTABLE)

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
        self.assertEqual(mesh.index_data_flags, trade.DataFlag.OWNED|trade.DataFlag.MUTABLE)

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
        self.assertEqual(mesh.index_data_flags, trade.DataFlag.OWNED|trade.DataFlag.MUTABLE)
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

    def test_data_access_not_mutable(self):
        mesh = primitives.cube_solid()
        # TODO split this once there's a mesh where only one or the other would
        #   be true (maybe with zero-copy loading of PLYs / STLs?)
        self.assertEqual(mesh.index_data_flags, trade.DataFlag(0))
        self.assertEqual(mesh.vertex_data_flags, trade.DataFlag(0))

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

        mesh = importer.mesh(0)
        custom_attribute_id = mesh.attribute_id(importer.mesh_attribute_for_name("_CUSTOM_ATTRIBUTE"))

        with self.assertRaisesRegex(NotImplementedError, "access to this vertex format is not implemented yet, sorry"):
            mesh.attribute(custom_attribute_id)
        with self.assertRaisesRegex(NotImplementedError, "access to this vertex format is not implemented yet, sorry"):
            mesh.mutable_attribute(custom_attribute_id)
        with self.assertRaisesRegex(NotImplementedError, "access to this vertex format is not implemented yet, sorry"):
            mesh.attribute(importer.mesh_attribute_for_name("_CUSTOM_ATTRIBUTE"))
        with self.assertRaisesRegex(NotImplementedError, "access to this vertex format is not implemented yet, sorry"):
            mesh.mutable_attribute(importer.mesh_attribute_for_name("_CUSTOM_ATTRIBUTE"))

class Importer(unittest.TestCase):
    def test(self):
        manager = trade.ImporterManager()
        self.assertIn('StbImageImporter', manager.alias_list)
        self.assertEqual(manager.load_state('StbImageImporter'), pluginmanager.LoadState.NOT_LOADED)

        self.assertTrue(manager.load('StbImageImporter') & pluginmanager.LoadState.LOADED)
        self.assertEqual(manager.unload('StbImageImporter'), pluginmanager.LoadState.NOT_LOADED)

        with self.assertRaisesRegex(RuntimeError, "can't load plugin"):
            manager.load('NonexistentImporter')
        with self.assertRaisesRegex(RuntimeError, "can't unload plugin"):
            manager.unload('NonexistentImporter')

    def test_no_file_opened(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        self.assertFalse(importer.is_opened)

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
            importer.mesh_level_count(0)
        with self.assertRaises(IndexError):
            importer.mesh_name(0)
        with self.assertRaises(IndexError):
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

        with self.assertRaises(IndexError):
            importer.image1d(0)
        with self.assertRaises(IndexError):
            importer.image2d(0, 1)
        with self.assertRaises(IndexError):
            importer.image2d(1)
        with self.assertRaises(IndexError):
            importer.image3d(0)

    def test_open_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')

        with self.assertRaisesRegex(RuntimeError, "opening nonexistent.png failed"):
            importer.open_file('nonexistent.png')
        with self.assertRaisesRegex(RuntimeError, "opening data failed"):
            importer.open_data(b'')

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
            self.assertEqual(importer.mesh_attribute_name(trade.MeshAttribute.CUSTOM(9)), "_CUSTOM_ATTRIBUTE")
            self.assertEqual(importer.mesh_attribute_for_name("_CUSTOM_ATTRIBUTE"), trade.MeshAttribute.CUSTOM(9))
        else:
            self.assertEqual(importer.mesh_attribute_name(trade.MeshAttribute.CUSTOM(7)), "_CUSTOM_ATTRIBUTE")
            self.assertEqual(importer.mesh_attribute_for_name("_CUSTOM_ATTRIBUTE"), trade.MeshAttribute.CUSTOM(7))

        mesh = importer.mesh(0)
        self.assertEqual(mesh.primitive, MeshPrimitive.TRIANGLES)
        self.assertTrue(mesh.has_attribute(importer.mesh_attribute_for_name("_CUSTOM_ATTRIBUTE")))

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

        with self.assertRaises(IndexError):
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

    # TODO image by name (in some gltf?)

    def test_image_level_oob(self):
        # importer refcounting tested in image2d
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))

        with self.assertRaises(IndexError):
            importer.image2d(0, 1)

    def test_image2d_data(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')

        with open(os.path.join(os.path.dirname(__file__), "rgb.png"), 'rb') as f:
            importer.open_data(f.read())

        image = importer.image2d(0)
        self.assertEqual(image.size, Vector2i(3, 2))

    def test_image2d_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_data(b'bla')

        with self.assertRaisesRegex(RuntimeError, "import failed"):
            importer.image2d(0)

class ImageConverter(unittest.TestCase):
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
