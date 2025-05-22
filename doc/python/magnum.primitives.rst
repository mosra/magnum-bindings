..
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
..

.. py:function:: magnum.primitives.capsule2d_wireframe
    :raise AssertionError: If :p:`hemisphere_rings` is less than :py:`1`
    :raise AssertionError: If :p:`cylinder_rings` is less than :py:`1`
.. py:function:: magnum.primitives.capsule3d_solid
    :raise AssertionError: If :p:`hemisphere_rings` is less than :py:`1`
    :raise AssertionError: If :p:`cylinder_rings` is less than :py:`1`
    :raise AssertionError: If :p:`segments` is less than :py:`3`
.. py:function:: magnum.primitives.capsule3d_wireframe
    :raise AssertionError: If :p:`hemisphere_rings` is less than :py:`1`
    :raise AssertionError: If :p:`cylinder_rings` is less than :py:`1`
    :raise AssertionError: If :p:`segments` is zero or not a multiple of
        :py:`4`

.. py:function:: magnum.primitives.circle2d_solid
    :raise AssertionError: If :p:`segments` is less than :py:`3`
.. py:function:: magnum.primitives.circle2d_wireframe
    :raise AssertionError: If :p:`segments` is less than :py:`3`
.. py:function:: magnum.primitives.circle3d_solid
    :raise AssertionError: If :p:`segments` is less than :py:`3`
.. py:function:: magnum.primitives.circle3d_wireframe
    :raise AssertionError: If :p:`segments` is less than :py:`3`

.. py:function:: magnum.primitives.cone_solid
    :raise AssertionError: If :p:`rings` is less than :py:`1`
    :raise AssertionError: If :p:`segments` is less than :py:`3`
.. py:function:: magnum.primitives.cone_wireframe
    :raise AssertionError: If :p:`segments` is zero or not a multiple of
        :py:`4`

.. py:function:: magnum.primitives.cylinder_solid
    :raise AssertionError: If :p:`rings` is less than :py:`1`
    :raise AssertionError: If :p:`segments` is less than :py:`3`
.. py:function:: magnum.primitives.cylinder_wireframe
    :raise AssertionError: If :p:`rings` is less than :py:`1`
    :raise AssertionError: If :p:`segments` is zero or not a multiple of
        :py:`4`

.. py:function:: magnum.primitives.uv_sphere_solid
    :raise AssertionError: If :p:`rings` is less than :py:`2`
    :raise AssertionError: If :p:`segments` is less than :py:`3`
.. py:function:: magnum.primitives.uv_sphere_wireframe
    :raise AssertionError: If :p:`rings` is zero or not a multiple of :py:`2`
    :raise AssertionError: If :p:`segments` is zero or not a multiple of
        :py:`4`
