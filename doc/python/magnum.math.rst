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

.. py:module:: magnum.math
    :data pi: :math:`\pi`
    :data pi_half: Half of a :math:`\pi`
    :data pi_quarter: Quarter of a :math:`\pi`
    :data tau: :math:`\tau`, or :math:`2 \pi`
    :data e: Euler's number
    :data sqrt2: Square root of 2
    :data sqrt3: Square root of 3
    :data sqrt_half: Square root of :math:`\frac{1}{2}`
    :data nan: Quiet NaN
    :data inf: Positive :math:`\infty`

    In the C++ API, math types are commonly used via :cpp:`typedef`\ s in the
    root namespace, only library-level generic code uses things like
    :dox:`Math::Vector<size, T> <Math::Vector>`. Since Python doesn't have
    templates or generics, there are no generic variants in the :ref:`magnum.math`
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
    another example, a function accepting a :ref:`Quaternion` will accept a
    :py:`((x, y, z), w)` tuple as well, but not :py:`(x, y, z, w)`, as that is
    not convertible to a pair of a three-component vector and a scalar.

    `Magnum math vs Python math`_
    =============================

    .. block-warning:: Subject to change

        Currently, doing :py:`from magnum import math` will bring in the
        Magnum's math module which at the moment *does not* contain the
        well-known Python APIs and constants. In particular, calling
        :ref:`magnum.math.sin()` expects an explicit :ref:`Deg` / :ref:`Rad`
        type, while Python's :ref:`math.sin()` doesn't. This will get resolved
        either by making all Python overloads present in the same module or
        giving the user an option whether to use Magnum math or Python math.
        For now, to avoid confusion, do for example this:

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
    the :ref:`Deg` / :ref:`Rad` types, :ref:`math.pi <magnum.math.pi>` or
    :ref:`math.sin <magnum.math.sin()>`) use the :cpp:`double` variant of the
    underlying C++ API --- the extra arithmetic cost is negligible to the
    Python-to-C++ function call overhead.

    On the other hand, matrix and vector types are exposed in both the float
    and double variants.

    `Implicit conversions; NumPy compatibility`_
    ============================================

    All vector classes are implicitly convertible from a tuple of correct size
    and type as well as from/to type implementing the buffer protocol, and
    these can be also converted back to lists using list comprehensions. This
    makes them fully compatible with :ref:`numpy.ndarray`, so the following
    expressions are completely valid:

    ..
        >>> import numpy as np

    .. code:: pycon

        >>> Matrix4.translation(np.array([1.5, 0.7, 3.3]))
        Matrix(1, 0, 0, 1.5,
               0, 1, 0, 0.7,
               0, 0, 1, 3.3,
               0, 0, 0, 1)

    .. code:: pycon

        >>> m = Matrix4.scaling((0.5, 0.5, 1.0))
        >>> np.array(m.diagonal())
        array([0.5, 0.5, 1. , 1. ], dtype=float32)

    For matrices it's a bit more complicated, since Magnum is using
    column-major layout while numpy defaults to row-major (but can do
    column-major as well). To ensure proper conversions, the buffer protocol
    implementation for matrix types handles the layout conversion as well.
    While the matrix are implicitly convertible from/to types implementing a
    buffer protocol, they *are not* implicitly convertible from/to plain tuples
    like vectors are.

    To simplify the implementation, Magnum matrices are convertible only from
    32-bit and 64-bit floating-point types (:py:`'f'` and :py:`'d'` numpy
    ``dtype``). In the other direction, unless overridden using ``dtype`` or
    ``order``, the created numpy array matches Magnum data type and layout:

    .. code:: pycon

        >>> a = Matrix3(np.array(
        ...     [[1.0, 2.0, 3.0],
        ...      [4.0, 5.0, 6.0],
        ...      [7.0, 8.0, 9.0]]))
        >>> a[0] # first column
        Vector(1, 4, 7)

    ..
        In order to minimize floating-point printing differences across
        architectures (ARM Mac has a different output below otherwise)
        >>> np.set_printoptions(precision=5)

    .. code:: pycon

        >>> b = np.array(Matrix3.rotation(Deg(45.0)))
        >>> b.strides[0] # column-major storage
        4
        >>> b[0] # first column, 32-bit floats
        array([ 0.70711, -0.70711,  0.     ], dtype=float32)

    ..
        A bit longer for doubles
        >>> np.set_printoptions(precision=7)

    .. code:: pycon

        >>> c = np.array(Matrix3.rotation(Deg(45.0)), order='C', dtype='d')
        >>> c.strides[0] # row-major storage (overridden)
        24
        >>> c[0] # first column, 64-bit floats (overridden)
        array([ 0.7071068, -0.7071068,  0.       ])

    ..
        Reset back
        >>> np.set_printoptions()

    `Major differences to the C++ API`_
    ===================================

    -   All vector and matrix classes implement :py:`len()`, which is used
        instead of e.g. :dox:`Math::Vector::Size`. Works on both classes
        and instances.
    -   :dox:`Math::Matrix3::from()` / :dox:`Math::Matrix4::from()` are named
        :ref:`Matrix3.from_()` / :ref:`Matrix4.from_()` because :py:`from` is
        a Python keyword and thus can't be used as a name.
    -   :dox:`Math::isInf()` and :dox:`Math::isNan()` are named
        :ref:`math.isinf() <magnum.math.isinf()>` and
        :ref:`math.isnan() <magnum.math.isnan()>` for consistency with the
        Python :ref:`math` module
    -   :cpp:`Math::gather()` and :cpp:`Math::scatter()` operations are
        implemented as real swizzles:

        .. code:: pycon

            >>> a = Vector4(1.5, 0.3, -1.0, 1.0)
            >>> b = Vector4(7.2, 2.3, 1.1, 0.0)
            >>> a.wxy = b.xwz
            >>> a
            Vector(0, 1.1, -1, 7.2)

    -   :py:`mat[a][b] = c` on matrices doesn't do the expected thing, use
        :py:`mat[a, b] = c` instead
    -   :cpp:`Math::BitVector::set()` doesn't exist, use ``[]`` instead
    -   While both boolean and bitwise operations on :cpp:`Math::BitVector`
        behave the same to ensure consistency in generic code, this is not
        possible to do in Python. Here the boolean operations behave like
        if :py:`any()` was applied before doing the operation.

    `Static constructors and instance method / property overloads`_
    ---------------------------------------------------------------

    While not common in Python, the :ref:`Matrix4.scaling()` /
    :ref:`Matrix4.rotation()` methods mimic the C++ equivalent --- calling
    :py:`Matrix4.scaling(vec)` will return a scaling matrix, while
    :py:`mat.scaling()` returns the 3x3 scaling part of the matrix. With
    :ref:`Matrix4.translation`, it's a bit more involved --- calling
    :py:`Matrix4.translation(vec)` will return a translation matrix, while
    :py:`mat.translation` is a read-write property accessing the fourth column
    of the matrix. Similarly for the :ref:`Matrix3` class.

