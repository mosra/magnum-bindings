..
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
..

.. py:class:: magnum.text.FontManager
    :summary: Manager for :ref:`AbstractFont` plugin instances

    Each plugin returned by :ref:`instantiate()` or :ref:`load_and_instantiate()`
    references its owning :ref:`FontManager` through
    :ref:`AbstractFont.manager`, ensuring the manager is not deleted before the
    plugin instances are.

.. TODO couldn't the plugin_interface etc. docs be parsed from pybind's docs?
    repeating them for every plugin is annoying

.. py:class:: magnum.text.AbstractFont
    :data plugin_interface: Plugin interface string
    :data plugin_search_paths: Plugin search paths
    :data plugin_suffix: Plugin suffix
    :data plugin_metadata_suffix: Plugin metadata suffix

    Similarly to C++, font plugins are loaded through :ref:`FontManager`:

    ..
        >>> from magnum import text

    .. code:: py

        >>> manager = text.FontManager()
        >>> font = manager.load_and_instantiate('StbTrueTypeFont')

    Unlike C++, errors in both API usage and file parsing are reported by
    raising an exception. See particular function documentation for detailed
    behavior.

.. py:function:: magnum.text.AbstractFont.open_data
    :raise RuntimeError: If file opening fails

.. py:function:: magnum.text.AbstractFont.open_file
    :raise RuntimeError: If file opening fails

    For compatibility with :ref:`os.path`, on Windows this function converts
    all backslashes in :p:`filename` to forward slashes before passing it to
    :dox:`Text::AbstractFont::openFile()`, which expects forward slashes as
    directory separators on all platforms.

.. py:property:: magnum.text.AbstractFont.size
    :raise AssertionError: If no file is opened
.. py:property:: magnum.text.AbstractFont.ascent
    :raise AssertionError: If no file is opened
.. py:property:: magnum.text.AbstractFont.descent
    :raise AssertionError: If no file is opened
.. py:property:: magnum.text.AbstractFont.line_height
    :raise AssertionError: If no file is opened
.. py:property:: magnum.text.AbstractFont.glyph_count
    :raise AssertionError: If no file is opened
.. py:function:: magnum.text.AbstractFont.glyph_id
    :raise AssertionError: If no file is opened
.. py:function:: magnum.text.AbstractFont.glyph_size
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`glyph` is negative or not less than
        :ref:`glyph_count`
.. py:function:: magnum.text.AbstractFont.glyph_advance
    :raise AssertionError: If no file is opened
    :raise IndexError: If :p:`glyph` is negative or not less than
        :ref:`glyph_count`
.. py:function:: magnum.text.AbstractFont.fill_glyph_cache
    :raise AssertionError: If no file is opened
.. py:function:: magnum.text.AbstractFont.create_shaper
    :raise AssertionError: If no file is opened

.. TODO remove the copies once the base class methods don't leak to subclasses
.. py:property:: magnum.text.RendererCore.cursor
    :raise AssertionError: If setting this property while rendering is in
        progress
.. py:property:: magnum.text.Renderer.cursor
    :raise AssertionError: If setting this property while rendering is in
        progress
.. py:property:: magnum.text.RendererGL.cursor
    :raise AssertionError: If setting this property while rendering is in
        progress

.. py:property:: magnum.text.RendererCore.alignment
    :raise AssertionError: If setting this property while rendering is in
        progress
.. py:property:: magnum.text.Renderer.alignment
    :raise AssertionError: If setting this property while rendering is in
        progress
.. py:property:: magnum.text.RendererGL.alignment
    :raise AssertionError: If setting this property while rendering is in
        progress

.. py:property:: magnum.text.RendererCore.line_advance
    :raise AssertionError: If setting this property while rendering is in
        progress
.. py:property:: magnum.text.Renderer.line_advance
    :raise AssertionError: If setting this property while rendering is in
        progress
.. py:property:: magnum.text.RendererGL.line_advance
    :raise AssertionError: If setting this property while rendering is in
        progress

.. py:property:: magnum.text.RendererGL.index_type
    :raise AssertionError: If setting this property while rendering is in
        progress

.. py:function:: magnum.text.RendererCore.add
    :raise AssertionError: If :p:`shaper` font isn't present in
        :ref:`glyph_cache`
.. py:function:: magnum.text.Renderer.add
    :raise AssertionError: If :p:`shaper` font isn't present in
        :ref:`glyph_cache`
.. py:function:: magnum.text.RendererGL.add
    :raise AssertionError: If :p:`shaper` font isn't present in
        :ref:`glyph_cache`

.. py:function:: magnum.text.RendererGL.render(self, shaper: magnum.text.AbstractShaper, size: float, text: str, features: list[magnum.text.FeatureRange])
    :raise AssertionError: If :p:`shaper` font isn't present in
        :ref:`glyph_cache`

.. py:enum:: magnum.text.Feature

    The equivalent to C++ :dox:`Text::feature()` is passing the four-character
    code to the constructor:

    ..
        >>> from magnum import text

    .. code:: pycon

        >>> feature = text.Feature('kern')
        >>> feature.name
        'KERNING'

.. py:enum:: magnum.text.Script

    The equivalent to C++ :dox:`Text::script()` is passing the four-character
    code to the constructor:

    ..
        >>> from magnum import text

    .. code:: pycon

        >>> script = text.Script('Latn')
        >>> script.name
        'LATIN'
