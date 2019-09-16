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

.. py:class:: magnum.shaders.VertexColor2D
    :data POSITION: Vertex position
    :data COLOR3: Three-component vertex color
    :data COLOR4: Four-component vertex color

.. py:class:: magnum.shaders.VertexColor3D
    :data POSITION: Vertex position
    :data COLOR3: Three-component vertex color
    :data COLOR4: Four-component vertex color

.. py:class:: magnum.shaders.Phong
    :data POSITION: Vertex position
    :data NORMAL: Normal direction
    :data TANGENT: Tangent direction
    :data TEXTURE_COORDINATES: 2D texture coordinates
    :data COLOR3: Three-component vertex color
    :data COLOR4: Four-component vertex color

.. py:property:: magnum.shaders.Phong.alpha_mask
    :raise AttributeError: If the shader was not created with `Flags.ALPHA_MASK`
.. py:property:: magnum.shaders.Phong.light_positions
    :raise ValueError: If list length is different from `light_count`
.. py:property:: magnum.shaders.Phong.light_colors
    :raise ValueError: If list length is different from `light_count`