.. py:function:: magnum.Matrix2x2.inverted_orthogonal
    :raise ValueError: If the matrix is not orthogonal
.. py:function:: magnum.Matrix2x2d.inverted_orthogonal
    :raise ValueError: If the matrix is not orthogonal
.. py:function:: magnum.Matrix3x3.inverted_orthogonal
    :raise ValueError: If the matrix is not orthogonal
.. py:function:: magnum.Matrix3x3d.inverted_orthogonal
    :raise ValueError: If the matrix is not orthogonal
.. py:function:: magnum.Matrix4x4.inverted_orthogonal
    :raise ValueError: If the matrix is not orthogonal
.. py:function:: magnum.Matrix4x4d.inverted_orthogonal
    :raise ValueError: If the matrix is not orthogonal
.. py:function:: magnum.Matrix3.inverted_orthogonal
    :raise ValueError: If the matrix is not orthogonal
.. py:function:: magnum.Matrix3d.inverted_orthogonal
    :raise ValueError: If the matrix is not orthogonal
.. py:function:: magnum.Matrix4.inverted_orthogonal
    :raise ValueError: If the matrix is not orthogonal
.. py:function:: magnum.Matrix4d.inverted_orthogonal
    :raise ValueError: If the matrix is not orthogonal

.. py:function:: magnum.Matrix3.reflection
    :raise ValueError: If :p:`normal` is not normalized
