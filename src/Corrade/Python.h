#ifndef Corrade_Python_h
#define Corrade_Python_h
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
#include <Corrade/Utility/Assert.h>

namespace Corrade {

template<class T> inline pybind11::handle pyHandleFromInstance(T& obj) {
    /** @todo don't tell me there's no API for this either, ugh */
    return pybind11::detail::get_object_handle(&obj, pybind11::detail::get_type_info(typeid(T)));
}

template<class T> inline pybind11::object pyObjectFromInstance(T& obj) {
    /* reinterpret_borrow?! seriously?! */
    return pybind11::reinterpret_borrow<pybind11::object>(pyHandleFromInstance(obj));
}

template<class T> inline T& pyInstanceFromHandle(pybind11::handle handle) {
    /** @todo and this?! ugh! there's handle.cast() but that calls
        caster.load(handle, true) which we DO NOT WANT */
    /* Stolen from pybind11::class_::def_buffer(). Not sure what exactly it
       does, but I assume caster is implicitly convertible to Class& and thus
       can magically access the actual Class from the PyObject. */
    pybind11::detail::make_caster<T> caster;
    CORRADE_INTERNAL_ASSERT(caster.load(handle, /*convert=*/false));
    return caster;
}

}

#endif
