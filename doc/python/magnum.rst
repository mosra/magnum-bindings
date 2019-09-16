..
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
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

.. roles used for all other docs

.. doctest setup
    >>> from magnum import *

.. py:module:: magnum
    :data BUILD_STATIC: Static library build
    :data TARGET_GL: OpenGL interoperability
    :data TARGET_GLES: OpenGL ES target
    :data TARGET_GLES2: OpenGL ES 2.0 target
    :data TARGET_WEBGL: WebGL target
    :data TARGET_VK: Vulkan interoperability

.. py:class:: magnum.ImageView1D

    See `ImageView2D` for more information.

.. py:class:: magnum.ImageView2D

    TODO: remove this once m.css stops ignoring the first caption on a page
    #######################################################################

    `Memory ownership and reference counting`_
    ==========================================

    Similarly to `corrade.containers.ArrayView` (and unlike in C++), the view
    keeps a reference to the original memory owner object in the `owner` field,
    meaning that calling :py:`del` on the original object will *not* invalidate
    the view. Slicing a view creates a new view referencing the same original
    object, without any dependency on the previous view. That means a long
    chained slicing operation will not cause increased memory usage.

    The `owner` is :py:`None` if the view is empty.

.. py:class:: magnum.ImageView3D

    See `ImageView2D` for more information.

.. py:class:: magnum.MutableImageView1D

    See `ImageView2D` for more information.

.. py:class:: magnum.MutableImageView2D

    See `ImageView2D` for more information.

.. py:class:: magnum.MutableImageView3D

    See `ImageView2D` for more information.
