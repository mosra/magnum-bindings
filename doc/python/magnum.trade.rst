..
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022 Vladimír Vondruš <mosra@centrum.cz>

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

.. py:function:: magnum.trade.MeshData.attribute_id
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
    :raise KeyError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`name`

    Compared to the C++ API, there's no
    :dox:`Trade::MeshData::findAttributeId()`, the desired workflow is instead
    calling :ref:`attribute_id()` and catching an exception if not found.

.. py:function:: magnum.trade.MeshData.attribute_format
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
    :raise KeyError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`name`
.. py:function:: magnum.trade.MeshData.attribute_offset
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
    :raise KeyError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`name`
.. py:function:: magnum.trade.MeshData.attribute_stride
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
    :raise KeyError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`name`
.. py:function:: magnum.trade.MeshData.attribute_array_size
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
    :raise KeyError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`name`
.. py:function:: magnum.trade.MeshData.attribute
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
    :raise KeyError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`name`
.. py:function:: magnum.trade.MeshData.mutable_attribute
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`attribute_count()`
    :raise KeyError: If :p:`id` is negative or not less than
        :ref:`attribute_count()` for :p:`name`
    :raise AttributeError: If :ref:`vertex_data_flags` doesn't contain
        :ref:`DataFlags.MUTABLE`

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
.. py:function:: magnum.trade.SceneData.field_flags
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`field_count`
    :raise KeyError: If :p:`name` does not exist
.. py:function:: magnum.trade.SceneData.field_type
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`field_count`
    :raise KeyError: If :p:`name` does not exist
.. py:function:: magnum.trade.SceneData.field_size
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`field_count`
    :raise KeyError: If :p:`name` does not exist
.. py:function:: magnum.trade.SceneData.field_array_size
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`field_count`
    :raise KeyError: If :p:`name` does not exist
.. py:function:: magnum.trade.SceneData.field_id
    :raise KeyError: If :p:`name` does not exist
.. py:function:: magnum.trade.SceneData.field_object_offset
    :raise IndexError: If :p:`field_id` is negative or not less than
        :ref:`field_count`
    :raise KeyError: If :p:`field_name` does not exist
    :raise IndexError: If :p:`object` is negative or not less than
        :ref:`mapping_bound`
    :raise IndexError: If :p:`offset` is negative or larger than
        :ref:`field_size()` for given field
    :raise LookupError: If :p:`object` is not found
.. py:function:: magnum.trade.SceneData.has_field_object
    :raise IndexError: If :p:`field_id` is negative or not less than
        :ref:`field_count`
    :raise KeyError: If :p:`field_name` does not exist
    :raise IndexError: If :p:`object` is negative or not less than
        :ref:`mapping_bound`

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

.. TODO this needs distinction by parameter names, at least

.. py:function:: magnum.trade.AbstractImporter.scene
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If scene import fails
    :raise IndexError: If :p:`id` is negative or not less than :ref:`scene`
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

.. TODO this needs distinction by parameter names, at least

.. py:function:: magnum.trade.AbstractImporter.mesh
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If mesh import fails
    :raise IndexError: If :p:`id` is negative or not less than :ref:`mesh_count`
    :raise KeyError: If :p:`name` was not found

.. py:property:: magnum.trade.AbstractImporter.texture_count
    :raise AssertionError: If no file is opened
.. py:function:: magnum.trade.AbstractImporter.texture_for_name
    :raise AssertionError: If no file is opened
.. py:function:: magnum.trade.AbstractImporter.texture_name
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`id` is negative or not less than :ref:`texture_count`

.. TODO this needs distinction by parameter names, at least

.. py:function:: magnum.trade.AbstractImporter.texture
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If texture import fails
    :raise IndexError: If :p:`id` is negative or not less than :ref:`texture_count`
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

.. TODO this needs distinction by parameter names, at least

.. py:function:: magnum.trade.AbstractImporter.image1d
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If image import fails
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`image1d_count`
    :raise KeyError: If :p:`name` was not found
.. py:function:: magnum.trade.AbstractImporter.image2d
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If image import fails
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`image2d_count`
    :raise KeyError: If :p:`name` was not found
.. py:function:: magnum.trade.AbstractImporter.image3d
    :raise AssertionError: If no file is opened
    :raise RuntimeError: If image import fails
    :raise IndexError: If :p:`id` is negative or not less than
        :ref:`image3d_count`
    :raise KeyError: If :p:`name` was not found

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

.. py:function:: magnum.trade.AbstractImageConverter.convert
    :raise RuntimeError: If image conversion fails

.. py:function:: magnum.trade.AbstractImageConverter.convert_to_file
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

.. py:function:: magnum.trade.AbstractSceneConverter.convert
    :raise AssertionError: If :ref:`trade.SceneConverterFeatures.CONVERT_MESH`
        is not supported
    :raise RuntimeError: If conversion fails

.. py:function:: magnum.trade.AbstractSceneConverter.convert_in_place
    :raise AssertionError: If :ref:`trade.SceneConverterFeatures.CONVERT_MESH_IN_PLACE`
        is not supported
    :raise RuntimeError: If conversion fails

.. py:function:: magnum.trade.AbstractSceneConverter.convert_to_file
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

.. py:property:: magnum.trade.AbstractSceneConverter.scene_count
    :raise AssertionError: If no conversion is in progress

.. py:function:: magnum.trade.AbstractSceneConverter.add
    :raise AssertionError: If corresponding
        :ref:`SceneConverterFeatures.ADD_* <SceneConverterFeatures>` is not
        supported, or alternatively at least one of
        :ref:`SceneConverterFeatures.CONVERT_MESH`,
        :ref:`SceneConverterFeatures.CONVERT_MESH_TO_DATA` or
        :ref:`SceneConverterFeatures.CONVERT_MESH_TO_FILE` is not supported
        for meshes
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
