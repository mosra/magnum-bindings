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

import os
import unittest

from corrade import containers
from magnum import *
from magnum import materialtools, trade
import magnum

class Copy(unittest.TestCase):
    def test(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "material.gltf"))

        material = importer.material(0)
        self.assertEqual(material.attribute_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)

        copy = materialtools.copy(material)
        self.assertEqual(copy.attribute_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)

class Filter(unittest.TestCase):
    def test(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        # This adds extra Diffuse attributes for BaseColor and a Phong type,
        # don't want
        importer.configuration['phongMaterialFallback'] = False
        importer.open_file(os.path.join(os.path.dirname(__file__), "material.gltf"))

        material = importer.material(0)
        self.assertEqual(material.types, trade.MaterialTypes.PBR_METALLIC_ROUGHNESS|trade.MaterialTypes.PBR_CLEAR_COAT)
        self.assertEqual(material.layer_count, 2)
        self.assertEqual(material.attribute_count(0), 3)
        self.assertEqual(material.attribute_count(1), 8)

        # Unlike MeshData or SceneData the tools always produce a new copy of
        # the attribute list, so we don't need to worry about data ownership
        # and such

        attributes_to_keep = containers.BitArray.direct_init(material.attribute_data_offset(material.layer_count), True)
        attributes_to_keep[material.attribute_id(trade.MaterialAttribute.DOUBLE_SIDED)] = False
        attributes_to_keep[material.attribute_data_offset(1) + material.attribute_id(1, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_MATRIX)] = False
        filtered_attributes = materialtools.filter_attributes(material, attributes_to_keep, ~trade.MaterialTypes.PBR_CLEAR_COAT)
        self.assertEqual(filtered_attributes.types, trade.MaterialTypes.PBR_METALLIC_ROUGHNESS)
        self.assertEqual(filtered_attributes.layer_count, 2)
        self.assertEqual(filtered_attributes.attribute_count(0), 2)
        self.assertEqual(filtered_attributes.attribute_count(1), 7)
        self.assertFalse(filtered_attributes.has_attribute(trade.MaterialAttribute.DOUBLE_SIDED))
        self.assertFalse(filtered_attributes.has_attribute(1, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_MATRIX))

        layers_to_keep = containers.BitArray.direct_init(material.layer_count, True)
        layers_to_keep[1] = False
        filtered_layers = materialtools.filter_layers(material, layers_to_keep, ~trade.MaterialTypes.PBR_CLEAR_COAT)
        self.assertEqual(filtered_layers.types, trade.MaterialTypes.PBR_METALLIC_ROUGHNESS)
        self.assertEqual(filtered_layers.layer_count, 1)
        self.assertEqual(filtered_layers.attribute_count(), 3)

        filtered_attributes_layers = materialtools.filter_attributes_layers(material, attributes_to_keep, layers_to_keep, ~trade.MaterialTypes.PBR_CLEAR_COAT)
        self.assertEqual(filtered_attributes_layers.types, trade.MaterialTypes.PBR_METALLIC_ROUGHNESS)
        self.assertEqual(filtered_attributes_layers.layer_count, 1)
        self.assertEqual(filtered_attributes_layers.attribute_count(), 2)
        self.assertFalse(filtered_attributes_layers.has_attribute(trade.MaterialAttribute.DOUBLE_SIDED))

    def test_invalid_size(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        # This adds extra Diffuse attributes for BaseColor and a Phong type,
        # don't want
        importer.configuration['phongMaterialFallback'] = False
        importer.open_file(os.path.join(os.path.dirname(__file__), "material.gltf"))

        material = importer.material(0)
        self.assertEqual(material.types, trade.MaterialTypes.PBR_METALLIC_ROUGHNESS|trade.MaterialTypes.PBR_CLEAR_COAT)
        self.assertEqual(material.layer_count, 2)
        self.assertEqual(material.attribute_count(0), 3)
        self.assertEqual(material.attribute_count(1), 8)

        with self.assertRaisesRegex(AssertionError, "expected 11 bits but got 12"):
            materialtools.filter_attributes(material, containers.BitArray.value_init(12))
        with self.assertRaisesRegex(AssertionError, "expected 2 bits but got 3"):
            materialtools.filter_layers(material, containers.BitArray.value_init(3))
        with self.assertRaisesRegex(AssertionError, "expected 11 attribute bits but got 12"):
            materialtools.filter_attributes_layers(material, containers.BitArray.value_init(12), containers.BitArray.value_init(2))
        with self.assertRaisesRegex(AssertionError, "expected 2 layer bits but got 3"):
            materialtools.filter_attributes_layers(material, containers.BitArray.value_init(11), containers.BitArray.value_init(3))

class Merge(unittest.TestCase):
    def test(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        # This adds extra Diffuse attributes for BaseColor and a Phong type,
        # don't want
        importer.configuration['phongMaterialFallback'] = False
        importer.open_file(os.path.join(os.path.dirname(__file__), "material.gltf"))

        a = importer.material('A material with a layer')
        self.assertEqual(a.types, trade.MaterialTypes.PBR_METALLIC_ROUGHNESS|trade.MaterialTypes.PBR_CLEAR_COAT)
        self.assertEqual(a.layer_count, 2)
        self.assertEqual(a.attribute_count(0), 3)
        self.assertEqual(a.attribute_count(1), 8)

        b = importer.material('Material with an empty layer')
        self.assertEqual(b.types, trade.MaterialTypes.PBR_CLEAR_COAT)
        self.assertEqual(b.layer_count, 2)
        self.assertEqual(b.attribute_count(0), 1)
        self.assertEqual(b.attribute_count(1), 3)

        merged = materialtools.merge(a, b, materialtools.MergeConflicts.KEEP_FIRST_IF_SAME_TYPE)
        self.assertEqual(merged.types, trade.MaterialTypes.PBR_METALLIC_ROUGHNESS|trade.MaterialTypes.PBR_CLEAR_COAT)
        self.assertEqual(merged.layer_count, 2)
        self.assertEqual(merged.attribute_count(0), 4)
        self.assertEqual(merged.attribute_count(1), 8)

    def test_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "material.gltf"))

        a = importer.material('A material with a layer')
        b = importer.material('Material with an empty layer')

        with self.assertRaisesRegex(RuntimeError, "material merge failed"):
            materialtools.merge(a, b)

class PhongToPbrMetallicRoughness(unittest.TestCase):
    def test(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        # We actually *do* want the Phong stuff
        importer.configuration['phongMaterialFallback'] = True
        importer.open_file(os.path.join(os.path.dirname(__file__), "material.gltf"))
        material = importer.material(0)

        # Keep just the diffuse color, drop everything else
        attributes_to_keep = containers.BitArray.value_init(material.attribute_data_offset(material.layer_count))
        attributes_to_keep[material.attribute_id(trade.MaterialAttribute.DIFFUSE_COLOR)] = True
        layers_to_keep = containers.BitArray.value_init(material.layer_count)
        layers_to_keep[0] = True
        filtered = materialtools.filter_attributes_layers(material, attributes_to_keep, layers_to_keep, ~(trade.MaterialTypes.PBR_METALLIC_ROUGHNESS|trade.MaterialTypes.PBR_CLEAR_COAT))
        self.assertEqual(filtered.types, trade.MaterialTypes.PHONG)
        self.assertEqual(filtered.layer_count, 1)
        self.assertEqual(filtered.attribute_count(), 1)
        self.assertTrue(filtered.has_attribute(trade.MaterialAttribute.DIFFUSE_COLOR))

        pbr = materialtools.phong_to_pbr_metallic_roughness(filtered)
        self.assertEqual(pbr.types, trade.MaterialTypes.PBR_METALLIC_ROUGHNESS)
        self.assertEqual(pbr.layer_count, 1)
        self.assertEqual(pbr.attribute_count(), 1)
        self.assertTrue(pbr.has_attribute(trade.MaterialAttribute.BASE_COLOR))

    def test_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "material.gltf"))
        material = importer.material('Specular/glossiness')

        with self.assertRaisesRegex(RuntimeError, "material conversion failed"):
            materialtools.phong_to_pbr_metallic_roughness(material, materialtools.PhongToPbrMetallicRoughnessFlags.FAIL_ON_UNCONVERTIBLE_ATTRIBUTES)

class RemoveDuplicates(unittest.TestCase):
    def test(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "material.gltf"))
        a = importer.material(0)
        b = importer.material(1)
        c = importer.material(2)

        indices, count = materialtools.remove_duplicates([a, b, a, c, c, b])
        self.assertEqual(indices, [0, 1, 0, 3, 3, 1])
        self.assertEqual(count, 3)
