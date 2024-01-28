#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>
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
import platform
import sys
import tempfile
import unittest

from corrade import containers, pluginmanager
from magnum import *
from magnum import primitives, scenetools, trade
import magnum

class ImageData(unittest.TestCase):
    def test(self):
        # The only way to get an image instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "rgb.png"))

        image = importer.image2d(0)
        self.assertFalse(image.is_compressed)
        self.assertEqual(image.storage.alignment, 1) # libPNG has 4 tho
        self.assertEqual(image.format, PixelFormat.RGB8_UNORM)
        self.assertEqual(image.pixel_size, 3)
        self.assertEqual(image.size, Vector2i(3, 2))
        self.assertIsNone(image.owner)

        with self.assertRaisesRegex(AttributeError, "image is not compressed"):
            image.compressed_format

    def test_compressed(self):
        # The only way to get an image instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('DdsImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "rgba_dxt1.dds"))
        image = importer.image2d(0)
        self.assertEqual(len(image.data), 8)
        self.assertTrue(image.is_compressed)
        self.assertEqual(image.compressed_format, CompressedPixelFormat.BC1_RGBA_UNORM)
        # TODO: remaining compressed properties

        with self.assertRaisesRegex(AttributeError, "image is compressed"):
            image.storage
        with self.assertRaisesRegex(AttributeError, "image is compressed"):
            image.format
        with self.assertRaisesRegex(AttributeError, "image is compressed"):
            image.pixel_size
        with self.assertRaisesRegex(AttributeError, "image is compressed"):
            image.pixels
        with self.assertRaisesRegex(AttributeError, "image is compressed"):
            image.mutable_pixels

    def test_convert_view(self):
        # The only way to get an image instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "rgb.png"))
        image = importer.image2d(0)

        view = ImageView2D(image)
        mutable_view = MutableImageView2D(image)

        with self.assertRaisesRegex(RuntimeError, "image is not compressed"):
            CompressedImageView2D(image)
        with self.assertRaisesRegex(RuntimeError, "image is not compressed"):
            MutableCompressedImageView2D(image)

    def test_convert_view_compressed(self):
        # The only way to get an image instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('DdsImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "rgba_dxt1.dds"))
        image = importer.image2d(0)

        view = CompressedImageView2D(image)
        mutable_view = MutableCompressedImageView2D(image)

        with self.assertRaisesRegex(RuntimeError, "image is compressed"):
            view = ImageView2D(image)
        with self.assertRaisesRegex(RuntimeError, "image is compressed"):
            mutable_view = MutableImageView2D(image)

    def test_data_access(self):
        # The only way to get an image instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "rgb.png"))

        image = importer.image2d(0)
        image_refcount = sys.getrefcount(image)
        self.assertEqual(image.storage.alignment, 1) # libPNG has 4 tho
        self.assertEqual(image.format, PixelFormat.RGB8_UNORM)
        self.assertEqual(image.size, Vector2i(3, 2))

        data = image.data
        self.assertEqual(len(data), 3*3*2)
        self.assertEqual(data[9 + 6 + 2], 181) # libPNG has 12 +
        self.assertIs(data.owner, image)
        self.assertEqual(sys.getrefcount(image), image_refcount + 1)

        del data
        self.assertEqual(sys.getrefcount(image), image_refcount)

        mutable_data = image.data
        self.assertEqual(len(mutable_data), 3*3*2)
        self.assertEqual(mutable_data[9 + 6 + 2], 181) # libPNG has 12 +
        self.assertIs(mutable_data.owner, image)
        self.assertEqual(sys.getrefcount(image), image_refcount + 1)

        del mutable_data
        self.assertEqual(sys.getrefcount(image), image_refcount)

    def test_mutable_data_access(self):
        # The only way to get an image instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "rgb.png"))

        image = importer.image2d(0)
        self.assertEqual(image.data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)

        data = image.data
        mutable_data = image.mutable_data
        self.assertEqual(data[13], 254)
        self.assertEqual(mutable_data[13], 254)

        mutable_data[13] = 76
        self.assertEqual(data[13], 76)

    def test_pixels_access(self):
        # The only way to get an image instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "rgb.png"))

        image = importer.image2d(0)
        image_refcount = sys.getrefcount(image)
        self.assertEqual(image.storage.alignment, 1) # libPNG has 4 tho
        self.assertEqual(image.format, PixelFormat.RGB8_UNORM)
        self.assertEqual(image.size, Vector2i(3, 2))

        pixels = image.pixels
        self.assertEqual(pixels.size, (2, 3))
        self.assertEqual(pixels.stride, (9, 3))
        self.assertEqual(pixels.format, '3B')
        self.assertEqual(pixels[0, 2], Color3(0.792157, 0.996078, 0.466667))
        self.assertEqual(pixels[1, 0], Color3(0.870588, 0.678431, 0.709804))
        self.assertIs(pixels.owner, image)
        self.assertEqual(sys.getrefcount(image), image_refcount + 1)

        del pixels
        self.assertEqual(sys.getrefcount(image), image_refcount)

    def test_mutable_pixels_access(self):
        # The only way to get an image instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "rgb.png"))

        image = importer.image2d(0)
        self.assertEqual(image.data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)

        pixels = image.pixels
        mutable_pixels = image.mutable_pixels
        self.assertEqual(pixels[0, 2], Color3(0.792157, 0.996078, 0.466667))
        self.assertEqual(mutable_pixels[0, 2], Color3(0.792157, 0.996078, 0.466667))

        mutable_pixels[0, 2] *= 0.5
        self.assertEqual(pixels[0, 2], Color3(0.396078, 0.498039, 0.235294))

    def test_data_access_not_mutable(self):
        pass
        # TODO implement once there's a way to get immutable ImageData, either
        #   by "deserializing" a binary blob, or by mmapping a KTX file etc.

    def test_pixels_access_unsupported_format(self):
        # The only way to get an image instance is through a manager
        importer = trade.ImporterManager().load_and_instantiate('DdsImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "dxt10-depth32f-stencil8ui.dds"))

        image = importer.image2d(0)
        self.assertEqual(image.format, PixelFormat.DEPTH32F_STENCIL8UI)

        with self.assertRaisesRegex(NotImplementedError, "access to PixelFormat.DEPTH32F_STENCIL8UI is not implemented yet, sorry"):
            image.pixels

class MaterialData(unittest.TestCase):
    def test_layer_properties(self):
        self.assertEqual(trade.MaterialLayer.CLEAR_COAT.string, "ClearCoat")

    def test_attribute_properties(self):
        self.assertEqual(trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE.string, "LayerFactorTextureSwizzle")

    def test_texture_swizzle_properties(self):
        self.assertEqual(trade.MaterialTextureSwizzle.GB.component_count, 2)

    def test(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        # This adds extra Diffuse attributes for BaseColor, don't want
        importer.configuration['phongMaterialFallback'] = False
        importer.open_file(os.path.join(os.path.dirname(__file__), 'material.gltf'))

        material = importer.material(0)
        material_empty = importer.material(1)

        self.assertEqual(material.attribute_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)
        self.assertEqual(material.layer_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)
        self.assertEqual(material.types, trade.MaterialTypes.PBR_METALLIC_ROUGHNESS|trade.MaterialTypes.PBR_CLEAR_COAT)

        self.assertEqual(material.layer_count, 2)
        self.assertEqual(material.attribute_data_offset(1), 3)
        self.assertEqual(material.attribute_data_offset(2), 11)
        self.assertTrue(material.has_layer("ClearCoat"))
        self.assertFalse(material_empty.has_layer("ClearCoat"))
        self.assertTrue(material.has_layer(trade.MaterialLayer.CLEAR_COAT))
        self.assertFalse(material_empty.has_layer(trade.MaterialLayer.CLEAR_COAT))
        self.assertEqual(material.layer_id("ClearCoat"), 1)
        self.assertEqual(material.layer_id(trade.MaterialLayer.CLEAR_COAT), 1)
        self.assertEqual(material.layer_name(1), "ClearCoat")
        self.assertAlmostEqual(material.layer_factor(1), 0.7)
        self.assertAlmostEqual(material.layer_factor("ClearCoat"), 0.7)
        self.assertAlmostEqual(material.layer_factor(trade.MaterialLayer.CLEAR_COAT), 0.7)
        self.assertEqual(material.layer_factor_texture(1), 2)
        self.assertEqual(material.layer_factor_texture("ClearCoat"), 2)
        self.assertEqual(material.layer_factor_texture(trade.MaterialLayer.CLEAR_COAT), 2)
        # TODO test with something where the swizzle isn't default to verify
        #   it's querying the right layer
        self.assertEqual(material.layer_factor_texture_swizzle(1), trade.MaterialTextureSwizzle.R)
        self.assertEqual(material.layer_factor_texture_swizzle("ClearCoat"), trade.MaterialTextureSwizzle.R)
        self.assertEqual(material.layer_factor_texture_swizzle(trade.MaterialLayer.CLEAR_COAT), trade.MaterialTextureSwizzle.R)
        self.assertEqual(material.layer_factor_texture_matrix(1), Matrix3.translation((0.25, -0.5)))
        self.assertEqual(material.layer_factor_texture_matrix("ClearCoat"), Matrix3.translation((0.25, -0.5)))
        self.assertEqual(material.layer_factor_texture_matrix(trade.MaterialLayer.CLEAR_COAT), Matrix3.translation((0.25, -0.5)))
        self.assertEqual(material.layer_factor_texture_coordinates(1), 13)
        self.assertEqual(material.layer_factor_texture_coordinates("ClearCoat"), 13)
        self.assertEqual(material.layer_factor_texture_coordinates(trade.MaterialLayer.CLEAR_COAT), 13)
        # TODO test with something where the layer isn't 0, KHR_image_ktx is
        #   too annoying
        self.assertEqual(material.layer_factor_texture_layer(1), 0)
        self.assertEqual(material.layer_factor_texture_layer("ClearCoat"), 0)
        self.assertEqual(material.layer_factor_texture_layer(trade.MaterialLayer.CLEAR_COAT), 0)

        self.assertEqual(material.attribute_count(1), 8)
        self.assertEqual(material.attribute_count("ClearCoat"), 8)
        self.assertEqual(material.attribute_count(trade.MaterialLayer.CLEAR_COAT), 8)
        self.assertEqual(material.attribute_count(1), 8)
        self.assertEqual(material.attribute_count(), 3)

        self.assertTrue(material.has_attribute(1, "LayerFactorTexture"))
        self.assertFalse(material.has_attribute(1, "LayerFactorTextureSwizzle"))
        self.assertTrue(material.has_attribute(1, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE))
        self.assertFalse(material.has_attribute(1, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE))
        self.assertTrue(material.has_attribute("ClearCoat", "LayerFactorTexture"))
        self.assertFalse(material.has_attribute("ClearCoat", "LayerFactorTextureSwizzle"))
        self.assertTrue(material.has_attribute("ClearCoat", trade.MaterialAttribute.LAYER_FACTOR_TEXTURE))
        self.assertFalse(material.has_attribute("ClearCoat", trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE))
        self.assertTrue(material.has_attribute(trade.MaterialLayer.CLEAR_COAT, "LayerFactorTexture"))
        self.assertFalse(material.has_attribute(trade.MaterialLayer.CLEAR_COAT, "LayerFactorTextureSwizzle"))
        self.assertTrue(material.has_attribute(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE))
        self.assertFalse(material.has_attribute(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE))
        self.assertTrue(material.has_attribute("DoubleSided"))
        self.assertFalse(material.has_attribute("BaseColorTexture"))
        self.assertTrue(material.has_attribute(trade.MaterialAttribute.DOUBLE_SIDED))
        self.assertFalse(material.has_attribute(trade.MaterialAttribute.BASE_COLOR_TEXTURE))

        self.assertEqual(material.attribute_id(1, "RoughnessTexture"), 6)
        self.assertEqual(material.attribute_id(1, trade.MaterialAttribute.ROUGHNESS_TEXTURE), 6)
        self.assertEqual(material.attribute_id("ClearCoat", "RoughnessTexture"), 6)
        self.assertEqual(material.attribute_id("ClearCoat", trade.MaterialAttribute.ROUGHNESS_TEXTURE), 6)
        self.assertEqual(material.attribute_id(trade.MaterialLayer.CLEAR_COAT, "RoughnessTexture"), 6)
        self.assertEqual(material.attribute_id(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.ROUGHNESS_TEXTURE), 6)
        self.assertEqual(material.attribute_id("DoubleSided"), 2)
        self.assertEqual(material.attribute_id(trade.MaterialAttribute.DOUBLE_SIDED), 2)

        self.assertEqual(material.attribute_name(1, 6), "RoughnessTexture")
        self.assertEqual(material.attribute_name("ClearCoat", 6), "RoughnessTexture")
        self.assertEqual(material.attribute_name(trade.MaterialLayer.CLEAR_COAT, 6), "RoughnessTexture")
        self.assertEqual(material.attribute_name(2), "DoubleSided")

        self.assertEqual(material.attribute_type(1, 4), trade.MaterialAttributeType.MATRIX3X3)
        self.assertEqual(material.attribute_type(1, "LayerFactorTextureMatrix"), trade.MaterialAttributeType.MATRIX3X3)
        self.assertEqual(material.attribute_type(1, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_MATRIX), trade.MaterialAttributeType.MATRIX3X3)
        self.assertEqual(material.attribute_type("ClearCoat", 4), trade.MaterialAttributeType.MATRIX3X3)
        self.assertEqual(material.attribute_type("ClearCoat", "LayerFactorTextureMatrix"), trade.MaterialAttributeType.MATRIX3X3)
        self.assertEqual(material.attribute_type("ClearCoat", trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_MATRIX), trade.MaterialAttributeType.MATRIX3X3)
        self.assertEqual(material.attribute_type(trade.MaterialLayer.CLEAR_COAT, 4), trade.MaterialAttributeType.MATRIX3X3)
        self.assertEqual(material.attribute_type(trade.MaterialLayer.CLEAR_COAT, "LayerFactorTextureMatrix"), trade.MaterialAttributeType.MATRIX3X3)
        self.assertEqual(material.attribute_type(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_MATRIX), trade.MaterialAttributeType.MATRIX3X3)
        self.assertEqual(material.attribute_type(2), trade.MaterialAttributeType.BOOL)
        self.assertEqual(material.attribute_type("DoubleSided"), trade.MaterialAttributeType.BOOL)
        self.assertEqual(material.attribute_type(trade.MaterialAttribute.DOUBLE_SIDED), trade.MaterialAttributeType.BOOL)

        self.assertEqual(material.attribute(1, 3), 13)
        self.assertEqual(material.attribute(1, "LayerFactorTextureCoordinates"), 13)
        self.assertEqual(material.attribute(1, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_COORDINATES), 13)
        self.assertEqual(material.attribute("ClearCoat", 3), 13)
        self.assertEqual(material.attribute("ClearCoat", "LayerFactorTextureCoordinates"), 13)
        self.assertEqual(material.attribute("ClearCoat", trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_COORDINATES), 13)
        self.assertEqual(material.attribute(trade.MaterialLayer.CLEAR_COAT, 3), 13)
        self.assertEqual(material.attribute(trade.MaterialLayer.CLEAR_COAT, "LayerFactorTextureCoordinates"), 13)
        self.assertEqual(material.attribute(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_COORDINATES), 13)
        self.assertEqual(material.attribute(2), True)
        self.assertEqual(material.attribute("DoubleSided"), True)
        self.assertEqual(material.attribute(trade.MaterialAttribute.DOUBLE_SIDED), True)

        self.assertTrue(material.is_double_sided)
        self.assertEqual(material.alpha_mode, trade.MaterialAlphaMode.MASK)
        self.assertEqual(material.alpha_mask, 0.36899998784065247)

    def test_attribute_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'material.gltf'))

        material = importer.material(0)

        # Boolean, scalar, vector, matrix
        self.assertEqual(material.attribute(trade.MaterialAttribute.DOUBLE_SIDED), True)
        self.assertEqual(material.attribute(trade.MaterialAttribute.ALPHA_MASK), 0.36899998784065247)
        self.assertEqual(material.attribute(trade.MaterialAttribute.BASE_COLOR), Vector4(0.3, 0.4, 0.5, 0.8))
        self.assertEqual(material.attribute(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_MATRIX), Matrix3(
            (1.0, 0.0, 0.0),
            (0.0, 1.0, 0.0),
            (0.25, -0.5, 1.0)))

        # Texture swizzle, string
        self.assertEqual(material.attribute(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.ROUGHNESS_TEXTURE_SWIZZLE), trade.MaterialTextureSwizzle.G)
        self.assertEqual(material.attribute(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.LAYER_NAME), "ClearCoat")

    def test_attribute_access_unsupported_format(self):
        # TODO test this once Assimp or Ufbx or serialized data loading exists
        #  to have a Buffer or a Pointer attribute
        pass

    def test_layer_oob(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        # This adds extra Diffuse attributes for BaseColor, don't want
        importer.configuration['phongMaterialFallback'] = False
        importer.open_file(os.path.join(os.path.dirname(__file__), 'material.gltf'))

        material = importer.material(0)
        material_empty = importer.material(1)
        material_empty_layer = importer.material(2)

        # TODO these are all printed with '' around except for an IndexError,
        #   why? Is that implicit behavior of the KeyError? Ugh??
        # https://stackoverflow.com/a/24999035
        with self.assertRaisesRegex(IndexError, "index 3 out of range for 2 layers"):
            # Passing 2 works
            material.attribute_data_offset(3)
        with self.assertRaisesRegex(KeyError, "FlearFoat not found among 2 layers"):
            material.layer_id("FlearFoat")
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.layer_id(trade.MaterialLayer.CLEAR_COAT)
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.layer_name(2)
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.layer_factor(2)
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.layer_factor("FlearFoat")
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.layer_factor(trade.MaterialLayer.CLEAR_COAT)
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.layer_factor_texture(2)
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.layer_factor_texture("FlearFoat")
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.layer_factor_texture(trade.MaterialLayer.CLEAR_COAT)
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.layer_factor_texture_swizzle(2)
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.layer_factor_texture_swizzle("FlearFoat")
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.layer_factor_texture_swizzle(trade.MaterialLayer.CLEAR_COAT)
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.layer_factor_texture_matrix(2)
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.layer_factor_texture_matrix("FlearFoat")
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.layer_factor_texture_matrix(trade.MaterialLayer.CLEAR_COAT)
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.layer_factor_texture_coordinates(2)
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.layer_factor_texture_coordinates("FlearFoat")
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.layer_factor_texture_coordinates(trade.MaterialLayer.CLEAR_COAT)
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.layer_factor_texture_layer(2)
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.layer_factor_texture_layer("FlearFoat")
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.layer_factor_texture_layer(trade.MaterialLayer.CLEAR_COAT)

        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.attribute_count(2)
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.attribute_count("FlearFoat")
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.attribute_count(trade.MaterialLayer.CLEAR_COAT)

        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.has_attribute(2, "LayerFactor")
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.has_attribute(2, trade.MaterialAttribute.LAYER_FACTOR)
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.has_attribute("FlearFoat", "LayerFactor")
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.has_attribute("FlearFoat", trade.MaterialAttribute.LAYER_FACTOR)
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.has_attribute(trade.MaterialLayer.CLEAR_COAT, "LayerFactor")
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.has_attribute(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.LAYER_FACTOR)

        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.attribute_id(2, "LayerFactor")
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.attribute_id(2, trade.MaterialAttribute.LAYER_FACTOR)
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.attribute_id("FlearFoat", "LayerFactor")
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.attribute_id("FlearFoat", trade.MaterialAttribute.LAYER_FACTOR)
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.attribute_id(trade.MaterialLayer.CLEAR_COAT, "LayerFactor")
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.attribute_id(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.LAYER_FACTOR)

        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.attribute_name(2, 0)
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.attribute_name("FlearFoat", 0)
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.attribute_name(trade.MaterialLayer.CLEAR_COAT, 0)

        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.attribute_type(2, 0)
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.attribute_type(2, "LayerFactor")
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.attribute_type(2, trade.MaterialAttribute.LAYER_FACTOR)
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.attribute_type("FlearFoat", 0)
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.attribute_type("FlearFoat", "LayerFactor")
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.attribute_type("FlearFoat", trade.MaterialAttribute.LAYER_FACTOR)
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.attribute_type(trade.MaterialLayer.CLEAR_COAT, 0)
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.attribute_type(trade.MaterialLayer.CLEAR_COAT, "LayerFactor")
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.attribute_type(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.LAYER_FACTOR)

        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.attribute(2, 0)
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.attribute(2, "LayerFactor")
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 layers"):
            material.attribute(2, trade.MaterialAttribute.LAYER_FACTOR)
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.attribute("FlearFoat", 0)
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.attribute("FlearFoat", "LayerFactor")
        with self.assertRaisesRegex(KeyError, "name FlearFoat not found among 2 layers"):
            material.attribute("FlearFoat", trade.MaterialAttribute.LAYER_FACTOR)
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.attribute(trade.MaterialLayer.CLEAR_COAT, 0)
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.attribute(trade.MaterialLayer.CLEAR_COAT, "LayerFactor")
        with self.assertRaisesRegex(KeyError, "MaterialLayer.CLEAR_COAT not found among 1 layers"):
            material_empty.attribute(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.LAYER_FACTOR)

    def test_attribute_oob(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        # This adds extra Diffuse attributes for BaseColor, don't want
        importer.configuration['phongMaterialFallback'] = False
        importer.open_file(os.path.join(os.path.dirname(__file__), 'material.gltf'))

        material = importer.material(0)
        material_empty_layer = importer.material(2)

        # TODO these are all printed with '' around except for an IndexError,
        #   why? Is that implicit behavior of the KeyError? Ugh??
        # https://stackoverflow.com/a/24999035
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in layer 0"):
            material.layer_factor_texture(0)
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in layer ClearCoat"):
            material_empty_layer.layer_factor_texture("ClearCoat")
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in MaterialLayer.CLEAR_COAT"):
            material_empty_layer.layer_factor_texture(trade.MaterialLayer.CLEAR_COAT)
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in layer 0"):
            material.layer_factor_texture_swizzle(0)
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in layer ClearCoat"):
            material_empty_layer.layer_factor_texture_swizzle("ClearCoat")
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in MaterialLayer.CLEAR_COAT"):
            material_empty_layer.layer_factor_texture_swizzle(trade.MaterialLayer.CLEAR_COAT)
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in layer 0"):
            material.layer_factor_texture_matrix(0)
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in layer ClearCoat"):
            material_empty_layer.layer_factor_texture_matrix("ClearCoat")
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in MaterialLayer.CLEAR_COAT"):
            material_empty_layer.layer_factor_texture_matrix(trade.MaterialLayer.CLEAR_COAT)
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in layer 0"):
            material.layer_factor_texture_coordinates(0)
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in layer ClearCoat"):
            material_empty_layer.layer_factor_texture_coordinates("ClearCoat")
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in MaterialLayer.CLEAR_COAT"):
            material_empty_layer.layer_factor_texture_coordinates(trade.MaterialLayer.CLEAR_COAT)
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in layer 0"):
            material.layer_factor_texture_layer(0)
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in layer ClearCoat"):
            material_empty_layer.layer_factor_texture_layer("ClearCoat")
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE not found in MaterialLayer.CLEAR_COAT"):
            material_empty_layer.layer_factor_texture_layer(trade.MaterialLayer.CLEAR_COAT)

        with self.assertRaisesRegex(KeyError, "attribute LayerFactorTextureSwizzle not found in layer 1"):
            material.attribute_id(1, "LayerFactorTextureSwizzle")
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE not found in layer 1"):
            material.attribute_id(1, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE)
        with self.assertRaisesRegex(KeyError, "attribute LayerFactorTextureSwizzle not found in layer ClearCoat"):
            material.attribute_id("ClearCoat", "LayerFactorTextureSwizzle")
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE not found in layer ClearCoat"):
            material.attribute_id("ClearCoat", trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE)
        with self.assertRaisesRegex(KeyError, "attribute LayerFactorTextureSwizzle not found in MaterialLayer.CLEAR_COAT"):
            material.attribute_id(trade.MaterialLayer.CLEAR_COAT, "LayerFactorTextureSwizzle")
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE not found in MaterialLayer.CLEAR_COAT"):
            material.attribute_id(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE)
        with self.assertRaisesRegex(KeyError, "attribute DiffuseTexure not found in the base material"):
            material.attribute_id("DiffuseTexure")
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.DIFFUSE_TEXTURE not found in the base material"):
            material.attribute_id(trade.MaterialAttribute.DIFFUSE_TEXTURE)

        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 attributes in layer 1"):
            material.attribute_name(1, 8)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 attributes in layer ClearCoat"):
            material.attribute_name("ClearCoat", 8)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 attributes in MaterialLayer.CLEAR_COAT"):
            material.attribute_name(trade.MaterialLayer.CLEAR_COAT, 8)
        with self.assertRaisesRegex(IndexError, "index 3 out of range for 3 attributes in the base material"):
            material.attribute_name(3)

        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 attributes in layer 1"):
            material.attribute_type(1, 8)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 attributes in layer ClearCoat"):
            material.attribute_type("ClearCoat", 8)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 attributes in MaterialLayer.CLEAR_COAT"):
            material.attribute_type(trade.MaterialLayer.CLEAR_COAT, 8)
        with self.assertRaisesRegex(KeyError, "attribute LayerFactorTextureSwizzle not found in layer 1"):
            material.attribute_type(1, "LayerFactorTextureSwizzle")
        with self.assertRaisesRegex(KeyError, "attribute LayerFactorTextureSwizzle not found in layer ClearCoat"):
            material.attribute_type("ClearCoat", "LayerFactorTextureSwizzle")
        with self.assertRaisesRegex(KeyError, "attribute LayerFactorTextureSwizzle not found in MaterialLayer.CLEAR_COAT"):
            material.attribute_type(trade.MaterialLayer.CLEAR_COAT, "LayerFactorTextureSwizzle")
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE not found in layer 1"):
            material.attribute_type(1, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE)
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE not found in layer ClearCoat"):
            material.attribute_type("ClearCoat", trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE)
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE not found in MaterialLayer.CLEAR_COAT"):
            material.attribute_type(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE)
        with self.assertRaisesRegex(IndexError, "index 3 out of range for 3 attributes in the base material"):
            material.attribute_type(3)
        with self.assertRaisesRegex(KeyError, "attribute DiffuseTexure not found in the base material"):
            material.attribute_type("DiffuseTexure")
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.DIFFUSE_TEXTURE not found in the base material"):
            material.attribute_type(trade.MaterialAttribute.DIFFUSE_TEXTURE)

        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 attributes in layer 1"):
            material.attribute(1, 8)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 attributes in layer ClearCoat"):
            material.attribute("ClearCoat", 8)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 attributes in MaterialLayer.CLEAR_COAT"):
            material.attribute(trade.MaterialLayer.CLEAR_COAT, 8)
        with self.assertRaisesRegex(KeyError, "attribute LayerFactorTextureSwizzle not found in layer 1"):
            material.attribute(1, "LayerFactorTextureSwizzle")
        with self.assertRaisesRegex(KeyError, "attribute LayerFactorTextureSwizzle not found in layer ClearCoat"):
            material.attribute("ClearCoat", "LayerFactorTextureSwizzle")
        with self.assertRaisesRegex(KeyError, "attribute LayerFactorTextureSwizzle not found in MaterialLayer.CLEAR_COAT"):
            material.attribute(trade.MaterialLayer.CLEAR_COAT, "LayerFactorTextureSwizzle")
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE not found in layer 1"):
            material.attribute(1, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE)
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE not found in layer ClearCoat"):
            material.attribute("ClearCoat", trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE)
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE not found in MaterialLayer.CLEAR_COAT"):
            material.attribute(trade.MaterialLayer.CLEAR_COAT, trade.MaterialAttribute.LAYER_FACTOR_TEXTURE_SWIZZLE)
        with self.assertRaisesRegex(IndexError, "index 3 out of range for 3 attributes in the base material"):
            material.attribute(3)
        with self.assertRaisesRegex(KeyError, "attribute DiffuseTexure not found in the base material"):
            material.attribute("DiffuseTexure")
        with self.assertRaisesRegex(KeyError, "MaterialAttribute.DIFFUSE_TEXTURE not found in the base material"):
            material.attribute(trade.MaterialAttribute.DIFFUSE_TEXTURE)

class MeshAttributeData(unittest.TestCase):
    def test_init_1d(self):
        a = array.array('H', [3, 7, 16, 29998])
        a_refcount = sys.getrefcount(a)

        b = trade.MeshAttributeData(trade.MeshAttribute.OBJECT_ID, VertexFormat.UNSIGNED_SHORT, a)
        b_refcount = sys.getrefcount(b)
        self.assertEqual(b.name, trade.MeshAttribute.OBJECT_ID)
        self.assertEqual(b.format, VertexFormat.UNSIGNED_SHORT)
        self.assertEqual(b.array_size, 0)
        self.assertEqual(b.morph_target_id, -1)
        self.assertIs(b.owner, a)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        data = b.data
        self.assertEqual(data.size, (4, 1))
        self.assertEqual(data.stride, (2, 2))
        self.assertEqual(data.format, 'H')
        self.assertIs(data.owner, a)
        # Returns a 2D view always, transpose and take the first element to
        # "flatten" it.
        self.assertEqual(list(data.transposed(0, 1)[0]), [3, 7, 16, 29998])
        # The data reference the original array, not the MeshAttributeData
        # instance
        self.assertEqual(sys.getrefcount(b), b_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)

        del b
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        del data
        self.assertEqual(sys.getrefcount(a), a_refcount)

    def test_init_2d(self):
        a = array.array('f', [1.0, 0.0, 0.0,
                              0.0, -1.0, 0.0])
        a_refcount = sys.getrefcount(a)

        b = trade.MeshAttributeData(trade.MeshAttribute.NORMAL, VertexFormat.VECTOR3, containers.StridedArrayView1D(a).expanded(0, (2, 3)), morph_target_id=37)
        b_refcount = sys.getrefcount(b)
        self.assertEqual(b.name, trade.MeshAttribute.NORMAL)
        self.assertEqual(b.format, VertexFormat.VECTOR3)
        self.assertEqual(b.array_size, 0)
        self.assertEqual(b.morph_target_id, 37)
        self.assertIs(b.owner, a)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        data = b.data
        self.assertEqual(data.size, (2, 1))
        self.assertEqual(data.stride, (12, 12))
        self.assertEqual(data.format, '3f')
        self.assertIs(data.owner, a)
        # Returns a 2D view always, transpose and take the first element to
        # "flatten" it.
        self.assertEqual(list(data.transposed(0, 1)[0]), [(1.0, 0.0, 0.0), (0.0, -1.0, 0.0)])
        # The data reference the original array, not the MeshAttributeData
        # instance
        self.assertEqual(sys.getrefcount(b), b_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)

        del b
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        del data
        self.assertEqual(sys.getrefcount(a), a_refcount)

    def test_init_1d_array(self):
        data = array.array('Q', [0x0000ffff66663333, 0x00009999aaaacccc, 0x0000bbbb22227777, 0x00001111eeee8888])
        a = trade.MeshAttributeData(trade.MeshAttribute.JOINT_IDS, VertexFormat.UNSIGNED_SHORT, data, array_size=3)
        self.assertEqual(a.name, trade.MeshAttribute.JOINT_IDS)
        self.assertEqual(a.format, VertexFormat.UNSIGNED_SHORT)
        self.assertEqual(a.array_size, 3)
        self.assertEqual(a.morph_target_id, -1)

        data = a.data
        self.assertEqual(data.size, (4, 3))
        self.assertEqual(data.stride, (8, 2))
        self.assertEqual(data.format, 'H')
        # Getting all first, second and third array elements. Assumes Little
        # Endian.
        self.assertEqual(list(data.transposed(0, 1)[0]), [0x3333, 0xcccc, 0x7777, 0x8888])
        self.assertEqual(list(data.transposed(0, 1)[1]), [0x6666, 0xaaaa, 0x2222, 0xeeee])
        self.assertEqual(list(data.transposed(0, 1)[2]), [0xffff, 0x9999, 0xbbbb, 0x1111])

    def test_init_2d_array(self):
        data = array.array('H', [0x3333, 0x6666, 0xffff, 0xcccc, 0xaaaa, 0x9999, 0x7777, 0x2222, 0xbbbb, 0x8888, 0xeeee, 0x1111])
        a = trade.MeshAttributeData(trade.MeshAttribute.JOINT_IDS, VertexFormat.UNSIGNED_SHORT, containers.StridedArrayView1D(data).expanded(0, (4, 3)), array_size=3)
        self.assertEqual(a.name, trade.MeshAttribute.JOINT_IDS)
        self.assertEqual(a.format, VertexFormat.UNSIGNED_SHORT)
        self.assertEqual(a.array_size, 3)
        self.assertEqual(a.morph_target_id, -1)

        data = a.data
        self.assertEqual(data.size, (4, 3))
        self.assertEqual(data.stride, (6, 2))
        self.assertEqual(data.format, 'H')
        # Getting all first, second and third array elements
        self.assertEqual(list(data.transposed(0, 1)[0]), [0x3333, 0xcccc, 0x7777, 0x8888])
        self.assertEqual(list(data.transposed(0, 1)[1]), [0x6666, 0xaaaa, 0x2222, 0xeeee])
        self.assertEqual(list(data.transposed(0, 1)[2]), [0xffff, 0x9999, 0xbbbb, 0x1111])

    def test_init_1d_invalid(self):
        data = array.array('Q', [0, 0, 0])
        data_byte = array.array('B', [0, 0, 0])
        # To check that messages properly handle the case of no format string
        data_byte_no_format = containers.ArrayView(data_byte)
        self.assertEqual(containers.StridedArrayView1D(data_byte_no_format).format, None)

        with self.assertRaisesRegex(AssertionError, "VertexFormat.UNSIGNED_INT is not a valid format for MeshAttribute.POSITION"):
            trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.UNSIGNED_INT, data)
        with self.assertRaisesRegex(AssertionError, "data type Q has 8 bytes but VertexFormat.MATRIX3X3B_NORMALIZED expects at least 9"):
            trade.MeshAttributeData(trade.MeshAttribute.CUSTOM(57), VertexFormat.MATRIX3X3B_NORMALIZED, data)
        with self.assertRaisesRegex(AssertionError, "data type Q has 8 bytes but array of 3 VertexFormat.VECTOR3UB expects at least 9"):
            trade.MeshAttributeData(trade.MeshAttribute.CUSTOM(57), VertexFormat.VECTOR3UB, data, array_size=3)
        with self.assertRaisesRegex(AssertionError, "data type B has 1 bytes but VertexFormat.UNSIGNED_SHORT expects at least 2"):
            trade.MeshAttributeData(trade.MeshAttribute.OBJECT_ID, VertexFormat.UNSIGNED_SHORT, data_byte_no_format)
        with self.assertRaisesRegex(AssertionError, "expected vertex count to fit into 32 bits but got 4294967296"):
            trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.VECTOR3B, containers.StridedArrayView1D(data)[:1].broadcasted(0, 0x100000000))
        with self.assertRaisesRegex(AssertionError, "expected stride to fit into 16 bits but got 32768"):
            trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.VECTOR3B, containers.StridedArrayView1D(data)[::4096])
        with self.assertRaisesRegex(AssertionError, "expected stride to fit into 16 bits but got -32769"):
            trade.MeshAttributeData(trade.MeshAttribute.OBJECT_ID, VertexFormat.UNSIGNED_BYTE, containers.StridedArrayView1D(data_byte)[::32769].flipped(0))
        with self.assertRaisesRegex(AssertionError, "MeshAttribute.POSITION can't be an array attribute"):
            trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.VECTOR3B, data, array_size=2)
        with self.assertRaisesRegex(AssertionError, "MeshAttribute.JOINT_IDS has to be an array attribute"):
            trade.MeshAttributeData(trade.MeshAttribute.JOINT_IDS, VertexFormat.UNSIGNED_INT, data)
        with self.assertRaisesRegex(AssertionError, "expected morph target ID to be either -1 or less than 128 but got -2"):
            trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.VECTOR3B, data, morph_target_id=-2)
        with self.assertRaisesRegex(AssertionError, "expected morph target ID to be either -1 or less than 128 but got 128"):
            trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.VECTOR3B, data, morph_target_id=128)
        with self.assertRaisesRegex(AssertionError, "morph target not allowed for MeshAttribute.OBJECT_ID"):
            trade.MeshAttributeData(trade.MeshAttribute.OBJECT_ID, VertexFormat.UNSIGNED_INT, data, morph_target_id=3)

    def test_init_2d_invalid(self):
        data = containers.StridedArrayView1D(array.array('I', [0, 0, 0, 0, 0, 0])).expanded(0, (3, 2))
        data_byte = containers.StridedArrayView1D(array.array('B', [0, 0, 0])).expanded(0, (3, 1))
        # To check that messages properly handle the case of no format string
        data_byte_no_format = containers.StridedArrayView1D(containers.ArrayView(array.array('B', [0, 0, 0]))).expanded(0, (3, 1))
        self.assertEqual(data_byte_no_format.format, None)

        with self.assertRaisesRegex(AssertionError, "VertexFormat.UNSIGNED_INT is not a valid format for MeshAttribute.POSITION"):
            trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.UNSIGNED_INT, data)
        with self.assertRaisesRegex(AssertionError, "2-item second dimension of data type I has 8 bytes but VertexFormat.MATRIX3X3B_NORMALIZED expects at least 9"):
            trade.MeshAttributeData(trade.MeshAttribute.CUSTOM(57), VertexFormat.MATRIX3X3B_NORMALIZED, data)
        with self.assertRaisesRegex(AssertionError, "2-item second dimension of data type I has 8 bytes but array of 3 VertexFormat.VECTOR3UB expects at least 9"):
            trade.MeshAttributeData(trade.MeshAttribute.CUSTOM(57), VertexFormat.VECTOR3UB, data, array_size=3)
        with self.assertRaisesRegex(AssertionError, "1-item second dimension of data type B has 1 bytes but VertexFormat.UNSIGNED_SHORT expects at least 2"):
            trade.MeshAttributeData(trade.MeshAttribute.OBJECT_ID, VertexFormat.UNSIGNED_SHORT, data_byte_no_format)
        with self.assertRaisesRegex(AssertionError, "second view dimension is not contiguous"):
            trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.VECTOR3UB, data[::1,::2])
        with self.assertRaisesRegex(AssertionError, "expected vertex count to fit into 32 bits but got 4294967296"):
            trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.VECTOR3B, data[:1].broadcasted(0, 0x100000000))
        with self.assertRaisesRegex(AssertionError, "expected stride to fit into 16 bits but got 32768"):
            trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.VECTOR3B, data[::4096])
        with self.assertRaisesRegex(AssertionError, "expected stride to fit into 16 bits but got -32769"):
            trade.MeshAttributeData(trade.MeshAttribute.OBJECT_ID, VertexFormat.UNSIGNED_BYTE, data_byte[::32769].flipped(0))
        with self.assertRaisesRegex(AssertionError, "MeshAttribute.POSITION can't be an array attribute"):
            trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.VECTOR3B, data, array_size=2)
        with self.assertRaisesRegex(AssertionError, "MeshAttribute.JOINT_IDS has to be an array attribute"):
            trade.MeshAttributeData(trade.MeshAttribute.JOINT_IDS, VertexFormat.UNSIGNED_INT, data)
        with self.assertRaisesRegex(AssertionError, "expected morph target ID to be either -1 or less than 128 but got -2"):
            trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.VECTOR3B, data, morph_target_id=-2)
        with self.assertRaisesRegex(AssertionError, "expected morph target ID to be either -1 or less than 128 but got 128"):
            trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.VECTOR3B, data, morph_target_id=128)
        with self.assertRaisesRegex(AssertionError, "morph target not allowed for MeshAttribute.OBJECT_ID"):
            trade.MeshAttributeData(trade.MeshAttribute.OBJECT_ID, VertexFormat.UNSIGNED_INT, data, morph_target_id=3)

    def test_data_access_unsupported_format(self):
        data = array.array('Q', [0, 0, 0])
        a = trade.MeshAttributeData(trade.MeshAttribute.CUSTOM(57), VertexFormat.MATRIX3X2B_NORMALIZED, data)

        with self.assertRaisesRegex(NotImplementedError, "access to VertexFormat.MATRIX3X2B_NORMALIZED is not implemented yet, sorry"):
            a.data

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

    def test_init_empty(self):
        mesh = trade.MeshData(MeshPrimitive.TRIANGLES, 21)
        self.assertIs(mesh.owner, None)
        self.assertFalse(mesh.is_indexed)
        self.assertEqual(mesh.primitive, MeshPrimitive.TRIANGLES)
        self.assertEqual(mesh.vertex_count, 21)
        self.assertEqual(mesh.attribute_count(), 0)

    def test(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        # This adds extra attributes for joints and weights, don't want
        importer.configuration['compatibilitySkinningAttributes'] = False
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)
        self.assertEqual(mesh.primitive, MeshPrimitive.TRIANGLES)
        self.assertEqual(mesh.index_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)
        self.assertEqual(mesh.vertex_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)
        self.assertIsNone(mesh.owner)

        # Index properties
        self.assertTrue(mesh.is_indexed)
        self.assertEqual(mesh.index_count, 3)
        self.assertEqual(mesh.index_type, MeshIndexType.UNSIGNED_SHORT)
        self.assertEqual(mesh.index_offset, 0)
        self.assertEqual(mesh.index_stride, 2)

        self.assertEqual(mesh.vertex_count, 3)
        self.assertEqual(mesh.attribute_count(), 9)
        self.assertEqual(mesh.attribute_count(morph_target_id=37), 0)

        # Attribute properties by ID
        self.assertEqual(mesh.attribute_name(2), trade.MeshAttribute.POSITION)
        # Custom attribute. On deprecated builds there are extra backwards
        # compatibility JOINTS and WEIGHTS attributes.
        if magnum.BUILD_DEPRECATED:
            self.assertEqual(mesh.attribute_name(6), trade.MeshAttribute.CUSTOM(10))
        else:
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
        self.assertFalse(mesh.has_attribute(trade.MeshAttribute.POSITION, morph_target_id=37))
        self.assertEqual(mesh.attribute_count(trade.MeshAttribute.POSITION), 1)
        self.assertEqual(mesh.attribute_count(trade.MeshAttribute.TEXTURE_COORDINATES), 2)
        self.assertEqual(mesh.attribute_count(trade.MeshAttribute.TANGENT), 0)
        self.assertEqual(mesh.attribute_id(trade.MeshAttribute.POSITION), 2)
        self.assertEqual(mesh.attribute_id(trade.MeshAttribute.TEXTURE_COORDINATES, id=1), 4)
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
        self.assertEqual(index_data[2], 2)
        self.assertEqual(mutable_index_data[2], 2)

        mutable_index_data[2] = 76
        self.assertEqual(index_data[2], 76)

    def test_mutable_vertex_data_access(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh(0)
        self.assertEqual(mesh.vertex_data_flags, trade.DataFlags.OWNED|trade.DataFlags.MUTABLE)

        vertex_data = mesh.vertex_data
        mutable_vertex_data = mesh.mutable_vertex_data
        # The color attribute is at offset 20, G channel is the next byte
        self.assertEqual(vertex_data[21], 51)
        self.assertEqual(mutable_vertex_data[21], 51)

        mutable_vertex_data[21] = 76
        self.assertEqual(vertex_data[21], 76)

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
        self.assertEqual(mesh.index_data_flags, trade.DataFlags.GLOBAL)
        self.assertEqual(mesh.vertex_data_flags, trade.DataFlags.GLOBAL)

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

        # Access by OOB ID. Deprecated build contains additional 2 backwards
        # compatibility skinning attributes.
        if magnum.BUILD_DEPRECATED:
            indexOutOfRangeMessage = "index 11 out of range for 11 attributes"
        else:
            indexOutOfRangeMessage = "index 9 out of range for 9 attributes"
        with self.assertRaisesRegex(IndexError, indexOutOfRangeMessage):
            mesh.attribute_name(mesh.attribute_count())
        with self.assertRaisesRegex(IndexError, indexOutOfRangeMessage):
            mesh.attribute_id(mesh.attribute_count())
        with self.assertRaisesRegex(IndexError, indexOutOfRangeMessage):
            mesh.attribute_format(mesh.attribute_count())
        with self.assertRaisesRegex(IndexError, indexOutOfRangeMessage):
            mesh.attribute_offset(mesh.attribute_count())
        with self.assertRaisesRegex(IndexError, indexOutOfRangeMessage):
            mesh.attribute_stride(mesh.attribute_count())
        with self.assertRaisesRegex(IndexError, indexOutOfRangeMessage):
            mesh.attribute_array_size(mesh.attribute_count())
        with self.assertRaisesRegex(IndexError, indexOutOfRangeMessage):
            mesh.attribute(mesh.attribute_count())
        with self.assertRaisesRegex(IndexError, indexOutOfRangeMessage):
            mesh.mutable_attribute(mesh.attribute_count())

        # Access by nonexistent name
        with self.assertRaisesRegex(KeyError, "index 0 out of range for 0 MeshAttribute.TANGENT attributes"):
            mesh.attribute_id(trade.MeshAttribute.TANGENT)
        with self.assertRaisesRegex(KeyError, "index 0 out of range for 0 MeshAttribute.TANGENT attributes"):
            mesh.attribute_format(trade.MeshAttribute.TANGENT)
        with self.assertRaisesRegex(KeyError, "index 0 out of range for 0 MeshAttribute.TANGENT attributes"):
            mesh.attribute_offset(trade.MeshAttribute.TANGENT)
        with self.assertRaisesRegex(KeyError, "index 0 out of range for 0 MeshAttribute.TANGENT attributes"):
            mesh.attribute_stride(trade.MeshAttribute.TANGENT)
        with self.assertRaisesRegex(KeyError, "index 0 out of range for 0 MeshAttribute.TANGENT attributes"):
            mesh.attribute_array_size(trade.MeshAttribute.TANGENT)
        with self.assertRaisesRegex(KeyError, "index 0 out of range for 0 MeshAttribute.TANGENT attributes"):
            mesh.attribute(trade.MeshAttribute.TANGENT)
        with self.assertRaisesRegex(KeyError, "index 0 out of range for 0 MeshAttribute.TANGENT attributes"):
            mesh.mutable_attribute(trade.MeshAttribute.TANGENT)

        # Access by existing name + OOB ID
        with self.assertRaisesRegex(KeyError, "index 2 out of range for 2 MeshAttribute.TEXTURE_COORDINATES attributes"):
            mesh.attribute_id(trade.MeshAttribute.TEXTURE_COORDINATES, id=2)
        with self.assertRaisesRegex(KeyError, "index 2 out of range for 2 MeshAttribute.TEXTURE_COORDINATES attributes"):
            mesh.attribute_format(trade.MeshAttribute.TEXTURE_COORDINATES, id=2)
        with self.assertRaisesRegex(KeyError, "index 2 out of range for 2 MeshAttribute.TEXTURE_COORDINATES attributes"):
            mesh.attribute_offset(trade.MeshAttribute.TEXTURE_COORDINATES, id=2)
        with self.assertRaisesRegex(KeyError, "index 2 out of range for 2 MeshAttribute.TEXTURE_COORDINATES attributes"):
            mesh.attribute_stride(trade.MeshAttribute.TEXTURE_COORDINATES, id=2)
        with self.assertRaisesRegex(KeyError, "index 2 out of range for 2 MeshAttribute.TEXTURE_COORDINATES attributes"):
            mesh.attribute_array_size(trade.MeshAttribute.TEXTURE_COORDINATES, id=2)
        with self.assertRaisesRegex(KeyError, "index 2 out of range for 2 MeshAttribute.TEXTURE_COORDINATES attributes"):
            mesh.attribute(trade.MeshAttribute.TEXTURE_COORDINATES, id=2)
        with self.assertRaisesRegex(KeyError, "index 2 out of range for 2 MeshAttribute.TEXTURE_COORDINATES attributes"):
            mesh.mutable_attribute(trade.MeshAttribute.TEXTURE_COORDINATES, id=2)

        # Access by existing name + OOB morph target ID
        with self.assertRaisesRegex(KeyError, "index 0 out of range for 0 MeshAttribute.TEXTURE_COORDINATES attributes in morph target 37"):
            mesh.attribute_id(trade.MeshAttribute.TEXTURE_COORDINATES, morph_target_id=37)
        with self.assertRaisesRegex(KeyError, "index 0 out of range for 0 MeshAttribute.TEXTURE_COORDINATES attributes in morph target 37"):
            mesh.attribute_format(trade.MeshAttribute.TEXTURE_COORDINATES, morph_target_id=37)
        with self.assertRaisesRegex(KeyError, "index 0 out of range for 0 MeshAttribute.TEXTURE_COORDINATES attributes in morph target 37"):
            mesh.attribute_offset(trade.MeshAttribute.TEXTURE_COORDINATES, morph_target_id=37)
        with self.assertRaisesRegex(KeyError, "index 0 out of range for 0 MeshAttribute.TEXTURE_COORDINATES attributes in morph target 37"):
            mesh.attribute_stride(trade.MeshAttribute.TEXTURE_COORDINATES, morph_target_id=37)
        with self.assertRaisesRegex(KeyError, "index 0 out of range for 0 MeshAttribute.TEXTURE_COORDINATES attributes in morph target 37"):
            mesh.attribute_array_size(trade.MeshAttribute.TEXTURE_COORDINATES, morph_target_id=37)
        with self.assertRaisesRegex(KeyError, "index 0 out of range for 0 MeshAttribute.TEXTURE_COORDINATES attributes in morph target 37"):
            mesh.attribute(trade.MeshAttribute.TEXTURE_COORDINATES, morph_target_id=37)
        with self.assertRaisesRegex(KeyError, "index 0 out of range for 0 MeshAttribute.TEXTURE_COORDINATES attributes in morph target 37"):
            mesh.mutable_attribute(trade.MeshAttribute.TEXTURE_COORDINATES, morph_target_id=37)

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

        with self.assertRaisesRegex(NotImplementedError, "access to VertexFormat.MATRIX2X2 is not implemented yet, sorry"):
            mesh.attribute(custom_attribute_id)
        with self.assertRaisesRegex(NotImplementedError, "access to VertexFormat.MATRIX2X2 is not implemented yet, sorry"):
            mesh.mutable_attribute(custom_attribute_id)
        with self.assertRaisesRegex(NotImplementedError, "access to VertexFormat.MATRIX2X2 is not implemented yet, sorry"):
            mesh.attribute(custom_attribute)
        with self.assertRaisesRegex(NotImplementedError, "access to VertexFormat.MATRIX2X2 is not implemented yet, sorry"):
            mesh.mutable_attribute(custom_attribute)

class SceneFieldData(unittest.TestCase):
    def test_init_1d(self):
        a = array.array('Q', [3, 7, 166, 2872])
        b = array.array('h', [2, -1, 37, -1])
        a_refcount = sys.getrefcount(a)
        b_refcount = sys.getrefcount(b)

        c = trade.SceneFieldData(trade.SceneField.MESH_MATERIAL,
            trade.SceneMappingType.UNSIGNED_LONG, a,
            trade.SceneFieldType.SHORT, b,
            flags=trade.SceneFieldFlags.MULTI_ENTRY|trade.SceneFieldFlags.ORDERED_MAPPING)
        c_refcount = sys.getrefcount(c)
        self.assertEqual(c.flags, trade.SceneFieldFlags.MULTI_ENTRY|trade.SceneFieldFlags.ORDERED_MAPPING)
        self.assertEqual(c.name, trade.SceneField.MESH_MATERIAL)
        self.assertEqual(c.size, 4)
        self.assertEqual(c.mapping_type, trade.SceneMappingType.UNSIGNED_LONG)
        self.assertEqual(c.field_type, trade.SceneFieldType.SHORT)
        self.assertEqual(c.field_array_size, 0)
        self.assertIs(c.mapping_owner, a)
        self.assertIs(c.field_owner, b)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)

        mapping_data = c.mapping_data
        self.assertEqual(mapping_data.size, (4,))
        self.assertEqual(mapping_data.stride, (8,))
        self.assertEqual(mapping_data.format, 'Q')
        self.assertIs(mapping_data.owner, a)
        self.assertEqual(list(mapping_data), [3, 7, 166, 2872])

        # The data reference the original array, not the SceneFieldData
        # instance
        self.assertEqual(sys.getrefcount(c), c_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)

        field_data = c.field_data
        self.assertEqual(field_data.size, (4, 1))
        self.assertEqual(field_data.stride, (2, 2))
        self.assertEqual(field_data.format, 'h')
        self.assertIs(field_data.owner, b)
        # Returns a 2D view always, transpose and take the first element to
        # "flatten" it.
        self.assertEqual(list(field_data.transposed(0, 1)[0]), [2, -1, 37, -1])

        # The data reference the original array, not the SceneFieldData
        # instance
        self.assertEqual(sys.getrefcount(c), c_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)
        self.assertEqual(sys.getrefcount(b), b_refcount + 2)

        del c
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)

        del mapping_data
        del field_data
        self.assertEqual(sys.getrefcount(a), a_refcount)
        self.assertEqual(sys.getrefcount(b), b_refcount)

    def test_init_2d(self):
        a = array.array('I', [0, 1])
        b = array.array('f', [1.0, 0.0, 0.0,
                              0.0, -1.0, 0.0])
        a_refcount = sys.getrefcount(a)
        b_refcount = sys.getrefcount(b)

        c = trade.SceneFieldData(trade.SceneField.TRANSLATION,
            trade.SceneMappingType.UNSIGNED_INT, a,
            trade.SceneFieldType.VECTOR3, containers.StridedArrayView1D(b).expanded(0, (2, 3)),
            flags=trade.SceneFieldFlags.IMPLICIT_MAPPING)
        c_refcount = sys.getrefcount(c)
        self.assertEqual(c.flags, trade.SceneFieldFlags.IMPLICIT_MAPPING)
        self.assertEqual(c.name, trade.SceneField.TRANSLATION)
        self.assertEqual(c.size, 2)
        self.assertEqual(c.mapping_type, trade.SceneMappingType.UNSIGNED_INT)
        self.assertEqual(c.field_type, trade.SceneFieldType.VECTOR3)
        self.assertEqual(c.field_array_size, 0)
        self.assertIs(c.mapping_owner, a)
        self.assertIs(c.field_owner, b)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)

        mapping_data = c.mapping_data
        self.assertEqual(mapping_data.size, (2,))
        self.assertEqual(mapping_data.stride, (4,))
        self.assertEqual(mapping_data.format, 'I')
        self.assertIs(mapping_data.owner, a)
        self.assertEqual(list(mapping_data), [0, 1])

        # The data reference the original array, not the SceneFieldData
        # instance
        self.assertEqual(sys.getrefcount(c), c_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)

        field_data = c.field_data
        self.assertEqual(field_data.size, (2, 1))
        self.assertEqual(field_data.stride, (12, 12))
        self.assertEqual(field_data.format, '3f')
        self.assertIs(field_data.owner, b)
        # Returns a 2D view always, transpose and take the first element to
        # "flatten" it.
        self.assertEqual(list(field_data.transposed(0, 1)[0]), [(1.0, 0.0, 0.0), (0.0, -1.0, 0.0)])

        # The data reference the original array, not the SceneFieldData
        # instance
        self.assertEqual(sys.getrefcount(c), c_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)
        self.assertEqual(sys.getrefcount(b), b_refcount + 2)

        del c
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)

        del mapping_data
        del field_data
        self.assertEqual(sys.getrefcount(a), a_refcount)
        self.assertEqual(sys.getrefcount(b), b_refcount)

    def test_init_1d_array(self):
        a = array.array('B', [3, 7, 166, 72])
        b = array.array('Q', [0x0000ffff66663333, 0x00009999aaaacccc, 0x0000bbbb22227777, 0x00001111eeee8888])
        c = trade.SceneFieldData(trade.SceneField.CUSTOM(666),
            trade.SceneMappingType.UNSIGNED_BYTE, a,
            trade.SceneFieldType.UNSIGNED_SHORT, b,
            field_array_size=3)
        self.assertEqual(c.flags, trade.SceneFieldFlags.NONE)
        self.assertEqual(c.name, trade.SceneField.CUSTOM(666))
        self.assertEqual(c.size, 4)
        self.assertEqual(c.mapping_type, trade.SceneMappingType.UNSIGNED_BYTE)
        self.assertEqual(c.field_type, trade.SceneFieldType.UNSIGNED_SHORT)
        self.assertEqual(c.field_array_size, 3)

        mapping_data = c.mapping_data
        self.assertEqual(mapping_data.size, (4,))
        self.assertEqual(mapping_data.stride, (1,))
        self.assertEqual(mapping_data.format, 'B')
        self.assertEqual(list(mapping_data), [3, 7, 166, 72])

        field_data = c.field_data
        self.assertEqual(field_data.size, (4, 3))
        self.assertEqual(field_data.stride, (8, 2))
        self.assertEqual(field_data.format, 'H')
        # Getting all first, second and third array elements. Assumes Little
        # Endian.
        self.assertEqual(list(field_data.transposed(0, 1)[0]), [0x3333, 0xcccc, 0x7777, 0x8888])
        self.assertEqual(list(field_data.transposed(0, 1)[1]), [0x6666, 0xaaaa, 0x2222, 0xeeee])
        self.assertEqual(list(field_data.transposed(0, 1)[2]), [0xffff, 0x9999, 0xbbbb, 0x1111])

    def test_init_2d_array(self):
        a = array.array('B', [3, 7, 166, 72])
        b = array.array('H', [0x3333, 0x6666, 0xffff, 0xcccc, 0xaaaa, 0x9999, 0x7777, 0x2222, 0xbbbb, 0x8888, 0xeeee, 0x1111])
        c = trade.SceneFieldData(trade.SceneField.CUSTOM(666),
            trade.SceneMappingType.UNSIGNED_BYTE, a,
            trade.SceneFieldType.UNSIGNED_SHORT, containers.StridedArrayView1D(b).expanded(0, (4, 3)),
            field_array_size=3)
        self.assertEqual(c.flags, trade.SceneFieldFlags.NONE)
        self.assertEqual(c.name, trade.SceneField.CUSTOM(666))
        self.assertEqual(c.size, 4)
        self.assertEqual(c.mapping_type, trade.SceneMappingType.UNSIGNED_BYTE)
        self.assertEqual(c.field_type, trade.SceneFieldType.UNSIGNED_SHORT)
        self.assertEqual(c.field_array_size, 3)

        mapping_data = c.mapping_data
        self.assertEqual(mapping_data.size, (4,))
        self.assertEqual(mapping_data.stride, (1,))
        self.assertEqual(mapping_data.format, 'B')
        self.assertEqual(list(mapping_data), [3, 7, 166, 72])

        field_data = c.field_data
        self.assertEqual(field_data.size, (4, 3))
        self.assertEqual(field_data.stride, (6, 2))
        self.assertEqual(field_data.format, 'H')
        # Getting all first, second and third array elements. Assumes Little
        # Endian.
        self.assertEqual(list(field_data.transposed(0, 1)[0]), [0x3333, 0xcccc, 0x7777, 0x8888])
        self.assertEqual(list(field_data.transposed(0, 1)[1]), [0x6666, 0xaaaa, 0x2222, 0xeeee])
        self.assertEqual(list(field_data.transposed(0, 1)[2]), [0xffff, 0x9999, 0xbbbb, 0x1111])

    def test_init_bit_1d(self):
        a = array.array('H', [3, 7, 166, 2872])
        b = array.array('b', [1, 0, 1, 0])
        a_refcount = sys.getrefcount(a)
        b_refcount = sys.getrefcount(b)

        c = trade.SceneFieldData(trade.SceneField.CUSTOM(1337),
            trade.SceneMappingType.UNSIGNED_SHORT, a,
            containers.StridedArrayView1D(b).slice_bit(0),
            flags=trade.SceneFieldFlags.MULTI_ENTRY|trade.SceneFieldFlags.ORDERED_MAPPING)
        c_refcount = sys.getrefcount(c)
        self.assertEqual(c.flags, trade.SceneFieldFlags.MULTI_ENTRY|trade.SceneFieldFlags.ORDERED_MAPPING)
        self.assertEqual(c.name, trade.SceneField.CUSTOM(1337))
        self.assertEqual(c.size, 4)
        self.assertEqual(c.mapping_type, trade.SceneMappingType.UNSIGNED_SHORT)
        self.assertEqual(c.field_type, trade.SceneFieldType.BIT)
        self.assertEqual(c.field_array_size, 0)
        self.assertIs(c.mapping_owner, a)
        self.assertIs(c.field_owner, b)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)

        mapping_data = c.mapping_data
        self.assertEqual(mapping_data.size, (4,))
        self.assertEqual(mapping_data.stride, (2,))
        self.assertEqual(mapping_data.format, 'H')
        self.assertIs(mapping_data.owner, a)
        self.assertEqual(list(mapping_data), [3, 7, 166, 2872])

        # The data reference the original array, not the SceneFieldData
        # instance
        self.assertEqual(sys.getrefcount(c), c_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)

        field_data = c.field_data
        self.assertEqual(field_data.size, (4, 1))
        self.assertEqual(field_data.offset, 0)
        self.assertEqual(field_data.stride, (8, 1))
        self.assertIs(field_data.owner, b)
        # Returns a 2D view always, transpose and take the first element to
        # "flatten" it.
        self.assertEqual(list(field_data.transposed(0, 1)[0]), [True, False, True, False])

        # The data reference the original array, not the SceneFieldData
        # instance
        self.assertEqual(sys.getrefcount(c), c_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)
        self.assertEqual(sys.getrefcount(b), b_refcount + 2)

        del c
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)

        del mapping_data
        del field_data
        self.assertEqual(sys.getrefcount(a), a_refcount)
        self.assertEqual(sys.getrefcount(b), b_refcount)

    def test_init_bit_2d(self):
        a = array.array('H', [3, 7, 166, 2872])
        b = containers.BitArray.value_init(4*2)
        b[0] = True
        b[1] = True
        b[4] = True
        b[7] = True
        a_refcount = sys.getrefcount(a)
        b_refcount = sys.getrefcount(b)

        c = trade.SceneFieldData(trade.SceneField.CUSTOM(1337),
            trade.SceneMappingType.UNSIGNED_SHORT, a,
            containers.StridedBitArrayView1D(b).expanded(0, (4, 2)),
            flags=trade.SceneFieldFlags.MULTI_ENTRY|trade.SceneFieldFlags.ORDERED_MAPPING)
        c_refcount = sys.getrefcount(c)
        self.assertEqual(c.flags, trade.SceneFieldFlags.MULTI_ENTRY|trade.SceneFieldFlags.ORDERED_MAPPING)
        self.assertEqual(c.name, trade.SceneField.CUSTOM(1337))
        self.assertEqual(c.size, 4)
        self.assertEqual(c.mapping_type, trade.SceneMappingType.UNSIGNED_SHORT)
        self.assertEqual(c.field_type, trade.SceneFieldType.BIT)
        self.assertEqual(c.field_array_size, 2)
        self.assertIs(c.mapping_owner, a)
        self.assertIs(c.field_owner, b)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)

        mapping_data = c.mapping_data
        self.assertEqual(mapping_data.size, (4,))
        self.assertEqual(mapping_data.stride, (2,))
        self.assertEqual(mapping_data.format, 'H')
        self.assertIs(mapping_data.owner, a)
        self.assertEqual(list(mapping_data), [3, 7, 166, 2872])

        # The data reference the original array, not the SceneFieldData
        # instance
        self.assertEqual(sys.getrefcount(c), c_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)

        field_data = c.field_data
        self.assertEqual(field_data.size, (4, 2))
        self.assertEqual(field_data.offset, 0)
        self.assertEqual(field_data.stride, (2, 1))
        self.assertIs(field_data.owner, b)
        # Getting all first and second array elements
        self.assertEqual(list(field_data.transposed(0, 1)[0]), [True, False, True, False])
        self.assertEqual(list(field_data.transposed(0, 1)[1]), [True, False, False, True])

        # The data reference the original array, not the SceneFieldData
        # instance
        self.assertEqual(sys.getrefcount(c), c_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)
        self.assertEqual(sys.getrefcount(b), b_refcount + 2)

        del c
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)

        del mapping_data
        del field_data
        self.assertEqual(sys.getrefcount(a), a_refcount)
        self.assertEqual(sys.getrefcount(b), b_refcount)

    def test_init_1d_invalid(self):
        data = array.array('Q', [0, 0, 0])
        data_byte = array.array('B', [0, 0, 0])
        # To check that messages properly handle the case of no format string
        data_byte_no_format = containers.ArrayView(data_byte)
        self.assertEqual(containers.StridedArrayView1D(data_byte_no_format).format, None)

        with self.assertRaisesRegex(AssertionError, "expected SceneField.TRANSLATION mapping and field view to have the same size but got 2 and 3"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_SHORT, data[:2],
                trade.SceneFieldType.VECTOR2, data)
        with self.assertRaisesRegex(AssertionError, "SceneFieldType.UNSIGNED_SHORT is not a valid type for SceneField.TRANSLATION"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_LONG, data,
                trade.SceneFieldType.UNSIGNED_SHORT, data)
        with self.assertRaisesRegex(AssertionError, "data type B has 1 bytes but SceneMappingType.UNSIGNED_SHORT expects at least 2"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_SHORT, data_byte,
                trade.SceneFieldType.VECTOR2, data)
        with self.assertRaisesRegex(AssertionError, "data type B has 1 bytes but SceneMappingType.UNSIGNED_SHORT expects at least 2"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_SHORT, data_byte_no_format,
                trade.SceneFieldType.VECTOR2, data)
        with self.assertRaisesRegex(AssertionError, "data type Q has 8 bytes but SceneFieldType.VECTOR3 expects at least 12"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_SHORT, data,
                trade.SceneFieldType.VECTOR3, data)
        with self.assertRaisesRegex(AssertionError, "data type Q has 8 bytes but array of 3 SceneFieldType.FLOAT expects at least 12"):
            trade.SceneFieldData(trade.SceneField.CUSTOM(76),
                trade.SceneMappingType.UNSIGNED_SHORT, data,
                trade.SceneFieldType.FLOAT, data, field_array_size=3)
        with self.assertRaisesRegex(AssertionError, "data type B has 1 bytes but SceneFieldType.SHORT expects at least 2"):
            trade.SceneFieldData(trade.SceneField.MESH_MATERIAL,
                trade.SceneMappingType.UNSIGNED_INT, data,
                trade.SceneFieldType.SHORT, data_byte_no_format)
        with self.assertRaisesRegex(AssertionError, "expected mapping view stride to fit into 16 bits but got 32768"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_SHORT, containers.StridedArrayView1D(data)[::4096],
                trade.SceneFieldType.VECTOR2, data[:1])
        with self.assertRaisesRegex(AssertionError, "expected mapping view stride to fit into 16 bits but got -32769"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_BYTE, containers.StridedArrayView1D(data_byte)[::32769].flipped(0),
                trade.SceneFieldType.VECTOR2, data[:1])
        with self.assertRaisesRegex(AssertionError, "expected field view stride to fit into 16 bits but got 32768"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_SHORT, data[:1],
                trade.SceneFieldType.VECTOR2, containers.StridedArrayView1D(data)[::4096])
        with self.assertRaisesRegex(AssertionError, "expected field view stride to fit into 16 bits but got -32769"):
            trade.SceneFieldData(trade.SceneField.CAMERA,
                trade.SceneMappingType.UNSIGNED_SHORT, data[:1],
                trade.SceneFieldType.UNSIGNED_BYTE, containers.StridedArrayView1D(data_byte)[::32769].flipped(0))
        with self.assertRaisesRegex(AssertionError, "SceneField.MESH can't be an array field"):
            trade.SceneFieldData(trade.SceneField.MESH,
                trade.SceneMappingType.UNSIGNED_SHORT, data,
                trade.SceneFieldType.UNSIGNED_SHORT, data, field_array_size=3)
        with self.assertRaisesRegex(AssertionError, "can't pass SceneFieldFlags.MULTI_ENTRY for a SceneField.TRANSLATION view of SceneFieldType.VECTOR2"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_LONG, data,
                trade.SceneFieldType.VECTOR2, data, flags=trade.SceneFieldFlags.MULTI_ENTRY)
        with self.assertRaisesRegex(AssertionError, "use a string constructor for SceneFieldType.STRING_OFFSET16"):
            trade.SceneFieldData(trade.SceneField.CUSTOM(333),
                trade.SceneMappingType.UNSIGNED_LONG, data,
                trade.SceneFieldType.STRING_OFFSET16, data)
        with self.assertRaisesRegex(AssertionError, "use a bit constructor for SceneFieldType.BIT"):
            trade.SceneFieldData(trade.SceneField.CUSTOM(333),
                trade.SceneMappingType.UNSIGNED_LONG, data,
                trade.SceneFieldType.BIT, data)

    def test_init_2d_invalid(self):
        data_1d = array.array('Q', [0, 0, 0])
        data = containers.StridedArrayView1D(array.array('I', [0, 0, 0, 0, 0, 0])).expanded(0, (3, 2))
        data_byte = containers.StridedArrayView1D(array.array('B', [0, 0, 0])).expanded(0, (3, 1))
        # To check that messages properly handle the case of no format string
        data_byte_no_format = containers.StridedArrayView1D(containers.ArrayView(array.array('B', [0, 0, 0]))).expanded(0, (3, 1))
        self.assertEqual(data_byte_no_format.format, None)

        with self.assertRaisesRegex(AssertionError, "expected SceneField.TRANSLATION mapping and field view to have the same size but got 2 and 3"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_SHORT, data_1d[:2],
                trade.SceneFieldType.VECTOR2, data)
        with self.assertRaisesRegex(AssertionError, "SceneFieldType.UNSIGNED_SHORT is not a valid type for SceneField.TRANSLATION"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_LONG, data_1d,
                trade.SceneFieldType.UNSIGNED_SHORT, data)
        # SceneMappingType size checks are shared with the 1D variant, not
        # testing here again
        with self.assertRaisesRegex(AssertionError, "2-item second dimension of data type I has 8 bytes but SceneFieldType.VECTOR3 expects at least 12"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_SHORT, data_1d,
                trade.SceneFieldType.VECTOR3, data)
        with self.assertRaisesRegex(AssertionError, "2-item second dimension of data type I has 8 bytes but array of 3 SceneFieldType.FLOAT expects at least 12"):
            trade.SceneFieldData(trade.SceneField.CUSTOM(76),
                trade.SceneMappingType.UNSIGNED_SHORT, data_1d,
                trade.SceneFieldType.FLOAT, data, field_array_size=3)
        with self.assertRaisesRegex(AssertionError, "1-item second dimension of data type B has 1 bytes but SceneFieldType.SHORT expects at least 2"):
            trade.SceneFieldData(trade.SceneField.MESH_MATERIAL,
                trade.SceneMappingType.UNSIGNED_INT, data_1d,
                trade.SceneFieldType.SHORT, data_byte_no_format)
        with self.assertRaisesRegex(AssertionError, "second field view dimension is not contiguous"):
            trade.SceneFieldData(trade.SceneField.MESH,
                trade.SceneMappingType.UNSIGNED_SHORT, data_1d,
                trade.SceneFieldType.UNSIGNED_SHORT, data[::1,::2])
        # SceneMappingType stride checks are shared with the 1D variant, not
        # testing here again
        with self.assertRaisesRegex(AssertionError, "expected field view stride to fit into 16 bits but got 32768"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_SHORT, data_1d[:1],
                trade.SceneFieldType.VECTOR2, data[::4096])
        with self.assertRaisesRegex(AssertionError, "expected field view stride to fit into 16 bits but got -32769"):
            trade.SceneFieldData(trade.SceneField.CAMERA,
                trade.SceneMappingType.UNSIGNED_SHORT, data_1d[:1],
                trade.SceneFieldType.UNSIGNED_BYTE, data_byte[::32769].flipped(0))
        with self.assertRaisesRegex(AssertionError, "SceneField.MESH can't be an array field"):
            trade.SceneFieldData(trade.SceneField.MESH,
                trade.SceneMappingType.UNSIGNED_SHORT, data_1d,
                trade.SceneFieldType.UNSIGNED_SHORT, data, field_array_size=3)
        with self.assertRaisesRegex(AssertionError, "can't pass SceneFieldFlags.MULTI_ENTRY for a SceneField.TRANSLATION view of SceneFieldType.VECTOR2"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_LONG, data_1d,
                trade.SceneFieldType.VECTOR2, data, flags=trade.SceneFieldFlags.MULTI_ENTRY)
        with self.assertRaisesRegex(AssertionError, "use a string constructor for SceneFieldType.STRING_OFFSET16"):
            trade.SceneFieldData(trade.SceneField.CUSTOM(333),
                trade.SceneMappingType.UNSIGNED_LONG, data_1d,
                trade.SceneFieldType.STRING_OFFSET16, data)
        with self.assertRaisesRegex(AssertionError, "use a bit constructor for SceneFieldType.BIT"):
            trade.SceneFieldData(trade.SceneField.CUSTOM(333),
                trade.SceneMappingType.UNSIGNED_LONG, data_1d,
                trade.SceneFieldType.BIT, data)

    def test_init_bit_1d_invalid(self):
        data = array.array('Q', [0, 0, 0])
        data_bits = containers.BitArray.value_init(3)

        with self.assertRaisesRegex(AssertionError, "expected SceneField.CUSTOM\\(33\\) mapping and field view to have the same size but got 2 and 3"):
            trade.SceneFieldData(trade.SceneField.CUSTOM(33),
                trade.SceneMappingType.UNSIGNED_SHORT, data[:2],
                data_bits)
        with self.assertRaisesRegex(AssertionError, "SceneFieldType.BIT is not a valid type for SceneField.TRANSLATION"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_LONG, data,
                data_bits)
        # SceneMappingType size and stride checks are shared with the non-bit
        # variant, not testing here again
        with self.assertRaisesRegex(AssertionError, "expected field view stride to fit into 16 bits but got 32768"):
            trade.SceneFieldData(trade.SceneField.CUSTOM(33),
                trade.SceneMappingType.UNSIGNED_SHORT, data[:1],
                containers.StridedBitArrayView1D(data_bits)[::32768])
        with self.assertRaisesRegex(AssertionError, "expected field view stride to fit into 16 bits but got -32769"):
            trade.SceneFieldData(trade.SceneField.CUSTOM(33),
                trade.SceneMappingType.UNSIGNED_SHORT, data[:1],
                containers.StridedBitArrayView1D(data_bits)[::32769].flipped(0))
        with self.assertRaisesRegex(AssertionError, "can't pass SceneFieldFlags.OFFSET_ONLY for a SceneField.CUSTOM\\(33\\) view of SceneFieldType.BIT"):
            trade.SceneFieldData(trade.SceneField.CUSTOM(33),
                trade.SceneMappingType.UNSIGNED_LONG, data,
                data_bits, flags=trade.SceneFieldFlags.OFFSET_ONLY)

    def test_init_bit_2d_invalid(self):
        data = array.array('Q', [0, 0, 0])
        data_bits = containers.StridedBitArrayView1D(containers.BitArray.value_init(3*2)).expanded(0, (3, 2))
        data_bits_one_element = containers.StridedBitArrayView1D(containers.BitArray.value_init(3)).expanded(0, (3, 1))

        with self.assertRaisesRegex(AssertionError, "expected SceneField.CUSTOM\\(33\\) mapping and field view to have the same size but got 2 and 3"):
            trade.SceneFieldData(trade.SceneField.CUSTOM(33),
                trade.SceneMappingType.UNSIGNED_SHORT, data[:2],
                data_bits)
        with self.assertRaisesRegex(AssertionError, "SceneFieldType.BIT is not a valid type for SceneField.TRANSLATION"):
            trade.SceneFieldData(trade.SceneField.TRANSLATION,
                trade.SceneMappingType.UNSIGNED_LONG, data,
                data_bits)
        # SceneMappingType size and stride checks are shared with the non-bit
        # variant, not testing here again
        with self.assertRaisesRegex(AssertionError, "expected field view stride to fit into 16 bits but got 32768"):
            trade.SceneFieldData(trade.SceneField.CUSTOM(33),
                trade.SceneMappingType.UNSIGNED_SHORT, data[:1],
                data_bits[::16384])
        with self.assertRaisesRegex(AssertionError, "expected field view stride to fit into 16 bits but got -32769"):
            trade.SceneFieldData(trade.SceneField.CUSTOM(33),
                trade.SceneMappingType.UNSIGNED_SHORT, data[:1],
                data_bits_one_element[::32769].flipped(0))
        with self.assertRaisesRegex(AssertionError, "can't pass SceneFieldFlags.OFFSET_ONLY for a SceneField.CUSTOM\\(33\\) view of SceneFieldType.BIT"):
            trade.SceneFieldData(trade.SceneField.CUSTOM(33),
                trade.SceneMappingType.UNSIGNED_LONG, data,
                data_bits,
                flags=trade.SceneFieldFlags.OFFSET_ONLY)

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
        self.assertEqual(scene.field_count, 8)
        # TODO add some array extras once supported to have this different from
        #   the mapping bound
        self.assertEqual(scene.field_size_bound, 4)
        self.assertFalse(scene.is_2d)
        self.assertTrue(scene.is_3d)
        self.assertIsNone(scene.owner)

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
        scene_field_yes = importer.scene_field_for_name('yes')
        self.assertIsNotNone(scene_field_yes)
        yes_id = scene.field_id(scene_field_yes)

        translations = scene.field(translation_id)
        self.assertIsInstance(translations, containers.StridedArrayView1D)
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
        self.assertIsInstance(cameras, containers.StridedArrayView1D)
        self.assertEqual(cameras.size, (2, ))
        self.assertEqual(cameras.stride, (4, ))
        self.assertEqual(cameras.format, 'I')
        self.assertEqual(list(cameras), [1, 0])
        self.assertIs(cameras.owner, scene)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)

        del cameras
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

        yeses1 = scene.field(scene_field_yes)
        yeses2 = scene.field(yes_id)
        self.assertIsInstance(yeses1, containers.StridedBitArrayView1D)
        self.assertIsInstance(yeses2, containers.StridedBitArrayView1D)
        self.assertEqual(yeses1.size, (2, ))
        self.assertEqual(yeses2.size, (2, ))
        self.assertEqual(yeses1.stride, (1, ))
        self.assertEqual(yeses2.stride, (1, ))
        self.assertEqual(list(yeses1), [True, False])
        self.assertEqual(list(yeses2), [True, False])
        self.assertIs(yeses1.owner, scene)
        self.assertIs(yeses2.owner, scene)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 2)

        del yeses1
        del yeses2
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

        mutable_translations = scene.mutable_field(translation_id)
        self.assertIsInstance(mutable_translations, containers.MutableStridedArrayView1D)
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
        self.assertIsInstance(mutable_cameras, containers.MutableStridedArrayView1D)
        self.assertEqual(mutable_cameras.size, (2, ))
        self.assertEqual(mutable_cameras.stride, (4, ))
        self.assertEqual(mutable_cameras.format, 'I')
        self.assertEqual(list(mutable_cameras), [1, 0])
        self.assertIs(mutable_cameras.owner, scene)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 1)

        del mutable_cameras
        self.assertEqual(sys.getrefcount(scene), scene_refcount)

        mutable_yeses1 = scene.mutable_field(scene_field_yes)
        mutable_yeses2 = scene.mutable_field(yes_id)
        self.assertIsInstance(mutable_yeses1, containers.MutableStridedBitArrayView1D)
        self.assertIsInstance(mutable_yeses2, containers.MutableStridedBitArrayView1D)
        self.assertEqual(mutable_yeses1.size, (2, ))
        self.assertEqual(mutable_yeses2.size, (2, ))
        self.assertEqual(mutable_yeses1.stride, (1, ))
        self.assertEqual(mutable_yeses2.stride, (1, ))
        self.assertEqual(list(mutable_yeses1), [True, False])
        self.assertEqual(list(mutable_yeses2), [True, False])
        self.assertIs(mutable_yeses1.owner, scene)
        self.assertIs(mutable_yeses2.owner, scene)
        self.assertEqual(sys.getrefcount(scene), scene_refcount + 2)

        del mutable_yeses1
        del mutable_yeses2
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

        scene_field_yes = importer.scene_field_for_name('yes')
        self.assertIsNotNone(scene_field_yes)
        yes_id = scene.field_id(scene_field_yes)

        yeses1 = scene.field(scene_field_yes)
        yeses2 = scene.field(yes_id)
        mutable_yeses1 = scene.mutable_field(scene_field_yes)
        mutable_yeses2 = scene.mutable_field(yes_id)
        self.assertEqual(yeses1[0], True)
        self.assertEqual(yeses2[1], False)
        self.assertEqual(mutable_yeses1[0], True)
        self.assertEqual(mutable_yeses2[1], False)

        mutable_yeses1[0] = False
        mutable_yeses2[1] = True
        self.assertEqual(yeses1[0], False)
        self.assertEqual(yeses2[1], True)

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
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        scene = scenetools.filter_except_fields(importer.scene(0), [trade.SceneField.SKIN])
        self.assertEqual(scene.data_flags, trade.DataFlags.NONE)

        with self.assertRaisesRegex(AttributeError, "scene data is not mutable"):
            scene.mutable_mapping(0)
        with self.assertRaisesRegex(AttributeError, "scene data is not mutable"):
            scene.mutable_mapping(trade.SceneField.PARENT)
        with self.assertRaisesRegex(AttributeError, "scene data is not mutable"):
            scene.mutable_field(0)
        with self.assertRaisesRegex(AttributeError, "scene data is not mutable"):
            scene.mutable_field(trade.SceneField.PARENT)

    def test_field_oob(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        scene = importer.scene(0)

        # Access by OOB field ID
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 fields"):
            scene.field_name(scene.field_count)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 fields"):
            scene.field_flags(scene.field_count)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 fields"):
            scene.field_type(scene.field_count)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 fields"):
            scene.field_size(scene.field_count)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 fields"):
            scene.field_array_size(scene.field_count)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 fields"):
            scene.has_field_object(scene.field_count, 0)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 fields"):
            scene.field_object_offset(scene.field_count, 0)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 fields"):
            scene.mapping(scene.field_count)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 fields"):
            scene.mutable_mapping(scene.field_count)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 fields"):
            scene.field(scene.field_count)
        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 fields"):
            scene.mutable_field(scene.field_count)

        # Access by nonexistent field name
        with self.assertRaisesRegex(KeyError, "SceneField.SCALING not found among 8 fields"):
            scene.field_id(trade.SceneField.SCALING)
        with self.assertRaisesRegex(KeyError, "SceneField.SCALING not found among 8 fields"):
            scene.field_flags(trade.SceneField.SCALING)
        with self.assertRaisesRegex(KeyError, "SceneField.SCALING not found among 8 fields"):
            scene.field_type(trade.SceneField.SCALING)
        with self.assertRaisesRegex(KeyError, "SceneField.SCALING not found among 8 fields"):
            scene.field_size(trade.SceneField.SCALING)
        with self.assertRaisesRegex(KeyError, "SceneField.SCALING not found among 8 fields"):
            scene.field_array_size(trade.SceneField.SCALING)
        with self.assertRaisesRegex(KeyError, "SceneField.SCALING not found among 8 fields"):
            scene.has_field_object(trade.SceneField.SCALING, 0)
        with self.assertRaisesRegex(KeyError, "SceneField.SCALING not found among 8 fields"):
            scene.field_object_offset(trade.SceneField.SCALING, 0)
        with self.assertRaisesRegex(KeyError, "SceneField.SCALING not found among 8 fields"):
            scene.mapping(trade.SceneField.SCALING)
        with self.assertRaisesRegex(KeyError, "SceneField.SCALING not found among 8 fields"):
            scene.mutable_mapping(trade.SceneField.SCALING)
        with self.assertRaisesRegex(KeyError, "SceneField.SCALING not found among 8 fields"):
            scene.field(trade.SceneField.SCALING)
        with self.assertRaisesRegex(KeyError, "SceneField.SCALING not found among 8 fields"):
            scene.mutable_field(trade.SceneField.SCALING)

        # OOB object ID
        with self.assertRaisesRegex(IndexError, "index 4 out of range for 4 objects"):
            scene.has_field_object(0, 4) # PARENT
        with self.assertRaisesRegex(IndexError, "index 4 out of range for 4 objects"):
            scene.has_field_object(trade.SceneField.PARENT, 4)
        with self.assertRaisesRegex(IndexError, "index 4 out of range for 4 objects"):
            scene.field_object_offset(0, 4) # PARENT
        with self.assertRaisesRegex(IndexError, "index 4 out of range for 4 objects"):
            scene.field_object_offset(trade.SceneField.PARENT, 4)

        # Lookup error
        with self.assertRaisesRegex(LookupError, "object 1 not found in field SceneField.CAMERA starting at offset 0"):
            scene.field_object_offset(4, 1) # CAMERA
        with self.assertRaisesRegex(LookupError, "object 1 not found in field SceneField.CAMERA starting at offset 0"):
            scene.field_object_offset(trade.SceneField.CAMERA, 1)

        # Lookup error due to field offset being at the end
        with self.assertRaisesRegex(LookupError, "object 1 not found in field SceneField.PARENT starting at offset 4"):
            scene.field_object_offset(0, 1, scene.field_size(0)) # PARENT
        with self.assertRaisesRegex(LookupError, "object 1 not found in field SceneField.PARENT starting at offset 4"):
            scene.field_object_offset(trade.SceneField.PARENT, 1, scene.field_size(trade.SceneField.PARENT))

        # OOB field offset (offset == size is allowed, tested above)
        with self.assertRaisesRegex(IndexError, "offset 5 out of range for a field of size 4"):
            scene.field_object_offset(0, 1, scene.field_size(0) + 1) # PARENT
        with self.assertRaisesRegex(IndexError, "offset 5 out of range for a field of size 4"):
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

        with self.assertRaisesRegex(NotImplementedError, "access to SceneFieldType.STRING_OFFSET32 is not implemented yet, sorry"):
            scene.field(string_field_id)
        with self.assertRaisesRegex(NotImplementedError, "access to SceneFieldType.STRING_OFFSET32 is not implemented yet, sorry"):
            scene.mutable_field(string_field_id)
        with self.assertRaisesRegex(NotImplementedError, "access to SceneFieldType.STRING_OFFSET32 is not implemented yet, sorry"):
            scene.field(string_field)
        with self.assertRaisesRegex(NotImplementedError, "access to SceneFieldType.STRING_OFFSET32 is not implemented yet, sorry"):
            scene.mutable_field(string_field)

class TextureData(unittest.TestCase):
    def test(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'texture.gltf'))

        texture = importer.texture("A texture")
        self.assertEqual(texture.type, trade.TextureType.TEXTURE2D)
        self.assertEqual(texture.minification_filter, SamplerFilter.NEAREST)
        self.assertEqual(texture.magnification_filter, SamplerFilter.LINEAR)
        self.assertEqual(texture.mipmap_filter, SamplerMipmap.NEAREST)
        self.assertEqual(texture.wrapping, (SamplerWrapping.MIRRORED_REPEAT, SamplerWrapping.CLAMP_TO_EDGE, SamplerWrapping.REPEAT))
        self.assertEqual(texture.image, 1)

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

        # TGA importer is loaded directly
        importer = manager.load_and_instantiate('TgaImporter')
        self.assertEqual(importer.metadata.name, 'TgaImporter')

        manager.set_preferred_plugins('TgaImporter', ['StbImageImporter', 'DevIlImageImporter'])

        # TGA importer is loaded from the preferred implementation
        importer = manager.load_and_instantiate('TgaImporter')
        self.assertEqual(importer.metadata.name, 'StbImageImporter')

    def test_set_preferred_plugins_alias_not_found(self):
        manager = trade.ImporterManager()
        with self.assertRaises(KeyError):
            manager.set_preferred_plugins('ApngImporter', [])

    def test_register_external_manager(self):
        # This scenario is stupid in practice, but want to test it on the
        # Importer API for consistency
        converter_manager = trade.ImageConverterManager()
        converter_manager_refcount = sys.getrefcount(converter_manager)

        manager = trade.ImporterManager()
        manager.register_external_manager(converter_manager)
        self.assertEqual(sys.getrefcount(converter_manager), converter_manager_refcount + 1)

        del manager
        self.assertEqual(sys.getrefcount(converter_manager), converter_manager_refcount)

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
            importer.material_count
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.material_for_name('')
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.material_name(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.material(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.material('')

        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.texture_count
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.texture_for_name('')
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.texture_name(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.texture(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            importer.texture('')

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
        texture_importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        texture_importer.open_file(os.path.join(os.path.dirname(__file__), 'texture.gltf'))

        mesh_importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        mesh_importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        material_importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        material_importer.open_file(os.path.join(os.path.dirname(__file__), 'material.gltf'))

        scene_importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        scene_importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        with self.assertRaisesRegex(IndexError, "index 3 out of range for 3 entries"):
            scene_importer.scene_name(3)
        with self.assertRaisesRegex(IndexError, "index 5 out of range for 5 entries"):
            scene_importer.object_name(5)
        with self.assertRaisesRegex(IndexError, "index 3 out of range for 3 entries"):
            scene_importer.scene(3)

        with self.assertRaisesRegex(IndexError, "index 5 out of range for 5 entries"):
            mesh_importer.mesh_level_count(5)
        with self.assertRaisesRegex(IndexError, "index 5 out of range for 5 entries"):
            mesh_importer.mesh_name(5)
        with self.assertRaisesRegex(IndexError, "index 5 out of range for 5 entries"):
            mesh_importer.mesh(5)

        with self.assertRaisesRegex(IndexError, "index 5 out of range for 5 entries"):
            material_importer.material_name(5)
        with self.assertRaisesRegex(IndexError, "index 5 out of range for 5 entries"):
            material_importer.material(5)

        with self.assertRaisesRegex(IndexError, "index 3 out of range for 3 entries"):
            texture_importer.texture_name(3)
        with self.assertRaisesRegex(IndexError, "index 3 out of range for 3 entries"):
            texture_importer.texture(3)

        with self.assertRaisesRegex(IndexError, "index 0 out of range for 0 entries"):
            texture_importer.image1d_level_count(0)
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 entries"):
            texture_importer.image2d_level_count(2)
        with self.assertRaisesRegex(IndexError, "index 0 out of range for 0 entries"):
            texture_importer.image3d_level_count(0)

        with self.assertRaisesRegex(IndexError, "index 0 out of range for 0 entries"):
            texture_importer.image1d_name(0)
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 entries"):
            texture_importer.image2d_name(2)
        with self.assertRaisesRegex(IndexError, "index 0 out of range for 0 entries"):
            texture_importer.image3d_name(0)

        with self.assertRaisesRegex(IndexError, "index 0 out of range for 0 entries"):
            texture_importer.image1d(0)
        with self.assertRaisesRegex(IndexError, "index 2 out of range for 2 entries"):
            texture_importer.image2d(2)
        with self.assertRaisesRegex(IndexError, "index 0 out of range for 0 entries"):
            texture_importer.image3d(0)

    def test_open_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')

        with self.assertRaisesRegex(RuntimeError, "opening nonexistent.png failed"):
            importer.open_file('nonexistent.png')
        with self.assertRaisesRegex(RuntimeError, "opening data failed"):
            importer.open_data(b'')

    def test_open_data_not_supported(self):
        importer = trade.ImporterManager().load_and_instantiate('AnySceneImporter')

        with self.assertRaisesRegex(AssertionError, "feature not supported"):
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
        self.assertEqual(scene.field_count, 8)
        self.assertTrue(scene.has_field(importer.scene_field_for_name('aString')))

    def test_scene_by_name(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        scene = importer.scene("A scene")
        self.assertEqual(scene.field_count, 8)

    def test_scene_by_name_not_found(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        with self.assertRaisesRegex(KeyError, "name Nonexistent not found among 3 entries"):
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
        # TODO clean up once the compatibilitySkinningAttributes option is
        #   gone (until then it'll still return different IDs, regardless of it
        #   being enabled)
        if magnum.BUILD_DEPRECATED:
            self.assertIsNone(importer.mesh_attribute_name(trade.MeshAttribute.CUSTOM(9)))
        else:
            self.assertIsNone(importer.mesh_attribute_name(trade.MeshAttribute.CUSTOM(7)))
        self.assertIsNone(importer.mesh_attribute_for_name("_CUSTOM_ATTRIBUTE"))

        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        self.assertEqual(importer.mesh_count, 5)
        self.assertEqual(importer.mesh_level_count(0), 1)
        self.assertEqual(importer.mesh_name(0), 'Indexed mesh')
        self.assertEqual(importer.mesh_for_name('Indexed mesh'), 0)

        # It should work after opening
        # TODO clean up once the compatibilitySkinningAttributes option is
        #   gone (until then it'll still return different IDs, regardless of it
        #   being enabled)
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

        with self.assertRaisesRegex(IndexError, "level 1 out of range for 1 entries"):
            importer.mesh(0, 1)

    def test_mesh_by_name(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        mesh = importer.mesh('Non-indexed mesh')
        self.assertEqual(mesh.primitive, MeshPrimitive.TRIANGLES)

    def test_mesh_by_name_not_found(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        with self.assertRaisesRegex(KeyError, "name Nonexistent not found among 5 entries"):
            importer.mesh('Nonexistent')

    def test_mesh_by_name_level_oob(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        with self.assertRaisesRegex(IndexError, "level 1 out of range for 1 entries"):
            importer.mesh('Non-indexed mesh', 1)

    def test_mesh_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))

        with self.assertRaisesRegex(RuntimeError, "import failed"):
            importer.mesh(2)
        with self.assertRaisesRegex(RuntimeError, "import failed"):
            importer.mesh('A broken mesh')

    def test_material(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')

        importer.open_file(os.path.join(os.path.dirname(__file__), 'material.gltf'))
        self.assertEqual(importer.material_count, 5)
        self.assertEqual(importer.material_name(2), 'Material with an empty layer')
        self.assertEqual(importer.material_for_name('Material with an empty layer'), 2)

        material = importer.material(2)
        self.assertEqual(material.layer_count, 2)

    def test_material_by_name(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'material.gltf'))

        material = importer.material("Material with an empty layer")
        self.assertEqual(material.layer_count, 2)

    def test_material_by_name_not_found(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'material.gltf'))

        with self.assertRaisesRegex(KeyError, "name Nonexistent not found among 5 entries"):
            importer.material('Nonexistent')

    def test_material_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'material.gltf'))

        with self.assertRaisesRegex(RuntimeError, "import failed"):
            importer.material(3)
        with self.assertRaisesRegex(RuntimeError, "import failed"):
            importer.material("A broken material")

    def test_texture(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')

        importer.open_file(os.path.join(os.path.dirname(__file__), 'texture.gltf'))
        self.assertEqual(importer.texture_count, 3)
        self.assertEqual(importer.texture_name(1), 'A broken texture')
        self.assertEqual(importer.texture_for_name('A broken texture'), 1)

        texture = importer.texture(2)
        self.assertEqual(texture.image, 1)

    def test_texture_by_name(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'texture.gltf'))

        texture = importer.texture("A texture")
        self.assertEqual(texture.image, 1)

    def test_texture_by_name_not_found(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'texture.gltf'))

        with self.assertRaisesRegex(KeyError, "name Nonexistent not found among 3 entries"):
            importer.texture('Nonexistent')

    def test_texture_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'texture.gltf'))

        with self.assertRaisesRegex(RuntimeError, "import failed"):
            importer.texture(1)
        with self.assertRaisesRegex(RuntimeError, "import failed"):
            importer.texture("A broken texture")

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

    def test_image2d_level_oob(self):
        # importer refcounting tested in image2d
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))

        with self.assertRaisesRegex(IndexError, "level 1 out of range for 1 entries"):
            importer.image2d(0, 1)

    def test_image2d_by_name(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'texture.gltf'))

        image = importer.image2d('A named image')
        self.assertEqual(image.size, Vector2i(3, 2))

    def test_image2d_by_name_not_found(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'texture.gltf'))

        with self.assertRaisesRegex(KeyError, "name Nonexistent not found among 2 entries"):
            importer.image2d('Nonexistent')

    def test_image2d_data(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')

        with open(os.path.join(os.path.dirname(__file__), "rgb.png"), 'rb') as f:
            importer.open_data(f.read())

        image = importer.image2d(0)
        self.assertEqual(image.size, Vector2i(3, 2))

    def test_image2d_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'texture.gltf'))

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

    # TODO test also 1D and 3D variants for more robustness

    def test_image2d(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))
        image = importer.image2d(0)
        self.assertEqual(image.size, Vector2i(3, 2))

        converter = trade.ImageConverterManager().load_and_instantiate('StbResizeImageConverter')
        converter.configuration['size'] = "1 1"

        # Both ImageView and ImageData should work
        converted1 = converter.convert(image)
        converted2 = converter.convert(ImageView2D(image))
        self.assertEqual(converted1.size, Vector2i(1, 1))
        self.assertEqual(converted2.size, Vector2i(1, 1))

    def test_image2d_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))
        image = importer.image2d(0)
        self.assertEqual(image.size, Vector2i(3, 2))

        converter = trade.ImageConverterManager().load_and_instantiate('StbResizeImageConverter')
        # not setting any size

        with self.assertRaisesRegex(RuntimeError, "conversion failed"):
            converter.convert(image)

    def test_compressed_image2d(self):
        importer = trade.ImporterManager().load_and_instantiate('DdsImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgba_dxt1.dds'))
        image = importer.image2d(0)
        self.assertTrue(image.is_compressed)

        converter = trade.ImageConverterManager().load_and_instantiate('BcDecImageConverter')

        # Both ImageData and CompressedImageView should work
        converted1 = converter.convert(image)
        converted2 = converter.convert(CompressedImageView2D(image))
        self.assertFalse(converted1.is_compressed)
        self.assertFalse(converted2.is_compressed)

    def test_compressed_image2d_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('DdsImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgba_dxt1.dds'))
        image = importer.image2d(0)
        self.assertTrue(image.is_compressed)

        converter = trade.ImageConverterManager().load_and_instantiate('EtcDecImageConverter')

        with self.assertRaisesRegex(RuntimeError, "conversion failed"):
            converter.convert(image)

    def test_image2d_to_file(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))
        image = importer.image2d(0)

        converter = trade.ImageConverterManager().load_and_instantiate('StbImageConverter')

        with tempfile.TemporaryDirectory() as tmp:
            # Both ImageData and ImageView should work
            converter.convert_to_file(image, os.path.join(tmp, "image1.png"))
            converter.convert_to_file(ImageView2D(image), os.path.join(tmp, "image2.png"))
            self.assertTrue(os.path.exists(os.path.join(tmp, "image1.png")))
            self.assertTrue(os.path.exists(os.path.join(tmp, "image2.png")))

    def test_image2d_to_file_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))
        image = importer.image2d(0)

        converter = trade.ImageConverterManager().load_and_instantiate('StbImageConverter')

        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(RuntimeError, "conversion failed"):
                converter.convert_to_file(image, os.path.join(tmp, "image.hdr"))

    def test_compressed_image2d_to_file(self):
        importer = trade.ImporterManager().load_and_instantiate('DdsImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgba_dxt1.dds'))
        image = importer.image2d(0)
        self.assertTrue(image.is_compressed)

        converter = trade.ImageConverterManager().load_and_instantiate('KtxImageConverter')

        with tempfile.TemporaryDirectory() as tmp:
            # Both ImageData and CompressedImageView should work
            converter.convert_to_file(image, os.path.join(tmp, "image1.ktx2"))
            converter.convert_to_file(CompressedImageView2D(image), os.path.join(tmp, "image2.ktx2"))
            self.assertTrue(os.path.exists(os.path.join(tmp, "image1.ktx2")))
            self.assertTrue(os.path.exists(os.path.join(tmp, "image2.ktx2")))

    def test_compressed_image2d_to_file_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('DdsImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgba_dxt1.dds'))
        image = importer.image2d(0)
        self.assertTrue(image.is_compressed)

        converter = trade.ImageConverterManager().load_and_instantiate('KtxImageConverter')
        # Set something stupid in the config to make it fail
        converter.configuration['swizzle'] = "haha"

        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(RuntimeError, "conversion failed"):
                converter.convert_to_file(image, os.path.join(tmp, "image.ktx2"))

