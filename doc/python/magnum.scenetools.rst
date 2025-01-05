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

.. py:function:: magnum.scenetools.filter_fields
    :raise AssertionError: If size of :p:`fields_to_keep` is different than
        :ref:`trade.SceneData.field_count`

.. py:function:: magnum.scenetools.filter_field_entries
    :raise AssertionError: If any field in :p:`entries_to_keep` does not exist
        in :p:`scene`
    :raise AssertionError: If any field in :p:`entries_to_keep` is listed more
        than once
    :raise AssertionError: If size of any array in :p:`entries_to_keep` does
        not match :ref:`trade.SceneData.field_size()` for given field

.. py:function:: magnum.scenetools.filter_objects
    :raise AssertionError: If size of :p:`objects_to_keep` is different than
        :ref:`trade.SceneData.mapping_bound`

.. py:function:: magnum.scenetools.parents_breadth_first
    :raise AssertionError: If :p:`scene` does not have
        :ref:`trade.SceneField.PARENT`

.. py:function:: magnum.scenetools.children_depth_first
    :raise AssertionError: If :p:`scene` does not have
        :ref:`trade.SceneField.PARENT`

.. py:function:: magnum.scenetools.absolute_field_transformations2d(scene: magnum.trade.SceneData, field: magnum.trade.SceneField, global_transformation: magnum.Matrix3)
    :raise KeyError: If :p:`field` does not exist in :p:`scene`
    :raise AssertionError: If :p:`scene` is not 2D
    :raise AssertionError: If :p:`scene` does not have
        :ref:`trade.SceneField.PARENT`

.. py:function:: magnum.scenetools.absolute_field_transformations2d(scene: magnum.trade.SceneData, field_id: int, global_transformation: magnum.Matrix3)
    :raise IndexError: If :p:`field_id` negative or not less than
        :ref:`trade.SceneData.field_count`
    :raise AssertionError: If :p:`scene` is not 2D
    :raise AssertionError: If :p:`scene` does not have
        :ref:`trade.SceneField.PARENT`

.. py:function:: magnum.scenetools.absolute_field_transformations3d(scene: magnum.trade.SceneData, field: magnum.trade.SceneField, global_transformation: magnum.Matrix4)
    :raise KeyError: If :p:`field` does not exist in :p:`scene`
    :raise AssertionError: If :p:`scene` is not 2D
    :raise AssertionError: If :p:`scene` does not have
        :ref:`trade.SceneField.PARENT`

.. py:function:: magnum.scenetools.absolute_field_transformations3d(scene: magnum.trade.SceneData, field_id: int, global_transformation: magnum.Matrix4)
    :raise IndexError: If :p:`field_id` negative or not less than
        :ref:`trade.SceneData.field_count`
    :raise AssertionError: If :p:`scene` is not 2D
    :raise AssertionError: If :p:`scene` does not have
        :ref:`trade.SceneField.PARENT`
