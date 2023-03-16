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

.. roles used for all other docs

.. doctest setup
    >>> from magnum import *

.. py:module:: magnum
    :data BUILD_DEPRECATED: Build with deprecated features enabled
    :data BUILD_STATIC: Static library build
    :data TARGET_GL: OpenGL interoperability
    :data TARGET_GLES: OpenGL ES target
    :data TARGET_GLES2: OpenGL ES 2.0 target
    :data TARGET_WEBGL: WebGL target
    :data TARGET_EGL: EGL target
    :data TARGET_VK: Vulkan interoperability

.. py:class:: magnum.Image1D

    See :ref:`Image2D` for more information.

.. py:class:: magnum.Image2D

    An owning counterpart to :ref:`ImageView2D` / :ref:`MutableImageView2D`.
    Holds its own data buffer, thus doesn't have an equivalent to
    :ref:`ImageView2D.owner`. The :ref:`data` and :ref:`pixels` views allow
    mutable access. Implicitly convertible to :ref:`ImageView2D` /
    :ref:`MutableImageView2D`, so all APIs consuming image views work with this
    type as well.

.. py:class:: magnum.Image3D

    See :ref:`Image2D` for more information.

.. py:class:: magnum.ImageView1D

    See :ref:`ImageView2D` for more information.

.. py:class:: magnum.ImageView2D

    :TODO: remove this line once m.css stops ignoring first caption on a page

    `Memory ownership and reference counting`_
    ==========================================

    Similarly to :ref:`corrade.containers.ArrayView` (and unlike in C++), the
    view keeps a reference to the original memory owner object in the
    :ref:`owner` field, meaning that calling :py:`del` on the original object
    will *not* invalidate the view. Slicing a view creates a new view
    referencing the same original object, without any dependency on the
    previous view. That means a long chained slicing operation will not cause
    increased memory usage.

    The :ref:`owner` is :py:`None` if the view is empty.

    `Pixel data access`_
    ====================

    The class makes use of Python's dynamic nature and provides direct access
    to pixel data in their concrete types via :ref:`pixels`. The returned views
    point to the underlying image data, element access coverts to a type
    corresponding to a particular :ref:`PixelFormat` and for
    performance-oriented access the view implements a buffer protocol with a
    corresponding type annotation.

    Normalized formats (such as :ref:`PixelFormat.RGB8_UNORM` but also
    :ref:`PixelFormat.RGBA8_SRGB`) are unpacked to a corresponding
    floating-point representation in element access and packed from a
    floating-point representation in mutable acess. The type annotation is
    however still matching the original type (such as :py:`'3B'` / :py:`'4B'`
    in these cases), so code consuming these via the buffer protocol needs to
    handle the normalization explicitly if needed.

    ..
        >>> from magnum import *
        >>> import numpy as np
        >>> import array

    .. code:: pycon

        >>> data = array.array('B', [0xf3, 0x2a, 0x80, 0x23, 0x00, 0xff, 0x00, 0xff])
        >>> image = ImageView2D(PixelFormat.RGBA8_SRGB, (2, 1), data)
        >>> image.pixels[0, 0] # sRGB -> float conversion
        Vector(0.896269, 0.0231534, 0.215861, 0.137255)
        >>> np.array(image.pixels, copy=False)[0]
        array([[243,  42, 128,  35],
               [  0, 255,   0, 255]], dtype=uint8)

.. py:class:: magnum.ImageView3D

    See :ref:`ImageView2D` for more information.

.. py:class:: magnum.MutableImageView1D

    See :ref:`ImageView2D` for more information. The only difference to the
    non-mutable variant is that it's possible to modify the image through
    :ref:`data` and :ref:`pixels`.

.. py:class:: magnum.MutableImageView2D

    See :ref:`ImageView2D` for more information. The only difference to the
    non-mutable variant is that it's possible to modify the image through
    :ref:`data` and :ref:`pixels`.

.. py:class:: magnum.MutableImageView3D

    See :ref:`ImageView2D` for more information. The only difference to the
    non-mutable variant is that it's possible to modify the image through
    :ref:`data` and :ref:`pixels`.

.. py:function:: magnum.ImageView1D.__init__(self, arg0: magnum.ImageView1D)
    :raise RuntimeError: If :ref:`trade.ImageData1D.is_compressed` is :py:`True`

    This function is used to implement implicit conversion from
    :ref:`trade.ImageData1D` in the :ref:`trade` module.

.. py:function:: magnum.ImageView2D.__init__(self, arg0: magnum.ImageView2D)
    :raise RuntimeError: If :ref:`trade.ImageData2D.is_compressed` is :py:`True`

    This function is used to implement implicit conversion from
    :ref:`trade.ImageData2D` in the :ref:`trade` module.

.. py:function:: magnum.ImageView3D.__init__(self, arg0: magnum.ImageView3D)
    :raise RuntimeError: If :ref:`trade.ImageData3D.is_compressed` is :py:`True`

    This function is used to implement implicit conversion from
    :ref:`trade.ImageData3D` in the :ref:`trade` module.

.. py:function:: magnum.MutableImageView1D.__init__(self, arg0: magnum.MutableImageView1D)
    :raise RuntimeError: If :ref:`trade.ImageData1D.is_compressed` is :py:`True`

    This function is used to implement implicit conversion from
    :ref:`trade.ImageData1D` in the :ref:`trade` module.

.. py:function:: magnum.MutableImageView2D.__init__(self, arg0: magnum.MutableImageView2D)
    :raise RuntimeError: If :ref:`trade.ImageData2D.is_compressed` is :py:`True`

    This function is used to implement implicit conversion from
    :ref:`trade.ImageData2D` in the :ref:`trade` module.

.. py:function:: magnum.MutableImageView3D.__init__(self, arg0: magnum.MutableImageView3D)
    :raise RuntimeError: If :ref:`trade.ImageData3D.is_compressed` is :py:`True`

    This function is used to implement implicit conversion from
    :ref:`trade.ImageData3D` in the :ref:`trade` module.