.. py:function:: magnum.Matrix3d.reflection
    :raise ValueError: If :p:`normal` is not normalized
.. py:function:: magnum.Matrix4.reflection
    :raise ValueError: If :p:`normal` is not normalized
.. py:function:: magnum.Matrix4d.reflection
    :raise ValueError: If :p:`normal` is not normalized
.. py:function:: magnum.Matrix3.rotation(self)
    :raise ValueError: If the normalized rotation part is not orthogonal
.. py:function:: magnum.Matrix3d.rotation(self)
    :raise ValueError: If the normalized rotation part is not orthogonal
.. py:function:: magnum.Matrix4.rotation(self)
    :raise ValueError: If the normalized rotation part is not orthogonal
.. py:function:: magnum.Matrix4d.rotation(self)
    :raise ValueError: If the normalized rotation part is not orthogonal
.. py:function:: magnum.Matrix3.rotation_normalized
    :raise ValueError: If the rotation part is not orthogonal
.. py:function:: magnum.Matrix3d.rotation_normalized
    :raise ValueError: If the rotation part is not orthogonal
.. py:function:: magnum.Matrix4.rotation_normalized
    :raise ValueError: If the rotation part is not orthogonal
.. py:function:: magnum.Matrix4d.rotation_normalized
    :raise ValueError: If the rotation part is not orthogonal
.. py:function:: magnum.Matrix3.uniform_scaling_squared
    :raise ValueError: If the matrix doesn't have uniform scaling
.. py:function:: magnum.Matrix3d.uniform_scaling_squared
    :raise ValueError: If the matrix doesn't have uniform scaling
.. py:function:: magnum.Matrix4.uniform_scaling_squared
    :raise ValueError: If the matrix doesn't have uniform scaling
.. py:function:: magnum.Matrix4d.uniform_scaling_squared
    :raise ValueError: If the matrix doesn't have uniform scaling
.. py:function:: magnum.Matrix3.uniform_scaling
    :raise ValueError: If the matrix doesn't have uniform scaling
.. py:function:: magnum.Matrix3d.uniform_scaling
    :raise ValueError: If the matrix doesn't have uniform scaling
.. py:function:: magnum.Matrix4.uniform_scaling
    :raise ValueError: If the matrix doesn't have uniform scaling
.. py:function:: magnum.Matrix4d.uniform_scaling
    :raise ValueError: If the matrix doesn't have uniform scaling
.. py:function:: magnum.Matrix3.inverted_rigid
    :raise ValueError: If the matrix doesn't represent a rigid transformation
.. py:function:: magnum.Matrix3d.inverted_rigid
    :raise ValueError: If the matrix doesn't represent a rigid transformation
.. py:function:: magnum.Matrix4.inverted_rigid
    :raise ValueError: If the matrix doesn't represent a rigid transformation
.. py:function:: magnum.Matrix4d.inverted_rigid
    :raise ValueError: If the matrix doesn't represent a rigid transformation

.. py:function:: magnum.math.half_angle(normalized_a: magnum.Quaternion, normalized_b: magnum.Quaternion)
    :raise ValueError: If either of the quaternions is not normalized
.. py:function:: magnum.math.half_angle(normalized_a: magnum.Quaterniond, normalized_b: magnum.Quaterniond)
    :raise ValueError: If either of the quaternions is not normalized
.. py:function:: magnum.math.lerp(normalized_a: magnum.Quaternion, normalized_b: magnum.Quaternion, t: float)
    :raise ValueError: If either of the quaternions is not normalized
.. py:function:: magnum.math.lerp(normalized_a: magnum.Quaterniond, normalized_b: magnum.Quaterniond, t: float)
    :raise ValueError: If either of the quaternions is not normalized
.. py:function:: magnum.math.lerp_shortest_path(normalized_a: magnum.Quaternion, normalized_b: magnum.Quaternion, t: float)
    :raise ValueError: If either of the quaternions is not normalized
.. py:function:: magnum.math.lerp_shortest_path(normalized_a: magnum.Quaterniond, normalized_b: magnum.Quaterniond, t: float)
    :raise ValueError: If either of the quaternions is not normalized
