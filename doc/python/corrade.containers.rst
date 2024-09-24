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

.. doctest setup
    >>> from corrade import containers
    >>> import array

.. py:class:: corrade.containers.ArrayView

    Provides an untyped one-dimensional read-only view on a contiguous memory
    range. Convertible both to and from Python objects supporting the Buffer
    Protocol. The buffer type is lost in the conversion process and the memory
    is always treated as plain bytes:

    .. code:: pycon

        >>> a = b'hello'
        >>> b = containers.ArrayView(a)
        >>> chr(b[2])
        'l'
        >>> bytes(b[1:4])
        b'ell'

    See the :ref:`StridedArrayView1D` and its multi-dimensional variants for an
    alternative that can provide typed access. The :ref:`ArrayView` is
    immutable, see :ref:`MutableArrayView` for the mutable alternative. All
    slicing operations are supported, specifying a non-trivial stride will
    return :ref:`StridedArrayView1D` instead of :ref:`ArrayView`.

    `Memory ownership and reference counting`_
    ==========================================

    Unlike in C++, the view keeps a reference to the original memory owner
    object in the :ref:`owner` field, meaning that calling :py:`del` on the
    original object will *not* invalidate the view. Slicing a view creates a
    new view referencing the same original object, without any dependency on
    the previous view. That means a long chained slicing operation will not
    cause increased memory usage.

    .. code:: pycon

        >>> b.owner is a
        True
        >>> b[1:4][:-1].owner is a
        True

    The :py:`owner` is :py:`None` if the view is empty.

    `Comparison to Python's memoryview`_
    ====================================

    The :ref:`ArrayView` class is equivalent to one-dimensional
    :ref:`memoryview` with a stride of :py:`1`. For multiple dimensions and
    non-trivial strides, :ref:`StridedArrayView1D` and friends provide a
    superset of :ref:`memoryview` features.

.. py:class:: corrade.containers.MutableArrayView

    Equivalent to :ref:`ArrayView`, but implementing :ref:`__setitem__()` as
    well.

.. py:class:: corrade.containers.StridedArrayView1D

    Provides a typed one-dimensional read-only view on a memory range with
    custom stride values. Convertible both to and from Python objects
    supporting the Buffer Protocol, preserving the dimensionality, type and
    stride information:

    .. code:: pycon

        >>> a = array.array('f', [2.5, 3.14, -1.75, 53.2])
        >>> b = containers.StridedArrayView1D(memoryview(a)[::2])
        >>> b[0]
        2.5
        >>> b[1]
        -1.75

    See :ref:`StridedArrayView2D`, :ref:`StridedArrayView3D`,
    :ref:`StridedArrayView4D`, :ref:`MutableStridedArrayView1D` and others for
    multi-dimensional and mutable equivalents.

    `Memory ownership and reference counting`_
    ==========================================

    Similarly to :ref:`ArrayView`, the view keeps a reference to the original
    memory owner object in the :ref:`owner` field. Slicing a view creates a
    new view referencing the same original object, without any dependency on
    the previous view. The :py:`owner` is :py:`None` if the view is empty.

    `Comparison to Python's memoryview`_
    ====================================

    The :ref:`StridedArrayView1D` and its multi-dimensional variants are
    equivalent to any :ref:`memoryview`, but additionally supporting
    multi-dimensional slicing as well (which raises :ref:`NotImplementedError`
    in Py3.7 :ref:`memoryview`).

.. py:function:: corrade.containers.StridedArrayView1D.__getitem__(self, i: int)
    :raise IndexError: If :p:`i` is out of range
    :raise NotImplementedError: If the view was created from a buffer and
        :ref:`format <StridedArrayView1D.format>` is not one of :py:`'b'`,
        :py:`'B'`, :py:`'h'`, :py:`'H'`, :py:`'i'`, :py:`'I'`, :py:`'q'`,
        :py:`'Q'`, :py:`'f'` or :py:`'d'`
.. py:function:: corrade.containers.StridedArrayView1D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0`
.. py:function:: corrade.containers.StridedArrayView1D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0`
.. py:function:: corrade.containers.StridedArrayView1D.expanded
    :raise IndexError: If :p:`dimension` is not :py:`0`
    :raise ValueError: If product of :p:`size` is not equal to size in
        :p:`dimension`

.. py:class:: corrade.containers.MutableStridedArrayView1D

    Equivalent to :ref:`StridedArrayView1D`, but implementing
    :ref:`__setitem__()` as well.

