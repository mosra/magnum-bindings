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
#include <Magnum/Mesh.h>

#include "magnum/bootstrap.h"

#ifdef MAGNUM_BUILD_STATIC
#include "magnum/staticconfigure.h"
#endif

namespace py = pybind11;

namespace magnum { namespace {

void magnum(py::module& m) {
    py::enum_<MeshPrimitive>{m, "MeshPrimitive", "Mesh primitive type"}
        .value("POINTS", MeshPrimitive::Points)
        .value("LINES", MeshPrimitive::Lines)
        .value("LINE_LOOP", MeshPrimitive::LineLoop)
        .value("LINE_STRIP", MeshPrimitive::LineStrip)
        .value("TRIANGLES", MeshPrimitive::Triangles)
        .value("TRIANGLE_STRIP", MeshPrimitive::TriangleStrip)
        .value("TRIANGLE_FAN", MeshPrimitive::TriangleFan);

    py::enum_<MeshIndexType>{m, "MeshIndexType", "Mesh index type"}
        .value("UNSIGNED_BYTE", MeshIndexType::UnsignedByte)
        .value("UNSIGNED_SHORT", MeshIndexType::UnsignedShort)
        .value("UNSIGNED_INT", MeshIndexType::UnsignedInt);
}

}}

PYBIND11_MODULE(_magnum, m) {
    m.doc() = "Root Magnum module";

    magnum::magnum(m);

    py::module math = m.def_submodule("math");
    magnum::math(m, math);

    /* In case Magnum is a bunch of static libraries, put everything into a
       single shared lib to make it easier to install (which is the point of
       static builds) and avoid issues with multiply-defined global symbols.

       These need to be defined in the order they depend on. */
    #ifdef MAGNUM_BUILD_STATIC
    #ifdef Magnum_GL_FOUND
    py::module gl = m.def_submodule("gl");
    magnum::gl(gl);
    #endif

    #ifdef Magnum_SceneGraph_FOUND
    py::module scenegraph = m.def_submodule("scenegraph");
    magnum::scenegraph(scenegraph);
    #endif

    #ifdef Magnum_Trade_FOUND
    py::module trade = m.def_submodule("trade");
    magnum::trade(trade);
    #endif

    #ifdef Magnum_MeshTools_FOUND
    /* Depends on trade and gl */
    py::module meshtools = m.def_submodule("meshtools");
    magnum::meshtools(meshtools);
    #endif

    #ifdef Magnum_Primitives_FOUND
    /* Depends on trade */
    py::module primitives = m.def_submodule("primitives");
    magnum::primitives(primitives);
    #endif

    #ifdef Magnum_Shaders_FOUND
    /* Depends on gl */
    py::module shaders = m.def_submodule("shaders");
    magnum::shaders(shaders);
    #endif

    /* Keep the doc in sync with platform/__init__.py */
    py::module platform = m.def_submodule("platform");
    platform.doc() = "Platform-specific application and context creation";

    #ifdef Magnum_GlfwApplication_FOUND
    py::module glfw = platform.def_submodule("glfw");
    magnum::platform::glfw(glfw);
    #endif

    #ifdef Magnum_Sdl2Application_FOUND
    py::module sdl2 = platform.def_submodule("sdl2");
    magnum::platform::sdl2(sdl2);
    #endif

    #ifdef Magnum_WindowlessEglApplication_FOUND
    py::module egl = platform.def_submodule("egl");
    magnum::platform::egl(egl);
    #endif

    #ifdef Magnum_WindowlessGlxApplication_FOUND
    py::module glx = platform.def_submodule("glx");
    magnum::platform::glx(glx);
    #endif
    #endif
}