.. py:function:: magnum.math.slerp(normalized_a: magnum.Quaternion, normalized_b: magnum.Quaternion, t: float)
    :raise ValueError: If either of the quaternions is not normalized
.. py:function:: magnum.math.slerp(normalized_a: magnum.Quaterniond, normalized_b: magnum.Quaterniond, t: float)
    :raise ValueError: If either of the quaternions is not normalized
.. py:function:: magnum.math.slerp_shortest_path(normalized_a: magnum.Quaternion, normalized_b: magnum.Quaternion, t: float)
    :raise ValueError: If either of the quaternions is not normalized
.. py:function:: magnum.math.slerp_shortest_path(normalized_a: magnum.Quaterniond, normalized_b: magnum.Quaterniond, t: float)
    :raise ValueError: If either of the quaternions is not normalized
.. py:function:: magnum.Quaternion.rotation(angle: magnum.Rad, normalized_axis: magnum.Vector3)
    :raise ValueError: If :p:`normalized_axis` is not normalized
.. py:function:: magnum.Quaternion.rotation(normalized_from: magnum.Vector3, normalized_to: magnum.Vector3)
    :raise ValueError: If either of the vectors is not normalized
.. py:function:: magnum.Quaterniond.rotation(angle: magnum.Rad, normalized_axis: magnum.Vector3d)
    :raise ValueError: If :p:`normalized_axis` is not normalized
.. py:function:: magnum.Quaternion.reflection
    :raise ValueError: If :p:`normal` is not normalized
.. py:function:: magnum.Quaterniond.reflection
    :raise ValueError: If :p:`normal` is not normalized
.. py:function:: magnum.Quaternion.from_matrix
    :raise ValueError: If :p:`matrix` is not a rotation
.. py:function:: magnum.Quaterniond.from_matrix
    :raise ValueError: If :p:`matrix` is not a rotation
.. py:function:: magnum.Quaternion.angle
    :raise ValueError: If the quaternion is not normalized
.. py:function:: magnum.Quaterniond.angle
    :raise ValueError: If the quaternion is not normalized
.. py:function:: magnum.Quaternion.axis
    :raise ValueError: If the quaternion is not normalized
.. py:function:: magnum.Quaterniond.axis
    :raise ValueError: If the quaternion is not normalized
.. py:function:: magnum.Quaternion.inverted_normalized
    :raise ValueError: If the quaternion is not normalized
.. py:function:: magnum.Quaterniond.inverted_normalized
    :raise ValueError: If the quaternion is not normalized
.. py:function:: magnum.Quaternion.transform_vector_normalized
    :raise ValueError: If the quaternion is not normalized
.. py:function:: magnum.Quaterniond.transform_vector_normalized
    :raise ValueError: If the quaternion is not normalized

.. py:function:: magnum.math.angle(normalized_a: magnum.Vector2, normalized_b: magnum.Vector2)
    :raise ValueError: If either of the vectors is not normalized
.. py:function:: magnum.math.angle(normalized_a: magnum.Vector2d, normalized_b: magnum.Vector2d)
    :raise ValueError: If either of the vectors is not normalized
.. py:function:: magnum.math.angle(normalized_a: magnum.Vector3, normalized_b: magnum.Vector3)
    :raise ValueError: If either of the vectors is not normalized
.. py:function:: magnum.math.angle(normalized_a: magnum.Vector3d, normalized_b: magnum.Vector3d)
    :raise ValueError: If either of the vectors is not normalized
.. py:function:: magnum.math.angle(normalized_a: magnum.Vector4, normalized_b: magnum.Vector4)
    :raise ValueError: If either of the vectors is not normalized
.. py:function:: magnum.math.angle(normalized_a: magnum.Vector4d, normalized_b: magnum.Vector4d)
    :raise ValueError: If either of the vectors is not normalized
.. py:function:: magnum.Vector2.projected_onto_normalized
    :raise ValueError: If :p:`line` is not normalized
.. py:function:: magnum.Vector2d.projected_onto_normalized
    :raise ValueError: If :p:`line` is not normalized
.. py:function:: magnum.Vector3.projected_onto_normalized
    :raise ValueError: If :p:`line` is not normalized
