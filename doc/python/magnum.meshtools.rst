..
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>

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

.. py:function:: magnum.meshtools.compress_indices
    :raise AssertionError: If :p:`mesh` is not indexed

.. py:function:: magnum.meshtools.concatenate
    :raise AssertionError: If :p:`meshes` is empty
    :raise AssertionError: If any of the :p:`meshes` is
        :ref:`MeshPrimitive.LINE_STRIP`, :ref:`MeshPrimitive.LINE_LOOP`,
        :ref:`MeshPrimitive.TRIANGLE_STRIP` or
        :ref:`MeshPrimitive.TRIANGLE_FAN`
    :raise AssertionError: If all :p:`meshes` don't have the same
        :ref:`MeshPrimitive`

.. py:function:: magnum.meshtools.duplicate
    :raise AssertionError: If :p:`mesh` is not indexed

.. py:function:: magnum.meshtools.filter_attributes
    :raise AssertionError: If size of :p:`attributes_to_keep` is different than
        :p:`mesh` attribute count

.. py:function:: magnum.meshtools.generate_indices
    :raise AssertionError: If :p:`mesh` is not :ref:`MeshPrimitive.LINE_STRIP`,
        :ref:`MeshPrimitive.LINE_LOOP`, :ref:`MeshPrimitive.TRIANGLE_STRIP` or
        :ref:`MeshPrimitive.TRIANGLE_FAN`

.. py:function:: magnum.meshtools.interleave
    :raise AssertionError: If any attribute in :p:`extra` has the data size
        different from :p:`mesh` vertex count

.. py:function:: magnum.meshtools.transform2d
    :raise KeyError: If :p:`mesh` doesn't have
        :ref:`trade.MeshAttribute.POSITION` of index :p:`id` (and in morph
        target :p:`morph_target_id` if not :py:`-1`)
    :raise AssertionError: If :ref:`trade.MeshAttribute.POSITION` are not 2D

.. py:function:: magnum.meshtools.transform2d_in_place
    :raise AssertionError: If :p:`mesh` vertex data aren't
        :ref:`trade.DataFlags.MUTABLE`
    :raise KeyError: If :p:`mesh` doesn't have
        :ref:`trade.MeshAttribute.POSITION` of index :p:`id` (and in morph
        target :p:`morph_target_id` if not :py:`-1`)
    :raise AssertionError: If :ref:`trade.MeshAttribute.POSITION` are not
        :ref:`VertexFormat.VECTOR2`

.. py:function:: magnum.meshtools.transform3d
    :raise KeyError: If :p:`mesh` doesn't have
        :ref:`trade.MeshAttribute.POSITION` of index :p:`id` (and in morph
        target :p:`morph_target_id` if not :py:`-1`)
    :raise AssertionError: If :ref:`trade.MeshAttribute.POSITION` are not 3D

.. py:function:: magnum.meshtools.transform3d_in_place
    :raise AssertionError: If :p:`mesh` vertex data aren't
        :ref:`trade.DataFlags.MUTABLE`
    :raise KeyError: If :p:`mesh` doesn't have
        :ref:`trade.MeshAttribute.POSITION` of index :p:`id` (and in morph
        target :p:`morph_target_id` if not :py:`-1`)
    :raise AssertionError: If :ref:`trade.MeshAttribute.POSITION` are not
        :ref:`VertexFormat.VECTOR3`

.. py:function:: magnum.meshtools.transform_texture_coordinates2d
    :raise KeyError: If :p:`mesh` doesn't have
        :ref:`trade.MeshAttribute.TEXTURE_COORDINATES` of index :p:`id` (and in
        morph target :p:`morph_target_id` if not :py:`-1`)

.. py:function:: magnum.meshtools.transform_texture_coordinates2d_in_place
    :raise AssertionError: If :p:`mesh` vertex data aren't
        :ref:`trade.DataFlags.MUTABLE`
    :raise KeyError: If :p:`mesh` doesn't have
        :ref:`trade.MeshAttribute.TEXTURE_COORDINATES` of index :p:`id` (and in
        morph target :p:`morph_target_id` if not :py:`-1`)
    :raise AssertionError: If :ref:`trade.MeshAttribute.TEXTURE_COORDINATES`
        are not :ref:`VertexFormat.VECTOR2`
