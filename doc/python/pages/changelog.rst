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

Changelog
#########

:ref-prefix:
    corrade
    magnum
:some_directive: TODO: why can't the title below "just work" and I have to
    add some bogus content before?

`Changes since 2020.06`_
========================

-   Minimal supported CMake version is now 3.5, changed from 3.4, since CMake
    3.27+ warns if a compatibility with CMake below 3.5 is requested. Older
    versions are not supported anymore and all workarounds for them were
    removed. This is a conservative change, as there are no known supported
    distributions which would have anything older than 3.5.
-   Exposed the :ref:`corrade.BUILD_DEPRECATED` and
    :ref:`magnum.BUILD_DEPRECATED` constants, as some features may work
    differently depending on these being enabled or not and it's useful to be
    able to query this
-   Exposed the new :ref:`containers.BitArray`, :ref:`containers.BitArrayView`,
    :ref:`containers.StridedBitArrayView1D` containers, their mutable and
    multi-dimensional counterparts as well as the
    :ref:`containers.StridedArrayView*D.slice_bit() <containers.StridedArrayView1D.slice_bit()>`
    utility
-   Exposed missing :ref:`Vector4` constructor from a :ref:`Vector3` and a
    W component and :ref:`Vector3` from :ref:`Vector2` and a Z component
-   Renamed :py:`Matrix3.from()` / :py:`Matrix4.from()` to :ref:`Matrix3.from_()`
    / :ref:`Matrix4.from_()` because :py:`from` is a Python keyword and it
    would be silly to have to write :py:`getattr(Matrix4, 'from')` just to use
    these APIs
-   Exposed newly added off-center variants of
    :ref:`Matrix4.orthographic_projection()` and
    :ref:`Matrix3.projection()`
-   Exposed remaining vector/scalar, exponential and other functions in the
    :ref:`math <magnum.math>` library
-   Exposed the :ref:`CompressedPixelFormat` enum, various pixel-format-related
    helper APIs are now properties on :ref:`PixelFormat` and
    :ref:`CompressedPixelFormat`
-   Exposed :ref:`CompressedImage2D`, :ref:`CompressedImageView2D`,
    :ref:`MutableCompressedImageView2D` and their 1D and 3D counterparts
-   Exposed :ref:`Color3.from_xyz()`, :ref:`Color3.from_linear_rgb_int()`,
    :ref:`Color3.to_xyz()`, :ref:`Color3.to_linear_rgb_int()` and equivalent
    APIs on :ref:`Color4`
-   Exposed unsigned :ref:`Range1Dui`, :ref:`Range2Dui` and :ref:`Range3Dui`
    types in addition to the signed variants
-   Exposed new :ref:`Quaternion.rotation()`, :ref:`Quaternion.reflection()`,
    :ref:`Quaternion.reflect_vector()`, :ref:`Quaternion.xyzw` and
    :ref:`Quaternion.wxyz` APIs
-   Exposed :ref:`gl.Context` and its platform-specific subclasses for EGL, WGL
    and GLX
-   Exposed :ref:`gl.Framebuffer.attach_texture()` and missing sRGB, depth
    and stencil :ref:`gl.TextureFormat` values (see :gh:`mosra/magnum-bindings#14`)
-   Exposed :ref:`gl.Renderer.set_blend_function()`,
    :ref:`gl.Renderer.set_blend_equation()` and related enums (see :gh:`mosra/magnum-bindings#9`)
-   Exposed :ref:`gl.Renderer.Feature.CLIP_DISTANCEn <gl.Renderer.Feature.CLIP_DISTANCE0>`
    values that are new since 2020.06
-   Exposed new instancing, texture transformation, normal-mapping-related and
    lighting features in :ref:`shaders.PhongGL`
-   Exposed new instancing and texture transformation features in
    :ref:`shaders.FlatGL2D` and :ref:`shaders.FlatGL3D`
-   Exposed :ref:`shaders.DistanceFieldVectorGL2D`,
    :ref:`shaders.DistanceFieldVectorGL3D`, :ref:`shaders.VectorGL2D` and
    :ref:`shaders.VectorGL3D` shaders
-   Renamed all helper ``Python.h`` headers to ``PythonBindings.h`` to avoid
    issues with shitty IDE indexers such as Eclipse, confusing these with
    Python's ``<Python.h>``