class SceneConverter(unittest.TestCase):
    def test_scenecontents_for_importer(self):
        # Silly, yes, but don't want to enable StanfordImporter just for this
        # test case
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))

        self.assertEqual(trade.SceneContents.FOR(importer), trade.SceneContents.IMAGES2D|trade.SceneContents.NAMES)

    def test_scenecontents_for_importer_not_opened(self):
        # Silly, yes, but don't want to enable StanfordImporter just for this
        # test case
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')

        with self.assertRaisesRegex(AssertionError, "no file opened"):
            trade.SceneContents.FOR(importer)

    def test_scenecontents_for_converter(self):
        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        self.assertEqual(trade.SceneContents.FOR(converter), trade.SceneContents.MESHES|trade.SceneContents.NAMES)

    def test(self):
        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')
        self.assertEqual(converter.features, trade.SceneConverterFeatures.CONVERT_MESH_TO_FILE|trade.SceneConverterFeatures.CONVERT_MESH_TO_DATA)
        self.assertEqual(converter.flags, trade.SceneConverterFlags.NONE)

        converter.flags = trade.SceneConverterFlags.VERBOSE
        self.assertEqual(converter.flags, trade.SceneConverterFlags.VERBOSE)

    def test_mesh(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        mesh = importer.mesh('Indexed mesh')

        converter_manager = trade.SceneConverterManager()
        if 'MeshOptimizerSceneConverter' not in converter_manager.plugin_list:
            self.skipTest("MeshOptimizerSceneConverter plugin not available")

        converter = converter_manager.load_and_instantiate('MeshOptimizerSceneConverter')

        converted_mesh = converter.convert(mesh)
        self.assertEqual(converted_mesh.index_count, mesh.index_count)

    def test_mesh_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        mesh = importer.mesh('Non-indexed mesh')

        converter_manager = trade.SceneConverterManager()
        if 'MeshOptimizerSceneConverter' not in converter_manager.plugin_list:
            self.skipTest("MeshOptimizerSceneConverter plugin not available")

        converter = converter_manager.load_and_instantiate('MeshOptimizerSceneConverter')

        with self.assertRaisesRegex(RuntimeError, "conversion failed"):
            converted_mesh = converter.convert(mesh)

    def test_mesh_not_supported(self):
        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with self.assertRaisesRegex(AssertionError, "mesh conversion not supported"):
            converter.convert(primitives.cube_solid())

    def test_mesh_in_place(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        mesh = importer.mesh('Indexed mesh')

        converter_manager = trade.SceneConverterManager()
        if 'MeshOptimizerSceneConverter' not in converter_manager.plugin_list:
            self.skipTest("MeshOptimizerSceneConverter plugin not available")

        converter = converter_manager.load_and_instantiate('MeshOptimizerSceneConverter')

        converter.convert_in_place(mesh)
        self.assertEqual(mesh.index_count, 3)

    def test_mesh_in_place_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        mesh = importer.mesh('Non-indexed mesh')

        converter_manager = trade.SceneConverterManager()
        if 'MeshOptimizerSceneConverter' not in converter_manager.plugin_list:
            self.skipTest("MeshOptimizerSceneConverter plugin not available")

        converter = converter_manager.load_and_instantiate('MeshOptimizerSceneConverter')

        with self.assertRaisesRegex(RuntimeError, "conversion failed"):
            converter.convert_in_place(mesh)

    def test_mesh_in_place_not_supported(self):
        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with self.assertRaisesRegex(AssertionError, "mesh conversion not supported"):
            converter.convert_in_place(primitives.cube_solid())

    def test_mesh_to_file(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        mesh = importer.mesh(1)

        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.convert_to_file(mesh, os.path.join(tmp, "mesh.ply"))
            self.assertTrue(os.path.exists(os.path.join(tmp, "mesh.ply")))

    def test_mesh_to_file_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        mesh = importer.mesh(1)

        converter = trade.SceneConverterManager().load_and_instantiate('AnySceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(RuntimeError, "conversion failed"):
                converter.convert_to_file(mesh, os.path.join(tmp, "mesh.obj"))

    def test_mesh_to_file_not_supported(self):
        converter_manager = trade.SceneConverterManager()
        if 'MeshOptimizerSceneConverter' not in converter_manager.plugin_list:
            self.skipTest("MeshOptimizerSceneConverter plugin not available")

        converter = converter_manager.load_and_instantiate('MeshOptimizerSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(AssertionError, "mesh conversion not supported"):
                converter.convert_to_file(primitives.cube_solid(), os.path.join(tmp, "mesh.foo"))

    def test_batch_file(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        mesh = importer.mesh(1)

        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "mesh.ply"))
            self.assertTrue(converter.is_converting)
            self.assertEqual(converter.mesh_count, 0)

            self.assertEqual(converter.add(mesh), 0)
            self.assertEqual(converter.mesh_count, 1)

            converter.end_file()
            self.assertFalse(converter.is_converting)

            self.assertTrue(os.path.exists(os.path.join(tmp, "mesh.ply")))

    def test_batch_file_begin_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        mesh = importer.mesh(1)

        converter = trade.SceneConverterManager().load_and_instantiate('AnySceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(RuntimeError, "beginning the conversion failed"):
                converter.begin_file(os.path.join(tmp, "mesh.obj"))

    def test_batch_file_end_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        mesh = importer.mesh(1)

        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "mesh.ply"))
            with self.assertRaisesRegex(RuntimeError, "ending the conversion failed"):
                converter.end_file()

    def test_batch_file_not_supported(self):
        converter_manager = trade.SceneConverterManager()
        if 'MeshOptimizerSceneConverter' not in converter_manager.plugin_list:
            self.skipTest("MeshOptimizerSceneConverter plugin not available")

        converter = converter_manager.load_and_instantiate('MeshOptimizerSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaisesRegex(AssertionError, "feature not supported"):
                converter.begin_file(os.path.join(tmp, "mesh.foo"))

    def test_batch_add_mesh_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        mesh = importer.mesh('Point mesh')

        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "mesh.ply"))
            with self.assertRaisesRegex(RuntimeError, "adding the mesh failed"):
                converter.add(mesh)

    def test_batch_add_mesh_not_supported(self):
        # TODO implement once there's a converter that doesn't support meshes
        #   or has only in-place conversion (MeshOptimizerSceneConverter
        #   support this with begin())
        pass

    def test_batch_set_mesh_attribute_name(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        mesh = importer.mesh('Custom mesh attribute')

        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            filename = os.path.join(tmp, "mesh.gltf")
            converter.begin_file(filename)
            converter.set_mesh_attribute_name(importer.mesh_attribute_for_name('_FOOBARTHINGY'), '_FOOBARTHINGY')
            converter.add(mesh)
            converter.end_file()

            with open(filename, 'r') as f:
                self.assertIn('_FOOBARTHINGY', f.read())

    def test_batch_set_mesh_attribute_name_not_custom(self):
        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            filename = os.path.join(tmp, "mesh.gltf")
            converter.begin_file(filename)

            with self.assertRaisesRegex(AssertionError, "not a custom attribute"):
                converter.set_mesh_attribute_name(trade.MeshAttribute.POSITION, 'foo')

    def test_batch_set_mesh_attribute_name_not_supported(self):
        # TODO implement once there's a converter that doesn't support meshes
        pass

    def test_batch_add_material(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'material.gltf'))
        material = importer.material("Material with an empty layer")

        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            filename = os.path.join(tmp, "file.gltf")
            converter.begin_file(filename)
            self.assertEqual(converter.material_count, 0)

            converter.add(material, "Material with an empty layer")
            self.assertEqual(converter.material_count, 1)

            converter.end_file()

            with open(filename, 'r') as f:
                self.assertIn("Material with an empty layer", f.read())

    def test_batch_add_material_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'material.gltf'))
        # References a texture, which means conversion will fail due to the
        # texture not being added before
        material = importer.material(0)

        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "file.gltf"))

            with self.assertRaisesRegex(RuntimeError, "adding the material failed"):
                converter.add(material)

    def test_batch_add_material_not_supported(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'material.gltf'))
        material = importer.material("Material with an empty layer")

        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "file.ply"))

            with self.assertRaisesRegex(AssertionError, "material conversion not supported"):
                converter.add(material)

    def test_batch_add_scene(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        scene = importer.scene("A default scene that's empty")

        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            filename = os.path.join(tmp, "scene.gltf")
            converter.begin_file(filename)
            self.assertEqual(converter.scene_count, 0)

            converter.add(scene, "A default scene that's empty")
            self.assertEqual(converter.scene_count, 1)

            converter.end_file()

            with open(filename, 'r') as f:
                self.assertIn("A default scene that's empty", f.read())

    def test_batch_add_scene_failed(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene = importer.scene(0)
        self.assertFalse(scene.is_3d)

        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "scene.gltf"))

            with self.assertRaisesRegex(RuntimeError, "adding the scene failed"):
                converter.add(scene)

    def test_batch_add_scene_not_supported(self):
        # Static builds with non-static plugins cause assertions with non-owned
        # array deleters used by PrimitiveImporter, skip in that case
        if magnum.BUILD_STATIC:
            self.skipTest("dynamic PrimitiveImporter doesn't work with a static build")

        importer = trade.ImporterManager().load_and_instantiate('PrimitiveImporter')
        importer.open_data(containers.ArrayView())

        scene = importer.scene(0)

        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "scene.ply"))

            with self.assertRaisesRegex(AssertionError, "scene conversion not supported"):
                converter.add(scene)

    def test_batch_set_default_scene(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        scene = importer.scene("A default scene that's empty")

        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            filename = os.path.join(tmp, "scene.gltf")
            converter.begin_file(filename)
            converter.add(scene, "A default scene that's empty")
            converter.set_default_scene(0)
            converter.end_file()

            with open(filename, 'r') as f:
                self.assertIn('"scene": 0', f.read())

    def test_batch_set_default_scene_out_of_range(self):
        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "scene.gltf"))

            with self.assertRaisesRegex(AssertionError, "index 1 out of range for 0 scenes"):
                converter.set_default_scene(1)

    def test_batch_set_default_scene_not_supported(self):
        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "scene.ply"))

            with self.assertRaisesRegex(AssertionError, "feature not supported"):
                converter.set_default_scene(0)

    def test_batch_set_scene_field_name(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        # Keep only the single custom numeric attribute in the scene (plus
        # hierarchy and transformation that's required by glTF)
        # TODO clean up once there's a possibility to create scenes from
        # scratch
        scene = scenetools.filter_only_fields(importer.scene("A scene"),             [
            trade.SceneField.PARENT,
            trade.SceneField.TRANSLATION,
            importer.scene_field_for_name('aNumber'),
        ])

        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            filename = os.path.join(tmp, "scene.gltf")
            converter.begin_file(filename)
            converter.set_scene_field_name(importer.scene_field_for_name('aNumber'), 'aNumber')
            converter.add(scene)
            converter.end_file()

            with open(filename, 'r') as f:
                self.assertIn('"aNumber":', f.read())

    def test_batch_set_scene_field_name_not_custom(self):
        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "scene.gltf"))

            with self.assertRaisesRegex(AssertionError, "not a custom field"):
                converter.set_scene_field_name(trade.SceneField.SCALING, 'foo')

    def test_batch_set_scene_field_name_not_supported(self):
        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "scene.ply"))

            with self.assertRaisesRegex(AssertionError, "feature not supported"):
                converter.set_scene_field_name(trade.SceneField.CUSTOM(1), 'foo')

    def test_batch_add_image2d(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))

        image = importer.image2d(0)

        image_converter_manager = trade.ImageConverterManager()
        scene_converter_manager = trade.SceneConverterManager()
        scene_converter_manager.register_external_manager(image_converter_manager)

        converter = scene_converter_manager.load_and_instantiate('GltfSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            filename = os.path.join(tmp, "file.gltf")
            converter.begin_file(filename)
            self.assertEqual(converter.image2d_count, 0)

            converter.add(image, "A PNG image")
            self.assertEqual(converter.image2d_count, 1)

            converter.end_file()

            with open(filename, 'r') as f:
                self.assertIn("A PNG image", f.read())

    def test_batch_add_image2d_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))

        image = importer.image2d(0)

        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')
        # Nonsense image converter
        converter.configuration['imageConverter'] = 'ThisIsNoImageConverter'

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "file.gltf"))

            with self.assertRaisesRegex(RuntimeError, "adding the image failed"):
                converter.add(image)

    def test_batch_add_image2d_not_supported(self):
        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))
        image = importer.image2d(0)

        importer = trade.ImporterManager().load_and_instantiate('DdsImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), "rgba_dxt1.dds"))
        compressed_image = importer.image2d(0)

        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "file.ply"))

            with self.assertRaisesRegex(AssertionError, "2D image conversion not supported"):
                converter.add(image)
            with self.assertRaisesRegex(AssertionError, "compressed 2D image conversion not supported"):
                converter.add(compressed_image)

    def test_batch_add_importer_contents(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'two-meshes.gltf'))

        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "two-meshes.gltf"))
            self.assertEqual(converter.mesh_count, 0)

            # Nothing like that in the file
            converter.add_importer_contents(importer, trade.SceneContents.SCENES|trade.SceneContents.CAMERAS)
            self.assertEqual(converter.mesh_count, 0)

            converter.add_importer_contents(importer)
            self.assertEqual(converter.mesh_count, 2)

    def test_batch_add_importer_contents_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'two-meshes.gltf'))

        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "file.ply"))

            with self.assertRaisesRegex(RuntimeError, "adding importer contents failed"):
                converter.add_importer_contents(importer)

    def test_batch_add_importer_contents_not_opened(self):
        importer = trade.ImporterManager().load_and_instantiate('AnySceneImporter')

        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "file.gltf"))

            with self.assertRaisesRegex(AssertionError, "the importer is not opened"):
                converter.add_importer_contents(importer)

    def test_batch_add_supported_importer_contents(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))

        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            filename = os.path.join(tmp, "two-meshes.gltf")
            converter.begin_file(filename)
            self.assertEqual(converter.mesh_count, 0)

            # Nothing like that in the file
            converter.add_supported_importer_contents(importer, trade.SceneContents.MESHES)
            self.assertEqual(converter.mesh_count, 0)

            # It contains cameras, nodes and scenes, none of which is supported
            # by the converter
            converter.add_supported_importer_contents(importer)
            self.assertEqual(converter.mesh_count, 0)

    def test_batch_add_supported_importer_contents_failed(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'two-meshes.gltf'))

        converter = trade.SceneConverterManager().load_and_instantiate('StanfordSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            filename = os.path.join(tmp, "two-meshes.gltf")
            converter.begin_file(filename)

            with self.assertRaisesRegex(RuntimeError, "adding importer contents failed"):
                converter.add_supported_importer_contents(importer)

    def test_batch_add_supported_importer_contents_not_opened(self):
        importer = trade.ImporterManager().load_and_instantiate('AnySceneImporter')

        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')

        with tempfile.TemporaryDirectory() as tmp:
            converter.begin_file(os.path.join(tmp, "file.gltf"))

            with self.assertRaisesRegex(AssertionError, "the importer is not opened"):
                converter.add_supported_importer_contents(importer)

    def test_batch_no_conversion_in_progress(self):
        importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'mesh.gltf'))
        mesh = importer.mesh('Custom mesh attribute')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'scene.gltf'))
        scene = importer.scene("A default scene that's empty")
        importer.open_file(os.path.join(os.path.dirname(__file__), 'material.gltf'))
        material = importer.material("Material with an empty layer")

        importer = trade.ImporterManager().load_and_instantiate('StbImageImporter')
        importer.open_file(os.path.join(os.path.dirname(__file__), 'rgb.png'))
        image = importer.image2d(0)

        converter = trade.SceneConverterManager().load_and_instantiate('GltfSceneConverter')
        self.assertFalse(converter.is_converting)

        with self.assertRaisesRegex(AssertionError, "no conversion in progress"):
            converter.end_file()
        with self.assertRaisesRegex(AssertionError, "no conversion in progress"):
            converter.mesh_count
        with self.assertRaisesRegex(AssertionError, "no conversion in progress"):
            converter.add(mesh)
        with self.assertRaisesRegex(AssertionError, "no conversion in progress"):
            converter.set_mesh_attribute_name(trade.MeshAttribute.CUSTOM(1), 'foobar')
        with self.assertRaisesRegex(AssertionError, "no conversion in progress"):
            converter.material_count
        with self.assertRaisesRegex(AssertionError, "no conversion in progress"):
            converter.add(material)
        with self.assertRaisesRegex(AssertionError, "no conversion in progress"):
            converter.scene_count
        with self.assertRaisesRegex(AssertionError, "no conversion in progress"):
            converter.add(scene)
        with self.assertRaisesRegex(AssertionError, "no conversion in progress"):
            converter.set_default_scene(0)
        with self.assertRaisesRegex(AssertionError, "no conversion in progress"):
            converter.set_scene_field_name(trade.SceneField.CUSTOM(1), 'foobar')
        with self.assertRaisesRegex(AssertionError, "no conversion in progress"):
            converter.image2d_count
        with self.assertRaisesRegex(AssertionError, "no conversion in progress"):
            converter.add(image)
        with self.assertRaisesRegex(AssertionError, "no conversion in progress"):
            converter.add_importer_contents(importer)
        with self.assertRaisesRegex(AssertionError, "no conversion in progress"):
            converter.add_supported_importer_contents(importer)
