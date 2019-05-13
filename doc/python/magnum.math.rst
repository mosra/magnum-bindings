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

.. py:module:: magnum.math

    In the C++ API, math types are commonly used via :cpp:`typedef`\ s in the
    root namespace, only library-level generic code uses things like
    :dox:`Math::Vector<size, T> <Math::Vector>`. Since Python doesn't have
    templates or generics, there are no generic variants in the `magnum.math`
    module, all the concrete types are in the root module with the same names
    as in the C++ variant.

    All math structures are instantiated for the most common sizes and types
    and so they all share a very similar API. As in C++, main differences are
    between floating-point types and integral types (one having normalization
    and projection while the other having bitwise operations) and extra
    convenience methods added for vectors of particular size.

    .. container:: m-row

        .. container:: m-col-m-6

            .. code-figure::

                .. code:: c++

                    using namespace Magnum;
                    Vector4i b{3, 4, 5, 6};
                    b.b() *= 3;
                    b.xy() += {1, -1};
                    Debug{} << b;
                    // Vector(4, 3, 15, 6)

                C++

        .. container:: m-col-m-6

            .. code-figure::

                .. code:: pycon

                    >>> from magnum import *
                    >>> b = Vector4i(3, 4, 5, 6)
                    >>> b.b *= 3
                    >>> b.xy += (1, -1)
                    >>> b
                    Vector(4, 3, 15, 6)

                Python

    As shown above, all math types are constructible from a (nested) tuple of
    matching type, matching the convenience of C++11 uniform initializers. As
    another example, a function accepting a `Quaternion` will accept a
    :py:`((x, y, z), w)` tuple as well, but not :py:`(x, y, z, w)`, as that is
    not convertible to a pair of a three-component vector and a scalar.

    `Magnum math vs Python math`_
    =============================

    .. block-warning:: Subject to change

        Currently, doing :py:`from magnum import math` will bring in the
        Magnum's math module which at the moment *does not* contain the
        well-known Python APIs and constants. In particular, calling `math.sin()`
        expects an explicit `Deg` / `Rad` type, while Python's :py:`math.sin()`
        doesn't. This will get resolved either by making all Python overloads
        present in the same module or giving the user an option whether to use
        Magnum math or Python math. For now, to avoid confusion, do for example
        this:

        .. code:: pycon

            >>> import math
            >>> import magnum.math as mmath
            >>> mmath.sin(Deg(45.0))
            0.7071067811865475
            >>> math.sin(45.0*math.pi/180)
            0.7071067811865475

    `Float vs double overloads`_
    ============================

    Since Python doesn't really differentiate between 32bit and 64bit doubles,
    all *scalar* functions taking or returning a floating-point type (such as
    the `Deg` / `Rad` types, `math.pi` or `math.sin()`) use the :cpp:`double`
    variant of the underlying C++ API --- the extra arithmetic cost is
    negligible to the Python-to-C++ function call overhead.

    On the other hand, matrix and vector types are exposed in both the float
    and double variants.

    `Major differences to the C++ API`_
    ===================================

    -   All vector and matrix classes implement :py:`len()`, which is used
        instead of e.g. :dox:`Math::Vector::Size`. Works on both classes
        and instances.
    -   :py:`mat[a][b] = c` on matrices doesn't do the expected thing, use
        :py:`mat[a, b] = c` instead
    -   :cpp:`Math::BoolVector::set()` doesn't exist, use ``[]`` instead
    -   While both boolean and bitwise operations on :cpp:`Math::BoolVector`
        behave the same to ensure consistency in generic code, this is not
        possible to do in Python. Here the boolean operations behave like
        if :py:`any()` was applied before doing the operation.

    .. block-warning:: Subject to change

        The :dox:`Math::swizzle()` operation is not yet available in the Python
        API. Thanks to better flexibility of the Python language this will get
        implemented as a *real* swizzle, allowing for convenient expressions
        like :py:`vec.xz = (3.5, 0.1)`.

    `Static constructors and instance method overloads`_
    ----------------------------------------------------

    While not common in Python, the `Matrix4.scaling()` / `Matrix4.rotation()`
    methods mimic the C++ equivalent --- calling `Matrix4.scaling()` will
    return a scaling matrix, while :py:`mat.scaling()` returns the 3x3 scaling
    part of the matrix. Similarly for the `Matrix3` class.

    .. block-warning:: Subject to change

        On the other hand, there's currently just `Matrix3.translation()` and
        the corresponding :py:`mat.translation` property is temporarily
        available as an underscored `Matrix3._translation`. This will change
        later.

.. py:data:: magnum.math.pi
    :summary: :math:`\pi`

.. py:data:: magnum.math.pi_half
    :summary: Half of a :math:`\pi`

.. py:data:: magnum.math.pi_quarter
    :summary: Quarter of a :math:`\pi`

.. py:data:: magnum.math.tau
    :summary: :math:`\tau`

.. py:data:: magnum.math.e
    :summary: Euler's number

.. py:data:: magnum.math.sqrt2
    :summary: Square root of 2

.. py:data:: magnum.math.sqrt3
    :summary: Square root of 3

.. py:data:: magnum.math.sqrt_half
    :summary: Square root of :math:`\frac{1}{2}`

.. py:data:: magnum.math.nan
    :summary: Quiet NaN

.. py:data:: magnum.math.inf
    :summary: Positive :math:`\infty`
