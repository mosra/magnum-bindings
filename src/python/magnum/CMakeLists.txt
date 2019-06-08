#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
#             Vladimír Vondruš <mosra@centrum.cz>
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#

# *Not* REQUIRED
find_package(Magnum COMPONENTS GL Shaders SceneGraph MeshTools Primitives Trade)

set(magnum_SRCS
    magnum.cpp
    math.cpp
    math.matrixfloat.cpp
    math.matrixdouble.cpp
    math.vectorfloat.cpp
    math.vectorintegral.cpp)

pybind11_add_module(magnum ${magnum_SRCS})
target_include_directories(magnum PRIVATE ${PROJECT_SOURCE_DIR}/src/python)
target_link_libraries(magnum PRIVATE Magnum::Magnum)
set_target_properties(magnum PROPERTIES
    FOLDER "python"
    OUTPUT_NAME "_magnum"
    LIBRARY_OUTPUT_DIRECTORY ${output_dir})

if(Magnum_GL_FOUND)
    set(magnum_gl_SRCS
        gl.cpp)

    pybind11_add_module(magnum_gl ${magnum_gl_SRCS})
    target_include_directories(magnum_gl PRIVATE ${PROJECT_SOURCE_DIR}/src/python)
    target_link_libraries(magnum_gl PRIVATE Magnum::GL)
    set_target_properties(magnum_gl PROPERTIES
        FOLDER "python"
        OUTPUT_NAME "gl"
        LIBRARY_OUTPUT_DIRECTORY ${output_dir}/magnum)
endif()

if(Magnum_MeshTools_FOUND)
    set(magnum_meshtools_SRCS
        meshtools.cpp)

    pybind11_add_module(magnum_meshtools ${magnum_meshtools_SRCS})
    target_include_directories(magnum_meshtools PRIVATE ${PROJECT_SOURCE_DIR}/src/python)
    target_link_libraries(magnum_meshtools PRIVATE Magnum::MeshTools)
    set_target_properties(magnum_meshtools PROPERTIES
        FOLDER "python"
        OUTPUT_NAME "meshtools"
        LIBRARY_OUTPUT_DIRECTORY ${output_dir}/magnum)
endif()

if(Magnum_Primitives_FOUND)
    set(magnum_primitives_SRCS
        primitives.cpp)

    pybind11_add_module(magnum_primitives ${magnum_primitives_SRCS})
    target_include_directories(magnum_primitives PRIVATE ${PROJECT_SOURCE_DIR}/src/python)
    target_link_libraries(magnum_primitives PRIVATE Magnum::Primitives)
    set_target_properties(magnum_primitives PROPERTIES
        FOLDER "python"
        OUTPUT_NAME "primitives"
        LIBRARY_OUTPUT_DIRECTORY ${output_dir}/magnum)
endif()

if(Magnum_SceneGraph_FOUND)
    set(magnum_scenegraph_SRCS
        scenegraph.cpp
        scenegraph.matrix.cpp
        scenegraph.trs.cpp)

    pybind11_add_module(magnum_scenegraph ${magnum_scenegraph_SRCS})
    target_include_directories(magnum_scenegraph PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/python)
    target_link_libraries(magnum_scenegraph PRIVATE Magnum::SceneGraph)
    set_target_properties(magnum_scenegraph PROPERTIES
        FOLDER "python"
        OUTPUT_NAME "scenegraph"
        LIBRARY_OUTPUT_DIRECTORY ${output_dir}/magnum)
endif()

if(Magnum_Shaders_FOUND)
    set(magnum_shaders_SRCS
        shaders.cpp)

    pybind11_add_module(magnum_shaders ${magnum_shaders_SRCS})
    target_include_directories(magnum_shaders PRIVATE ${PROJECT_SOURCE_DIR}/src/python)
    target_link_libraries(magnum_shaders PRIVATE Magnum::Shaders)
    set_target_properties(magnum_shaders PROPERTIES
        FOLDER "python"
        OUTPUT_NAME "shaders"
        LIBRARY_OUTPUT_DIRECTORY ${output_dir}/magnum)
endif()

if(Magnum_Trade_FOUND)
    set(magnum_trade_SRCS
        trade.cpp)

    pybind11_add_module(magnum_trade ${magnum_trade_SRCS})
    target_include_directories(magnum_trade PRIVATE ${PROJECT_SOURCE_DIR}/src/python)
    target_link_libraries(magnum_trade PRIVATE Magnum::Trade)
    set_target_properties(magnum_trade PROPERTIES
        FOLDER "python"
        OUTPUT_NAME "trade"
        LIBRARY_OUTPUT_DIRECTORY ${output_dir}/magnum)
endif()

file(GENERATE OUTPUT ${output_dir}/magnum/__init__.py
    INPUT ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py)

add_subdirectory(platform)
