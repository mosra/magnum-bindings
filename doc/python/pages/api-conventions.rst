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

Python API conventions
######################

:summary: Basic rules and good practices for both Python binding develpoers and
    users.

`API naming`_
=============

-   mapping is made as clear as possible, the user should not need assistance
    to know what's the corresponding name in Python
-   modules ``lowercase``, words *not* separated with underscores (which means
    C++ namespaces have to be named clearly and tersely to make this possible)
-   class names ``CamelCase``
-   function names ``snake_case``
-   constants and enums ``UPPERCASE``, again underscores omitted if it doesn't
    hurt readability

`Namespaces`_
-------------

.. class:: m-table

=================================== ============================
C++                                 Python
=================================== ============================
:dox:`Magnum::Math`                 `magnum.math`
:dox:`Magnum::SceneGraph`           `magnum.scenegraph`
=================================== ============================

`Classes`_
----------

.. class:: m-table

=================================== ============================
C++                                 Python
=================================== ============================
:dox:`Vector2i`                     `Vector2i`
:dox:`GL::Buffer`                   `gl.Buffer`
=================================== ============================

`Functions`_
------------

.. class:: m-table

================================================ ==========================
C++                                              Python
================================================ ==========================
:dox:`Math::angle()`                             `math.angle()`
:dox:`Vector2::xAxis() <Math::Vector2::xAxis()>` `Vector2.x_axis()`
:cpp:`v.isZero()`                                :py:`v.is_zero()`
:cpp:`m.transformVector(a)`                      :py:`m.transform_vector(a)`
================================================ ==========================

`Enums`_
--------

.. class:: m-table

============================================== ============================
C++                                            Python
============================================== ============================
:dox:`PixelFormat::RGB8Unorm`                  `PixelFormat.RGB8UNORM`
:dox:`MeshPrimitive::TriangleStrip`            :py:`MeshPrimitive.TRIANGLE_STRIP`
============================================== ============================

`Constants`_
------------

Apart from :dox:`Math::Constants`, which are exposed directly as members of the
`math` submodule to mimic Python's :py:`math`, most of the constants used
throughout the C++ API are related to templates. Those are, where applicable,
converted to Python builtins such as :py:`len()`.

.. class:: m-table

============================================== ============================
C++                                            Python
============================================== ============================
:dox:`Constants::pi() <Math::Constants::pi()>` `math.pi`
:dox:`Math::Vector::Size`                      :py:`len(vec)`
============================================== ============================

`Initialization tags`_
----------------------

Since overloading based on argument types is not a common thing to do in Python
(and it adds extra overhead in pybind11), all initialization tags are converted
to static constructors instead:

.. container:: m-row

    .. container:: m-col-m-6

        .. code-figure::

            .. code:: c++

                Matrix4 a{Math::IdentityInit, 5.0f};
                GL::Buffer b{NoCreate};

            C++

    .. container:: m-col-m-6

        .. code-figure::

            .. code:: py

                a = Matrix4.identity_init(5.0)
                b = gl.Buffer.no_create()

            Python

There's no equivalent for the :dox:`Math::NoInit <Math::NoInitT>` tag, as
such optimization doesn't make much sense when instances are copied back
and forth between C++ and Python. Similarly, the :dox:`NoCreate <NoCreateT>`
tag makes sense only in C++ which differentiates between stack-allocated and
heap-allocated instances. In Python it's enough to simply set an instance to
:py:`None` to achieve the same effect.

`Name import conventions`_
==========================

Similarly to C++, where it's encouraged to do something like

.. code:: c++

    namespace YourProject {
        using namespace Magnum;
    }

and then use Magnum C++ APIs unprefixed from inside that namespace, the
recommended Python workflow is similar. Note that importing the root module
*does not* import submodules, so you are expected to import those on an
as-needed basis as well.

.. code:: py

    from magnum import *
    from magnum import gl, platform

In particular, both the C++ and the Python API is designed in a way to prevent
too generic or confusing names in the root namespace / module and also keeping
it relatively clean and small, without too many symbols. On the other hand, the
subnamespaces *do* have generic names. The :dox:`GL::version()` /
`gl.version()` API is one example --- it's tucked in a subnamespace so the
generic name isn't a problem, but you wouldn't find anything of similar
genericity in the root namespace / module.

`Basic guarantees`_
===================

-   All types printable using :dox:`Utility::Debug` implement :py:`__repr__()`
    on the Python side, producing the exact same output.