.. py:function:: corrade.containers.MutableStridedArrayView1D.__getitem__(self, i: int)
    :raise IndexError: If :p:`i` is out of range
    :raise NotImplementedError: If the view was created from a buffer and
        :ref:`format <MutableStridedArrayView1D.format>` is not one of
        :py:`'b'`, :py:`'B'`, :py:`'h'`, :py:`'H'`, :py:`'i'`, :py:`'I'`,
        :py:`'q'`, :py:`'Q'`, :py:`'f'` or :py:`'d'`
.. py:function:: corrade.containers.MutableStridedArrayView1D.__setitem__(self, i: int, value: handle)
    :raise IndexError: If :p:`i` is out of range
    :raise NotImplementedError: If the view was created from a buffer and
        :ref:`format <MutableStridedArrayView1D.format>` is not one of
        :py:`'b'`, :py:`'B'`, :py:`'h'`, :py:`'H'`, :py:`'i'`, :py:`'I'`,
        :py:`'q'`, :py:`'Q'`, :py:`'f'` or :py:`'d'`
.. py:function:: corrade.containers.MutableStridedArrayView1D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0`
.. py:function:: corrade.containers.MutableStridedArrayView1D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0`
.. py:function:: corrade.containers.MutableStridedArrayView1D.expanded
    :raise IndexError: If :p:`dimension` is not :py:`0`
    :raise ValueError: If product of :p:`size` is not equal to size in
        :p:`dimension`

.. py:class:: corrade.containers.StridedArrayView2D

    See :ref:`StridedArrayView1D` for more information.

.. py:function:: corrade.containers.StridedArrayView2D.__getitem__(self, i: tuple[int, int])
    :raise IndexError: If :p:`i` is out of range
    :raise NotImplementedError: If the view was created from a buffer and
        :ref:`format <StridedArrayView2D.format>` is not one of :py:`'b'`,
        :py:`'B'`, :py:`'h'`, :py:`'H'`, :py:`'i'`, :py:`'I'`, :py:`'q'`,
        :py:`'Q'`, :py:`'f'` or :py:`'d'`
.. py:function:: corrade.containers.StridedArrayView2D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0` or :py:`1`
.. py:function:: corrade.containers.StridedArrayView2D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0` or :py:`1`
.. py:function:: corrade.containers.StridedArrayView2D.transposed
    :raise IndexError: If :p:`a` or :p:`b` is not :py:`0` or :py:`1` or if
        they're the same
.. py:function:: corrade.containers.StridedArrayView2D.expanded
    :raise IndexError: If :p:`dimension` is not :py:`0` or :py:`1`
    :raise ValueError: If product of :p:`size` is not equal to size in
        :p:`dimension`

.. py:class:: corrade.containers.MutableStridedArrayView2D

    See :ref:`StridedArrayView1D` and :ref:`MutableStridedArrayView1D` for more
    information.

.. py:function:: corrade.containers.MutableStridedArrayView2D.__getitem__(self, i: tuple[int, int])
    :raise IndexError: If :p:`i` is out of range
    :raise NotImplementedError: If the view was created from a buffer and
        :ref:`format <MutableStridedArrayView2D.format>` is not one of
        :py:`'b'`, :py:`'B'`, :py:`'h'`, :py:`'H'`, :py:`'i'`, :py:`'I'`,
        :py:`'q'`, :py:`'Q'`, :py:`'f'` or :py:`'d'`
.. py:function:: corrade.containers.MutableStridedArrayView2D.__setitem__(self, i: tuple[int, int], value: handle)
    :raise IndexError: If :p:`i` is out of range
    :raise NotImplementedError: If the view was created from a buffer and
        :ref:`format <MutableStridedArrayView2D.format>` is not one of
        :py:`'b'`, :py:`'B'`, :py:`'h'`, :py:`'H'`, :py:`'i'`, :py:`'I'`,
        :py:`'q'`, :py:`'Q'`, :py:`'f'` or :py:`'d'`
.. py:function:: corrade.containers.MutableStridedArrayView2D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0` or :py:`1`
.. py:function:: corrade.containers.MutableStridedArrayView2D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0` or :py:`1`
.. py:function:: corrade.containers.MutableStridedArrayView2D.transposed
    :raise IndexError: If :p:`a` or :p:`b` is not :py:`0` or :py:`1` or if
        they're the same
.. py:function:: corrade.containers.MutableStridedArrayView2D.expanded
    :raise IndexError: If :p:`dimension` is not :py:`0` or :py:`1`
    :raise ValueError: If product of :p:`size` is not equal to size in
        :p:`dimension`

