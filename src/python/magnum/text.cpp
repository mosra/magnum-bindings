/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022 Vladimír Vondruš <mosra@centrum.cz>

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
#include <Magnum/ImageView.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/DistanceFieldGlyphCache.h>
#include <Magnum/Text/Renderer.h>

#include "corrade/pluginmanager.h"
#include "magnum/bootstrap.h"

namespace magnum {

namespace {

/* For some reason having ...Args as the second (and not last) template
   argument does not work. So I'm listing all variants used more than once. */
template<class R, R(Text::AbstractFont::*f)() const> R checkOpened(Text::AbstractFont& self) {
    if(!self.isOpened()) {
        PyErr_SetString(PyExc_AssertionError, "no file opened");
        throw py::error_already_set{};
    }
    return (self.*f)();
}
template<class R, class Arg1, R(Text::AbstractFont::*f)(Arg1)> R checkOpened(Text::AbstractFont& self, Arg1 arg1) {
    if(!self.isOpened()) {
        PyErr_SetString(PyExc_AssertionError, "no file opened");
        throw py::error_already_set{};
    }
    return (self.*f)(arg1);
}

}

void text(py::module_& m) {
    m.doc() = "Text rendering";

    /* AbstractFont depends on this */
    py::module_::import("corrade.pluginmanager");

    #ifndef MAGNUM_BUILD_STATIC
    /* These are a part of the same module in the static build, no need to
       import (also can't import because there it's _magnum.*) */
    py::module_::import("magnum.gl");
    #endif

    /* Glyph caches */
    py::class_<Text::AbstractGlyphCache> abstractGlyphCache{m, "AbstractGlyphCache", "Base for glyph caches"};
    abstractGlyphCache
        /** @todo features */
        .def_property_readonly("texture_size", &Text::AbstractGlyphCache::textureSize, "Glyph cache texture size")
        .def_property_readonly("padding", &Text::AbstractGlyphCache::padding, "Glyph padding")
        /** @todo glyph iteration and population */
        ;
    py::class_<Text::GlyphCache, Text::AbstractGlyphCache> glyphCache{m, "GlyphCache", "Glyph cache"};
    glyphCache
        .def(py::init<GL::TextureFormat, const Vector2i&, const Vector2i&, const Vector2i&>(), "Constructor", py::arg("internal_format"), py::arg("original_size"), py::arg("size"), py::arg("padding"))
        .def(py::init<GL::TextureFormat, const Vector2i&, const Vector2i&>(), "Constructor", py::arg("internal_format"), py::arg("size"), py::arg("padding") = Vector2i{})
        .def(py::init<const Vector2i&, const Vector2i&, const Vector2i&>(), "Constructor", py::arg("original_size"), py::arg("size"), py::arg("padding"))
        .def(py::init<const Vector2i&, const Vector2i&>(), "Constructor", py::arg("size"), py::arg("padding") = Vector2i{})
        /* The default behavior when returning a reference seems to be that it
           increfs the originating instance and decrefs it again after the
           variable gets deleted. This is verified in test_text_gl.py to be
           extra sure. */
        .def_property_readonly("texture", &Text::GlyphCache::texture, "Cache texture");
    py::class_<Text::DistanceFieldGlyphCache, Text::GlyphCache> distanceFieldGlyphCache{m, "DistanceFieldGlyphCache", "Glyph cache with distance field rendering"};
    distanceFieldGlyphCache
        .def(py::init<const Vector2i&, const Vector2i&, UnsignedInt>(), "Constructor", py::arg("original_size"), py::arg("size"), py::arg("radius"))
        /** @todo setDistanceFieldImage, once needed for anything */
        ;

    /* Font */
    py::class_<Text::AbstractFont, PluginManager::PyPluginHolder<Text::AbstractFont>> abstractFont{m, "AbstractFont", "Interface for font plugins"};
    abstractFont
        /** @todo features */
        .def_property_readonly("is_opened", &Text::AbstractFont::isOpened, "Whether any file is opened")
        .def("open_data", [](Text::AbstractFont& self, Containers::ArrayView<const char> data, Float size) {
            /** @todo log redirection -- but we'd need assertions to not be
                part of that so when it dies, the user can still see why */
            if(self.openData(data, size)) return;

            PyErr_SetString(PyExc_RuntimeError, "opening data failed");
            throw py::error_already_set{};
        }, "Open raw data", py::arg("data"), py::arg("size"))
        .def("open_file", [](Text::AbstractFont& self, const std::string& filename, Float size) {
            /** @todo log redirection -- but we'd need assertions to not be
                part of that so when it dies, the user can still see why */
            if(self.openFile(filename, size)) return;

            PyErr_Format(PyExc_RuntimeError, "opening %s failed", filename.data());
            throw py::error_already_set{};
        }, "Open a file", py::arg("filename"), py::arg("size"))
        .def("close", &Text::AbstractFont::close, "Close currently opened file")

        .def_property_readonly("size", checkOpened<Float, &Text::AbstractFont::size>, "Font size")
        .def_property_readonly("ascent", checkOpened<Float, &Text::AbstractFont::ascent>, "Font ascent")
        .def_property_readonly("descent", checkOpened<Float, &Text::AbstractFont::descent>, "Font descent")
        .def_property_readonly("line_height", checkOpened<Float, &Text::AbstractFont::lineHeight>, "Line height")
        .def("glyph_id", checkOpened<UnsignedInt, char32_t, &Text::AbstractFont::glyphId>, "Glyph ID for given character", py::arg("character"))
        .def("glyph_advance", checkOpened<Vector2, UnsignedInt, &Text::AbstractFont::glyphAdvance>, "Glyph advance", py::arg("glyph"))
        .def("fill_glyph_cache", [](Text::AbstractFont& self, Text::AbstractGlyphCache& cache, const std::string& characters) {
            if(!self.isOpened()) {
                PyErr_SetString(PyExc_AssertionError, "no file opened");
                throw py::error_already_set{};
            }
            return self.fillGlyphCache(cache, characters);
        }, "Fill glyph cache with given character set", py::arg("cache"), py::arg("characters"))
        /** @todo createGlyphCache() */
        /** @todo layout and AbstractLayouter, once needed for anything */
        ;
    corrade::plugin(abstractFont);

    py::class_<PluginManager::Manager<Text::AbstractFont>, PluginManager::AbstractManager> fontManager{m, "FontManager", "Manager for font plugins"};
    corrade::manager(fontManager);

    py::enum_<Text::Alignment>{m, "Alignment", "Text rendering alignment"}
        .value("LINE_LEFT", Text::Alignment::LineLeft)
        .value("LINE_CENTER", Text::Alignment::LineCenter)
        .value("LINE_RIGHT", Text::Alignment::LineRight)
        .value("MIDDLE_LEFT", Text::Alignment::MiddleLeft)
        .value("MIDDLE_CENTER", Text::Alignment::MiddleCenter)
        .value("MIDDLE_RIGHT", Text::Alignment::MiddleRight)
        .value("TOP_LEFT", Text::Alignment::TopLeft)
        .value("TOP_CENTER", Text::Alignment::TopCenter)
        .value("TOP_RIGHT", Text::Alignment::TopRight)
        .value("LINE_CENTER_INTEGRAL", Text::Alignment::LineCenterIntegral)
        .value("MIDDLE_LEFT_INTEGRAL", Text::Alignment::MiddleLeftIntegral)
        .value("MIDDLE_CENTER_INTEGRAL", Text::Alignment::MiddleCenterIntegral)
        .value("MIDDLE_RIGHT_INTEGRAL", Text::Alignment::MiddleRightIntegral);

    /** @todo any reason to expose a 3D renderer? it isn't any different
        currently */
    py::class_<Text::Renderer2D> renderer2D{m, "Renderer2D", "2D text renderer"};
    renderer2D
        .def(py::init<Text::AbstractFont&, const Text::GlyphCache&, Float, Text::Alignment>(), "Constructor", py::arg("font"), py::arg("cache"), py::arg("size"), py::arg("alignment") = Text::Alignment::LineLeft)
        .def_property_readonly("capacity", &Text::Renderer2D::capacity, "Capacity for rendered glyphs")
        .def_property_readonly("rectangle", &Text::Renderer2D::rectangle, "Rectangle spanning the rendered text")
        /** @todo are the buffers useful for anything? */
        /* The default behavior when returning a reference seems to be that it
           increfs the originating instance and decrefs it again after the
           variable gets deleted. This is verified in test_text.py to be extra
           sure. */
        .def_property_readonly("mesh", &Text::Renderer2D::mesh, "Mesh")
        .def("reserve", &Text::Renderer2D::reserve, "Reserve capacity for renderered glyphs", py::arg("glyph_count"), py::arg("vertex_buffer_usage") = GL::BufferUsage::StaticDraw, py::arg("index_buffer_usage") = GL::BufferUsage::StaticDraw)
        .def("render", static_cast<void(Text::Renderer2D::*)(const std::string&)>(&Text::Renderer2D::render), "Render text", py::arg("text"));
}

}

#ifndef MAGNUM_BUILD_STATIC
/* TODO: remove declaration when https://github.com/pybind/pybind11/pull/1863
   is released */
extern "C" PYBIND11_EXPORT PyObject* PyInit_text();
PYBIND11_MODULE(text, m) {
    magnum::text(m);
}
#endif
