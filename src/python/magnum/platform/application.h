#ifndef magnum_platform_application_h
#define magnum_platform_application_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025
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
#include <Corrade/Containers/StringStl.h> /** @todo drop once we have our string casters */

#include "Corrade/Containers/OptionalPythonBindings.h" /* for pointers() */

#include "corrade/EnumOperators.h"
#include "magnum/bootstrap.h"

namespace magnum { namespace platform {

template<class T, class Trampoline, class Holder> void application(py::class_<T, Trampoline, Holder>& c) {
    py::class_<typename T::Configuration> configuration{c, "Configuration", "Configuration"};

    py::enum_<typename T::Configuration::WindowFlag> configurationWindowFlags{configuration, "WindowFlag", "WindowFlag"};
    configurationWindowFlags
        .value("RESIZABLE", T::Configuration::WindowFlag::Resizable);
    corrade::enumOperators(configurationWindowFlags);

    /** @todo drop this in favor of named constructor arguments, that's what
        the Configuration tries to emulate after all */
    configuration
        .def(py::init())
        .def_property("title",
            /** @todo drop std::string in favor of our own string caster */
            [](typename T::Configuration& self) -> std::string {
                return self.title();
            },
            [](typename T::Configuration& self, const std::string& title) {
                self.setTitle(title);
            }, "Window title")
        .def_property("size", &T::Configuration::size,
            [](typename T::Configuration& self, const Vector2i& size) {
                self.setSize(size);
            }, "Window size")
        .def_property("window_flags",
            [](typename T::Configuration& self) {
                return typename T::Configuration::WindowFlag(typename std::underlying_type<typename T::Configuration::WindowFlag>::type(self.windowFlags()));
            },
            [](typename T::Configuration& self, typename T::Configuration::WindowFlag flags) {
                self.setWindowFlags(flags);
            }, "Window flags");
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
        .def_property_readonly("window_size", &T::windowSize, "Window size")
        .def_property_readonly("framebuffer_size", &T::framebufferSize, "Framebuffer size")
        .def_property_readonly("dpi_scaling", static_cast<Vector2(T::*)() const>(&T::dpiScaling), "DPI scaling")
        /* Mouse handling */
        .def_property("cursor", &T::cursor, &T::setCursor, "Cursor type")
        .def("warp_cursor", &T::warpCursor, "Warp mouse cursor to given coordinates")
        /* Event handlers */
        .def("exit_event", &T::exitEvent, "Exit event")
        .def("viewport_event", &T::viewportEvent, "Viewport event")
        .def("draw_event", &T::drawEvent, "Draw event")
        .def("key_press_event", &T::keyPressEvent, "Key press event")
        .def("key_release_event", &T::keyReleaseEvent, "Key release event")
        .def("pointer_press_event", &T::pointerPressEvent, "Pointer press event")
        .def("pointer_release_event", &T::pointerReleaseEvent, "Pointer release event")
        .def("pointer_move_event", &T::pointerMoveEvent, "Pointer move event")
        .def("scroll_event", &T::scrollEvent, "Scroll event")
        /** @todo text input/editing event */
        ;

    py::enum_<typename T::Modifier> modifiers{c, "Modifier", "Keyboard modifier"};
    modifiers
        .value("SHIFT", T::Modifier::Shift)
        .value("CTRL", T::Modifier::Ctrl)
        .value("ALT", T::Modifier::Alt)
        .value("SUPER", T::Modifier::Super);
    corrade::enumOperators(modifiers);

    py::enum_<typename T::Key>{c, "Key", "Key"}
        .value("UNKNOWN", T::Key::Unknown)
        .value("LEFT_SHIFT", T::Key::LeftShift)
        .value("RIGHT_SHIFT", T::Key::RightShift)
        .value("LEFT_CTRL", T::Key::LeftCtrl)
        .value("RIGHT_CTRL", T::Key::RightCtrl)
        .value("LEFT_ALT", T::Key::LeftAlt)
        .value("RIGHT_ALT", T::Key::RightAlt)
        .value("LEFT_SUPER", T::Key::LeftSuper)
        .value("RIGHT_SUPER", T::Key::RightSuper)

        .value("ENTER", T::Key::Enter)
        .value("ESC", T::Key::Esc)

        .value("UP", T::Key::Up)
        .value("DOWN", T::Key::Down)
        .value("LEFT", T::Key::Left)
        .value("RIGHT", T::Key::Right)
        .value("HOME", T::Key::Home)
        .value("END", T::Key::End)
        .value("PAGE_UP", T::Key::PageUp)
        .value("PAGE_DOWN", T::Key::PageDown)
        .value("BACKSPACE", T::Key::Backspace)
        .value("INSERT", T::Key::Insert)
        .value("DELETE", T::Key::Delete)

        .value("F1", T::Key::F1)
        .value("F2", T::Key::F2)
        .value("F3", T::Key::F3)
        .value("F4", T::Key::F4)
        .value("F5", T::Key::F5)
        .value("F6", T::Key::F6)
        .value("F7", T::Key::F7)
        .value("F8", T::Key::F8)
        .value("F9", T::Key::F9)
        .value("F10", T::Key::F10)
        .value("F11", T::Key::F11)
        .value("F12", T::Key::F12)

        .value("SPACE", T::Key::Space)
        .value("TAB", T::Key::Tab)
        .value("QUOTE", T::Key::Quote)
        .value("COMMA", T::Key::Comma)
        .value("PERIOD", T::Key::Period)
        .value("MINUS", T::Key::Minus)
        .value("PLUS", T::Key::Plus)
        .value("SLASH", T::Key::Slash)
        .value("PERCENT", T::Key::Percent)
        .value("SEMICOLON", T::Key::Semicolon)
        .value("EQUAL", T::Key::Equal)
        .value("LEFT_BRACKET", T::Key::LeftBracket)
        .value("RIGHT_BRACKET", T::Key::RightBracket)
        .value("BACKSLASH", T::Key::Backslash)
        .value("BACKQUOTE", T::Key::Backquote)

        /* World1 / World2 supported only by GlfwApplication, omitted */

        .value("ZERO", T::Key::Zero)
        .value("ONE", T::Key::One)
        .value("TWO", T::Key::Two)
        .value("THREE", T::Key::Three)
        .value("FOUR", T::Key::Four)
        .value("FIVE", T::Key::Five)
        .value("SIX", T::Key::Six)
        .value("SEVEN", T::Key::Seven)
        .value("EIGHT", T::Key::Eight)
        .value("NINE", T::Key::Nine)

        .value("A", T::Key::A)
        .value("B", T::Key::B)
        .value("C", T::Key::C)
        .value("D", T::Key::D)
        .value("E", T::Key::E)
        .value("F", T::Key::F)
        .value("G", T::Key::G)
        .value("H", T::Key::H)
        .value("I", T::Key::I)
        .value("J", T::Key::J)
        .value("K", T::Key::K)
        .value("L", T::Key::L)
        .value("M", T::Key::M)
        .value("N", T::Key::N)
        .value("O", T::Key::O)
        .value("P", T::Key::P)
        .value("Q", T::Key::Q)
        .value("R", T::Key::R)
        .value("S", T::Key::S)
        .value("T", T::Key::T)
        .value("U", T::Key::U)
        .value("V", T::Key::V)
        .value("W", T::Key::W)
        .value("X", T::Key::X)
        .value("Y", T::Key::Y)
        .value("Z", T::Key::Z)

        .value("NUM_ZERO", T::Key::NumZero)
        .value("NUM_ONE", T::Key::NumOne)
        .value("NUM_TWO", T::Key::NumTwo)
        .value("NUM_THREE", T::Key::NumThree)
        .value("NUM_FOUR", T::Key::NumFour)
        .value("NUM_FIVE", T::Key::NumFive)
        .value("NUM_SIX", T::Key::NumSix)
        .value("NUM_SEVEN", T::Key::NumSeven)
        .value("NUM_EIGHT", T::Key::NumEight)
        .value("NUM_NINE", T::Key::NumNine)
        .value("NUM_DECIMAL", T::Key::NumDecimal)
        .value("NUM_DIVIDE", T::Key::NumDivide)
        .value("NUM_MULTIPLY", T::Key::NumMultiply)
        .value("NUM_SUBTRACT", T::Key::NumSubtract)
        .value("NUM_ADD", T::Key::NumAdd)
        .value("NUM_ENTER", T::Key::NumEnter)
        .value("NUM_EQUAL", T::Key::NumEqual);

    /* The PointerEventSource and Pointer enums are defined for each
       application separately, as each has a different set of values */
}

template<class T, class ...Args> void exitEvent(py::class_<T, Args...>& c) {
    c
        .def_property("accepted", &T::isAccepted, &T::setAccepted, "Accepted status of the event");
}

template<class T, class ...Args> void viewportEvent(py::class_<T, Args...>& c) {
    c
        .def_property_readonly("window_size", &T::windowSize, "Window size")
        .def_property_readonly("framebuffer_size", &T::framebufferSize, "Framebuffer size")
        .def_property_readonly("dpi_scaling", &T::dpiScaling, "DPI scaling");
}

template<class T, class ...Args> void inputEvent(py::class_<T, Args...>& c) {
    c.def_property("accepted", &T::isAccepted, &T::setAccepted, "Accepted status of the event");
}

template<class T, class ...Args> void keyEvent(py::class_<T, Args...>& c) {
    c
        .def_property_readonly("key", &T::key, "Key")
        /** @todo key name? useful? useles?? */
        .def_property_readonly("modifiers", [](T& self) {
            return typename decltype(self.modifiers())::Type(enumCastUnderlyingType(self.modifiers()));
        }, "Modifiers")
        .def_property_readonly("is_repeated", &T::isRepeated, "Whether the key press is repeated");
}

template<class T, class ...Args> void pointerEvent(py::class_<T, Args...>& c) {
    c
        .def_property_readonly("source", &T::source, "Pointer event source")
        .def_property_readonly("pointer", &T::pointer, "Pointer type that was pressed or released")
        .def_property_readonly("is_primary", &T::isPrimary, "Whether the pointer is primary")
        .def_property_readonly("id", &T::id, "Pointer ID")
        .def_property_readonly("position", &T::position, "Position")
        .def_property_readonly("modifiers", [](T& self) {
            return typename decltype(self.modifiers())::Type(enumCastUnderlyingType(self.modifiers()));
        }, "Keyboard modifiers");
}

template<class T, class ...Args> void pointerMoveEvent(py::class_<T, Args...>& c) {
    c
        .def_property_readonly("source", &T::source, "Pointer event source")
        .def_property_readonly("pointer", &T::pointer, "Pointer type that was added or removed from the set of pressed pointers")
        .def_property_readonly("pointers", [](T& self) {
            return typename decltype(self.pointers())::Type(enumCastUnderlyingType(self.pointers()));
        }, "Pointer types pressed in this event")
        .def_property_readonly("is_primary", &T::isPrimary, "Whether the pointer is primary")
        .def_property_readonly("id", &T::id, "Pointer ID")
        .def_property_readonly("position", &T::position, "Position")
        .def_property_readonly("relative_position", &T::relativePosition, "Relative position")
        .def_property_readonly("modifiers", [](T& self) {
            return typename decltype(self.modifiers())::Type(enumCastUnderlyingType(self.modifiers()));
        }, "Keyboard modifiers");
}

template<class T, class ...Args> void scrollEvent(py::class_<T, Args...>& c) {
    c
        .def_property_readonly("offset", &T::offset, "Offset")
        .def_property_readonly("position", &T::position, "Position")
        .def_property_readonly("modifiers", [](T& self) {
            return typename decltype(self.modifiers())::Type(enumCastUnderlyingType(self.modifiers()));
        }, "Keyboard modifiers");
}

}}

#endif
