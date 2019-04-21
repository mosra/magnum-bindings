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
        self.assertEqual(Rad(Deg(90.0)), Rad(1.570796326794896))

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
