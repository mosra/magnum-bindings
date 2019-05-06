/*
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
*/

#include <pybind11/pybind11.h>
#include <Magnum/Shaders/VertexColor.h>

#include "magnum/bootstrap.h"
#include "magnum/NonDestructible.h"

namespace magnum { namespace {

template<UnsignedInt dimensions> void vertexColor(NonDestructibleBase<Shaders::VertexColor<dimensions>, GL::AbstractShaderProgram>& c) {
    /* Attributes */
    c.attr("COLOR3") = GL::DynamicAttribute{
        GL::DynamicAttribute::Kind::Generic, 3,
        GL::DynamicAttribute::Components::Three,
        GL::DynamicAttribute::DataType::Float};
    c.attr("COLOR4") = GL::DynamicAttribute{
        GL::DynamicAttribute::Kind::Generic, 3,
        GL::DynamicAttribute::Components::Four,
        GL::DynamicAttribute::DataType::Float};

    /* Methods */
    c
        .def(py::init(), "Constructor")

        /* Using lambdas to avoid method chaining getting into signatures */

        // TODO: make writeonly once https://github.com/pybind/pybind11/pull/1144
        // is released
        .def_property("transformation_projection_matrix", [](Shaders::VertexColor<dimensions>&) {
            return MatrixTypeFor<dimensions, Float>{};
        }, &Shaders::VertexColor<dimensions>::setTransformationProjectionMatrix,
            "Transformation and projection matrix");
}

void shaders(py::module& m) {
    m.import("magnum.gl");

    /* 2D/3D vertex color shader */
    {
        NonDestructibleBase<Shaders::VertexColor2D, GL::AbstractShaderProgram> vertexColor2D{m,
            "VertexColor2D", "2D vertex color shader"};
        NonDestructibleBase<Shaders::VertexColor3D, GL::AbstractShaderProgram> vertexColor3D{m,
            "VertexColor3D", "3D vertex color shader"};
        vertexColor2D.attr("POSITION") = GL::DynamicAttribute{
            GL::DynamicAttribute::Kind::Generic, 0,
            GL::DynamicAttribute::Components::Two,
            GL::DynamicAttribute::DataType::Float};
        vertexColor3D.attr("POSITION") = GL::DynamicAttribute{
            GL::DynamicAttribute::Kind::Generic, 0,
            GL::DynamicAttribute::Components::Three,
            GL::DynamicAttribute::DataType::Float};
        vertexColor(vertexColor2D);
        vertexColor(vertexColor3D);
    }
}

}}

PYBIND11_MODULE(shaders, m) {
    m.doc() = "Builtin shaders";

    magnum::shaders(m);
}