.. py:class:: corrade.containers.StridedArrayView3D

    See :ref:`StridedArrayView1D` for more information.

.. py:function:: corrade.containers.StridedArrayView3D.__getitem__(self, i: tuple[int, int, int])
    :raise IndexError: If :p:`i` is out of range
    :raise NotImplementedError: If the view was created from a buffer and
        :ref:`format <StridedArrayView3D.format>` is not one of :py:`'b'`,
        :py:`'B'`, :py:`'h'`, :py:`'H'`, :py:`'i'`, :py:`'I'`, :py:`'q'`,
        :py:`'Q'`, :py:`'f'` or :py:`'d'`
.. py:function:: corrade.containers.StridedArrayView3D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` or :py:`2`
.. py:function:: corrade.containers.StridedArrayView3D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` or :py:`2`
.. py:function:: corrade.containers.StridedArrayView3D.transposed
    :raise IndexError: If :p:`a` or :p:`b` is not :py:`0`, :py:`1` or :py:`2`
        or if  they're the same
.. py:function:: corrade.containers.StridedArrayView3D.expanded
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` or :py:`2`
    :raise ValueError: If product of :p:`size` is not equal to size in
        :p:`dimension`

.. py:class:: corrade.containers.MutableStridedArrayView3D

    See :ref:`StridedArrayView1D` and :ref:`MutableStridedArrayView1D` for more
    information.

.. py:function:: corrade.containers.MutableStridedArrayView3D.__getitem__(self, i: tuple[int, int, int])
    :raise IndexError: If :p:`i` is out of range
    :raise NotImplementedError: If the view was created from a buffer and
        :ref:`format <MutableStridedArrayView3D.format>` is not one of
        :py:`'b'`, :py:`'B'`, :py:`'h'`, :py:`'H'`, :py:`'i'`, :py:`'I'`,
        :py:`'q'`, :py:`'Q'`, :py:`'f'` or :py:`'d'`
.. py:function:: corrade.containers.MutableStridedArrayView3D.__setitem__(self, i: tuple[int, int, int], value: handle)
    :raise IndexError: If :p:`i` is out of range
    :raise NotImplementedError: If the view was created from a buffer and
        :ref:`format <MutableStridedArrayView3D.format>` is not one of
        :py:`'b'`, :py:`'B'`, :py:`'h'`, :py:`'H'`, :py:`'i'`, :py:`'I'`,
        :py:`'q'`, :py:`'Q'`, :py:`'f'` or :py:`'d'`
.. py:function:: corrade.containers.MutableStridedArrayView3D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` or :py:`2`
.. py:function:: corrade.containers.MutableStridedArrayView3D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` or :py:`2`
.. py:function:: corrade.containers.MutableStridedArrayView3D.transposed
    :raise IndexError: If :p:`a` or :p:`b` is not :py:`0`, :py:`1` or :py:`2`
        or if  they're the same
.. py:function:: corrade.containers.MutableStridedArrayView3D.expanded
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` or :py:`2`
    :raise ValueError: If product of :p:`size` is not equal to size in
        :p:`dimension`

.. py:class:: corrade.containers.StridedArrayView4D

    See :ref:`StridedArrayView1D` for more information.

.. py:function:: corrade.containers.StridedArrayView4D.__getitem__(self, i: tuple[int, int, int, int])
    :raise IndexError: If :p:`i` is out of range
    :raise NotImplementedError: If the view was created from a buffer and
        :ref:`format <StridedArrayView4D.format>` is not one of :py:`'b'`,
        :py:`'B'`, :py:`'h'`, :py:`'H'`, :py:`'i'`, :py:`'I'`, :py:`'q'`,
        :py:`'Q'`, :py:`'f'` or :py:`'d'`
.. py:function:: corrade.containers.StridedArrayView4D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` :py:`2` or
        :py:`3`
.. py:function:: corrade.containers.StridedArrayView4D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` :py:`2` or
        :py:`3`
.. py:function:: corrade.containers.StridedArrayView4D.transposed
    :raise IndexError: If :p:`a` or :p:`b` is not :py:`0`, :py:`1` :py:`2` or
        :py:`3` or if  they're the same

.. py:class:: corrade.containers.MutableStridedArrayView4D

    See :ref:`StridedArrayView1D` and :ref:`MutableStridedArrayView1D` for more
    information.