.. py:function:: magnum.Vector3d.projected_onto_normalized
    :raise ValueError: If :p:`line` is not normalized
.. py:function:: magnum.Vector4.projected_onto_normalized
    :raise ValueError: If :p:`line` is not normalized
.. py:function:: magnum.Vector4d.projected_onto_normalized
    :raise ValueError: If :p:`line` is not normalized

.. For pickling, because py::pickle() doesn't seem to have a way to set the
   __setstate__ / __getstate__ docs directly FFS

.. py:function:: magnum.Deg.__getstate__
    :summary: Dumps the in-memory representation as a float
.. py:function:: magnum.Rad.__getstate__
    :summary: Dumps the in-memory representation as a float

.. py:function:: magnum.BitVector2.__getstate__
    :summary: Dumps the in-memory representation of vector bits
.. py:function:: magnum.BitVector3.__getstate__
    :summary: Dumps the in-memory representation of vector bits
.. py:function:: magnum.BitVector4.__getstate__
    :summary: Dumps the in-memory representation of vector bits

.. py:function:: magnum.Vector2.__getstate__
    :summary: Dumps the in-memory representation of vector components
.. py:function:: magnum.Vector3.__getstate__
    :summary: Dumps the in-memory representation of vector components
.. py:function:: magnum.Vector4.__getstate__
    :summary: Dumps the in-memory representation of vector components
.. py:function:: magnum.Vector2d.__getstate__
    :summary: Dumps the in-memory representation of vector components
.. py:function:: magnum.Vector3d.__getstate__
    :summary: Dumps the in-memory representation of vector components
.. py:function:: magnum.Vector4d.__getstate__
    :summary: Dumps the in-memory representation of vector components
.. py:function:: magnum.Vector2i.__getstate__
    :summary: Dumps the in-memory representation of vector components
.. py:function:: magnum.Vector3i.__getstate__
    :summary: Dumps the in-memory representation of vector components
.. py:function:: magnum.Vector4i.__getstate__
    :summary: Dumps the in-memory representation of vector components
.. py:function:: magnum.Vector2ui.__getstate__
    :summary: Dumps the in-memory representation of vector components
.. py:function:: magnum.Vector3ui.__getstate__
    :summary: Dumps the in-memory representation of vector components
.. py:function:: magnum.Vector4ui.__getstate__
    :summary: Dumps the in-memory representation of vector components
.. py:function:: magnum.Color3.__getstate__
    :summary: Dumps the in-memory representation of vector components
.. py:function:: magnum.Color4.__getstate__
    :summary: Dumps the in-memory representation of vector components

.. py:function:: magnum.Matrix2x2.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix2x3.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix2x4.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix3x2.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix3x3.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix3x4.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix4x2.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix4x3.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix4x4.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix2x2d.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix2x3d.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix2x4d.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix3x2d.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix3x3d.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix3x4d.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix4x2d.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix4x3d.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix4x4d.__getstate__
    :summary: Dumps the in-memory representation of matrix components

.. py:function:: magnum.Matrix3.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix4.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix3d.__getstate__
    :summary: Dumps the in-memory representation of matrix components
.. py:function:: magnum.Matrix4d.__getstate__
    :summary: Dumps the in-memory representation of matrix components

.. py:function:: magnum.Quaternion.__getstate__
    :summary: Dumps the in-memory representation of quaternion components
.. py:function:: magnum.Quaterniond.__getstate__
    :summary: Dumps the in-memory representation of quaternion components

.. py:function:: magnum.Range1D.__getstate__
    :summary: Dumps the in-memory representation of range min/max components
.. py:function:: magnum.Range2D.__getstate__
    :summary: Dumps the in-memory representation of range min/max components
.. py:function:: magnum.Range3D.__getstate__
    :summary: Dumps the in-memory representation of range min/max components
.. py:function:: magnum.Range1Dd.__getstate__
    :summary: Dumps the in-memory representation of range min/max components
.. py:function:: magnum.Range2Dd.__getstate__
    :summary: Dumps the in-memory representation of range min/max components
