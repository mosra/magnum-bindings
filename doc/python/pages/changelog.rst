..
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020 Vladimír Vondruš <mosra@centrum.cz>

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

`2020.06`_
==========

Released 2020-06-27, tagged as
:gh:`v2020.06 <mosra/magnum-bindings/releases/tag/v2020.06>`.

-   Exposed `Matrix4.cofactor()`, `Matrix4.comatrix()`, `Matrix4.adjugate()`
    (and equivalents in other matrix sizes), and `Matrix4.normal_matrix()`
-   Exposed `gl.AbstractFramebuffer.blit()` functions and related enums
-   Exposed more keys in `platform.sdl2.Application` and
    `platform.glfw.Application`
-   Exposed `gl.AbstractTexture.unbind()`
-   Exposed `trade.AbstractImporter.image2d_level_count()` and related APIs for
    1D and 3D
-   Exposed `trade.MeshData` and related APIs, the previous
    ``trade.MeshData3D`` APIs are removed
-   Exposed new APIs and tangent support in the `primitives` library
-   `platform.sdl2.Application` and `platform.glfw.Application` now provide a
    clear error instead of "pure virtual method call" in case ``draw_event()``
    is not implemented
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