.. py:function:: corrade.containers.MutableStridedArrayView4D.__getitem__(self, i: tuple[int, int, int, int])
    :raise IndexError: If :p:`i` is out of range
    :raise NotImplementedError: If the view was created from a buffer and
        :ref:`format <MutableStridedArrayView4D.format>` is not one of
        :py:`'b'`, :py:`'B'`, :py:`'h'`, :py:`'H'`, :py:`'i'`, :py:`'I'`,
        :py:`'q'`, :py:`'Q'`, :py:`'f'` or :py:`'d'`
.. py:function:: corrade.containers.MutableStridedArrayView4D.__setitem__(self, i: tuple[int, int, int, int], value: handle)
    :raise IndexError: If :p:`i` is out of range
    :raise NotImplementedError: If the view was created from a buffer and
        :ref:`format <MutableStridedArrayView4D.format>` is not one of
        :py:`'b'`, :py:`'B'`, :py:`'h'`, :py:`'H'`, :py:`'i'`, :py:`'I'`,
        :py:`'q'`, :py:`'Q'`, :py:`'f'` or :py:`'d'`
.. py:function:: corrade.containers.MutableStridedArrayView4D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` :py:`2` or
        :py:`3`
.. py:function:: corrade.containers.MutableStridedArrayView4D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` :py:`2` or
        :py:`3`
.. py:function:: corrade.containers.MutableStridedArrayView4D.transposed
    :raise IndexError: If :p:`a` or :p:`b` is not :py:`0`, :py:`1` :py:`2` or
        :py:`3` or if  they're the same

.. py:class:: corrade.containers.BitArray

    An owning counterpart to :ref:`BitArrayView` / :ref:`MutableBitArrayView`.
    Holds its own data buffer, thus doesn't have an equivalent to
    :ref:`BitArrayView.owner`. Implicitly convertible to :ref:`BitArrayView`,
    :ref:`MutableBitArrayView`, :ref:`StridedBitArrayView1D` and
    :ref:`MutableStridedBitArrayView1D`, so all APIs consuming (strided) bit
    array views work with this type as well.

.. py:class:: corrade.containers.BitArrayView

    Comparex to an :ref:`ArrayView`, which operates with byte-sized types,
    provides a view on individual bits. Convertible from a :ref:`BitArrayView`.
    See :ref:`StridedBitArrayView1D` and others for more generic bit views. :ref:`BitArrayView` is immutable, see :ref:`MutableBitArrayView` for the
    mutable alternative. All slicing operations are supported, specifying a
    non-trivial stride will return a :ref:`StridedBitArrayView1D` instead of a :ref:`BitArrayView`.

    `Memory ownership and reference counting`_
    ==========================================

    Similarly to :ref:`ArrayView`, the view keeps a reference to the original
    memory owner object in the :ref:`owner` field. Slicing a view creates a
    new view referencing the same original object, without any dependency on
    the previous view. The :py:`owner` is :py:`None` if the view is empty.

.. py:class:: corrade.containers.MutableBitArrayView

    Equivalent to :ref:`BitArrayView`, but implementing :ref:`__setitem__()` as
    well.

.. py:class:: corrade.containers.StridedBitArrayView1D

    Provides one-dimensional read-only view on a memory range with custom
    stride values. See :ref:`StridedBitArrayView2D`,
    :ref:`StridedBitArrayView3D`, :ref:`StridedBitArrayView4D`,
    :ref:`MutableStridedBitArrayView1D` and others for multi-dimensional and
    mutable equivalents.

    `Memory ownership and reference counting`_
    ==========================================

    Similarly to :ref:`BitArrayView`, the view keeps a reference to the
    original memory owner object in the :ref:`owner` field. Slicing a view
    creates a new view referencing the same original object, without any
    dependency on the previous view. The :py:`owner` is :py:`None` if the view
    is empty.

.. py:function:: corrade.containers.StridedBitArrayView1D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0`
.. py:function:: corrade.containers.StridedBitArrayView1D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0`
.. py:function:: corrade.containers.StridedBitArrayView1D.expanded
    :raise IndexError: If :p:`dimension` is not :py:`0`
    :raise ValueError: If product of :p:`size` is not equal to size in
        :p:`dimension`

.. py:class:: corrade.containers.MutableStridedBitArrayView1D

    Equivalent to :ref:`StridedBitArrayView1D`, but implementing
    :ref:`__setitem__()` as well.

.. py:function:: corrade.containers.MutableStridedBitArrayView1D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0`
.. py:function:: corrade.containers.MutableStridedBitArrayView1D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0`
.. py:function:: corrade.containers.MutableStridedBitArrayView1D.expanded
    :raise IndexError: If :p:`dimension` is not :py:`0`
    :raise ValueError: If product of :p:`size` is not equal to size in
        :p:`dimension`

