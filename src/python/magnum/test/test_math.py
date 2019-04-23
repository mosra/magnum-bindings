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

import unittest

from magnum import *
from magnum import math

class Angle(unittest.TestCase):
    def test_init(self):
        a = Deg()
        b = Rad.zero_init()
        c = Deg(90.0)
        self.assertEqual(a, Deg(0.0))
        self.assertEqual(b, Rad(0.0))
        self.assertEqual(c, Deg(90.0))

    def test_conversion(self):
        self.assertEqual(Rad(Deg(90.0)), Rad(math.pi_half))

    def test_ops(self):
        self.assertEqual(-Deg(30.0), Deg(-30.0))
        self.assertEqual(Deg(30.0) + Deg(45.0), Deg(75.0))
        self.assertEqual(Deg(75.0) - Deg(45.0), Deg(30.0))
        self.assertEqual(Deg(45.0)*2.0, Deg(90.0))
        self.assertEqual(Deg(90.0)/2.0, Deg(45.0))
        self.assertEqual(Deg(180.0)/Deg(9.0), 20.0)

    def test_inplace_ops(self):
        a = Deg(30.0)
        a += Deg(45.0)
        self.assertEqual(a, Deg(75.0))

        a = Deg(75.0)
        a -= Deg(45.0)
        self.assertEqual(a, Deg(30.0))

        a = Deg(45.0)
        a *= 2.0
        self.assertEqual(a, Deg(90.0))

        a = Deg(90.0)
        a /= 2.0
        self.assertEqual(a, Deg(45.0))

    def test_repr(self):
        self.assertEqual(repr(Deg(45.3)), 'Deg(45.3)')

class BoolVector(unittest.TestCase):
    def test_init(self):
        a = BoolVector2()
        b = BoolVector2.zero_init()
        c = BoolVector2(0b11)
        self.assertFalse(a.any())
        self.assertTrue(c.all())
        self.assertEqual(a, BoolVector2(0b00))
        self.assertEqual(b, BoolVector2(0b00))
        self.assertEqual(c, BoolVector2(0b11))

    def test_length(self):
        self.assertEqual(BoolVector3.__len__(), 3)
        #self.assertEqual(len(BoolVector3), 3) TODO: Y not?
        self.assertEqual(len(BoolVector4()), 4)

    def test_set_get(self):
        a = BoolVector4(0b1010)
        self.assertEqual(a[1], 1)

        a[2] = True
        self.assertEqual(a, BoolVector4(0b1110))

    def test_iterate(self):
        a = [i for i in BoolVector4(0b1010)]
        self.assertEqual(a, [False, True, False, True])

    def test_ops(self):
        a = BoolVector3(0b101) | BoolVector3(0b010)
        self.assertEqual(a, BoolVector3(0b111))

    def test_repr(self):
        self.assertEqual(repr(BoolVector4(0b0101)), 'BoolVector(0b0101)')

class Constants(unittest.TestCase):
    def test(self):
        self.assertAlmostEqual(math.sqrt2**2, 2.0, 6)
        self.assertAlmostEqual(math.sqrt3**2, 3.0)

