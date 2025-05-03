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

# setUpModule gets called before everything else, skipping if GL tests can't
# be run
from . import GLTestCase, setUpModule

from corrade import pluginmanager
import magnum
from magnum import *
from magnum import gl, text

class GlyphCacheGL(GLTestCase):
    def test(self):
        cache = text.GlyphCacheGL(PixelFormat.R8_UNORM, (128, 128), (2, 2))

        self.assertEqual(cache.format, PixelFormat.R8_UNORM)
        self.assertEqual(cache.processed_format, PixelFormat.R8_UNORM)
        self.assertEqual(cache.size, (128, 128, 1))
        self.assertEqual(cache.processed_size, (128, 128, 1))
        self.assertEqual(cache.padding, (2, 2))

        cache_refcount = sys.getrefcount(cache)

        # Returned texture references the cache to ensure it doesn't get GC'd
        # before the texture instances
        texture = cache.texture
        self.assertEqual(sys.getrefcount(cache), cache_refcount + 1)

        # Deleting the texture should decrease cache refcount again
        del texture
        self.assertEqual(sys.getrefcount(cache), cache_refcount)

    def test_implicit_padding(self):
        # Ensure the default is correct to avoid artifacts
        cache = text.GlyphCacheGL(PixelFormat.R8_UNORM, (128, 128))
        self.assertEqual(cache.padding, (1, 1))

    def test_fill(self):
        # Tested here and not in test_text.Font as the AbstractGlyphCache base
        # isn't instantiable
        cache = text.GlyphCacheGL(PixelFormat.R8_UNORM, (128, 128))
        font = text.FontManager().load_and_instantiate('StbTrueTypeFont')

        font.open_file(os.path.join(os.path.dirname(__file__), 'Oxygen.ttf'), 16.0)
        self.assertTrue(font.is_opened)
        self.assertTrue(font.fill_glyph_cache(cache, "abcd"))

    def test_fill_no_file_opened(self):
        # Tested here and not in test_text.Font as the AbstractGlyphCache base
        # isn't instantiable
        cache = text.GlyphCacheGL(PixelFormat.R8_UNORM, (128, 128))
        font = text.FontManager().load_and_instantiate('StbTrueTypeFont')
        self.assertFalse(font.is_opened)

        with self.assertRaisesRegex(AssertionError, "no file opened"):
            font.fill_glyph_cache(cache, "abcd")

class DistanceFieldGlyphCacheGL(GLTestCase):
    def test(self):
        cache = text.DistanceFieldGlyphCacheGL((1024, 1024), (128, 128), 2)

        self.assertEqual(cache.size, (1024, 1024, 1))
        self.assertEqual(cache.size, (1024, 1024, 1))
        self.assertEqual(cache.padding, (2, 2))

class Renderer2D(GLTestCase):
    @unittest.skipIf(magnum.TARGET_GLES2, "Needs OES_mapbuffer on ES2 and extension queries are not exposed to Python yet")
    def test(self):
        font = text.FontManager().load_and_instantiate('StbTrueTypeFont')
        font.open_file(os.path.join(os.path.dirname(__file__), 'Oxygen.ttf'), 16.0)

        cache = text.GlyphCacheGL(PixelFormat.R8_UNORM, (128, 128))
        font.fill_glyph_cache(cache, "hello")

        renderer = text.Renderer2D(font, cache, 1.0)
        renderer.reserve(16)
        renderer.render("hello")

        self.assertEqual(renderer.rectangle, Range2D((0.0, -0.270134), (2.50727, 1.0632)))
