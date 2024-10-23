/*
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
*/

#include <pybind11/pybind11.h>
#include <Magnum/Platform/GlfwApplication.h>

#include "Corrade/PythonBindings.h"

#include "magnum/bootstrap.h"
#include "magnum/platform/application.h"
#include "magnum/platform/holder.h"

namespace magnum { namespace platform {

namespace {
    int argc = 0;
}

void glfw(py::module_& m) {
    m.doc() = "GLFW-based platform integration";

    struct PublicizedApplication: Platform::Application {
        explicit PublicizedApplication(const Configuration& configuration, const GLConfiguration& glConfiguration): Platform::Application{Arguments{argc, nullptr}, configuration, glConfiguration} {}

        void drawEvent() override {
            PyErr_SetString(PyExc_NotImplementedError, "the application has to provide a draw_event() method");
            throw py::error_already_set{};
        }

        void exitEvent(ExitEvent& event) override {
            /* The base implementation does this, otherwise the exit event is
               always cancelled. It's private so we can't call it directly. */
            event.setAccepted();
        }
        void viewportEvent(ViewportEvent&) override {}
        void keyPressEvent(KeyEvent&) override {}
        void keyReleaseEvent(KeyEvent&) override {}
        void pointerPressEvent(PointerEvent&) override {}
        void pointerReleaseEvent(PointerEvent&) override {}
        void pointerMoveEvent(PointerMoveEvent&) override {}
        void scrollEvent(ScrollEvent&) override {}

        /* The base doesn't have a virtual destructor because in C++ it's never
           deleted through a pointer to the base. Here we need it, though. */
        virtual ~PublicizedApplication() = default;
    };

    struct PyApplication: PublicizedApplication {
        using PublicizedApplication::PublicizedApplication;

        /* PYBIND11_OVERLOAD_NAME() calls object_api::operator() with implicit
           template param, which is return_value_policy::automatic_reference.
           That later gets changed to return_value_policy::copy in
           type_caster_base::cast() and there's no way to override that. */

        void exitEvent(ExitEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "exit_event",
                exitEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }

        void viewportEvent(ViewportEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "viewport_event",
                viewportEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }

        void drawEvent() override {
            #ifdef __clang__
            /* ugh pybind don't tell me I AM THE FIRST ON EARTH to get a
               warning here. Why there's no PYBIND11_OVERLOAD_NAME_ARG()
               variant *with* arguments and one without? */
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
            #endif
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "draw_event",
                drawEvent
            );
            #ifdef __clang__
            #pragma GCC diagnostic pop
            #endif
        }

        void keyPressEvent(KeyEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "key_press_event",
                keyPressEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }
        void keyReleaseEvent(KeyEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "key_release_event",
                keyReleaseEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }

        void pointerPressEvent(PointerEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "pointer_press_event",
                pointerPressEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }
        void pointerReleaseEvent(PointerEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "pointer_release_event",
                pointerReleaseEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }
        void pointerMoveEvent(PointerMoveEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "pointer_move_event",
                pointerMoveEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }
        void scrollEvent(ScrollEvent& event) override {
            PYBIND11_OVERLOAD_NAME(
                void,
                PublicizedApplication,
                "scroll_event",
                scrollEvent,
                /* Have to use std::ref() otherwise pybind tries to copy
                   it and fails */
                std::ref(event)
            );
        }
    };

    py::class_<PublicizedApplication, PyApplication, ApplicationHolder<PublicizedApplication>> glfwApplication{m, "Application", "GLFW application"};
    glfwApplication
        .def_property("swap_interval", nullptr,
            [](PublicizedApplication& self, Int interval) {
                self.setSwapInterval(interval);
            }, "Swap interval")
        .def("main_loop_iteration", &PyApplication::mainLoopIteration, "Run one iteration of application main loop");

    PyNonDestructibleClass<PublicizedApplication::ExitEvent> exitEvent_{glfwApplication, "ExitEvent", "Exit event"};
    PyNonDestructibleClass<PublicizedApplication::ViewportEvent> viewportEvent_{glfwApplication, "ViewportEvent", "Viewport event"};
    PyNonDestructibleClass<PublicizedApplication::InputEvent> inputEvent_{glfwApplication, "InputEvent", "Base for input events"};
    py::class_<PublicizedApplication::KeyEvent, PublicizedApplication::InputEvent> keyEvent_{glfwApplication, "KeyEvent", "Key event"};
    py::class_<PublicizedApplication::PointerEvent, PublicizedApplication::InputEvent> pointerEvent_{glfwApplication, "PointerEvent", "Pointer event"};
    py::class_<PublicizedApplication::PointerMoveEvent, PublicizedApplication::InputEvent> pointerMoveEvent_{glfwApplication, "PointerMoveEvent", "Pointer move event"};
    py::class_<PublicizedApplication::ScrollEvent, PublicizedApplication::InputEvent> scrollEvent_{glfwApplication, "ScrollEvent", "Scroll event"};

    py::enum_<Platform::Application::PointerEventSource>{glfwApplication, "PointerEventSource", "Pointer event source"}
        .value("MOUSE", Platform::Application::PointerEventSource::Mouse);

    py::enum_<Platform::Application::Pointer> pointer{glfwApplication, "Pointer", "Pointer"};
    pointer
        .value("MOUSE_LEFT", Platform::Application::Pointer::MouseLeft)
        .value("MOUSE_MIDDLE", Platform::Application::Pointer::MouseMiddle)
        .value("MOUSE_RIGHT", Platform::Application::Pointer::MouseRight)
        .value("MOUSE_BUTTON4", Platform::Application::Pointer::MouseButton4)
        .value("MOUSE_BUTTON5", Platform::Application::Pointer::MouseButton5);
    corrade::enumOperators(pointer);

    py::enum_<Platform::Application::Cursor>{glfwApplication, "Cursor", "Cursor type"}
        .value("ARROW", Platform::Application::Cursor::Arrow)
        .value("TEXT_INPUT", Platform::Application::Cursor::TextInput)
        .value("CROSSHAIR", Platform::Application::Cursor::Crosshair)
        #ifdef GLFW_RESIZE_NWSE_CURSOR
        .value("RESIZE_NWSE", Platform::Application::Cursor::ResizeNWSE)
        .value("RESIZE_NESW", Platform::Application::Cursor::ResizeNESW)
        #endif
        .value("RESIZE_WE", Platform::Application::Cursor::ResizeWE)
        .value("RESIZE_NS", Platform::Application::Cursor::ResizeNS)
        #ifdef GLFW_RESIZE_NWSE_CURSOR
        .value("RESIZE_ALL", Platform::Application::Cursor::ResizeAll)
        .value("NO", Platform::Application::Cursor::No)
        #endif
        .value("HAND", Platform::Application::Cursor::Hand)
        .value("HIDDEN", Platform::Application::Cursor::Hidden)
        .value("HIDDEN_LOCKED", Platform::Application::Cursor::HiddenLocked);

    application(glfwApplication);
    exitEvent(exitEvent_);
    viewportEvent(viewportEvent_);
    inputEvent(inputEvent_);
    keyEvent(keyEvent_);
    pointerEvent(pointerEvent_);
    pointerMoveEvent(pointerMoveEvent_);
    scrollEvent(scrollEvent_);
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
