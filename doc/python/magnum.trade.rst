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

.. py:class:: magnum.trade.ImageData1D

    See :ref:`ImageData2D` for more information.

.. py:class:: magnum.trade.ImageData2D

    Implicitly convertible to :ref:`ImageView2D` / :ref:`MutableImageView2D`,
    so all APIs consuming image views work with this type as well.

    `Memory ownership and reference counting`_
    ==========================================

    The class can be both owning an non-owning depending on the value of
    :ref:`data_flags`. If they contain neither :ref:`DataFlags.OWNED` nor
    :ref:`DataFlags.GLOBAL`, the :ref:`owner` property references the object
    actually owning the pixel data the image points to. This ensures calling
    :py:`del` on the original object will *not* invalidate the data.

    `Pixel data access`_
    ====================

    The class makes use of Python's dynamic nature and provides direct access
    to pixel data in their concrete types via :ref:`pixels`. See
    :ref:`ImageView2D` documentation for more information and usage example.

    Compared to :ref:`Image2D` and :ref:`ImageView2D` / :ref:`MutableImageView2D`,
    the :ref:`data` and :ref:`pixels` views are immutable and mutable access
    is provided depending on the value of :ref:`data_flags` via
    :ref:`mutable_data` and :ref:`mutable_pixels`.

.. py:class:: magnum.trade.ImageData3D

    See :ref:`ImageData2D` for more information.

.. py:property:: magnum.trade.ImageData1D.storage
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`
.. py:property:: magnum.trade.ImageData2D.storage
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`
.. py:property:: magnum.trade.ImageData3D.storage
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`

.. py:property:: magnum.trade.ImageData1D.mutable_data
    :raise AttributeError: If :ref:`data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`
.. py:property:: magnum.trade.ImageData2D.mutable_data
    :raise AttributeError: If :ref:`data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`
.. py:property:: magnum.trade.ImageData3D.mutable_data
    :raise AttributeError: If :ref:`data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`

.. py:property:: magnum.trade.ImageData1D.format
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`
.. py:property:: magnum.trade.ImageData2D.format
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`
.. py:property:: magnum.trade.ImageData3D.format
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`

.. py:property:: magnum.trade.ImageData1D.compressed_format
    :raise AttributeError: If :ref:`is_compressed` is :py:`False`
.. py:property:: magnum.trade.ImageData2D.compressed_format
    :raise AttributeError: If :ref:`is_compressed` is :py:`False`
.. py:property:: magnum.trade.ImageData3D.compressed_format
    :raise AttributeError: If :ref:`is_compressed` is :py:`False`

.. py:property:: magnum.trade.ImageData1D.pixel_size
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`
.. py:property:: magnum.trade.ImageData2D.pixel_size
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`
.. py:property:: magnum.trade.ImageData3D.pixel_size
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`

.. py:property:: magnum.trade.ImageData1D.pixels
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`
.. py:property:: magnum.trade.ImageData2D.pixels
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`
.. py:property:: magnum.trade.ImageData3D.pixels
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`

.. py:property:: magnum.trade.ImageData1D.mutable_pixels
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`
    :raise AttributeError: If :ref:`data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`
.. py:property:: magnum.trade.ImageData2D.mutable_pixels
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`
    :raise AttributeError: If :ref:`data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`
.. py:property:: magnum.trade.ImageData3D.mutable_pixels
    :raise AttributeError: If :ref:`is_compressed` is :py:`True`
    :raise AttributeError: If :ref:`data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`

.. py:enum:: magnum.trade.MeshAttribute

    The equivalent to C++ :dox:`Trade::meshAttributeCustom()` is creating an
    enum value using a ``CUSTOM()`` named constructor. The ``is_custom``
    property then matches :dox:`Trade::isMeshAttributeCustom()` and you can
    retrieve the custom ID again with a ``custom_value`` property.

    ..
        >>> from magnum import trade

    .. code:: pycon

        >>> attribute = trade.MeshAttribute.CUSTOM(17)
        >>> attribute.name
        'CUSTOM(17)'
        >>> attribute.is_custom
        True
        >>> attribute.custom_value
        17

.. py:class:: magnum.trade.MeshAttributeData

    Associates a typed data view with a name, vertex format and other mesh
    attribute properties, which can be subsequently put into a :ref:`MeshData`
    instance, for example with :ref:`meshtools.interleave()`. The data view can
    be either one-dimensional, for example a NumPy array:

    ..
        Just to verify the snippet below actually works (don't want the arrows
        shown in the docs, want to have it nicely wrapped)
    ..
        >>> from magnum import *
        >>> import numpy as np
        >>> data = np.array([(-0.5, 0.0), (+0.5, 0.0), ( 0.0, 0.5)], dtype='2f')
        >>> positions = trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.VECTOR2, data)

    .. code:: py

        data = np.array([(-0.5, 0.0),
                         (+0.5, 0.0),
                         ( 0.0, 0.5)], dtype='2f')
        positions = trade.MeshAttributeData(
            trade.MeshAttribute.POSITION,
            VertexFormat.VECTOR2,
            data)

    Or it can be two-dimensional, for example by expanding a flat array into a
    list of two-component vectors:

    ..
        Again to verify the snippet below actually works
    ..
        >>> from corrade import containers
        >>> import array
        >>> data = array.array('f', [-0.5, 0.0, +0.5, 0.0, 0.0, 0.5])
        >>> positions = trade.MeshAttributeData(trade.MeshAttribute.POSITION, VertexFormat.VECTOR2, containers.StridedArrayView1D(data).expanded(0, (3, 2)))

    .. code:: py

        data = array.array('f', [-0.5, 0.0,
                                 +0.5, 0.0,
                                  0.0, 0.5])
        positions = trade.MeshAttributeData(
            trade.MeshAttribute.POSITION,
            VertexFormat.VECTOR2,
            containers.StridedArrayView1D(data).expanded(0, (3, 2)))

    `Memory ownership and reference counting`_
    ==========================================

    On initialization, the instance inherits the
    :ref:`containers.StridedArrayView1D.owner <corrade.containers.StridedArrayView1D.owner>`
    object, storing it in the :ref:`owner` field, meaning that calling
    :py:`del` on the original data will *not* invalidate the instance.

    `Data access`_
    ==============

    Similarly to :ref:`MeshData`, the class makes use of Python's dynamic
    nature and provides direct access to attribute data in their concrete type
    via :ref:`data`. However, the :ref:`MeshAttributeData` is considered a low
    level API and thus a :ref:`containers.StridedArrayView2D <corrade.containers.StridedArrayView2D>`
    is returned always, even for non-array attributes. The returned view
    inherits the :ref:`owner` and element access coverts to a type
    corresponding to a particular :ref:`VertexFormat`. For example, extracting
    the data from the :py:`positions` attribute created above:

    .. code:: pycon

        >>> view = positions.data
        >>> view.owner is data
        True
        >>> view[1][0]
        Vector(0.5, 0)

.. py:function:: magnum.trade.MeshAttributeData.__init__(self, name: magnum.trade.MeshAttribute, format: magnum.VertexFormat, data: corrade.containers.StridedArrayView1D, array_size: int, morph_target_id: int)
    :raise AssertionError: If :p:`format` is not valid for :p:`name`
    :raise AssertionError: If :p:`data` size doesn't fit into 32 bits
    :raise AssertionError: If :p:`data` stride doesn't fit into 16 bits
    :raise AssertionError: If :p:`data` format size is smaller than size of
        :p:`format` at given :p:`array_size`
    :raise AssertionError: If :p:`morph_target_id` is less than :py:`-1` or
        greater than :py:`127`
    :raise AssertionError: If :p:`morph_target_id` is not allowed for :p:`name`
    :raise AssertionError: If :p:`array_size` is zero and :p:`name` is an array
        attribute
    :raise AssertionError: If :p:`array_size` is non-zero and :p:`name` can't
        be an array attribute
