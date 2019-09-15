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

.. py:module:: magnum.gl

    TODO: remove this once m.css stops ignoring the first caption on a page
    #######################################################################

    `NoCreate constructors`_
    ========================

    .. TODO: link to NoCreate once m.dox handles variables properly

    Compared to C++, the Python APIs don't have an alternative to the
    :dox:`NoCreate <NoCreateT>` constructor tag. In C++ these make it possible
    to have class members initialized before a GL context is present, but in
    Python there's no such limitation so these don't make sense.

.. py:class:: magnum.gl.Mesh

    TODO: remove this once m.css stops ignoring the first caption on a page
    #######################################################################

    `Buffer ownership and reference counting`_
    ==========================================

    Unlike in C++, where a :dox:`GL::Buffer` is either :dox:`std::move()`\ d
    into the mesh or referenced externally (with the user being responsible for
    its lifetime), the `gl.Mesh` object keeps references to all buffers added
    to it.
