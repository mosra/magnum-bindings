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
            font.glyph_id('A')
        with self.assertRaisesRegex(AssertionError, "no file opened"):
            font.glyph_advance(0)
        # fill_glyph_cache() not tested as it needs a GL context; assuming it's
        # correct

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
        self.assertEqual(font.glyph_id('A'), 36)
        self.assertEqual(font.glyph_advance(36), (11.7136, 0.0))

        # Deleting the font should decrease manager refcount again
        del font
        self.assertEqual(sys.getrefcount(manager), manager_refcount)

    def test_open_data(self):
        font = text.FontManager().load_and_instantiate('StbTrueTypeFont')

        with open(os.path.join(os.path.dirname(__file__), 'Oxygen.ttf'), 'rb') as f:
            font.open_data(f.read(), 16.0)

        self.assertEqual(font.size, 16.0)
