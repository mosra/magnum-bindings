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

.. py:module:: magnum.platform

    While concrete implementations of :py:`Application` and
    :py:`WindowlessApplication` classes are available in the submodules,
    there's a convenience logic importing the most fitting implementation
    that's available directly into the `platform` module, meaning you can do
    just this without having to think about a particular implementation, for
    example:

    .. code:: py

        from magnum import platform

        class MyApp(platform.Application): # platform.sdl2.Application
            ...

    The same goes for :py:`WindowlessApplication` implementations, for example:

    .. code:: py

        from magnum import platform

        class MyApp(platform.WindowlessApplication): # platform.egl.WindowlessApp
            ...

    .. block-warning:: Subject to change

        At the moment, there's no way to specify a different priority order or
        disable the auto-import altogether.