.. py:function:: magnum.Range3Dd.__getstate__
    :summary: Dumps the in-memory representation of range min/max components
.. py:function:: magnum.Range1Di.__getstate__
    :summary: Dumps the in-memory representation of range min/max components
.. py:function:: magnum.Range2Di.__getstate__
    :summary: Dumps the in-memory representation of range min/max components
.. py:function:: magnum.Range3Di.__getstate__
    :summary: Dumps the in-memory representation of range min/max components

.. py:function:: magnum.Deg.__setstate__
    :summary: Uses the float as the in-memory representation
.. py:function:: magnum.Rad.__setstate__
    :summary: Uses the float as the in-memory representation

.. py:function:: magnum.BitVector2.__setstate__
    :summary: Treats the data as the in-memory representation of vector bits
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.BitVector3.__setstate__
    :summary: Treats the data as the in-memory representation of vector bits
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.BitVector4.__setstate__
    :summary: Treats the data as the in-memory representation of vector bits
    :raise ValueError: If the data size doesn't match type size

.. py:function:: magnum.Vector2.__setstate__
    :summary: Treats the data as the in-memory representation of vector
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Vector3.__setstate__
    :summary: Treats the data as the in-memory representation of vector
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Vector4.__setstate__
    :summary: Treats the data as the in-memory representation of vector
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Vector2d.__setstate__
    :summary: Treats the data as the in-memory representation of vector
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Vector3d.__setstate__
    :summary: Treats the data as the in-memory representation of vector
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Vector4d.__setstate__
    :summary: Treats the data as the in-memory representation of vector
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Vector2i.__setstate__
    :summary: Treats the data as the in-memory representation of vector
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Vector3i.__setstate__
    :summary: Treats the data as the in-memory representation of vector
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Vector4i.__setstate__
    :summary: Treats the data as the in-memory representation of vector
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Vector2ui.__setstate__
    :summary: Treats the data as the in-memory representation of vector
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Vector3ui.__setstate__
    :summary: Treats the data as the in-memory representation of vector
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Vector4ui.__setstate__
    :summary: Treats the data as the in-memory representation of vector
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Color3.__setstate__
    :summary: Treats the data as the in-memory representation of vector
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Color4.__setstate__
    :summary: Treats the data as the in-memory representation of vector
        components
    :raise ValueError: If the data size doesn't match type size

.. py:function:: magnum.Matrix2x2.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix2x3.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix2x4.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix3x2.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix3x3.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix3x4.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix4x2.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix4x3.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix4x4.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix2x2d.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix2x3d.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix2x4d.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix3x2d.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix3x3d.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix3x4d.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix4x2d.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix4x3d.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix4x4d.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size

.. py:function:: magnum.Matrix3.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix4.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix3d.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Matrix4d.__setstate__
    :summary: Treats the data as the in-memory representation of matrix
        components
    :raise ValueError: If the data size doesn't match type size

.. py:function:: magnum.Quaternion.__setstate__
    :summary: Treats the data as the in-memory representation of quaternion
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Quaterniond.__setstate__
    :summary: Treats the data as the in-memory representation of quaternion
        components
    :raise ValueError: If the data size doesn't match type size

.. py:function:: magnum.Range1D.__setstate__
    :summary: Treats the data as the in-memory representation of range min/max
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Range2D.__setstate__
    :summary: Treats the data as the in-memory representation of range min/max
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Range3D.__setstate__
    :summary: Treats the data as the in-memory representation of range min/max
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Range1Dd.__setstate__
    :summary: Treats the data as the in-memory representation of range min/max
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Range2Dd.__setstate__
    :summary: Treats the data as the in-memory representation of range min/max
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Range3Dd.__setstate__
    :summary: Treats the data as the in-memory representation of range min/max
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Range1Di.__setstate__
    :summary: Treats the data as the in-memory representation of range min/max
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Range2Di.__setstate__
    :summary: Treats the data as the in-memory representation of range min/max
        components
    :raise ValueError: If the data size doesn't match type size
.. py:function:: magnum.Range3Di.__setstate__
    :summary: Treats the data as the in-memory representation of range min/max
        components
    :raise ValueError: If the data size doesn't match type size
