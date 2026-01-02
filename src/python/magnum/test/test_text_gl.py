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

class RendererGL(GLTestCase):
    def test(self):
        font = text.FontManager().load_and_instantiate('StbTrueTypeFont')
        font.open_file(os.path.join(os.path.dirname(__file__), 'Oxygen.ttf'), 16.0)

        cache = text.GlyphCacheGL(PixelFormat.R8_UNORM, (128, 128))
        cache_refcount = sys.getrefcount(cache)
        font.fill_glyph_cache(cache, "hello")

        shaper = font.create_shaper()
        shaper_refcount = sys.getrefcount(cache)

        # Passing the cache to the renderer should increase its refcount
        renderer = text.RendererGL(cache)
        renderer_refcount = sys.getrefcount(renderer)
        self.assertEqual(sys.getrefcount(cache), cache_refcount + 1)

        # OTOH the cache isn't owned by the renderer so the returned value
        # doesn't increase the renderer refcount
        cache_from_renderer = renderer.glyph_cache
        self.assertIs(cache_from_renderer, cache)
        self.assertEqual(sys.getrefcount(cache), cache_refcount + 2)
        self.assertEqual(sys.getrefcount(renderer), renderer_refcount)
        del cache_from_renderer
        self.assertEqual(sys.getrefcount(cache), cache_refcount + 1)
        self.assertEqual(sys.getrefcount(renderer), renderer_refcount)

        self.assertEqual(renderer.glyph_count, 0)
        self.assertEqual(renderer.glyph_capacity, 0)
        self.assertEqual(renderer.glyph_index_capacity, 0)
        self.assertEqual(renderer.glyph_vertex_capacity, 0)
        self.assertEqual(renderer.run_count, 0)
        self.assertEqual(renderer.run_capacity, 0)
        self.assertFalse(renderer.is_rendering)
        self.assertEqual(renderer.rendering_glyph_count, 0)
        self.assertEqual(renderer.rendering_run_count, 0)
        self.assertEqual(renderer.cursor, Vector2())
        self.assertEqual(renderer.alignment, text.Alignment.MIDDLE_CENTER)
        self.assertEqual(renderer.line_advance, 0.0)
        self.assertEqual(renderer.index_type, MeshIndexType.UNSIGNED_SHORT)

        renderer.reserve(16, 3)
        self.assertEqual(renderer.glyph_capacity, 16)
        self.assertEqual(renderer.glyph_index_capacity, 16)
        self.assertEqual(renderer.glyph_vertex_capacity, 16)
        self.assertEqual(renderer.run_capacity, 3)

        # Passing a shaper to add() should not increase its refcount
        renderer.add(shaper, 1.0, "he")
        self.assertEqual(sys.getrefcount(shaper), shaper_refcount)
        self.assertEqual(renderer.glyph_count, 0)
        self.assertEqual(renderer.run_count, 0)
        self.assertTrue(renderer.is_rendering)
        self.assertEqual(renderer.rendering_glyph_count, 2)
        self.assertEqual(renderer.rendering_run_count, 1)

        # Verifying the overload with a feature list. All variants should
        # implicitly convert to text.FeatureRange, the StbTrueTypeFont doesn't
        # make use of either.
        renderer.add(shaper, 1.0, "ll", [
            text.Feature.KERNING,
            (text.Feature.SMALL_CAPITALS, False),
            (text.Feature.ACCESS_ALL_ALTERNATES, 33),
        ])

        # Neither to render() it should
        rectangle, runs = renderer.render(shaper, 1.0, "o")
        self.assertEqual(sys.getrefcount(shaper), shaper_refcount)
        self.assertEqual(rectangle, Range2D((-1.25364, -0.666667), (1.25364, 0.666667)))
        self.assertEqual(runs, Range1Dui(0, 3))
        self.assertEqual(renderer.glyph_count, 5)
        self.assertEqual(renderer.run_count, 3)
        self.assertFalse(renderer.is_rendering)
        self.assertEqual(renderer.rendering_glyph_count, 5)
        self.assertEqual(renderer.rendering_run_count, 3)

        # Returned mesh references the renderer to ensure it doesn't get GC'd
        # before the mesh instances
        mesh = renderer.mesh
        self.assertEqual(sys.getrefcount(renderer), renderer_refcount + 1)
        self.assertEqual(mesh.count, 5*6)

        # Deleting the mesh should decrease renderer refcount again
        del mesh
        self.assertEqual(sys.getrefcount(renderer), renderer_refcount)

        renderer.cursor = (-15.0, 37.0)
        renderer.alignment = text.Alignment.LINE_LEFT
        renderer.line_advance = -7.0
        renderer.index_type = MeshIndexType.UNSIGNED_BYTE
        self.assertEqual(renderer.cursor, Vector2(-15.0, 37.0))
        self.assertEqual(renderer.alignment, text.Alignment.LINE_LEFT)
        self.assertEqual(renderer.line_advance, -7.0)
        self.assertEqual(renderer.index_type, MeshIndexType.UNSIGNED_BYTE)

        # Clear keeps the cursor, alignment etc properties
        renderer.clear()
        self.assertEqual(renderer.glyph_count, 0)
        self.assertEqual(renderer.run_count, 0)
        self.assertEqual(renderer.cursor, Vector2(-15.0, 37.0))
        self.assertEqual(renderer.alignment, text.Alignment.LINE_LEFT)
        self.assertEqual(renderer.line_advance, -7.0)
        self.assertEqual(renderer.index_type, MeshIndexType.UNSIGNED_BYTE)

        # Testing the argument-less render() also
        renderer.add(shaper, 1.0, "hello")
        rectangle2, runs2 = renderer.render()
        # The rectangle is different due to the different cursor & alignment
        self.assertEqual(rectangle2, Range2D((-15.0, 36.7299), (-12.4927, 38.0632)))
        self.assertEqual(runs2, Range1Dui(0, 1))
        self.assertEqual(renderer.glyph_count, 5)
        self.assertEqual(renderer.run_count, 1)
        self.assertFalse(renderer.is_rendering)

        # Only the index type stays after reset
        renderer.reset()
        self.assertEqual(renderer.glyph_count, 0)
        self.assertEqual(renderer.run_count, 0)
        self.assertEqual(renderer.cursor, Vector2())
        self.assertEqual(renderer.alignment, text.Alignment.MIDDLE_CENTER)
        self.assertEqual(renderer.line_advance, 0.0)
        self.assertEqual(renderer.index_type, MeshIndexType.UNSIGNED_BYTE)

        # Deleting the renderer should decrease cache refcount again
        del renderer
        self.assertEqual(sys.getrefcount(cache), cache_refcount)

    def test_setters_rendering_in_progress(self):
        font = text.FontManager().load_and_instantiate('StbTrueTypeFont')
        font.open_file(os.path.join(os.path.dirname(__file__), 'Oxygen.ttf'), 16.0)

        cache = text.GlyphCacheGL(PixelFormat.R8_UNORM, (128, 128))
        font.fill_glyph_cache(cache, "abcd")

        shaper = font.create_shaper()

        renderer = text.RendererGL(cache)
        renderer.add(shaper, 1.0, "a")
        self.assertTrue(renderer.is_rendering)

        with self.assertRaisesRegex(AssertionError, "rendering in progress"):
            renderer.cursor = Vector2()
        with self.assertRaisesRegex(AssertionError, "rendering in progress"):
            renderer.alignment = text.Alignment.LINE_RIGHT
        with self.assertRaisesRegex(AssertionError, "rendering in progress"):
            renderer.line_advance = 0.0
        with self.assertRaisesRegex(AssertionError, "rendering in progress"):
            renderer.index_type = MeshIndexType.UNSIGNED_INT

    def test_add_render_font_not_in_cache(self):
        font1 = text.FontManager().load_and_instantiate('StbTrueTypeFont')
        font2 = text.FontManager().load_and_instantiate('StbTrueTypeFont')
        font3 = text.FontManager().load_and_instantiate('StbTrueTypeFont')
        font1.open_file(os.path.join(os.path.dirname(__file__), 'Oxygen.ttf'), 16.0)
        font2.open_file(os.path.join(os.path.dirname(__file__), 'Oxygen.ttf'), 16.0)
        font3.open_file(os.path.join(os.path.dirname(__file__), 'Oxygen.ttf'), 16.0)

        cache = text.GlyphCacheGL(PixelFormat.R8_UNORM, (128, 128))
        font1.fill_glyph_cache(cache, "abcd")
        font3.fill_glyph_cache(cache, "abcd")

        shaper = font2.create_shaper()
        renderer = text.RendererGL(cache)

        with self.assertRaisesRegex(AssertionError, "shaper font not found among 2 fonts in associated glyph cache"):
            renderer.add(shaper, 1.0, "a")
        with self.assertRaisesRegex(AssertionError, "shaper font not found among 2 fonts in associated glyph cache"):
            renderer.render(shaper, 1.0, "a")