.. py:function:: magnum.trade.MeshAttributeData.__init__(self, name: magnum.trade.MeshAttribute, format: magnum.VertexFormat, data: corrade.containers.StridedArrayView2D, array_size: int, morph_target_id: int)
    :raise AssertionError: If :p:`format` is not valid for :p:`name`
    :raise AssertionError: If :p:`data` first dimension size doesn't fit into
        32 bits
    :raise AssertionError: If :p:`data` first dimension stride doesn't fit into
        16 bits
    :raise AssertionError: If :p:`data` second dimension isn't contiguous
    :raise AssertionError: If :p:`data` format size times second dimension size
        is smaller than size of :p:`format` at given :p:`array_size`
    :raise AssertionError: If :p:`morph_target_id` is less than :py:`-1` or
        greater than :py:`127`
    :raise AssertionError: If :p:`morph_target_id` is not allowed for :p:`name`
    :raise AssertionError: If :p:`array_size` is zero and :p:`name` is an array
        attribute
    :raise AssertionError: If :p:`array_size` is non-zero and :p:`name` can't
        be an array attribute

.. py:property:: magnum.trade.MeshAttributeData.data
    :raise NotImplementedError: If :ref:`format <MeshAttributeData.format>` is
        a half-float or matrix type

    A 2D view is returned always, non-array attributes have the second
    dimension size :py:`1`.

.. py:class:: magnum.trade.MeshData

    :TODO: remove this line once m.css stops ignoring first caption on a page

    `Memory ownership and reference counting`_
    ==========================================

    The class can be both owning an non-owning depending on the value of
    :ref:`index_data_flags` and :ref:`vertex_data_flags`. If they contain
    neither :ref:`DataFlags.OWNED` nor :ref:`DataFlags.GLOBAL`, the
    :ref:`owner` property references the object actually owning the index and
    vertex data the mesh points to. This ensures calling :py:`del` on the
    original object will *not* invalidate the data.

    `Index and attribute data access`_
    ==================================

    The class makes use of Python's dynamic nature and provides direct access
    to index and attribute data in their concrete types via :ref:`indices` and
    :ref:`attribute()`. The returned views point to the underlying mesh data,
    element access coverts to a type corresponding to a particular
    :ref:`VertexFormat` and for performance-oriented access the view implements
    a buffer protocol with a corresponding type annotation:

    ..
        >>> from magnum import primitives, trade
        >>> import numpy as np

    .. code:: pycon

        >>> mesh = primitives.cube_solid()
        >>> list(mesh.indices)[:10]
        [0, 1, 2, 0, 2, 3, 4, 5, 6, 4]
        >>> list(mesh.attribute(trade.MeshAttribute.POSITION))[:3]
        [Vector(-1, -1, 1), Vector(1, -1, 1), Vector(1, 1, 1)]
        >>> np.array(mesh.attribute(trade.MeshAttribute.NORMAL), copy=False)[2]
        array([0., 0., 1.], dtype=float32)

    Depending on the value of :ref:`index_data_flags` / :ref:`vertex_data_flags`
    it's also possible to access the data in a mutable way via
    :ref:`mutable_indices` and :ref:`mutable_attribute()`, for example to
    perform a static transformation of the mesh before passing it to OpenGL.

    Normalized formats (such as :ref:`VertexFormat.VECTOR3UB_NORMALIZED`) are
    unpacked to a corresponding floating-point representation in element access
    and packed from a floating-point representation in mutable acess. The type
    annotation is however still matching the original type (such as :py:`'3B'`
    in this case), so code consuming these via the buffer protocol needs to
    handle the normalization explicitly if needed.

.. py:property:: magnum.trade.MeshData.mutable_index_data
    :raise AttributeError: If :ref:`index_data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`
.. py:property:: magnum.trade.MeshData.mutable_vertex_data
    :raise AttributeError: If :ref:`vertex_data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`
.. py:property:: magnum.trade.MeshData.index_count
    :raise AttributeError: If :ref:`is_indexed` is :py:`False`
.. py:property:: magnum.trade.MeshData.index_type
    :raise AttributeError: If :ref:`is_indexed` is :py:`False`
.. py:property:: magnum.trade.MeshData.index_offset
    :raise AttributeError: If :ref:`is_indexed` is :py:`False`
.. py:property:: magnum.trade.MeshData.index_stride
    :raise AttributeError: If :ref:`is_indexed` is :py:`False`
.. py:property:: magnum.trade.MeshData.mutable_indices
    :raise AttributeError: If :ref:`index_data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`
.. py:function:: magnum.trade.MeshData.attribute_name
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`

.. py:function:: magnum.trade.MeshData.attribute_id(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
.. py:function:: magnum.trade.MeshData.attribute_id(self, name: magnum.trade.MeshAttribute, id: int, morph_target_id: int)
    :raise KeyError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`name` and :p:`morph_target_id`

    Compared to the C++ API, there's no
    :dox:`Trade::MeshData::findAttributeId()`, the desired workflow is instead
    calling :ref:`attribute_id()` and catching an exception if not found.

.. py:function:: magnum.trade.MeshData.attribute_format(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
.. py:function:: magnum.trade.MeshData.attribute_format(self, name: magnum.trade.MeshAttribute, id: int, morph_target_id: int)
    :raise KeyError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`name` and :p:`morph_target_id`
.. py:function:: magnum.trade.MeshData.attribute_offset(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
.. py:function:: magnum.trade.MeshData.attribute_offset(self, name: magnum.trade.MeshAttribute, id: int, morph_target_id: int)
    :raise KeyError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`name` and :p:`morph_target_id`
.. py:function:: magnum.trade.MeshData.attribute_stride(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
.. py:function:: magnum.trade.MeshData.attribute_stride(self, name: magnum.trade.MeshAttribute, id: int, morph_target_id: int)
    :raise KeyError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`name` and :p:`morph_target_id`
.. py:function:: magnum.trade.MeshData.attribute_array_size(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
.. py:function:: magnum.trade.MeshData.attribute_array_size(self, name: magnum.trade.MeshAttribute, id: int, morph_target_id: int)
    :raise KeyError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`name` and :p:`morph_target_id`
.. py:function:: magnum.trade.MeshData.attribute(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
    :raise NotImplementedError: if :ref:`attribute_array_size()` for given
        attribute isn't :py:`0`
.. py:function:: magnum.trade.MeshData.attribute(self, name: magnum.trade.MeshAttribute, id: int, morph_target_id: int)
    :raise KeyError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`name` and :p:`morph_target_id`
    :raise NotImplementedError: if :ref:`attribute_array_size()` for given
        attribute isn't :py:`0`
.. py:function:: magnum.trade.MeshData.mutable_attribute(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
    :raise AttributeError: If :ref:`vertex_data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`
    :raise NotImplementedError: if :ref:`attribute_array_size()` for given
        attribute isn't :py:`0`
.. py:function:: magnum.trade.MeshData.mutable_attribute(self, name: magnum.trade.MeshAttribute, id: int, morph_target_id: int)
    :raise KeyError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`name` and :p:`morph_target_id`
    :raise AttributeError: If :ref:`vertex_data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`
    :raise NotImplementedError: if :ref:`attribute_array_size()` for given
        attribute isn't :py:`0`

.. py:enum:: magnum.trade.MaterialLayer

    The equivalent to C++ :dox:`Trade::materialLayerName()` is the ``string``
    property, as ``name`` is reserved for the Python enum name.

    ..
        >>> from magnum import trade

    .. code:: pycon

        >>> layer = trade.MaterialLayer.CLEAR_COAT
        >>> layer.name
        'CLEAR_COAT'
        >>> layer.string
        'ClearCoat'

.. py:enum:: magnum.trade.MaterialAttribute

    The equivalent to C++ :dox:`Trade::materialAttributeName()` is the
    ``string`` property, as ``name`` is reserved for the Python enum name.

    ..
        >>> from magnum import trade

    .. code:: pycon

        >>> attribute = trade.MaterialAttribute.BASE_COLOR_TEXTURE_MATRIX
        >>> attribute.name
        'BASE_COLOR_TEXTURE_MATRIX'
        >>> attribute.string
        'BaseColorTextureMatrix'

.. py:enum:: magnum.trade.MaterialTextureSwizzle

    The ``component_count`` property matches :dox:`Trade::materialTextureSwizzleComponentCount()`.

    ..
        >>> from magnum import trade

    .. code:: pycon

        >>> trade.MaterialTextureSwizzle.GA.component_count
        2

.. py:class:: magnum.trade.MaterialData

    :TODO: remove this line once m.css stops ignoring first caption on a page

    `Attribute data access`_
    ========================

    The class makes use of Python's dynamic nature and provides direct access
    to attribute data in their concrete types via :ref:`attribute()`:

    ..
        >>> import os
        >>> from magnum import trade
        >>> importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        >>> importer.open_file('../../src/python/magnum/test/material.gltf')

    .. code:: pycon

        >>> material = importer.material(0)
        >>> material.attribute(trade.MaterialAttribute.BASE_COLOR)
        Vector(0.3, 0.4, 0.5, 0.8)
        >>> material.attribute(trade.MaterialAttribute.DOUBLE_SIDED)
        True

.. py:function:: magnum.trade.MaterialData.attribute_data_offset(self, layer: int)
    :raise IndexError: If :p:`layer` is negative or *greater* than
        :ref:`layer_count`
.. py:function:: magnum.trade.MaterialData.layer_id(self, layer: str)
    :raise KeyError: If :p:`layer` doesn't exist
.. py:function:: magnum.trade.MaterialData.layer_id(self, layer: magnum.trade.MaterialLayer)
    :raise KeyError: If :p:`layer` doesn't exist
.. py:function:: magnum.trade.MaterialData.layer_name(self, layer: int)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`

