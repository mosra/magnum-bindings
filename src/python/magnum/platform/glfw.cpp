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
#include <Magnum/Platform/GlfwApplication.h>

#include "magnum/bootstrap.h"
#include "magnum/platform/application.h"

namespace magnum { namespace platform {

namespace {
    int argc = 0;
}

void glfw(py::module& m) {
    m.doc() = "GLFW-based platform integration";

    struct PublicizedApplication: Platform::Application {
        explicit PublicizedApplication(const Configuration& configuration, const GLConfiguration& glConfiguration): Platform::Application{Arguments{argc, nullptr}, configuration, glConfiguration} {}

        /* MSVC dies with "error C3640: a referenced or virtual member function
           of a local class must be defined" if this is just `= 0` here. Since
           we're overriding this method below anyway, it doesn't have to be
           pure virtual. */
        #ifdef _MSC_VER
        void drawEvent() override {}
        #else
        void drawEvent() override = 0;
        #endif

        void mousePressEvent(MouseEvent&) override {}
        void mouseReleaseEvent(MouseEvent&) override {}
        void mouseMoveEvent(MouseMoveEvent&) override {}

        /* The base doesn't have a virtual destructor because in C++ it's never
           deleted through a pointer to the base. Here we need it, though. */
        virtual ~PublicizedApplication() {}
    };

    struct PyApplication: PublicizedApplication {
        using PublicizedApplication::PublicizedApplication;

        void drawEvent() override {
            #ifdef __clang__
            /* ugh pybind don't tell me I AM THE FIRST ON EARTH to get a
               warning here. Why there's no PYBIND11_OVERLOAD_PURE_NAME_ARG()
               variant *with* arguments and one without? */
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
            #endif
            PYBIND11_OVERLOAD_PURE_NAME(
                void,
                PublicizedApplication,
                "draw_event",
                drawEvent
            );
            #ifdef __clang__
            #pragma GCC diagnostic pop
            #endif
        }

        void mousePressEvent(MouseEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "mouse_press_event",
                mousePressEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }

        void mouseReleaseEvent(MouseEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "mouse_release_event",
                mouseReleaseEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }

        void mouseMoveEvent(MouseMoveEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "mouse_move_event",
                mouseMoveEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }
    };

    py::class_<PublicizedApplication, PyApplication> glfwApplication{m, "Application", "GLFW application"};
    /** @todo def_property_writeonly for swap_interval */

    py::class_<PyApplication::MouseEvent> mouseEvent_{glfwApplication, "MouseEvent", "Mouse event"};
    py::class_<PyApplication::MouseMoveEvent> mouseMoveEvent_{glfwApplication, "MouseMoveEvent", "Mouse move event"};

    application(glfwApplication);
    mouseEvent(mouseEvent_);
    mouseMoveEvent(mouseMoveEvent_);
}

}}

#ifndef MAGNUM_BUILD_STATIC
/* TODO: remove declaration when https://github.com/pybind/pybind11/pull/1863
   is released */
extern "C" PYBIND11_EXPORT PyObject* PyInit_glfw();
PYBIND11_MODULE(glfw, m) {
    magnum::platform::glfw(m);
}
#endif
