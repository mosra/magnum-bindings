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

Downloading and building
########################

.. role:: sh(code)
    :language: sh

:summary: Installation guide for the Python bindings.

Building of Python bindings is a similar process to
:dox:`building Magnum itself <building>` with an additional step involving
Python setuptools. Minimal set of tools and libraries required for building is:

-   C++ compiler with good C++11 support. Compilers which are tested to have
    everything needed are **GCC** >= 4.8.1, **Clang** >= 3.3 and **MSVC**
    >= 2015. On Windows you can also use **MinGW-w64**.
-   **CMake** >= 3.1
-   **Corrade** and **Magnum** installed
    :dox:`as described in their docs <building>`
-   **Python** >= 3.5 and :gh:`pybind11 <pybind/pybind11>`

`Prepared packages`_
====================

`ArchLinux packages`_
---------------------

In ``package/archlinux`` there is a development package, similar to the ones
in Magnum itself. They allow you to build and install the package directly from
the source tree.

.. code:: sh

    git clone git://github.com/mosra/magnum-bindings && cd magnum-bindings
    cd package/archlinux
    makepkg -fp PKGBUILD

.. block-warning:: Subject to change

    The bindings are not in ``master`` yet (:gh:`mosra/magnum-bindings#1`), use
    :sh:`git checkout python` or pass ``--branch python`` to :sh:`git clone` at
    the moment.

The PKGBUILD also contains a :sh:`check()` function which will run all unit
tests before packaging. That might sometimes fail or take too long, pass
``--nocheck`` to ``makepkg`` to skip that.

Once built, install the package using ``pacman``:

.. code:: sh

    sudo pacman -U magnum-bindings-*.pkg.tar.xz

`Manual build`_
===============

The source is available on GitHub at https://github.com/mosra/magnum-bindings.
Clone the repository with your favorite IDE or Git GUI, download currrent
snapshot as a compressed archive or use the command line:

.. code:: sh

    git clone git://github.com/mosra/magnum-bindings.git

.. block-warning:: Subject to change

    The bindings are not in ``master`` yet (:gh:`mosra/magnum-bindings#1`), use
    :sh:`git checkout python` or pass ``--branch python`` to :sh:`git clone` at
    the moment.

Assuming a Unix-based OS, the first step is to build the native libraries. The
bindings will be generated for all Corrade and Magnum libraries that are found,
ignoring the ones which aren't. If Corrade, Magnum and pybind11 are not in a
default location known to CMake, add their path to ``CMAKE_PREFIX_PATH``.

.. code:: sh

    mkdir build && cd build
    cmake .. \
        -DWITH_PYTHON=ON
    make

Note that pybind11 compilation is quite time- and memory-hungry, so you might
not want to run the build on all cores on memory-constrained systems. In the
build directory, CMake will create the desired Python package layout, meaning
the bindings can be used directly if you ``cd`` into ``build/src/python/magnum``.
For installing into a system-wide location, CMake generates a ``setup.py``
containing location of all built libraries for use with Python setuptools:

.. code:: sh

    cd build/src/python/magnum
    python setup.py install # or python3, sudo might be needed

`Running unit tests`_
---------------------

Essential functionality of the bindings is tested using Python's builtin
``unittest`` module. The tests currently assume a CMake build directory with
all binaries already built located in a ``build/`` directory in project root,
running them is then a matter of:

.. code:: sh

    cd src/python/magnum
    python -m unittest

.. block-warning:: Subject to change

    If the tests detect that one of `platform.WindowlessApplication`\ s is
    present, GL tests (suffixed with ``_gl``) will be run as well. Currently
    there's no way to blacklist them if windowless application implementations
    are compiled, you can only whitelist-run the remaining tests:

    .. code:: sh

        python -m unittest test.test_gl test.test_math # test.test_gl_gl is a GL test

For code coverage, `coverage.py <https://coverage.readthedocs.io/>`_ is used.
Get it via ``pip`` or as a system package.

.. code:: sh

    pip install coverage # sudo might be needed

Running the unit tests with coverage enabled is then a matter of executing the
following commands, the resulting HTML overview is located in
``htmlcov/index.html``:

.. code:: sh

    cd src/python/magnum
    coverage run -m unittest
    coverage html

`Continuous Integration`_
=========================

In ``package/ci/`` there is a ``travis.yml`` file that compiles and tests the
bindings on Linux GCC 4.8 + CMake 3.1 and on macOS. Online at
https://travis-ci.org/mosra/magnum-bindings, code coverage is reported to
https://codecov.io/gh/mosra/magnum-bindings.