-   Minor performance fixes (see :gh:`mosra/magnum-bindings#10`,
    :gh:`mosra/magnum-bindings#15`,
    :gh:`mosra/magnum-bindings#16`,
    :gh:`mosra/magnum-bindings#17`,
    :gh:`mosra/magnum-bindings#19`,
    :gh:`mosra/magnum-bindings#20`)
    Travis banned everyone from using their CI and so all Linux and macOS
    builds were migrated from Travis to Circle CI. See also
    :gh:`mosra/magnum#350` and :gh:`mosra/magnum#523`.
-   It's now possible to use ``<PackageName>_ROOT`` to point to install
    locations of dependencies such as Corrade on CMake 3.12+, in addition to
    putting them all together inside ``CMAKE_PREFIX_PATH``. See also
    :gh:`mosra/magnum#614`.
-   On CMake 3.16 and newer, ``FindMagnumBindings.cmake`` can provide
    additional details if some component is not found
-   The Homebrew package now uses ``std_cmake_args`` instead of hardcoded build
    type and install prefix, which resolves certain build issues (see
    :gh:`mosra/homebrew-magnum#6`)
-   Added a caster for :dox:`Containers::Optional`, allowing it to be used
    directly in function signatures and showing up on the Python side as either
    :py:`None` or the actual value
-   Various documentation fixes (see :gh:`mosra/magnum-bindings#11`)
-   Fixed copypaste errors in bindings for :ref:`Range2D.center_x()` /
    :ref:`Range2D.center_y()`, :ref:`Range3D.z()`, :ref:`Range3D.center_x()` /
    :ref:`Range3D.center_y()` / :ref:`Range3D.center_z()`
-   Fixed a copypaste error in
    :ref:`platform.sdl2.Application.PointerMoveEvent.relative_position` and
    :ref:`platform.glfw.Application.PointerMoveEvent.relative_position`
-   Fixed :ref:`platform.sdl2.Application.Modifier` and
    :ref:`platform.glfw.Application.Modifier` to behave properly
    as flags and not just as an enum
-   Exposed :ref:`meshtools.compress_indices()`, :ref:`meshtools.concatenate()`,
    :ref:`meshtools.copy()`, :ref:`meshtools.duplicate()`,
    :ref:`meshtools.filter_attributes()`,
    :ref:`meshtools.filter_except_attributes()`,
    :ref:`meshtools.filter_only_attributes()`,
    :ref:`meshtools.generate_indices()`, :ref:`meshtools.interleave()`,
    :ref:`meshtools.remove_duplicates()`,
    :ref:`meshtools.remove_duplicates_fuzzy()`, :ref:`meshtools.transform2d()`,
    :ref:`meshtools.transform2d_in_place()`, :ref:`meshtools.transform3d()`,
    :ref:`meshtools.transform3d_in_place()`,
    :ref:`meshtools.transform_texture_coordinates2d()` and
    :ref:`meshtools.transform_texture_coordinates2d_in_place()`
-   Exposed :ref:`platform.sdl2.Application.viewport_event` and
    :ref:`platform.glfw.Application.viewport_event` and a possibility
    to make the window resizable on startup
-   Exposed :ref:`platform.sdl2.Application.exit_event` and
    :ref:`platform.glfw.Application.exit_event`
-   Exposed :ref:`platform.sdl2.Application.dpi_scaling` and
    :ref:`platform.glfw.Application.dpi_scaling`
-   Exposed :ref:`platform.glfw.Application.swap_interval` and
    :ref:`platform.glfw.Application.main_loop_iteration`
-   Exposed :ref:`platform.sdl2.Application.cursor` and
    :ref:`platform.sdl2.Application.warp_cursor`, same for GLFW
-   Exposed :ref:`platform.sdl2.Application.is_key_pressed()` and
    :ref:`platform.glfw.Application.is_key_pressed()`
-   Exposed all :ref:`platform.sdl2.Application.Configuration.WindowFlags` and
    :ref:`platform.glfw.Application.Configuration.WindowFlags`
-   Exposed the new :ref:`primitives.CubeFlags`
-   Exposed the new :ref:`text.AbstractShaper`, :ref:`text.RendererCore`,
    :ref:`text.Renderer`, :ref:`text.RendererGL` classes as well as the new
    :ref:`text.Feature`, :ref:`text.Script` enums and the
    :ref:`text.FeatureRange` helper, plus more :ref:`text.Alignment` options
-   Exposed :ref:`trade.AbstractImporter.features` and
    :ref:`trade.AbstractImporter.flags` and corresponding enums
-   Exposed a basic interface of :ref:`trade.AbstractImageConverter` and
    :ref:`trade.AbstractSceneConverter`
