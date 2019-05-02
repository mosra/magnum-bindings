#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
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

import sys
import unittest

# setUpModule gets called before everything else, skipping if GL tests can't
# be run
from . import GLTestCase, setUpModule

from magnum import gl

class Buffer(GLTestCase):
    def test_init(self):
        a = gl.Buffer()
        self.assertNotEqual(a.id, 0)
        self.assertEqual(a.target_hint, gl.Buffer.TargetHint.ARRAY)

        b = gl.Buffer(gl.Buffer.TargetHint.ELEMENT_ARRAY)
        self.assertNotEqual(b.id, 0)
        self.assertEqual(b.target_hint, gl.Buffer.TargetHint.ELEMENT_ARRAY)

    def test_set_data(self):
        a = gl.Buffer()
        a.set_data(b'hello', gl.BufferUsage.STATIC_DRAW)

class DefaultFramebuffer(GLTestCase):
    def test(self):
        # Using it should not crash, leak or cause double-free issues
        self.assertTrue(gl.default_framebuffer is not None)

class Mesh(GLTestCase):
    def test_init(self):
        a = gl.Mesh(gl.MeshPrimitive.LINE_LOOP)
        self.assertNotEqual(a.id, 0)
        self.assertEqual(a.primitive, gl.MeshPrimitive.LINE_LOOP)

    def test_set_count(self):
        a = gl.Mesh()
        a.count = 15
        self.assertEqual(a.count, 15)

    def test_add_buffer(self):
        buffer = gl.Buffer()
        buffer_refcount = sys.getrefcount(buffer)

        # Adding a buffer to the mesh should increase its ref count
        mesh = gl.Mesh()
        mesh.add_vertex_buffer(buffer, 0, 8, gl.Attribute(gl.Attribute.Kind.GENERIC, 2, gl.Attribute.Components.TWO, gl.Attribute.DataType.FLOAT))
        self.assertEqual(sys.getrefcount(buffer), buffer_refcount + 1)

        # Deleting the mesh should decrease it again
        del mesh
        self.assertEqual(sys.getrefcount(buffer), buffer_refcount)
