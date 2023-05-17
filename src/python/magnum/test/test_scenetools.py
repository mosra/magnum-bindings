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
import unittest

from corrade import containers
from magnum import *
from magnum import scenetools, trade
import magnum

class Filter(unittest.TestCase):
    def test_fields(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene = importer.scene(0)
        scene_refcount = sys.getrefcount(scene)

        self.assertEqual(scene.field_count, 3)
        self.assertTrue(scene.has_field(trade.SceneField.TRANSLATION))
        self.assertTrue(scene.has_field(trade.SceneField.MESH))

        fields_to_keep = containers.BitArray.value_init(scene.field_count)
        fields_to_keep[scene.field_id(trade.SceneField.TRANSLATION)] = True
        fields_to_keep[scene.field_id(trade.SceneField.MESH)] = True

        filtered = scenetools.filter_fields(scene, fields_to_keep)
        filtered_refcount = sys.getrefcount(filtered)
        self.assertEqual(filtered.field_count, 2)
        self.assertTrue(filtered.has_field(trade.SceneField.TRANSLATION))
        self.assertTrue(filtered.has_field(trade.SceneField.MESH))
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)
        self.assertIs(filtered.owner, scene)

        # Subsequent filtering will still reference the original scene, not the
        # intermediates
        filtered2 = scenetools.filter_fields(filtered, containers.BitArray.direct_init(filtered.field_count, True))
        self.assertEqual(filtered2.field_count, 2)
        self.assertTrue(filtered2.has_field(trade.SceneField.TRANSLATION))
        self.assertTrue(filtered2.has_field(trade.SceneField.MESH))
        self.assertEqual(sys.getrefcount(filtered), filtered_refcount)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 2)
        self.assertIs(filtered2.owner, scene)

        del filtered
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)

        del filtered2
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

    def test_fields_invalid_size(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene = importer.scene(0)

        with self.assertRaisesRegex(AssertionError, "expected 3 bits but got 4"):
            scenetools.filter_fields(scene, containers.BitArray.value_init(4))

    def test_only_fields(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene = importer.scene(0)
        scene_refcount = sys.getrefcount(scene)

        self.assertEqual(scene.field_count, 3)

        # Fields that are not present in the mesh are deliberately ignored
        filtered = scenetools.filter_only_fields(scene, [trade.SceneField.LIGHT, trade.SceneField.MESH, trade.SceneField.TRANSLATION])
        filtered_refcount = sys.getrefcount(filtered)
        self.assertEqual(filtered.field_count, 2)
        self.assertTrue(filtered.has_field(trade.SceneField.TRANSLATION))
        self.assertTrue(filtered.has_field(trade.SceneField.MESH))
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)
        self.assertIs(filtered.owner, scene)

        # Subsequent filtering will still reference the original scene, not the
        # intermediates
        filtered2 = scenetools.filter_only_fields(filtered, [trade.SceneField.MESH, trade.SceneField.TRANSLATION])
        self.assertEqual(filtered2.field_count, 2)
        self.assertTrue(filtered2.has_field(trade.SceneField.TRANSLATION))
        self.assertTrue(filtered2.has_field(trade.SceneField.MESH))
        self.assertEqual(sys.getrefcount(filtered), filtered_refcount)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 2)
        self.assertIs(filtered2.owner, scene)

        del filtered
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)

        del filtered2
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

    def test_except_fields(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene = importer.scene(0)
        scene_refcount = sys.getrefcount(scene)

        self.assertEqual(scene.field_count, 3)

        # Fields that are not present in the mesh are deliberately ignored
        filtered = scenetools.filter_except_fields(scene, [trade.SceneField.SKIN, trade.SceneField.PARENT])
        filtered_refcount = sys.getrefcount(filtered)
        self.assertEqual(filtered.field_count, 2)
        self.assertTrue(filtered.has_field(trade.SceneField.TRANSLATION))
        self.assertTrue(filtered.has_field(trade.SceneField.MESH))
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)
        self.assertIs(filtered.owner, scene)

        # Subsequent filtering will still reference the original scene, not the
        # intermediates
        filtered2 = scenetools.filter_except_fields(filtered, [trade.SceneField.PARENT])
        self.assertEqual(filtered2.field_count, 2)
        self.assertTrue(filtered2.has_field(trade.SceneField.TRANSLATION))
        self.assertTrue(filtered2.has_field(trade.SceneField.MESH))
        self.assertEqual(sys.getrefcount(filtered), filtered_refcount)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 2)
        self.assertIs(filtered2.owner, scene)

        del filtered
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)

        del filtered2
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

    def test_field_entries(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "scene.gltf"))

        scene = importer.scene(0)
        scene_refcount = sys.getrefcount(scene)
        self.assertEqual(scene.field_count, 8)
        self.assertEqual(scene.field_size(trade.SceneField.PARENT), 4)
        self.assertEqual(scene.field_size(trade.SceneField.IMPORTER_STATE), 4)
        self.assertEqual(scene.field_size(trade.SceneField.TRANSFORMATION), 4)
        self.assertEqual(scene.field_size(trade.SceneField.CAMERA), 2)

        # Remove two parents (and importer state, which is linked), one camera
        # and all but one transformation
        parents_to_keep = containers.BitArray.direct_init(scene.field_size(trade.SceneField.PARENT), True)
        parents_to_keep[0] = False
        parents_to_keep[2] = False

        transformations_to_keep = containers.BitArray.direct_init(scene.field_size(trade.SceneField.TRANSFORMATION), False)
        transformations_to_keep[3] = True

        cameras_to_keep = containers.BitArray.direct_init(scene.field_size(trade.SceneField.CAMERA), True)
        cameras_to_keep[1] = False

        filtered1 = scenetools.filter_field_entries(scene, [
            (trade.SceneField.PARENT, parents_to_keep),
            (trade.SceneField.IMPORTER_STATE, parents_to_keep),
            (trade.SceneField.TRANSFORMATION, transformations_to_keep),
            (trade.SceneField.CAMERA, cameras_to_keep)
        ])
        filtered2 = scenetools.filter_field_entries(scene, [
            (scene.field_id(trade.SceneField.PARENT), parents_to_keep),
            (scene.field_id(trade.SceneField.IMPORTER_STATE), parents_to_keep),
            (scene.field_id(trade.SceneField.TRANSFORMATION), transformations_to_keep),
            (scene.field_id(trade.SceneField.CAMERA), cameras_to_keep)
        ])
        self.assertEqual(filtered1.field_count, 8)
        self.assertEqual(filtered2.field_count, 8)
        self.assertEqual(filtered1.field_size(trade.SceneField.PARENT), 2)
        self.assertEqual(filtered2.field_size(trade.SceneField.PARENT), 2)
        self.assertEqual(filtered1.field_size(trade.SceneField.TRANSFORMATION), 1)
        self.assertEqual(filtered2.field_size(trade.SceneField.TRANSFORMATION), 1)
        self.assertEqual(filtered1.field_size(trade.SceneField.CAMERA), 1)
        self.assertEqual(filtered2.field_size(trade.SceneField.CAMERA), 1)
        # The original scene isn't referenced by these, it's a full copy
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

    def test_field_entries_invalid(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "scene.gltf"))

        scene = importer.scene(0)
        scene_refcount = sys.getrefcount(scene)
        self.assertEqual(scene.field_count, 8)

        with self.assertRaisesRegex(AssertionError, "index 8 out of range for 8 fields"):
            scenetools.filter_field_entries(scene, [
                (8, containers.BitArrayView())
            ])
        with self.assertRaisesRegex(AssertionError, "field at index 1 not found"):
            scenetools.filter_field_entries(scene, [
                (trade.SceneField.CAMERA, containers.BitArray.value_init(2)),
                (trade.SceneField.LIGHT, containers.BitArrayView())
            ])

        with self.assertRaisesRegex(AssertionError, "field at index 2 listed more than once"):
            scenetools.filter_field_entries(scene, [
                (trade.SceneField.CAMERA, containers.BitArray.value_init(2)),
                (trade.SceneField.TRANSLATION, containers.BitArray.value_init(3)),
                (trade.SceneField.CAMERA, containers.BitArray.value_init(2))
            ])
        with self.assertRaisesRegex(AssertionError, "field 4 listed more than once"):
            scenetools.filter_field_entries(scene, [
                (scene.field_id(trade.SceneField.CAMERA), containers.BitArray.value_init(2)),
                (scene.field_id(trade.SceneField.TRANSLATION), containers.BitArray.value_init(3)),
                (scene.field_id(trade.SceneField.CAMERA), containers.BitArray.value_init(2))
            ])

        with self.assertRaisesRegex(AssertionError, "expected 3 bits for field 3 but got 4"):
            scenetools.filter_field_entries(scene, [
                (scene.field_id(trade.SceneField.TRANSLATION), containers.BitArray.value_init(4))
            ])
        with self.assertRaisesRegex(AssertionError, "expected 3 bits for field at index 1 but got 4"):
            scenetools.filter_field_entries(scene, [
                (trade.SceneField.CAMERA, containers.BitArray.value_init(2)),
                (trade.SceneField.TRANSLATION, containers.BitArray.value_init(4))
            ])

        with self.assertRaisesRegex(NotImplementedError, "filtering string fields is not implemented yet, sorry"):
            scenetools.filter_field_entries(scene, [
                (scene.field_id(importer.scene_field_for_name('aString')), containers.BitArray.value_init(1))
            ])
        with self.assertRaisesRegex(NotImplementedError, "filtering string fields is not implemented yet, sorry"):
            scenetools.filter_field_entries(scene, [
                (importer.scene_field_for_name('aString'), containers.BitArray.value_init(1))
            ])
        with self.assertRaisesRegex(NotImplementedError, "filtering bit fields is not implemented yet, sorry"):
            scenetools.filter_field_entries(scene, [
                (scene.field_id(importer.scene_field_for_name('yes')), containers.BitArray.value_init(2))
            ])
        with self.assertRaisesRegex(NotImplementedError, "filtering bit fields is not implemented yet, sorry"):
            scenetools.filter_field_entries(scene, [
                (importer.scene_field_for_name('yes'), containers.BitArray.value_init(2))
            ])

    def test_objects(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "scene.gltf"))

        scene = importer.scene(0)
        scene_refcount = sys.getrefcount(scene)
        self.assertEqual(scene.mapping_bound, 4)
        self.assertEqual(scene.field_count, 8)
        self.assertEqual(scene.field_size(trade.SceneField.PARENT), 4)
        self.assertEqual(scene.field_size(trade.SceneField.TRANSFORMATION), 4)
        self.assertEqual(scene.field_size(trade.SceneField.TRANSLATION), 3)
        self.assertEqual(scene.field_size(trade.SceneField.CAMERA), 2)

        # Two parents, transformations and translations gone
        objects_to_keep = containers.BitArray.direct_init(scene.mapping_bound, True)
        objects_to_keep[0] = False
        objects_to_keep[1] = False

        filtered = scenetools.filter_objects(scene, objects_to_keep)
        self.assertEqual(filtered.mapping_bound, 4)
        self.assertEqual(filtered.field_count, 8)
        self.assertEqual(filtered.field_size(trade.SceneField.PARENT), 2)
        self.assertEqual(filtered.field_size(trade.SceneField.TRANSFORMATION), 2)
        self.assertEqual(filtered.field_size(trade.SceneField.TRANSLATION), 1)
        self.assertEqual(filtered.field_size(trade.SceneField.CAMERA), 2)
        # The original scene isn't referenced by this one, it's a full copy
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

    def test_objects_invalid_size(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "scene.gltf"))

        scene = importer.scene(0)
        self.assertEqual(scene.mapping_bound, 4)

        with self.assertRaisesRegex(AssertionError, "expected 4 bits but got 3"):
            scenetools.filter_objects(scene, containers.BitArray.value_init(3))

