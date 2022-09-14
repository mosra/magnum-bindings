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

Developers Guide
################

:ref-prefix:
    corrade
    magnum
:summary: Checklists for developing new things in Magnum Python bindings
    themselves.

.. role:: cmake(code)
    :language: cmake
.. role:: cpp(code)
    :language: c++
.. role:: py(code)
    :language: py

`Checklist for adding / removing bindings for a library`_
=========================================================

1.  Add a corresponding ``Foo`` Magnum dependency to the :cmake:`find_package()`
    call in ``src/python/magnum/CMakeLists.txt``, create a ``magnum_foo_SRCS``
    variable with its source file(s).
2.  Add a :cmake:`if(Magnum_Foo_FOUND)` branch to the
    :cmake:`if(NOT MAGNUM_BUILD_STATIC)` condition, adding a new ``magnum_foo``
    module from ``magnum_foo_SRCS``, linking to ``Magnum::Foo`` and setting
    ``OUTPUT_NAME`` to ``foo``.
3.  Add a :cmake:`if(Magnum_Foo_FOUND)` branch to the :cmake:`else()`
    condition, ``APPEND``\ ing ``magnum_FOO_SRCS`` to ``magnum_SRCS`` and
    ``Magnum::Foo`` to ``magnum_LIBS``.
4.  Add :cpp:`void foo(py::module_& m);` forward declaration to
    ``magnum/bootstrap.h``, and :cpp:`#cmakdefeine Magnum_Foo_FOUND` to
    ``magnum/staticconfigure.h.cmake``.
5.  Implement ``void foo(py::module_& m)`` in ``src/python/magnum/foo.cpp``,
    add a :cpp:`PYBIND11_MODULE()` calling it.
6.  Add :cpp:`m.def_submodule("foo");` and a call to :cpp:`magnum:foo()`
    wrapped in :cpp:`#ifdef Magnum_Foo_FOUND` to the
    :cpp:`#ifdef MAGNUM_BUILD_STATIC` section of :cpp:`PYBIND11_MODULE()` in
    ``magnum/magnum.cpp``.
7.  Add the new module name to the list in ``magnum/__init__.py``.
8.  Add a line with `magnum_foo` to the :cmake:`foreach()` in
    ``src/python/CMakeLists.txt``, and then a corresponding :py:`'magnum.foo'`
    entry in ``src/python/setup.py.cmake``
9.  Add a ``magnum/test/test_foo.py`` test file, and potentially also
    ``magnum/test/test_foo_gl.py`` where is
    :py:`from . import GLTestCase, setUpModule` to skip the test if GL context
    doesn't exist, and the test cases derive from :py:`GLTestCase` instead of
    :py:`unittest.TestCase`.
10. Add the new module into the :py:`magnum.__all__` list in
    ``doc/python/conf.py``.
11. Add a ``doc/python/magnum.foo.rst`` documentation file for more detailed
    docs, if needed, and reference it from ``INPUT_DOCS``.
12. Add a ``doc/python/pages/changelog.rst`` entry.

For Corrade bindings it's similar.
