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

Changelog
#########

:ref-prefix:
    corrade
    magnum
:some_directive: TODO: why can't the title below "just work" and I have to
    add some bogus content before?

`Changes since 2020.06`_
========================

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
-   Exposed :ref:`Color3.from_xyz()`, :ref:`Color3.to_xyz()` and equivalent
    APIs on :ref:`Color4`
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
    :gh:`mosra/magnum-bindings#16`)
    Travis banned everyone from using their CI and so all Linux and macOS
    builds were migrated from Travis to Circle CI. See also
    :gh:`mosra/magnum#350` and :gh:`mosra/magnum#523`.
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
    :ref:`platform.sdl2.Application.MouseMoveEvent.relative_position` and
    :ref:`platform.glfw.Application.MouseMoveEvent.relative_position`
-   Fixed :ref:`platform.sdl2.Application.InputEvent.Modifier` and
    :ref:`platform.glfw.Application.InputEvent.Modifier` to behave properly
    as flags and not just as an enum
-   Exposed :ref:`platform.sdl2.Application.viewport_event` and
    :ref:`platform.glfw.Application.viewport_event` and a possibility
    to make the window resizable on startup
-   Exposed :ref:`platform.sdl2.Application.exit_event` and
    :ref:`platform.glfw.Application.exit_event`
-   Exposed :ref:`platform.glfw.Application.swap_interval` and
    :ref:`platform.glfw.Application.main_loop_iteration`
-   Exposed a basic interface of :ref:`trade.AbstractImageConverter` and
    :ref:`trade.AbstractSceneConverter`
-   Exposed :ref:`Color3.red()` and other convenience constructors (see
    :gh:`mosra/magnum-bindings#12`)
-   Exposed the :ref:`text` library
-   Fixed issues with an in-source build (see :gh:`mosra/magnum-bindings#13`)
-   All CMake build options are now prefixed with ``MAGNUM_``. For backwards
    compatibility, unless ``MAGNUM_BUILD_DEPRECATED`` is disabled and unless a
    prefixed option is already set during the initial run, the unprefixed
    options are still recognized with a warning. See also :gh:`mosra/corrade#139`.
-   Added a ``MAGNUM_PYTHON_BINDINGS_STATIC_PLUGINS`` CMake option for linking
    static plugins to the Python bindings module. See the
    :ref:`building documentation <std:doc:building>` for more information.

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
