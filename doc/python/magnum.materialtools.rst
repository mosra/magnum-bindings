..
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024
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

.. py:function:: magnum.materialtools.filter_attributes
    :raise AssertionError: If size of :p:`attributes_to_keep` is different than
        :ref:`trade.MaterialData.attribute_data_offset()` for
        :ref:`trade.MaterialData.layer_count`
.. py:function:: magnum.materialtools.filter_layers
    :raise AssertionError: If size of :p:`layers_to_keep` is different than
        :ref:`trade.MaterialData.layer_count`
.. py:function:: magnum.materialtools.filter_attributes_layers
    :raise AssertionError: If size of :p:`attributes_to_keep` is different than
        :ref:`trade.MaterialData.attribute_data_offset()` for
        :ref:`trade.MaterialData.layer_count`
    :raise AssertionError: If size of :p:`layers_to_keep` is different than
        :ref:`trade.MaterialData.layer_count`
.. py:function:: magnum.materialtools.merge
    :raise RuntimeError: If merge failed due to a conflict
.. py:function:: magnum.materialtools.phong_to_pbr_metallic_roughness
    :raise RuntimeError: If conversion failed