.. py:function:: magnum.trade.MaterialData.layer_factor(self, layer: int)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
.. py:function:: magnum.trade.MaterialData.layer_factor(self, layer: str)
    :raise KeyError: If :p:`layer` doesn't exist
.. py:function:: magnum.trade.MaterialData.layer_factor(self, layer: magnum.trade.MaterialLayer)
    :raise KeyError: If :p:`layer` doesn't exist

.. py:function:: magnum.trade.MaterialData.layer_factor_texture(self, layer: int)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.layer_factor_texture(self, layer: str)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.layer_factor_texture(self, layer: magnum.trade.MaterialLayer)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`

.. py:function:: magnum.trade.MaterialData.layer_factor_texture_swizzle(self, layer: int)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.layer_factor_texture_swizzle(self, layer: str)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.layer_factor_texture_swizzle(self, layer: magnum.trade.MaterialLayer)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`

.. py:function:: magnum.trade.MaterialData.layer_factor_texture_matrix(self, layer: int)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.layer_factor_texture_matrix(self, layer: str)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.layer_factor_texture_matrix(self, layer: magnum.trade.MaterialLayer)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`

.. py:function:: magnum.trade.MaterialData.layer_factor_texture_coordinates(self, layer: int)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.layer_factor_texture_coordinates(self, layer: str)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.layer_factor_texture_coordinates(self, layer: magnum.trade.MaterialLayer)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`

.. py:function:: magnum.trade.MaterialData.layer_factor_texture_layer(self, layer: int)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.layer_factor_texture_layer(self, layer: str)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.layer_factor_texture_layer(self, layer: magnum.trade.MaterialLayer)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :ref:`MaterialAttribute.LAYER_FACTOR_TEXTURE` isn't
        present in :p:`layer`

.. py:function:: magnum.trade.MaterialData.attribute_count(self, layer: int)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
.. py:function:: magnum.trade.MaterialData.attribute_count(self, layer: str)
    :raise KeyError: If :p:`layer` doesn't exist
.. py:function:: magnum.trade.MaterialData.attribute_count(self, layer: magnum.trade.MaterialLayer)
    :raise KeyError: If :p:`layer` doesn't exist

.. py:function:: magnum.trade.MaterialData.has_attribute(self, layer: int, name: str)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
.. py:function:: magnum.trade.MaterialData.has_attribute(self, layer: int, name: magnum.trade.MaterialAttribute)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
.. py:function:: magnum.trade.MaterialData.has_attribute(self, layer: str, name: str)
    :raise KeyError: If :p:`layer` doesn't exist
.. py:function:: magnum.trade.MaterialData.has_attribute(self, layer: str, name: magnum.trade.MaterialAttribute)
    :raise KeyError: If :p:`layer` doesn't exist
.. py:function:: magnum.trade.MaterialData.has_attribute(self, layer: magnum.trade.MaterialLayer, name: str)
    :raise KeyError: If :p:`layer` doesn't exist
.. py:function:: magnum.trade.MaterialData.has_attribute(self, layer: magnum.trade.MaterialLayer, name: magnum.trade.MaterialAttribute)
    :raise KeyError: If :p:`layer` doesn't exist

.. py:function:: magnum.trade.MaterialData.attribute_id(self, layer: int, name: str)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_id(self, layer: int, name: magnum.trade.MaterialAttribute)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_id(self, layer: str, name: str)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_id(self, layer: str, name: magnum.trade.MaterialAttribute)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_id(self, layer: magnum.trade.MaterialLayer, name: str)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_id(self, layer: magnum.trade.MaterialLayer, name: magnum.trade.MaterialAttribute)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_id(self, name: str)
    :raise KeyError: If :p:`name` isn't present in the base material
.. py:function:: magnum.trade.MaterialData.attribute_id(self, name: magnum.trade.MaterialAttribute)
    :raise KeyError: If :p:`name` isn't present in the base material