class Hierarchy(unittest.TestCase):
    def test_parents_breadth_first_children_depth_first(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "scene.gltf"))

        scene = importer.scene(0)

        parents_breadth_first = scenetools.parents_breadth_first(scene)
        children_depth_first = scenetools.children_depth_first(scene)
        self.assertEqual(parents_breadth_first, [
            # Root objects first, parent always before all its children
            (1, -1),
            (2, -1),
            (3, 2),
            (0, 3)
        ])
        self.assertEqual(children_depth_first, [
            # Object 1 has no children
            (1, 0),
            # Object 2 has one direct child and one grandchild, etc
            (2, 2),
                (3, 1),
                    (0, 0)
        ])

    def test_parents_breadth_first_children_depth_first_no_hierarchy(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "scene.gltf"))

        scene = scenetools.filter_except_fields(importer.scene(0), [trade.SceneField.PARENT])

        with self.assertRaisesRegex(AssertionError, "the scene has no hierarchy"):
            scenetools.parents_breadth_first(scene)
        with self.assertRaisesRegex(AssertionError, "the scene has no hierarchy"):
            scenetools.children_depth_first(scene)

    def test_absolute_field_transformations2d(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        # The only way to get a 2D scene is via PrimitiveImporter
        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene = importer.scene(0)
        self.assertTrue(scene.is_2d)

        transformations1 = scenetools.absolute_field_transformations2d(scene, trade.SceneField.MESH)
        transformations2 = scenetools.absolute_field_transformations2d(scene, scene.field_id(trade.SceneField.MESH))
        self.assertEqual(len(transformations1), scene.field_size(trade.SceneField.MESH))
        self.assertEqual(len(transformations2), scene.field_size(trade.SceneField.MESH))
        self.assertEqual(transformations1[0], Matrix3.translation((-4.5, -3.0)))
        self.assertEqual(transformations2[0], Matrix3.translation((-4.5, -3.0)))

    def test_absolute_field_transformations3d(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene = importer.scene(1)
        self.assertTrue(scene.is_3d)

        transformations1 = scenetools.absolute_field_transformations3d(scene, trade.SceneField.MESH)
        transformations2 = scenetools.absolute_field_transformations3d(scene, scene.field_id(trade.SceneField.MESH))
        self.assertEqual(len(transformations1), scene.field_size(trade.SceneField.MESH))
        self.assertEqual(len(transformations2), scene.field_size(trade.SceneField.MESH))
        self.assertEqual(transformations1[0], Matrix4.translation((-4.5, -3.0, 0.0)))
        self.assertEqual(transformations2[0], Matrix4.translation((-4.5, -3.0, 0.0)))

    def test_absolute_field_transformations_not_found(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene = importer.scene(0)

        with self.assertRaises(KeyError):
            scenetools.absolute_field_transformations2d(scene, trade.SceneField.LIGHT)
        with self.assertRaises(KeyError):
            scenetools.absolute_field_transformations3d(scene, trade.SceneField.LIGHT)
        with self.assertRaises(IndexError):
            scenetools.absolute_field_transformations2d(scene, scene.field_count)
        with self.assertRaises(IndexError):
            scenetools.absolute_field_transformations3d(scene, scene.field_count)

    def test_absolute_field_transformations_not_2d_not_3d(self):
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
            scenetools.absolute_field_transformations2d(scene3d, trade.SceneField.MESH)
        with self.assertRaisesRegex(AssertionError, "the scene is not 2D"):
            scenetools.absolute_field_transformations2d(scene3d, scene2d.field_id(trade.SceneField.MESH))
        with self.assertRaisesRegex(AssertionError, "the scene is not 3D"):
            scenetools.absolute_field_transformations3d(scene2d, trade.SceneField.MESH)
        with self.assertRaisesRegex(AssertionError, "the scene is not 3D"):
            scenetools.absolute_field_transformations3d(scene2d, scene3d.field_id(trade.SceneField.MESH))

    def test_absolute_field_transformations_no_hierarchy(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene2d = scenetools.filter_except_fields(importer.scene(0), [trade.SceneField.PARENT])
        scene3d = scenetools.filter_except_fields(importer.scene(1), [trade.SceneField.PARENT])
        self.assertFalse(scene2d.is_3d)
        self.assertFalse(scene3d.is_2d)

        with self.assertRaisesRegex(AssertionError, "the scene has no hierarchy"):
            scenetools.absolute_field_transformations2d(scene2d, trade.SceneField.MESH)
        with self.assertRaisesRegex(AssertionError, "the scene has no hierarchy"):
            scenetools.absolute_field_transformations2d(scene2d, scene2d.field_id(trade.SceneField.MESH))
        with self.assertRaisesRegex(AssertionError, "the scene has no hierarchy"):
            scenetools.absolute_field_transformations3d(scene3d, trade.SceneField.MESH)
        with self.assertRaisesRegex(AssertionError, "the scene has no hierarchy"):
            scenetools.absolute_field_transformations3d(scene3d, scene2d.field_id(trade.SceneField.MESH))