-   Exposed the whole interface of :ref:`trade.MeshData` and
    :ref:`trade.MeshAttributeData` including typed access to index and
    attribute data, together with :ref:`VertexFormat`, :ref:`trade.DataFlags`,
    :ref:`trade.AbstractImporter.mesh_attribute_name()` and
    :ref:`trade.AbstractImporter.mesh_attribute_for_name()`
-   Exposed the whole interface of :ref:`trade.MaterialData` including typed
    access to attribute data, together with
    :ref:`trade.AbstractImporter.material()` and related importer APIs
-   Exposed the whole interface of :ref:`trade.SceneData` and
    :ref:`trade.SceneFieldData` including typed access to mapping and field
    data, together with :ref:`trade.AbstractImporter.scene()` and related
    importer APIs
-   Exposed :ref:`Color3.red()` and other convenience constructors (see
    :gh:`mosra/magnum-bindings#12`)
-   Exposed the :ref:`materialtools`, :ref:`scenetools` and :ref:`text`
    libraries
-   Exposed :ref:`utility.copy()` for convenient, fast and safe copying of
    multi-dimensional strided arrays
-   Exposed the minimal interface of :ref:`utility.ConfigurationGroup` and
    :ref:`utility.Configuration`
-   Exposed :ref:`pluginmanager.AbstractManager.set_preferred_plugins()`,
    :ref:`pluginmanager.AbstractManager.register_external_manager()`, the base
    :ref:`pluginmanager.AbstractPlugin` class and
    :ref:`pluginmanager.PluginMetadata`
-   Fixed issues with an in-source build (see :gh:`mosra/magnum-bindings#13`)
-   All CMake build options are now prefixed with ``MAGNUM_``. For backwards
    compatibility, unless ``MAGNUM_BUILD_DEPRECATED`` is disabled and unless a
    prefixed option is already set during the initial run, the unprefixed
    options are still recognized with a warning. See also :gh:`mosra/corrade#139`.
-   Added a ``MAGNUM_PYTHON_BINDINGS_STATIC_PLUGINS`` CMake option for linking
    static plugins to the Python bindings module. See the
    :ref:`building documentation <std:doc:building>` for more information.
-   Added a ``MAGNUM_BUILD_PYTHON_BINDINGS_RTLD_GLOBAL`` CMake option to make
    the Python bindings module loaded into the global namespace instead of
    isolated in order to attempt to solve problems with duplicated globals when
    static builds of Corrade and Magnum are linked into multiple dynamic
    modules. See the :ref:`building documentation <std:doc:building>` for more
    information.

`2020.06`_
==========

Released 2020-06-27, tagged as
:gh:`v2020.06 <mosra/magnum-bindings/releases/tag/v2020.06>`.

-   Exposed :ref:`Matrix4.cofactor()`, :ref:`Matrix4.comatrix()`,
    :ref:`Matrix4.adjugate()` (and equivalents in other matrix sizes), and
    :ref:`Matrix4.normal_matrix()`
-   Exposed :ref:`gl.AbstractFramebuffer.blit()` functions and related enums
-   Exposed more keys in :ref:`platform.sdl2.Application` and
    :ref:`platform.glfw.Application`
-   Exposed :ref:`gl.AbstractTexture.unbind()`
-   Exposed :ref:`trade.AbstractImporter.image2d_level_count()` and related
    APIs for 1D and 3D
-   Exposed :ref:`trade.MeshData` and related APIs, the previous
    ``trade.MeshData3D`` APIs are removed
-   Exposed new APIs and tangent support in the :ref:`primitives` library
-   :ref:`platform.sdl2.Application` and :ref:`platform.glfw.Application` now
    provide a clear error instead of "pure virtual method call" in case
    ``draw_event()`` is not implemented
-   Library version is now exposed through ``MAGNUMBINDINGS_VERSION_YEAR``,
    ``MAGNUMBINDINGS_VERSION_MONTH``, ``MAGNUMBINDINGS_VERSION_COMMIT``,
    ``MAGNUMBINDINGS_VERSION_HASH`` and ``MAGNUMBINDINGS_VERSION_STRING``
    preprocessor defines in a new ``Magnum/versionBindings.h`` header. This
    header is not included by any other header to avoid trigerring a full
    rebuild when Git commit changes. If Git is not found, only the first two
    defines are present.

`2019.10`_
==========

Released 2019-10-24, tagged as
:gh:`v2019.10 <mosra/magnum-bindings/releases/tag/v2019.10>`.

Initial version. See :gh:`mosra/magnum#228`, :gh:`mosra/magnum-bindings#1`,
:gh:`mosra/magnum-bindings#2` and :gh:`mosra/magnum-bindings#6` for more
information.