class Vector(unittest.TestCase):
    def test_init(self):
        a = Vector4i()
        b = Vector3d.zero_init()
        c = Vector2i(44, -3)
        self.assertEqual(a, Vector4i(0, 0, 0, 0))
        self.assertEqual(b, Vector3d(0.0, 0.0, 0.0))
        self.assertEqual(c, Vector2i(44, -3))

    def test_static_methods(self):
        self.assertEqual(Vector2.y_scale(5), Vector2(1, 5))
        self.assertEqual(Vector3d.z_axis(-3), Vector3d(0, 0, -3))
        self.assertEqual(Vector3i.x_axis(), Vector3i(1, 0, 0))

    def test_length(self):
        self.assertEqual(Vector3.__len__(), 3)
        #self.assertEqual(len(Vector3), 3) TODO: Y not?
        self.assertEqual(len(Vector4i()), 4)

    def test_properties(self):
        a = Vector2i()
        a.x = 1
        a.y = 2
        self.assertEqual(a.x, 1)
        self.assertEqual(a.y, 2)
        self.assertEqual(a, Vector2i(1, 2))

        a = Vector3()
        a.x = 1.0
        a.y = 2.0
        a.z = 3.0
        self.assertEqual(a.x, 1.0)
        self.assertEqual(a.y, 2.0)
        self.assertEqual(a.z, 3.0)
        self.assertEqual(a, Vector3(1.0, 2.0, 3.0))

        a.xy = (-1.0, -2.0)
        self.assertEqual(a.xy, Vector2(-1.0, -2.0))
        self.assertEqual(a, Vector3(-1.0, -2.0, 3.0))

        a = Vector3()
        a.r = 1.0
        a.g = 2.0
        a.b = 3.0
        self.assertEqual(a.r, 1.0)
        self.assertEqual(a.g, 2.0)
        self.assertEqual(a.b, 3.0)
        self.assertEqual(a, Vector3(1.0, 2.0, 3.0))

        a = Vector4d()
        a.x = 1.0
        a.y = 2.0
        a.z = 3.0
        a.w = 4.0
        self.assertEqual(a.x, 1.0)
        self.assertEqual(a.y, 2.0)
        self.assertEqual(a.z, 3.0)
        self.assertEqual(a.w, 4.0)
        self.assertEqual(a, Vector4d(1.0, 2.0, 3.0, 4.0))

        a = Vector4d()
        a.r = 1.0
        a.g = 2.0
        a.b = 3.0
        a.a = 4.0
        self.assertEqual(a.r, 1.0)
        self.assertEqual(a.g, 2.0)
        self.assertEqual(a.b, 3.0)
        self.assertEqual(a.a, 4.0)
        self.assertEqual(a, Vector4d(1.0, 2.0, 3.0, 4.0))

        a.xy = (-1.0, -2.0)
        self.assertEqual(a.xy, Vector2d(-1.0, -2.0))
        self.assertEqual(a, Vector4d(-1.0, -2.0, 3.0, 4.0))

        a.xyz = (0.5, 0.25, 0.125)
        self.assertEqual(a.xyz, Vector3d(0.5, 0.25, 0.125))
        self.assertEqual(a, Vector4d(0.5, 0.25, 0.125, 4.0))

    def test_properties_rgb(self):
        a = Vector3()
        a.r = 1.0
        a.g = 2.0
        a.b = 3.0
        self.assertEqual(a.r, 1.0)
        self.assertEqual(a.g, 2.0)
        self.assertEqual(a.b, 3.0)
        self.assertEqual(a, Vector3(1.0, 2.0, 3.0))

        a = Vector4d()
        a.r = 1.0
        a.g = 2.0
        a.b = 3.0
        a.a = 4.0
        self.assertEqual(a.r, 1.0)
        self.assertEqual(a.g, 2.0)
        self.assertEqual(a.b, 3.0)
        self.assertEqual(a.a, 4.0)
        self.assertEqual(a, Vector4d(1.0, 2.0, 3.0, 4.0))

        a.rgb = (0.5, 0.25, 0.125)
        self.assertEqual(a.rgb, Vector3d(0.5, 0.25, 0.125))
        self.assertEqual(a, Vector4d(0.5, 0.25, 0.125, 4.0))

    def test_set_get(self):
        a = Vector3(1.0, 3.14, -13.37)
        self.assertAlmostEqual(a[1], 3.14, 6)

        a[2] = 0.13
        self.assertEqual(a, Vector3(1.0, 3.14, 0.13))

        b = Vector4i(3, 4, 5, 6)
        b.b *= 3
        b.xy += Vector2i(1, -1)
        self.assertEqual(b, Vector4i(4, 3, 15, 6))

    def test_iterate(self):
        a = [i for i in Vector4(1.0, 3.25, 3.5, -1.125)]
        # assertAlmostEqual doesn't work on lists so i'm using values directly
        # representable as floats to avoid the need for delta comparison
        self.assertEqual(a, [1.0, 3.25, 3.5, -1.125])

    def test_ops(self):
        self.assertEqual(math.dot(Vector2(0.5, 3.0), Vector2(2.0, 0.5)), 2.5)
        self.assertEqual(Deg(math.angle(
            Vector2(0.5, 3.0).normalized(),
            Vector2(2.0, 0.5).normalized())), Deg(66.5014333443446))
        self.assertEqual(Vector3(1.0, 2.0, 0.3).projected(Vector3.y_axis()),
                         Vector3.y_axis(2.0))
        self.assertEqual(Vector3(1.0, 2.0, 0.3).projected_onto_normalized(Vector3.y_axis()),
                         Vector3.y_axis(2.0))

    def test_ops_number_on_the_left(self):
        self.assertEqual(2.0*Vector2(1.0, -3.0), Vector2(2.0, -6.0))
        self.assertEqual(6.0/Vector2(2.0, -3.0), Vector2(3.0, -2.0))

    def test_repr(self):
        self.assertEqual(repr(Vector3(1.0, 3.14, -13.37)), 'Vector(1, 3.14, -13.37)')