.. py:function:: magnum.trade.MaterialData.attribute_name(self, layer: int, id: int)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_name(self, layer: str, id: int)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_name(self, layer: magnum.trade.MaterialLayer, id: int)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_name(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`

.. py:function:: magnum.trade.MaterialData.attribute_type(self, layer: int, id: int)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_type(self, layer: int, name: str)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_type(self, layer: int, name: magnum.trade.MaterialAttribute)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_type(self, layer: str, id: int)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_type(self, layer: str, name: str)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_type(self, layer: str, name: magnum.trade.MaterialAttribute)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_type(self, layer: magnum.trade.MaterialLayer, id: int)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_type(self, layer: magnum.trade.MaterialLayer, name: str)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_type(self, layer: magnum.trade.MaterialLayer, name: magnum.trade.MaterialAttribute)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute_type(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
.. py:function:: magnum.trade.MaterialData.attribute_type(self, name: str)
    :raise KeyError: If :p:`name` isn't present in the base material
.. py:function:: magnum.trade.MaterialData.attribute_type(self, name: magnum.trade.MaterialAttribute)
    :raise KeyError: If :p:`name` isn't present in the base material

.. py:function:: magnum.trade.MaterialData.attribute(self, layer: int, id: int)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute(self, layer: int, name: str)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute(self, layer: int, name: magnum.trade.MaterialAttribute)
    :raise IndexError: If :p:`layer` is negative or not less than
        :ref:`layer_count`
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute(self, layer: str, id: int)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute(self, layer: str, name: str)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute(self, layer: str, name: magnum.trade.MaterialAttribute)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute(self, layer: magnum.trade.MaterialLayer, id: int)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute(self, layer: magnum.trade.MaterialLayer, name: str)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute(self, layer: magnum.trade.MaterialLayer, name: magnum.trade.MaterialAttribute)
    :raise KeyError: If :p:`layer` doesn't exist
    :raise KeyError: If :p:`name` isn't present in :p:`layer`
.. py:function:: magnum.trade.MaterialData.attribute(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
.. py:function:: magnum.trade.MaterialData.attribute(self, name: str)
    :raise KeyError: If :p:`name` isn't present in the base material
.. py:function:: magnum.trade.MaterialData.attribute(self, name: magnum.trade.MaterialAttribute)
    :raise KeyError: If :p:`name` isn't present in the base material

.. py:enum:: magnum.trade.SceneField

    The equivalent to C++ :dox:`Trade::sceneFieldCustom()` is creating an enum
    value using a ``CUSTOM()`` named constructor. The ``is_custom``
    property then matches :dox:`Trade::isSceneFieldCustom()` and you can
    retrieve the custom ID again with a ``custom_value`` property.

    ..
        >>> from magnum import trade

    .. code:: pycon

        >>> attribute = trade.SceneField.CUSTOM(17)
        >>> attribute.name
        'CUSTOM(17)'
        >>> attribute.is_custom
        True
        >>> attribute.custom_value
        17

.. py:class:: magnum.trade.SceneFieldData

    Associates a pair of typed data views with a name, type and other scene
    field properties, which can be subsequently put into a :ref:`SceneData`
    instance, for example with :ref:`scenetools.combine_fields()`. The mapping
    data view is always one-dimensional. The field data view can be either
    one-dimensional, for example a NumPy array:

    ..
        Just to verify the snippet below actually works (don't want the arrows
        shown in the docs, want to have it nicely wrapped)
    ..
        >>> mapping_data = array.array('I', [0, 2, 7])
        >>> field_data = np.array([(-0.5, 0.0), (+0.5, 0.0), ( 0.0, 0.5)], dtype='2f')
        >>> translations = trade.SceneFieldData(trade.SceneField.TRANSLATION, trade.SceneMappingType.UNSIGNED_INT, mapping_data, trade.SceneFieldType.VECTOR2, field_data)

    .. code:: py

        mapping_data = array.array('I', [0, 2, 7])
        field_data = np.array([(-0.5, 0.0),
                               (+0.5, 0.0),
                               ( 0.0, 0.5)], dtype='2f')
        translations = trade.SceneFieldData(trade.SceneField.TRANSLATION,
            trade.SceneMappingType.UNSIGNED_INT, mapping_data,
            trade.SceneFieldType.VECTOR2, field_data)

    Or it can be two-dimensional, for example by expanding a flat array into a
    list of two-component vectors:

    ..
        Again to verify the snippet below actually works
    ..
        >>> field_data = array.array('f', [-0.5, 0.0, +0.5, 0.0, 0.0, 0.5])
        >>> translations = trade.SceneFieldData(trade.SceneField.TRANSLATION, trade.SceneMappingType.UNSIGNED_INT, mapping_data, trade.SceneFieldType.VECTOR2, containers.StridedArrayView1D(field_data).expanded(0, (3, 2)))

    .. code:: py

        field_data = array.array('f', [-0.5, 0.0,
                                       +0.5, 0.0,
                                        0.0, 0.5])
        translations = trade.SceneFieldData(trade.SceneField.TRANSLATION,
            trade.SceneMappingType.UNSIGNED_INT, mapping_data,
            trade.SceneFieldType.VECTOR2,
            containers.StridedArrayView1D(field_data).expanded(0, (3, 2)))

    `Memory ownership and reference counting`_
    ==========================================

    On initialization, the instance inherits the
    :ref:`containers.StridedArrayView1D.owner <corrade.containers.StridedArrayView1D.owner>`
    objects of both views, storing it in the :ref:`mapping_owner` and
    :ref:`field_owner` fields, meaning that calling :py:`del` on the original
    data will *not* invalidate the instance.

    `Data access`_
    ==============

    Similarly to :ref:`SceneData`, the class makes use of Python's dynamic
    nature and provides direct access to attribute data in their concrete type
    via :ref:`mapping_data` and :ref:`field_data`. However, the
    :ref:`SceneFieldData` is considered a low level API and thus a
    :ref:`containers.StridedArrayView2D <corrade.containers.StridedArrayView2D>`
    / :ref:`containers.StridedBitArrayView2D <corrade.containers.StridedBitArrayView2D>`
    is returned always for field data, even for non-array attributes. The
    returned views inherit the :ref:`mapping_owner` or :ref:`field_owner` and
    element access coverts to a type corresponding to a particular
    :ref:`SceneMappingType` or :ref:`SceneFieldType`. For example, extracting
    the data from the :py:`translations` field created above:

    .. code:: pycon

        >>> mapping = translations.mapping_data
        >>> field = translations.field_data
        >>> mapping.owner is mapping_data
        True
        >>> field.owner is field_data
        True
        >>> mapping[2]
        7
        >>> field[2][0]
        Vector(0, 0.5)

.. py:function:: magnum.trade.SceneFieldData.__init__(self, name: magnum.trade.SceneField, mapping_type: magnum.trade.SceneMappingType, mapping_data: corrade.containers.StridedArrayView1D, field_type: magnum.trade.SceneFieldType, field_data: corrade.containers.StridedArrayView1D, field_array_size: int, flags: magnum.trade.SceneFieldFlags)
    :raise AssertionError: If :p:`mapping_data` and :p:`field_data` don't have
        the same size
    :raise AssertionError: If :p:`field_type` is not valid for :p:`name`
    :raise AssertionError: If :p:`field_type` is a string type or
        :ref:`SceneFieldType.BIT`
    :raise AssertionError: If :p:`mapping_data` stride doesn't fit into 16 bits
    :raise AssertionError: If :p:`mapping_data` format size is smaller than
        size of :p:`mapping_type`
    :raise AssertionError: If :p:`field_data` stride doesn't fit into 16 bits
    :raise AssertionError: If :p:`field_data` format size is smaller than size
        of :p:`field_type` at given :p:`field_array_size`
    :raise AssertionError: If :p:`field_array_size` is non-zero and :p:`name`
        can't be an array field
    :raise AssertionError: If :p:`flags` contain
        :ref:`SceneFieldFlags.OFFSET_ONLY`,
        :ref:`SceneFieldFlags.NULL_TERMINATED_STRING` or values disallowed for
        a particular :p:`name`
.. py:function:: magnum.trade.SceneFieldData.__init__(self, name: magnum.trade.SceneField, mapping_type: magnum.trade.SceneMappingType, mapping_data: corrade.containers.StridedArrayView1D, field_type: magnum.trade.SceneFieldType, field_data: corrade.containers.StridedArrayView2D, field_array_size: int, flags: magnum.trade.SceneFieldFlags)
    :raise AssertionError: If :p:`mapping_data` and first dimension of
        :p:`field_data` don't have the same size
    :raise AssertionError: If :p:`field_type` is not valid for :p:`name`
    :raise AssertionError: If :p:`field_type` is a string type or
        :ref:`SceneFieldType.BIT`
    :raise AssertionError: If :p:`mapping_data` stride doesn't fit into 16 bits
    :raise AssertionError: If :p:`mapping_data` format size is smaller than
        size of :p:`mapping_type`
    :raise AssertionError: If :p:`field_data` first dimension stride doesn't
        fit into 16 bits
    :raise AssertionError: If :p:`field_data` second dimension isn't contiguous
    :raise AssertionError: If :p:`field_data` format size times second
        dimension size is smaller than size of :p:`field_type` at given :p:`field_array_size`
    :raise AssertionError: If :p:`field_array_size` is non-zero and :p:`name`
        can't be an array field
    :raise AssertionError: If :p:`flags` contain
        :ref:`SceneFieldFlags.OFFSET_ONLY`,
        :ref:`SceneFieldFlags.NULL_TERMINATED_STRING` or values disallowed for
        a particular :p:`name`

.. py:function:: magnum.trade.SceneFieldData.__init__(self, name: magnum.trade.SceneField, mapping_type: magnum.trade.SceneMappingType, mapping_data: corrade.containers.StridedArrayView1D, field_data: corrade.containers.StridedBitArrayView1D, flags: magnum.trade.SceneFieldFlags)
    :raise AssertionError: If :p:`mapping_data` and :p:`field_data` don't have
        the same size
    :raise AssertionError: If :ref:`SceneFieldType.BIT` is not valid for
        :p:`name`
    :raise AssertionError: If :p:`mapping_data` stride doesn't fit into 16 bits
    :raise AssertionError: If :p:`mapping_data` format size is smaller than
        size of :p:`mapping_type`
    :raise AssertionError: If :p:`field_data` stride doesn't fit into 16 bits
    :raise AssertionError: If :p:`flags` contain
        :ref:`SceneFieldFlags.OFFSET_ONLY`,
        :ref:`SceneFieldFlags.NULL_TERMINATED_STRING` or values disallowed for
        a particular :p:`name`
.. py:function:: magnum.trade.SceneFieldData.__init__(self, name: magnum.trade.SceneField, mapping_type: magnum.trade.SceneMappingType, mapping_data: corrade.containers.StridedArrayView1D, field_data: corrade.containers.StridedBitArrayView2D, flags: magnum.trade.SceneFieldFlags)
    :raise AssertionError: If :p:`mapping_data` and first dimension of
        :p:`field_data` don't have the same size
    :raise AssertionError: If :ref:`SceneFieldType.BIT` is not valid for
        :p:`name`
    :raise AssertionError: If :p:`mapping_data` stride doesn't fit into 16 bits
    :raise AssertionError: If :p:`mapping_data` format size is smaller than
        size of :p:`mapping_type`
    :raise AssertionError: If :p:`field_data` first dimension stride doesn't
        fit into 16 bits
    :raise AssertionError: If :p:`field_data` second dimension isn't contiguous
    :raise AssertionError: If :p:`flags` contain
        :ref:`SceneFieldFlags.OFFSET_ONLY`,
        :ref:`SceneFieldFlags.NULL_TERMINATED_STRING` or values disallowed for
        a particular :p:`name`

.. py:property:: magnum.trade.SceneFieldData.field_data
    :raise NotImplementedError: If :ref:`field_type` is a half-float or string
        type

    A :ref:`containers.StridedArrayView2D <corrade.containers.StridedArrayView2D>`
    or :ref:`containers.StridedBitArrayView2D <corrade.containers.StridedBitArrayView2D>`
    is returned always, non-array attributes have the second dimension size
    :py:`1`.

.. py:class:: magnum.trade.SceneData

    :TODO: remove this line once m.css stops ignoring first caption on a page

    `Memory ownership and reference counting`_
    ==========================================

    The class can be both owning an non-owning depending on the value of
    :ref:`data_flags`. If they contain neither :ref:`DataFlags.OWNED` nor
    :ref:`DataFlags.GLOBAL`, the :ref:`owner` property references the object
    actually owning the data the scene points to. This ensures calling
    :py:`del` on the original object will *not* invalidate the data.

    `Field data access`_
    ====================

    The class makes use of Python's dynamic nature and provides direct access
    to mapping and field data in their concrete types via :ref:`mapping()`
    and :ref:`field()`. The returned views point to the underlying scene data,
    element access coverts to a type corresponding to a particular
    :ref:`SceneFieldType` and for performance-oriented access the view
    implements a buffer protocol with a corresponding type annotation:

    ..
        >>> import os
        >>> from magnum import trade
        >>> importer = trade.ImporterManager().load_and_instantiate('GltfImporter')
        >>> importer.open_file('../../src/python/magnum/test/scene.gltf')

    .. code:: pycon

        >>> scene = importer.scene(0)
        >>> list(scene.mapping(trade.SceneField.TRANSLATION))
        [1, 3, 0]
        >>> list(scene.field(trade.SceneField.TRANSLATION))
        [Vector(1, 2, 3), Vector(4, 5, 6), Vector(7, 8, 9)]
        >>> np.array(scene.field(trade.SceneField.TRANSLATION), copy=False)[1]
        array([4., 5., 6.], dtype=float32)

    Depending on the value of :ref:`data_flags` it's also possible to access
    the data in a mutable way via :ref:`mutable_mapping()` and
    :ref:`mutable_field()`.

.. py:function:: magnum.trade.SceneData.field_name
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`field_count`
.. py:function:: magnum.trade.SceneData.field_flags(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`field_count`
.. py:function:: magnum.trade.SceneData.field_flags(self, name: magnum.trade.SceneField)
    :raise KeyError: If :p:`name` does not exist
.. py:function:: magnum.trade.SceneData.field_type(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`field_count`
.. py:function:: magnum.trade.SceneData.field_type(self, name: magnum.trade.SceneField)
    :raise KeyError: If :p:`name` does not exist
.. py:function:: magnum.trade.SceneData.field_size(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`field_count`
.. py:function:: magnum.trade.SceneData.field_size(self, name: magnum.trade.SceneField)
    :raise KeyError: If :p:`name` does not exist
.. py:function:: magnum.trade.SceneData.field_array_size(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`field_count`
.. py:function:: magnum.trade.SceneData.field_array_size(self, name: magnum.trade.SceneField)
    :raise KeyError: If :p:`name` does not exist
.. py:function:: magnum.trade.SceneData.field_id
    :raise KeyError: If :p:`name` does not exist
.. py:function:: magnum.trade.SceneData.field_object_offset(self, field_id: int, object: int, offset: int)
    :raise IndexError: If :p:`field_id` is negative or not less than
        :ref:`field_count`
    :raise IndexError: If :p:`object` is negative or not less than
        :ref:`mapping_bound`
    :raise IndexError: If :p:`offset` is negative or larger than
        :ref:`field_size()` for given field
    :raise LookupError: If :p:`object` is not found
.. py:function:: magnum.trade.SceneData.field_object_offset(self, field_name: magnum.trade.SceneField, object: int, offset: int)
    :raise KeyError: If :p:`field_name` does not exist
    :raise IndexError: If :p:`object` is negative or not less than
        :ref:`mapping_bound`
    :raise IndexError: If :p:`offset` is negative or larger than
        :ref:`field_size()` for given field
    :raise LookupError: If :p:`object` is not found
.. py:function:: magnum.trade.SceneData.has_field_object(self, field_id: int, object: int)
    :raise IndexError: If :p:`field_id` is negative or not less than
        :ref:`field_count`
    :raise IndexError: If :p:`object` is negative or not less than
        :ref:`mapping_bound`
.. py:function:: magnum.trade.SceneData.has_field_object(self, field_name: magnum.trade.SceneField, object: int)
    :raise KeyError: If :p:`field_name` does not exist
    :raise IndexError: If :p:`object` is negative or not less than
        :ref:`mapping_bound`
.. py:function:: magnum.trade.SceneData.mapping(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`field_count`
.. py:function:: magnum.trade.SceneData.mapping(self, name: magnum.trade.SceneField)
    :raise KeyError: If :p:`name` does not exist
.. py:function:: magnum.trade.SceneData.mutable_mapping(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`field_count`
    :raise AttributeError: If :ref:`data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`
.. py:function:: magnum.trade.SceneData.mutable_mapping(self, name: magnum.trade.SceneField)
    :raise KeyError: If :p:`name` does not exist
    :raise AttributeError: If :ref:`data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`
.. py:function:: magnum.trade.SceneData.field(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`field_count`
    :raise NotImplementedError: If :ref:`field_array_size()` for given field is
        not :py:`0`
    :raise NotImplementedError: If :ref:`field_type()` for given field is a
        string type
.. py:function:: magnum.trade.SceneData.field(self, name: magnum.trade.SceneField)
    :raise KeyError: If :p:`name` does not exist
    :raise NotImplementedError: If :ref:`field_array_size()` for given field is
        not :py:`0`
    :raise NotImplementedError: If :ref:`field_type()` for given field is a
        string type
.. py:function:: magnum.trade.SceneData.mutable_field(self, id: int)
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`field_count`
    :raise AttributeError: If :ref:`data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`
    :raise NotImplementedError: If :ref:`field_array_size()` for given field is
        not :py:`0`
    :raise NotImplementedError: If :ref:`field_type()` for given field is a
        string type
.. py:function:: magnum.trade.SceneData.mutable_field(self, name: magnum.trade.SceneField)
    :raise KeyError: If :p:`name` does not exist
    :raise AttributeError: If :ref:`data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`
    :raise NotImplementedError: If :ref:`field_array_size()` for given field is
        not :py:`0`
    :raise NotImplementedError: If :ref:`field_type()` for given field is a
        string type

.. py:class:: magnum.trade.ImporterManager
    :summary: Manager for :ref:`AbstractImporter` plugin instances

    Each plugin returned by :ref:`instantiate()` or :ref:`load_and_instantiate()`
    references its owning :ref:`ImporterManager` through
    :ref:`AbstractImporter.manager`, ensuring the manager is not deleted before
    the plugin instances are.

.. TODO couldn't the plugin_interface etc. docs be parsed from pybind's docs?
    repeating them for every plugin is annoying

.. py:class:: magnum.trade.AbstractImporter
    :data plugin_interface: Plugin interface string
    :data plugin_search_paths: Plugin search paths
    :data plugin_suffix: Plugin suffix
    :data plugin_metadata_suffix: Plugin metadata suffix

    Similarly to C++, importer plugins are loaded through :ref:`ImporterManager`:

    ..
        >>> from magnum import trade

    .. code:: py

        >>> manager = trade.ImporterManager()
        >>> importer = manager.load_and_instantiate('PngImporter')

    Unlike C++, errors in both API usage and file parsing are reported by
    raising an exception. See particular function documentation for detailed
    behavior.

.. py:function:: magnum.trade.AbstractImporter.open_data
    :raise AssertionError: If :ref:`trade.ImporterFeatures.OPEN_DATA` is not
        supported
    :raise RuntimeError: If file opening fails

.. py:function:: magnum.trade.AbstractImporter.open_file
    :raise RuntimeError: If file opening fails

    For compatibility with :ref:`os.path`, on Windows this function converts
    all backslashes in :p:`filename` to forward slashes before passing it to
    :dox:`Trade::AbstractImporter::openFile()`, which expects forward slashes
    as directory separators on all platforms.

.. py:property:: magnum.trade.AbstractImporter.default_scene
    :raise AssertionError: If no file is opened
.. py:property:: magnum.trade.AbstractImporter.scene_count
    :raise AssertionError: If no file is opened
.. py:property:: magnum.trade.AbstractImporter.object_count
    :raise AssertionError: If no file is opened
.. py:function:: magnum.trade.AbstractImporter.scene_for_name
    :raise AssertionError: If no file is opened
.. py:function:: magnum.trade.AbstractImporter.object_for_name
    :raise AssertionError: If no file is opened
.. py:function:: magnum.trade.AbstractImporter.scene_name
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`id` is negative or not less than :ref:`scene_count`
.. py:function:: magnum.trade.AbstractImporter.object_name
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`id` is negative or not less than :ref:`object_count`

.. py:function:: magnum.trade.AbstractImporter.scene(self, id: int)
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If scene import fails
    :raise IndexError: If :p:`id` is negative or not less than :ref:`scene`
.. py:function:: magnum.trade.AbstractImporter.scene(self, name: str)
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If scene import fails
    :raise KeyError: If :p:`name` was not found

.. py:property:: magnum.trade.AbstractImporter.mesh_count
    :raise AssertionError: If no file is opened
.. py:function:: magnum.trade.AbstractImporter.mesh_level_count
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`id` is negative or not less than :ref:`mesh_count`
.. py:function:: magnum.trade.AbstractImporter.mesh_for_name
    :raise AssertionError: If no file is opened
.. py:function:: magnum.trade.AbstractImporter.mesh_name
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`id` is negative or not less than :ref:`mesh_count`

.. py:function:: magnum.trade.AbstractImporter.mesh(self, id: int, level: int)
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If mesh import fails
    :raise IndexError: If :p:`id` is negative or not less than :ref:`mesh_count`
    :raise IndexError: If :p:`level` is negative or not less than
        :ref:`mesh_level_count()` for this mesh

.. py:function:: magnum.trade.AbstractImporter.mesh(self, name: str, level: int)
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If mesh import fails
    :raise KeyError: If :p:`name` was not found
    :raise IndexError: If :p:`level` is negative or not less than
        :ref:`mesh_level_count()` for this mesh

.. py:property:: magnum.trade.AbstractImporter.material_count
    :raise AssertionError: If no file is opened
.. py:function:: magnum.trade.AbstractImporter.material_for_name
    :raise AssertionError: If no file is opened
.. py:function:: magnum.trade.AbstractImporter.material_name
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`id` is negative or not less than :ref:`material_count`

.. py:function:: magnum.trade.AbstractImporter.material(self, id: int)
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If material import fails
    :raise IndexError: If :p:`id` is negative or not less than :ref:`material_count`
.. py:function:: magnum.trade.AbstractImporter.material(self, name: str)
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If material import fails
    :raise KeyError: If :p:`name` was not found

.. py:property:: magnum.trade.AbstractImporter.texture_count
    :raise AssertionError: If no file is opened
.. py:function:: magnum.trade.AbstractImporter.texture_for_name
    :raise AssertionError: If no file is opened
.. py:function:: magnum.trade.AbstractImporter.texture_name
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`id` is negative or not less than :ref:`texture_count`

.. py:function:: magnum.trade.AbstractImporter.texture(self, id: int)
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If texture import fails
    :raise IndexError: If :p:`id` is negative or not less than :ref:`texture_count`
.. py:function:: magnum.trade.AbstractImporter.texture(self, name: str)
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If texture import fails
    :raise KeyError: If :p:`name` was not found

.. py:property:: magnum.trade.AbstractImporter.image1d_count
    :raise AssertionError: If no file is opened
.. py:property:: magnum.trade.AbstractImporter.image2d_count
    :raise AssertionError: If no file is opened
.. py:property:: magnum.trade.AbstractImporter.image3d_count
    :raise AssertionError: If no file is opened

.. py:function:: magnum.trade.AbstractImporter.image1d_level_count
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`image1d_count`
.. py:function:: magnum.trade.AbstractImporter.image2d_level_count
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`image2d_count`
.. py:function:: magnum.trade.AbstractImporter.image3d_level_count
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`image3d_count`

.. py:function:: magnum.trade.AbstractImporter.image1d_for_name
    :raise AssertionError: If no file is opened
.. py:function:: magnum.trade.AbstractImporter.image2d_for_name
    :raise AssertionError: If no file is opened
.. py:function:: magnum.trade.AbstractImporter.image3d_for_name
    :raise AssertionError: If no file is opened

.. py:function:: magnum.trade.AbstractImporter.image1d_name
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`image1d_count`
.. py:function:: magnum.trade.AbstractImporter.image2d_name
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`image2d_count`
.. py:function:: magnum.trade.AbstractImporter.image3d_name
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`image3d_count`

.. py:function:: magnum.trade.AbstractImporter.image1d(self, id: int, level: int)
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If image import fails
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`image1d_count`
    :raise IndexError: If :p:`level` is negative or not less than
        :ref:`image1d_level_count()` for this image
.. py:function:: magnum.trade.AbstractImporter.image1d(self, name: str, level: int)
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If image import fails
    :raise KeyError: If :p:`name` was not found
    :raise IndexError: If :p:`level` is negative or not less than
        :ref:`image1d_level_count()` for this image
.. py:function:: magnum.trade.AbstractImporter.image2d(self, id: int, level: int)
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If image import fails
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`image2d_count`
    :raise IndexError: If :p:`level` is negative or not less than
        :ref:`image2d_level_count()` for this image
.. py:function:: magnum.trade.AbstractImporter.image2d(self, name: str, level: int)
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If image import fails
    :raise KeyError: If :p:`name` was not found
    :raise IndexError: If :p:`level` is negative or not less than
        :ref:`image2d_level_count()` for this image
.. py:function:: magnum.trade.AbstractImporter.image3d(self, id: int, level: int)
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If image import fails
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`image3d_count`
    :raise IndexError: If :p:`level` is negative or not less than
        :ref:`image3d_level_count()` for this image
.. py:function:: magnum.trade.AbstractImporter.image3d(self, name: str, level: int)
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If image import fails
    :raise KeyError: If :p:`name` was not found
    :raise IndexError: If :p:`level` is negative or not less than
        :ref:`image3d_level_count()` for this image

.. py:class:: magnum.trade.ImageConverterManager
    :summary: Manager for :ref:`AbstractImageConverter` plugin instances

    Each plugin returned by :ref:`instantiate()` or :ref:`load_and_instantiate()`
    references its owning :ref:`ImageConverterManager` through
    :ref:`AbstractImageConverter.manager`, ensuring the manager is not deleted
    before the plugin instances are.

.. TODO couldn't the plugin_interface etc. docs be parsed from pybind's docs?
    repeating them for every plugin is annoying

.. py:class:: magnum.trade.AbstractImageConverter
    :data plugin_interface: Plugin interface string
    :data plugin_search_paths: Plugin search paths
    :data plugin_suffix: Plugin suffix
    :data plugin_metadata_suffix: Plugin metadata suffix

    Similarly to C++, image converter plugins are loaded through
    :ref:`ImageConverterManager`:

    ..
        >>> from magnum import trade

    .. code:: py

        >>> manager = trade.ImageConverterManager()
        >>> converter = manager.load_and_instantiate('PngImageConverter')

    Unlike C++, errors in both API usage and file parsing are reported by
    raising an exception. See particular function documentation for detailed
    behavior.

.. py:function:: magnum.trade.AbstractImageConverter.convert(self, image: magnum.trade.ImageData1D)
    :raise RuntimeError: If image conversion fails
.. py:function:: magnum.trade.AbstractImageConverter.convert(self, image: magnum.trade.ImageData2D)
    :raise RuntimeError: If image conversion fails
.. py:function:: magnum.trade.AbstractImageConverter.convert(self, image: magnum.trade.ImageData3D)
    :raise RuntimeError: If image conversion fails
.. py:function:: magnum.trade.AbstractImageConverter.convert(self, image: magnum.ImageView1D)
    :raise RuntimeError: If image conversion fails
.. py:function:: magnum.trade.AbstractImageConverter.convert(self, image: magnum.ImageView2D)
    :raise RuntimeError: If image conversion fails
.. py:function:: magnum.trade.AbstractImageConverter.convert(self, image: magnum.ImageView3D)
    :raise RuntimeError: If image conversion fails
.. py:function:: magnum.trade.AbstractImageConverter.convert(self, image: magnum.CompressedImageView1D)
    :raise RuntimeError: If image conversion fails
.. py:function:: magnum.trade.AbstractImageConverter.convert(self, image: magnum.CompressedImageView2D)
    :raise RuntimeError: If image conversion fails
.. py:function:: magnum.trade.AbstractImageConverter.convert(self, image: magnum.CompressedImageView3D)
    :raise RuntimeError: If image conversion fails

.. py:function:: magnum.trade.AbstractImageConverter.convert_to_file(self, image: magnum.trade.ImageData1D, filename: str)
    :raise RuntimeError: If image conversion fails

    For compatibility with :ref:`os.path`, on Windows this function converts
    all backslashes in :p:`filename` to forward slashes before passing it to
    :dox:`Trade::AbstractImageConverter::convertToFile()`, which expects
    forward slashes as directory separators on all platforms.

.. py:function:: magnum.trade.AbstractImageConverter.convert_to_file(self, image: magnum.trade.ImageData2D, filename: str)
    :raise RuntimeError: If image conversion fails

    For compatibility with :ref:`os.path`, on Windows this function converts
    all backslashes in :p:`filename` to forward slashes before passing it to
    :dox:`Trade::AbstractImageConverter::convertToFile()`, which expects
    forward slashes as directory separators on all platforms.

.. py:function:: magnum.trade.AbstractImageConverter.convert_to_file(self, image: magnum.trade.ImageData3D, filename: str)
    :raise RuntimeError: If image conversion fails

    For compatibility with :ref:`os.path`, on Windows this function converts
    all backslashes in :p:`filename` to forward slashes before passing it to
    :dox:`Trade::AbstractImageConverter::convertToFile()`, which expects
    forward slashes as directory separators on all platforms.

.. py:function:: magnum.trade.AbstractImageConverter.convert_to_file(self, image: magnum.ImageView1D, filename: str)
    :raise RuntimeError: If image conversion fails

    For compatibility with :ref:`os.path`, on Windows this function converts
    all backslashes in :p:`filename` to forward slashes before passing it to
    :dox:`Trade::AbstractImageConverter::convertToFile()`, which expects
    forward slashes as directory separators on all platforms.

.. py:function:: magnum.trade.AbstractImageConverter.convert_to_file(self, image: magnum.ImageView2D, filename: str)
    :raise RuntimeError: If image conversion fails

    For compatibility with :ref:`os.path`, on Windows this function converts
    all backslashes in :p:`filename` to forward slashes before passing it to
    :dox:`Trade::AbstractImageConverter::convertToFile()`, which expects
    forward slashes as directory separators on all platforms.

.. py:function:: magnum.trade.AbstractImageConverter.convert_to_file(self, image: magnum.ImageView3D, filename: str)
    :raise RuntimeError: If image conversion fails

    For compatibility with :ref:`os.path`, on Windows this function converts
    all backslashes in :p:`filename` to forward slashes before passing it to
    :dox:`Trade::AbstractImageConverter::convertToFile()`, which expects
    forward slashes as directory separators on all platforms.

.. py:function:: magnum.trade.AbstractImageConverter.convert_to_file(self, image: magnum.CompressedImageView1D, filename: str)
    :raise RuntimeError: If image conversion fails

    For compatibility with :ref:`os.path`, on Windows this function converts
    all backslashes in :p:`filename` to forward slashes before passing it to
    :dox:`Trade::AbstractImageConverter::convertToFile()`, which expects
    forward slashes as directory separators on all platforms.

.. py:function:: magnum.trade.AbstractImageConverter.convert_to_file(self, image: magnum.CompressedImageView2D, filename: str)
    :raise RuntimeError: If image conversion fails

    For compatibility with :ref:`os.path`, on Windows this function converts
    all backslashes in :p:`filename` to forward slashes before passing it to
    :dox:`Trade::AbstractImageConverter::convertToFile()`, which expects
    forward slashes as directory separators on all platforms.

.. py:function:: magnum.trade.AbstractImageConverter.convert_to_file(self, image: magnum.CompressedImageView3D, filename: str)
    :raise RuntimeError: If image conversion fails

    For compatibility with :ref:`os.path`, on Windows this function converts
    all backslashes in :p:`filename` to forward slashes before passing it to
    :dox:`Trade::AbstractImageConverter::convertToFile()`, which expects
    forward slashes as directory separators on all platforms.

.. py:class:: magnum.trade.SceneConverterManager
    :summary: Manager for :ref:`AbstractSceneConverter` plugin instances

    Each plugin returned by :ref:`instantiate()` or :ref:`load_and_instantiate()`
    references its owning :ref:`SceneConverterManager` through
    :ref:`AbstractSceneConverter.manager`, ensuring the manager is not deleted
    before the plugin instances are.

.. py:enum:: magnum.trade.SceneContents

    The equivalent to C++ :dox:`Trade::sceneContentsFor()` is creating an enum
    value using a ``FOR()`` named constructor, passing either an
    :ref:`AbstractSceneConverter` or an opened :ref:`AbstractImporter` to it.

.. TODO couldn't the plugin_interface etc. docs be parsed from pybind's docs?
    repeating them for every plugin is annoying

.. py:class:: magnum.trade.AbstractSceneConverter
    :data plugin_interface: Plugin interface string
    :data plugin_search_paths: Plugin search paths
    :data plugin_suffix: Plugin suffix
    :data plugin_metadata_suffix: Plugin metadata suffix

    Similarly to C++, image converter plugins are loaded through
    :ref:`SceneConverterManager`:

    ..
        >>> from magnum import trade

    .. code:: py

        >>> manager = trade.SceneConverterManager()
        >>> converter = manager.load_and_instantiate('StanfordSceneConverter')

    Unlike C++, errors in both API usage and file parsing are reported by
    raising an exception. See particular function documentation for detailed
    behavior.

.. py:function:: magnum.trade.AbstractSceneConverter.convert(self, mesh: magnum.trade.MeshData)
    :raise AssertionError: If :ref:`trade.SceneConverterFeatures.CONVERT_MESH`
        is not supported
    :raise RuntimeError: If conversion fails

.. py:function:: magnum.trade.AbstractSceneConverter.convert_in_place(self, mesh: magnum.trade.MeshData)
    :raise AssertionError: If :ref:`trade.SceneConverterFeatures.CONVERT_MESH_IN_PLACE`
        is not supported
    :raise RuntimeError: If conversion fails

.. py:function:: magnum.trade.AbstractSceneConverter.convert_to_file(self, mesh: magnum.trade.MeshData, filename: str)
    :raise AssertionError: If neither
        :ref:`SceneConverterFeatures.CONVERT_MESH_TO_FILE` nor the
        combination of :ref:`SceneConverterFeatures.CONVERT_MULTIPLE_TO_FILE`
        and :ref:`SceneConverterFeatures.ADD_MESHES`
        is supported
    :raise RuntimeError: If conversion fails

    For compatibility with :ref:`os.path`, on Windows this function converts
    all backslashes in :p:`filename` to forward slashes before passing it to
    :dox:`Trade::AbstractSceneConverter::convertToFile()`, which expects
    forward slashes as directory separators on all platforms.

.. py:function:: magnum.trade.AbstractSceneConverter.begin_file
    :raise AssertionError: If neither
        :ref:`SceneConverterFeatures.CONVERT_MULTIPLE_TO_FILE` nor
        :ref:`SceneConverterFeatures.CONVERT_MESH_TO_FILE` is supported
    :raise RuntimeError: If beginning the conversion fails

    For compatibility with :ref:`os.path`, on Windows this function converts
    all backslashes in :p:`filename` to forward slashes before passing it to
    :dox:`Trade::AbstractSceneConverter::beginFile()`, which expects forward
    slashes as directory separators on all platforms.

.. py:function:: magnum.trade.AbstractSceneConverter.end_file
    :raise AssertionError: If no conversion is in progress
    :raise RuntimeError: If ending the conversion fails

.. py:property:: magnum.trade.AbstractSceneConverter.mesh_count
    :raise AssertionError: If no conversion is in progress

.. py:property:: magnum.trade.AbstractSceneConverter.material_count
    :raise AssertionError: If no conversion is in progress

.. py:property:: magnum.trade.AbstractSceneConverter.scene_count
    :raise AssertionError: If no conversion is in progress

.. py:function:: magnum.trade.AbstractSceneConverter.add(self, scene: magnum.trade.SceneData, name: str)
    :raise AssertionError: If :ref:`SceneConverterFeatures.ADD_SCENES` is not
        supported
    :raise AssertionError: If no conversion is in progress
    :raise RuntimeError: If adding the data fails

.. py:function:: magnum.trade.AbstractSceneConverter.add(self, mesh: magnum.trade.MeshData, name: str)
    :raise AssertionError: If :ref:`SceneConverterFeatures.ADD_MESHES` is not
        supported, or alternatively at least one of
        :ref:`SceneConverterFeatures.CONVERT_MESH`,
        :ref:`SceneConverterFeatures.CONVERT_MESH_TO_DATA` or
        :ref:`SceneConverterFeatures.CONVERT_MESH_TO_FILE` is not supported
    :raise AssertionError: If no conversion is in progress
    :raise RuntimeError: If adding the data fails

.. py:function:: magnum.trade.AbstractSceneConverter.add(self, material: magnum.trade.MaterialData, name: str)
    :raise AssertionError: If :ref:`SceneConverterFeatures.ADD_MATERIALS` is
        not supported
    :raise AssertionError: If no conversion is in progress
    :raise RuntimeError: If adding the data fails

.. py:function:: magnum.trade.AbstractSceneConverter.add(self, image: magnum.trade.ImageData2D, name: str)
    :raise AssertionError: If :ref:`ImageData2D.is_compressed` is :py:`False`
        and :ref:`SceneConverterFeatures.ADD_IMAGES2D` is not supported
    :raise AssertionError: If :ref:`ImageData2D.is_compressed` is :py:`True`
        and :ref:`SceneConverterFeatures.ADD_COMPRESSED_IMAGES2D` is not
        supported
    :raise AssertionError: If no conversion is in progress
    :raise RuntimeError: If adding the data fails

.. py:function:: magnum.trade.AbstractSceneConverter.set_mesh_attribute_name
    :raise AssertionError: If none of
        :ref:`SceneConverterFeatures.ADD_MESHES`,
        :ref:`SceneConverterFeatures.CONVERT_MESH`,
        :ref:`SceneConverterFeatures.CONVERT_MESH_IN_PLACE`,
        :ref:`SceneConverterFeatures.CONVERT_MESH_TO_DATA` or
        :ref:`SceneConverterFeatures.CONVERT_MESH_TO_FILE` is supported
    :raise AssertionError: If no conversion is in progress
    :raise AssertionError: If :p:`attribute` is not custom

.. py:function:: magnum.trade.AbstractSceneConverter.set_default_scene
    :raise AssertionError: If :ref:`trade.SceneConverterFeatures.ADD_SCENES`
        is not supported
    :raise AssertionError: If no conversion is in progress
    :raise AssertionError: If :p:`id` is negative or not less than
        :ref:`scene_count`

.. py:function:: magnum.trade.AbstractSceneConverter.set_scene_field_name
    :raise AssertionError: If :ref:`trade.SceneConverterFeatures.ADD_SCENES`
        is not supported
    :raise AssertionError: If no conversion is in progress
    :raise AssertionError: If :p:`field` is not custom

.. py:function:: magnum.trade.AbstractSceneConverter.add_importer_contents
    :raise AssertionError: If no conversion is in progress
    :raise RuntimeError: If adding the importer contents fails

.. py:function:: magnum.trade.AbstractSceneConverter.add_supported_importer_contents
    :raise AssertionError: If :p:`importer` is not opened
    :raise AssertionError: If no conversion is in progress
    :raise RuntimeError: If adding the importer contents fails
