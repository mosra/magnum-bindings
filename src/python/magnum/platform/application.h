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

#include "corrade/EnumOperators.h"
#include "magnum/bootstrap.h"

namespace magnum { namespace platform {

template<class T, class Trampoline> void application(py::class_<T, Trampoline>& c) {
    py::class_<typename T::Configuration> configuration{c, "Configuration", "Configuration"};
    configuration
        .def(py::init())
        .def_property("title", &T::Configuration::title,
            [](typename T::Configuration& self, const std::string& title) {
                self.setTitle(title);
            }, "Window title")
        .def_property("size", &T::Configuration::size,
            [](typename T::Configuration& self, const Vector2i& size) {
                self.setSize(size);
            }, "Window size");
        /** @todo others */

    py::class_<typename T::GLConfiguration> glConfiguration{c, "GLConfiguration", "OpenGL context configuration"};
    glConfiguration
        .def(py::init());
        /** @todo others */

    c
        /* Constructor */
        .def(py::init<const typename T::Configuration&, const typename T::GLConfiguration&>(), py::arg("configuration") = typename T::Configuration{}, py::arg("gl_configuration") = typename T::GLConfiguration{},
            "Constructor")
        /** @todo the nocreate ones */

        /* Basic things */
        .def("exec", &T::exec, "Execute application main loop")
        .def("exit", &T::exit, "Exit application main loop")

        /* Screen handling */
        .def("swap_buffers", &T::swapBuffers, "Swap buffers")
        /** @todo setMinimalLoopPeriod, needs a getter */
        .def("redraw", &T::redraw, "Redraw immediately")
        .def("window_size", &T::windowSize, "Window size")
        .def("framebuffer_size", &T::framebufferSize, "Framebuffer size")

        /* Event handlers */
        .def("draw_event", &T::drawEvent, "Draw event")
        .def("mouse_press_event", &T::mousePressEvent, "Mouse press event")
        .def("mouse_release_event", &T::mouseReleaseEvent, "Mouse release event")
        .def("mouse_move_event", &T::mouseMoveEvent, "Mouse move event")
        /** @todo more */
        ;
}

template<class T> void mouseEvent(py::class_<T>& c) {
    py::enum_<typename T::Button>{c, "Button", "Mouse button"}
        .value("LEFT", T::Button::Left)
        .value("MIDDLE", T::Button::Middle)
        .value("RIGHT", T::Button::Right);

    c
        .def_property_readonly("button", &T::button, "Button")
        .def_property_readonly("position", &T::position, "Position");
}

template<class T> void mouseMoveEvent(py::class_<T>& c) {
    py::enum_<typename T::Button> buttons{c, "Buttons", "Set of mouse buttons"};
    buttons
        .value("LEFT", T::Button::Left)
        .value("MIDDLE", T::Button::Middle)
        .value("RIGHT", T::Button::Right);
    corrade::enumOperators(buttons);

    c
        .def_property_readonly("position", &T::position, "Position")
        .def_property_readonly("buttons", [](T& self) {
            return typename T::Button(typename std::underlying_type<typename T::Button>::type(self.buttons()));
        }, "Mouse buttons");
}

}}
