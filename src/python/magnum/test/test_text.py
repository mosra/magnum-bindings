#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020, 2021, 2022, 2023, 2024, 2025
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
import sys
import unittest

from corrade import pluginmanager
from magnum import *
from magnum import text

class Font(unittest.TestCase):
    def test(self):
        manager = text.FontManager()
        self.assertIn('StbTrueTypeFont', manager.alias_list)
        self.assertEqual(manager.load_state('StbTrueTypeFont'), pluginmanager.LoadState.NOT_LOADED)

        self.assertTrue(manager.load('StbTrueTypeFont') & pluginmanager.LoadState.LOADED)
        self.assertEqual(manager.unload('StbTrueTypeFont'), pluginmanager.LoadState.NOT_LOADED)

        with self.assertRaisesRegex(RuntimeError, "can't load plugin"):
            manager.load('NonexistentFont')
        with self.assertRaisesRegex(RuntimeError, "can't unload plugin"):
            manager.unload('NonexistentFont')

    def test_no_file_opened(self):
        font = text.FontManager().load_and_instantiate('StbTrueTypeFont')
        self.assertFalse(font.is_opened)

        with self.assertRaisesRegex(AssertionError, "no file opened"):
            font.size
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            font.ascent
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            font.descent
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            font.line_height
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            font.glyph_count
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            font.glyph_id('A')
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            font.glyph_size(0)
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            font.glyph_advance(0)
        # fill_glyph_cache() not tested as it needs a GL context; verified in
        # test_text_gl instead
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            font.create_shaper()

    def test_open_failed(self):
        font = text.FontManager().load_and_instantiate('StbTrueTypeFont')

        with self.assertRaisesRegex(RuntimeError, "opening nonexistent.ttf failed"):
            font.open_file('nonexistent.ttf', 16.0)
        with self.assertRaisesRegex(RuntimeError, "opening data failed"):
            font.open_data(b'', 16.0)

    def test_open(self):
        manager = text.FontManager()
        manager_refcount = sys.getrefcount(manager)

        # Font references the manager to ensure it doesn't get GC'd before the
        # plugin instances
        font = manager.load_and_instantiate('StbTrueTypeFont')
        self.assertIs(font.manager, manager)
        self.assertEqual(sys.getrefcount(manager), manager_refcount + 1)

        font.open_file(os.path.join(os.path.dirname(__file__), 'Oxygen.ttf'), 16.0)
        self.assertTrue(font.is_opened)
        self.assertEqual(font.size, 16.0)
        self.assertEqual(font.ascent, 17.011186599731445)
        self.assertEqual(font.descent, -4.322147846221924)
        self.assertEqual(font.line_height, 21.33333396911621)
        self.assertEqual(font.glyph_count, 671)
        self.assertEqual(font.glyph_id('A'), 36)
        self.assertEqual(font.glyph_size(36), (12.0, 14.0))
        self.assertEqual(font.glyph_advance(36), (11.7136, 0.0))

        # Deleting the font should decrease manager refcount again
        del font
        self.assertEqual(sys.getrefcount(manager), manager_refcount)

    def test_open_data(self):
        font = text.FontManager().load_and_instantiate('StbTrueTypeFont')

        with open(os.path.join(os.path.dirname(__file__), 'Oxygen.ttf'), 'rb') as f:
            font.open_data(f.read(), 16.0)

        self.assertEqual(font.size, 16.0)

    def test_glyph_oob(self):
        font = text.FontManager().load_and_instantiate('StbTrueTypeFont')

        font.open_file(os.path.join(os.path.dirname(__file__), 'Oxygen.ttf'), 16.0)
        self.assertEqual(font.glyph_count, 671)

        with self.assertRaises(IndexError):
            font.glyph_size(671)
        with self.assertRaises(IndexError):
            font.glyph_advance(671)

    # Creating a shaper and accessing its properties is tested in Shaper below

class Feature(unittest.TestCase):
    def test_custom(self):
        feature = text.Feature('kern')
        self.assertEqual(feature, text.Feature.KERNING)

    def test_custom_invalid(self):
        with self.assertRaisesRegex(AssertionError, "expected a four-character code, got ss999"):
            text.Feature('ss999')

class FeatureRange(unittest.TestCase):
    def test(self):
        a = text.FeatureRange(text.Feature.SMALL_CAPITALS)
        self.assertEqual(a.feature, text.Feature.SMALL_CAPITALS)
        self.assertTrue(a.is_enabled)
        self.assertEqual(a.value, 1)

        b = text.FeatureRange(text.Feature.KERNING, 0)
        self.assertEqual(b.feature, text.Feature.KERNING)
        self.assertFalse(b.is_enabled)
        self.assertEqual(b.value, 0)

        # It should be possible to create it from a tuple also, to support
        # implicit conversion when passed to renderer add() or render()
        c1 = text.FeatureRange((text.Feature.KERNING, 1))
        c2 = text.FeatureRange((text.Feature.KERNING, True))
        self.assertEqual(c1.feature, text.Feature.KERNING)
        self.assertEqual(c2.feature, text.Feature.KERNING)
        self.assertTrue(c1.is_enabled)
        self.assertTrue(c2.is_enabled)
        self.assertEqual(c1.value, 1)
        self.assertEqual(c2.value, 1)

class Script(unittest.TestCase):
    def test_custom(self):
        han = text.Script('Hani')
        self.assertEqual(han, text.Script.HAN)

    def test_custom_invalid(self):
        with self.assertRaisesRegex(AssertionError, "expected a four-character code, got Hello"):
            text.Script('Hello')

class Shaper(unittest.TestCase):
    def test(self):
        font = text.FontManager().load_and_instantiate('StbTrueTypeFont')
        font.open_file(os.path.join(os.path.dirname(__file__), 'Oxygen.ttf'), 16.0)
        font_refcount = sys.getrefcount(font)

        shaper = font.create_shaper()
        self.assertIs(shaper.font, font)
        self.assertEqual(sys.getrefcount(font), font_refcount + 1)
        # StbTrueTypeFont doesn't support setting any of these so it should
        # return false
        self.assertFalse(shaper.set_script(text.Script.LATIN))
        self.assertFalse(shaper.set_language("en"))
        self.assertFalse(shaper.set_direction(text.ShapeDirection.LEFT_TO_RIGHT))

        del shaper
        self.assertEqual(sys.getrefcount(font), font_refcount)