.. py:class:: corrade.containers.StridedBitArrayView2D

    See :ref:`StridedBitArrayView1D` for more information.

.. py:function:: corrade.containers.StridedBitArrayView2D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0` or :py:`1`
.. py:function:: corrade.containers.StridedBitArrayView2D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0` or :py:`1`
.. py:function:: corrade.containers.StridedBitArrayView2D.transposed
    :raise IndexError: If :p:`a` or :p:`b` is not :py:`0` or :py:`1` or if
        they're the same
.. py:function:: corrade.containers.StridedBitArrayView2D.expanded
    :raise IndexError: If :p:`dimension` is not :py:`0` or :py:`1`
    :raise ValueError: If product of :p:`size` is not equal to size in
        :p:`dimension`

.. py:class:: corrade.containers.MutableStridedBitArrayView2D

    See :ref:`StridedBitArrayView1D` and :ref:`MutableStridedBitArrayView1D`
    for more information.

.. py:function:: corrade.containers.MutableStridedBitArrayView2D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0` or :py:`1`
.. py:function:: corrade.containers.MutableStridedBitArrayView2D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0` or :py:`1`
.. py:function:: corrade.containers.MutableStridedBitArrayView2D.transposed
    :raise IndexError: If :p:`a` or :p:`b` is not :py:`0` or :py:`1` or if
        they're the same
.. py:function:: corrade.containers.MutableStridedBitArrayView2D.expanded
    :raise IndexError: If :p:`dimension` is not :py:`0` or :py:`1`
    :raise ValueError: If product of :p:`size` is not equal to size in
        :p:`dimension`

.. py:class:: corrade.containers.StridedBitArrayView3D

    See :ref:`StridedBitArrayView1D` for more information.

.. py:function:: corrade.containers.StridedBitArrayView3D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` or :py:`2`
.. py:function:: corrade.containers.StridedBitArrayView3D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` or :py:`2`
.. py:function:: corrade.containers.StridedBitArrayView3D.transposed
    :raise IndexError: If :p:`a` or :p:`b` is not :py:`0`, :py:`1` or :py:`2`
        or if  they're the same
.. py:function:: corrade.containers.StridedBitArrayView3D.expanded
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` or :py:`2`
    :raise ValueError: If product of :p:`size` is not equal to size in
        :p:`dimension`

.. py:class:: corrade.containers.MutableStridedBitArrayView3D

    See :ref:`StridedBitArrayView1D` and :ref:`MutableStridedBitArrayView1D`
    for more information.

.. py:function:: corrade.containers.MutableStridedBitArrayView3D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` or :py:`2`
.. py:function:: corrade.containers.MutableStridedBitArrayView3D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` or :py:`2`
.. py:function:: corrade.containers.MutableStridedBitArrayView3D.transposed
    :raise IndexError: If :p:`a` or :p:`b` is not :py:`0`, :py:`1` or :py:`2`
        or if  they're the same
.. py:function:: corrade.containers.MutableStridedBitArrayView3D.expanded
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` or :py:`2`
    :raise ValueError: If product of :p:`size` is not equal to size in
        :p:`dimension`

.. py:class:: corrade.containers.StridedBitArrayView4D

    See :ref:`StridedBitArrayView1D` for more information.

.. py:function:: corrade.containers.StridedBitArrayView4D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` :py:`2` or
        :py:`3`
.. py:function:: corrade.containers.StridedBitArrayView4D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` :py:`2` or
        :py:`3`
.. py:function:: corrade.containers.StridedBitArrayView4D.transposed
    :raise IndexError: If :p:`a` or :p:`b` is not :py:`0`, :py:`1` :py:`2` or
        :py:`3` or if  they're the same

.. py:class:: corrade.containers.MutableStridedBitArrayView4D

    See :ref:`StridedBitArrayView1D` and :ref:`MutableStridedBitArrayView1D`
    for more information.

.. py:function:: corrade.containers.MutableStridedBitArrayView4D.flipped
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` :py:`2` or
        :py:`3`
.. py:function:: corrade.containers.MutableStridedBitArrayView4D.broadcasted
    :raise IndexError: If :p:`dimension` is not :py:`0`, :py:`1` :py:`2` or
        :py:`3`
.. py:function:: corrade.containers.MutableStridedBitArrayView4D.transposed
    :raise IndexError: If :p:`a` or :p:`b` is not :py:`0`, :py:`1` :py:`2` or
        :py:`3` or if  they're the same
