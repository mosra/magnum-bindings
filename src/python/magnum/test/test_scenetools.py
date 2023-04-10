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

import unittest

from corrade import containers
from magnum import *
from magnum import scenetools, trade
import magnum

class FlattenTransformationHierarchy(unittest.TestCase):
    def test_2d(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        # The only way to get a 2D scene is via PrimitiveImporter
        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene = importer.scene(0)
        self.assertTrue(scene.is_2d)

        transformations = scenetools.flatten_transformation_hierarchy2d(scene, trade.SceneField.MESH)
        self.assertEqual(len(transformations), scene.field_size(trade.SceneField.MESH))
        self.assertEqual(transformations[0], Matrix3.translation((-4.5, -3.0)))

    def test_2d_field_id(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        # The only way to get a 2D scene is via PrimitiveImporter
        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene = importer.scene(0)
        self.assertTrue(scene.is_2d)

        mesh_field = scene.field_id(trade.SceneField.MESH)

        transformations = scenetools.flatten_transformation_hierarchy2d(scene, mesh_field)
        self.assertEqual(len(transformations), scene.field_size(mesh_field))
        self.assertEqual(transformations[0], Matrix3.translation((-4.5, -3.0)))

    def test_3d(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene = importer.scene(1)
        self.assertTrue(scene.is_3d)

        transformations = scenetools.flatten_transformation_hierarchy3d(scene, trade.SceneField.MESH)
        self.assertEqual(len(transformations), scene.field_size(trade.SceneField.MESH))
        self.assertEqual(transformations[0], Matrix4.translation((-4.5, -3.0, 0.0)))

    def test_3d_field_id(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene = importer.scene(1)
        self.assertTrue(scene.is_3d)

        mesh_field = scene.field_id(trade.SceneField.MESH)

        transformations = scenetools.flatten_transformation_hierarchy3d(scene, mesh_field)
        self.assertEqual(len(transformations), scene.field_size(mesh_field))
        self.assertEqual(transformations[0], Matrix4.translation((-4.5, -3.0, 0.0)))

    def test_field_not_found(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene = importer.scene(0)

        with self.assertRaises(KeyError):
            scenetools.flatten_transformation_hierarchy2d(scene, trade.SceneField.LIGHT)
        with self.assertRaises(KeyError):
            scenetools.flatten_transformation_hierarchy3d(scene, trade.SceneField.LIGHT)
        with self.assertRaises(IndexError):
            scenetools.flatten_transformation_hierarchy2d(scene, scene.field_count)
        with self.assertRaises(IndexError):
            scenetools.flatten_transformation_hierarchy3d(scene, scene.field_count)

    def test_not_2d_not_3d(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene2d = importer.scene(0)
        scene3d = importer.scene(1)
        self.assertFalse(scene2d.is_3d)
        self.assertFalse(scene3d.is_2d)

        with self.assertRaisesRegex(AssertionError, "the scene is not 2D"):
            scenetools.flatten_transformation_hierarchy2d(scene3d, trade.SceneField.MESH)
        with self.assertRaisesRegex(AssertionError, "the scene is not 2D"):
            scenetools.flatten_transformation_hierarchy2d(scene3d, scene2d.field_id(trade.SceneField.MESH))
        with self.assertRaisesRegex(AssertionError, "the scene is not 3D"):
            scenetools.flatten_transformation_hierarchy3d(scene2d, trade.SceneField.MESH)
        with self.assertRaisesRegex(AssertionError, "the scene is not 3D"):
            scenetools.flatten_transformation_hierarchy3d(scene2d, scene3d.field_id(trade.SceneField.MESH))

    def test_no_hierarchy(self):
        # TODO implement once it's possible to create / import a scene without
        #   the parent field
        pass
