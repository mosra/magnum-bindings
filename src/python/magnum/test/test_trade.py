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
from magnum import trade

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
    def test(self):
        # The only way to get a mesh instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.glb'))

        mesh = importer.mesh(1)
        mesh_refcount = sys.getrefcount(mesh)
        self.assertTrue(mesh.is_indexed)
        self.assertEqual(mesh.primitive, MeshPrimitive.TRIANGLES)
        self.assertEqual(mesh.vertex_count, 3)
        self.assertEqual(mesh.index_count, 3)
        self.assertEqual(mesh.attribute_count, 2)
        # TODO: test more, once it's exposed

        index_data = mesh.index_data
        self.assertEqual(len(index_data), 3)
        self.assertIs(index_data.owner, mesh)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del index_data
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

        vertex_data = mesh.vertex_data
        self.assertEqual(len(vertex_data), 72)
        self.assertIs(vertex_data.owner, mesh)
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount + 1)

        del vertex_data
        self.assertEqual(sys.getrefcount(mesh), mesh_refcount)

    def test_nonindexed(self):
        # The only way to get a mesh instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.glb'))

        mesh = importer.mesh(0)
        self.assertFalse(mesh.is_indexed)

        # Accessing the index data should be possible, they're just empty
        self.assertEqual(len(mesh.index_data), 0)

        # Accessing any other index-related info should cause an exception
        with self.assertRaisesRegex(AttributeError, "mesh is not indexed"):
            mesh.index_count

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
            importer.image2d(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.image3d(0)

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
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.glb'))
        self.assertEqual(importer.mesh_count, 3)
        self.assertEqual(importer.mesh_level_count(0), 1)
        self.assertEqual(importer.mesh_name(0), 'Non-indexed mesh')
        self.assertEqual(importer.mesh_for_name('Non-indexed mesh'), 0)

        mesh = importer.mesh(0)
        self.assertEqual(mesh.primitive, MeshPrimitive.TRIANGLES)

    def test_mesh_level_oob(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.glb'))

        with self.assertRaises(IndexError):
            importer.mesh(0, 1)

    def test_mesh_by_name(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.glb'))

        mesh = importer.mesh('Non-indexed mesh')
        self.assertEqual(mesh.primitive, MeshPrimitive.TRIANGLES)

    def test_mesh_by_name_not_found(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.glb'))

        with self.assertRaises(KeyError):
            importer.mesh('Nonexistent')

    def test_mesh_by_name_level_oob(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.glb'))

        with self.assertRaises(IndexError):
            importer.mesh('Non-indexed mesh', 1)

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
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.glb'))
        mesh = importer.mesh(0)

        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.convert_to_file(mesh, os.path.join(tmp, "mesh.ply"))
            self.assertTrue(os.path.exists(os.path.join(tmp, "mesh.ply")))

    def test_mesh_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.glb'))
        mesh = importer.mesh(0)

        converter = trade.SceneConverterManager().load_and_instantiate('AnySceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(RuntimeError, "conversion failed"):
                converter.convert_to_file(mesh, os.path.join(tmp, "mesh.obj"))
