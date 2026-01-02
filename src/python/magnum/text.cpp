/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
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
#include <Corrade/Containers/ArrayViewStl.h> /** @todo drop once we have our list casters */
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/PairStl.h> /** @todo drop once we have our pair casters */
#include <Corrade/Containers/PointerStl.h>
#include <Corrade/Containers/StringStl.h> /** @todo drop once we have our string casters */
#include <Magnum/ImageView.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractShaper.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Text/Direction.h>
#include <Magnum/Text/DistanceFieldGlyphCacheGL.h>
#include <Magnum/Text/Feature.h>
#include <Magnum/Text/RendererGL.h>
#include <Magnum/Text/Script.h>

#include "corrade/pluginmanager.h"
#include "magnum/bootstrap.h"

#ifdef CORRADE_TARGET_WINDOWS
/* To allow people to conveniently use Python's os.path, we need to convert
   backslashes to forward slashes as all Corrade and Magnum APIs expect
   forward */
#include <Corrade/Utility/Path.h>
#endif

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
template<class R, R(Text::AbstractFont::*f)(UnsignedInt)> R checkOpenedBounds(Text::AbstractFont& self, UnsignedInt glyph) {
    if(!self.isOpened()) {
        PyErr_SetString(PyExc_AssertionError, "no file opened");
        throw py::error_already_set{};
    }
    if(glyph >= self.glyphCount()) {
        PyErr_SetNone(PyExc_IndexError);
        throw py::error_already_set{};
    }
    return (self.*f)(glyph);
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
    py::class_<Text::AbstractGlyphCache>{m, "AbstractGlyphCache", "Base for glyph caches"}
        /** @todo features */
        .def_property_readonly("format", &Text::AbstractGlyphCache::format, "Glyph cache format")
        .def_property_readonly("processed_format", &Text::AbstractGlyphCache::processedFormat, "Processed glyph cache format")
        .def_property_readonly("size", &Text::AbstractGlyphCache::size, "Glyph cache texture size")
        .def_property_readonly("processed_size", &Text::AbstractGlyphCache::processedSize, "Processed glyph cache texture size")
        .def_property_readonly("padding", &Text::AbstractGlyphCache::padding, "Glyph padding")
        /** @todo font / glyph iteration and population */
        /** @todo image, processedImage, setProcessedImage, once needed for
            anything */
        ;
    py::class_<Text::GlyphCacheGL, Text::AbstractGlyphCache>{m, "GlyphCacheGL", "OpenGL implementation of a glyph cache"}
        .def(py::init<PixelFormat, const Vector2i&, const Vector2i&>(), "Constructor", py::arg("format"), py::arg("size"), py::arg("padding") = Vector2i{1})
        /* The default behavior when returning a reference seems to be that it
           increfs the originating instance and decrefs it again after the
           variable gets deleted. This is verified in test_text_gl.py to be
           extra sure. */
        .def_property_readonly("texture", &Text::GlyphCacheGL::texture, "Cache texture");
    py::class_<Text::DistanceFieldGlyphCacheGL, Text::GlyphCacheGL>{m, "DistanceFieldGlyphCacheGL", "OpenGL glyph cache with distance field rendering"}
        .def(py::init<const Vector2i&, const Vector2i&, UnsignedInt>(), "Constructor", py::arg("size"), py::arg("processed_size"), py::arg("radius"));

    py::enum_<Text::Feature>{m, "Feature", "Open Type typographic feature"}
        .value("ACCESS_ALL_ALTERNATES", Text::Feature::AccessAllAlternates)
        .value("ABOVE_BASE_FORMS", Text::Feature::AboveBaseForms)
        .value("ABOVE_BASE_MARK_POSITIONING", Text::Feature::AboveBaseMarkPositioning)
        .value("ABOVE_BASE_SUBSTITUTIONS", Text::Feature::AboveBaseSubstitutions)
        .value("ALTERNATIVE_FRACTIONS", Text::Feature::AlternativeFractions)
        .value("AKHAND", Text::Feature::Akhand)
        .value("KERNING_FOR_ALTERNATE_PROPORTIONAL_WIDTHS", Text::Feature::KerningForAlternateProportionalWidths)
        .value("BELOW_BASE_FORMS", Text::Feature::BelowBaseForms)
        .value("BELOW_BASE_MARK_POSITIONING", Text::Feature::BelowBaseMarkPositioning)
        .value("BELOW_BASE_SUBSTITUTIONS", Text::Feature::BelowBaseSubstitutions)
        .value("CONTEXTUAL_ALTERNATES", Text::Feature::ContextualAlternates)
        .value("CASE_SENSITIVE_FORMS", Text::Feature::CaseSensitiveForms)
        .value("GLYPH_COMPOSITION_DECOMPOSITION", Text::Feature::GlyphCompositionDecomposition)
        .value("CONJUNCT_FORM_AFTER_RO", Text::Feature::ConjunctFormAfterRo)
        .value("CONTEXTUAL_HALF_WIDTH_SPACING", Text::Feature::ContextualHalfWidthSpacing)
        .value("CONJUNCT_FORMS", Text::Feature::ConjunctForms)
        .value("CONTEXTUAL_LIGATURES", Text::Feature::ContextualLigatures)
        .value("CENTERED_CJK_PUNCTUATION", Text::Feature::CenteredCjkPunctuation)
        .value("CAPITAL_SPACING", Text::Feature::CapitalSpacing)
        .value("CONTEXTUAL_SWASH", Text::Feature::ContextualSwash)
        .value("CURSIVE_POSITIONING", Text::Feature::CursivePositioning)
        .value("CHARACTER_VARIANTS1", Text::Feature::CharacterVariants1)
        .value("CHARACTER_VARIANTS2", Text::Feature::CharacterVariants2)
        .value("CHARACTER_VARIANTS3", Text::Feature::CharacterVariants3)
        .value("CHARACTER_VARIANTS4", Text::Feature::CharacterVariants4)
        .value("CHARACTER_VARIANTS5", Text::Feature::CharacterVariants5)
        .value("CHARACTER_VARIANTS6", Text::Feature::CharacterVariants6)
        .value("CHARACTER_VARIANTS7", Text::Feature::CharacterVariants7)
        .value("CHARACTER_VARIANTS8", Text::Feature::CharacterVariants8)
        .value("CHARACTER_VARIANTS9", Text::Feature::CharacterVariants9)
        .value("CHARACTER_VARIANTS10", Text::Feature::CharacterVariants10)
        .value("CHARACTER_VARIANTS11", Text::Feature::CharacterVariants11)
        .value("CHARACTER_VARIANTS12", Text::Feature::CharacterVariants12)
        .value("CHARACTER_VARIANTS13", Text::Feature::CharacterVariants13)
        .value("CHARACTER_VARIANTS14", Text::Feature::CharacterVariants14)
        .value("CHARACTER_VARIANTS15", Text::Feature::CharacterVariants15)
        .value("CHARACTER_VARIANTS16", Text::Feature::CharacterVariants16)
        .value("CHARACTER_VARIANTS17", Text::Feature::CharacterVariants17)
        .value("CHARACTER_VARIANTS18", Text::Feature::CharacterVariants18)
        .value("CHARACTER_VARIANTS19", Text::Feature::CharacterVariants19)
        .value("CHARACTER_VARIANTS20", Text::Feature::CharacterVariants20)
        .value("CHARACTER_VARIANTS21", Text::Feature::CharacterVariants21)
        .value("CHARACTER_VARIANTS22", Text::Feature::CharacterVariants22)
        .value("CHARACTER_VARIANTS23", Text::Feature::CharacterVariants23)
        .value("CHARACTER_VARIANTS24", Text::Feature::CharacterVariants24)
        .value("CHARACTER_VARIANTS25", Text::Feature::CharacterVariants25)
        .value("CHARACTER_VARIANTS26", Text::Feature::CharacterVariants26)
        .value("CHARACTER_VARIANTS27", Text::Feature::CharacterVariants27)
        .value("CHARACTER_VARIANTS28", Text::Feature::CharacterVariants28)
        .value("CHARACTER_VARIANTS29", Text::Feature::CharacterVariants29)
        .value("CHARACTER_VARIANTS30", Text::Feature::CharacterVariants30)
        .value("CHARACTER_VARIANTS31", Text::Feature::CharacterVariants31)
        .value("CHARACTER_VARIANTS32", Text::Feature::CharacterVariants32)
        .value("CHARACTER_VARIANTS33", Text::Feature::CharacterVariants33)
        .value("CHARACTER_VARIANTS34", Text::Feature::CharacterVariants34)
        .value("CHARACTER_VARIANTS35", Text::Feature::CharacterVariants35)
        .value("CHARACTER_VARIANTS36", Text::Feature::CharacterVariants36)
        .value("CHARACTER_VARIANTS37", Text::Feature::CharacterVariants37)
        .value("CHARACTER_VARIANTS38", Text::Feature::CharacterVariants38)
        .value("CHARACTER_VARIANTS39", Text::Feature::CharacterVariants39)
        .value("CHARACTER_VARIANTS40", Text::Feature::CharacterVariants40)
        .value("CHARACTER_VARIANTS41", Text::Feature::CharacterVariants41)
        .value("CHARACTER_VARIANTS42", Text::Feature::CharacterVariants42)
        .value("CHARACTER_VARIANTS43", Text::Feature::CharacterVariants43)
        .value("CHARACTER_VARIANTS44", Text::Feature::CharacterVariants44)
        .value("CHARACTER_VARIANTS45", Text::Feature::CharacterVariants45)
        .value("CHARACTER_VARIANTS46", Text::Feature::CharacterVariants46)
        .value("CHARACTER_VARIANTS47", Text::Feature::CharacterVariants47)
        .value("CHARACTER_VARIANTS48", Text::Feature::CharacterVariants48)
        .value("CHARACTER_VARIANTS49", Text::Feature::CharacterVariants49)
        .value("CHARACTER_VARIANTS50", Text::Feature::CharacterVariants50)
        .value("CHARACTER_VARIANTS51", Text::Feature::CharacterVariants51)
        .value("CHARACTER_VARIANTS52", Text::Feature::CharacterVariants52)
        .value("CHARACTER_VARIANTS53", Text::Feature::CharacterVariants53)
        .value("CHARACTER_VARIANTS54", Text::Feature::CharacterVariants54)
        .value("CHARACTER_VARIANTS55", Text::Feature::CharacterVariants55)
        .value("CHARACTER_VARIANTS56", Text::Feature::CharacterVariants56)
        .value("CHARACTER_VARIANTS57", Text::Feature::CharacterVariants57)
        .value("CHARACTER_VARIANTS58", Text::Feature::CharacterVariants58)
        .value("CHARACTER_VARIANTS59", Text::Feature::CharacterVariants59)
        .value("CHARACTER_VARIANTS60", Text::Feature::CharacterVariants60)
        .value("CHARACTER_VARIANTS61", Text::Feature::CharacterVariants61)
        .value("CHARACTER_VARIANTS62", Text::Feature::CharacterVariants62)
        .value("CHARACTER_VARIANTS63", Text::Feature::CharacterVariants63)
        .value("CHARACTER_VARIANTS64", Text::Feature::CharacterVariants64)
        .value("CHARACTER_VARIANTS65", Text::Feature::CharacterVariants65)
        .value("CHARACTER_VARIANTS66", Text::Feature::CharacterVariants66)
        .value("CHARACTER_VARIANTS67", Text::Feature::CharacterVariants67)
        .value("CHARACTER_VARIANTS68", Text::Feature::CharacterVariants68)
        .value("CHARACTER_VARIANTS69", Text::Feature::CharacterVariants69)
        .value("CHARACTER_VARIANTS70", Text::Feature::CharacterVariants70)
        .value("CHARACTER_VARIANTS71", Text::Feature::CharacterVariants71)
        .value("CHARACTER_VARIANTS72", Text::Feature::CharacterVariants72)
        .value("CHARACTER_VARIANTS73", Text::Feature::CharacterVariants73)
        .value("CHARACTER_VARIANTS74", Text::Feature::CharacterVariants74)
        .value("CHARACTER_VARIANTS75", Text::Feature::CharacterVariants75)
        .value("CHARACTER_VARIANTS76", Text::Feature::CharacterVariants76)
        .value("CHARACTER_VARIANTS77", Text::Feature::CharacterVariants77)
        .value("CHARACTER_VARIANTS78", Text::Feature::CharacterVariants78)
        .value("CHARACTER_VARIANTS79", Text::Feature::CharacterVariants79)
        .value("CHARACTER_VARIANTS80", Text::Feature::CharacterVariants80)
        .value("CHARACTER_VARIANTS81", Text::Feature::CharacterVariants81)
        .value("CHARACTER_VARIANTS82", Text::Feature::CharacterVariants82)
        .value("CHARACTER_VARIANTS83", Text::Feature::CharacterVariants83)
        .value("CHARACTER_VARIANTS84", Text::Feature::CharacterVariants84)
        .value("CHARACTER_VARIANTS85", Text::Feature::CharacterVariants85)
        .value("CHARACTER_VARIANTS86", Text::Feature::CharacterVariants86)
        .value("CHARACTER_VARIANTS87", Text::Feature::CharacterVariants87)
        .value("CHARACTER_VARIANTS88", Text::Feature::CharacterVariants88)
        .value("CHARACTER_VARIANTS89", Text::Feature::CharacterVariants89)
        .value("CHARACTER_VARIANTS90", Text::Feature::CharacterVariants90)
        .value("CHARACTER_VARIANTS91", Text::Feature::CharacterVariants91)
        .value("CHARACTER_VARIANTS92", Text::Feature::CharacterVariants92)
        .value("CHARACTER_VARIANTS93", Text::Feature::CharacterVariants93)
        .value("CHARACTER_VARIANTS94", Text::Feature::CharacterVariants94)
        .value("CHARACTER_VARIANTS95", Text::Feature::CharacterVariants95)
        .value("CHARACTER_VARIANTS96", Text::Feature::CharacterVariants96)
        .value("CHARACTER_VARIANTS97", Text::Feature::CharacterVariants97)
        .value("CHARACTER_VARIANTS98", Text::Feature::CharacterVariants98)
        .value("CHARACTER_VARIANTS99", Text::Feature::CharacterVariants99)
        .value("PETITE_CAPITALS_FROM_CAPITALS", Text::Feature::PetiteCapitalsFromCapitals)
        .value("SMALL_CAPITALS_FROM_CAPITALS", Text::Feature::SmallCapitalsFromCapitals)
        .value("DISTANCES", Text::Feature::Distances)
        .value("DISCRETIONARY_LIGATURES", Text::Feature::DiscretionaryLigatures)
        .value("DENOMINATORS", Text::Feature::Denominators)
        .value("DOTLESS_FORMS", Text::Feature::DotlessForms)
        .value("EXPERT_FORMS", Text::Feature::ExpertForms)
        .value("FINAL_GLYPH_ON_LINE_ALTERNATES", Text::Feature::FinalGlyphOnLineAlternates)
        .value("TERMINAL_FORMS", Text::Feature::TerminalForms)
        .value("TERMINAL_FORMS2", Text::Feature::TerminalForms2)
        .value("TERMINAL_FORMS3", Text::Feature::TerminalForms3)
        .value("FLATTENED_ACCENT_FORMS", Text::Feature::FlattenedAccentForms)
        .value("FRACTIONS", Text::Feature::Fractions)
        .value("FULL_WIDTHS", Text::Feature::FullWidths)
        .value("HALF_FORMS", Text::Feature::HalfForms)
        .value("HALANT_FORMS", Text::Feature::HalantForms)
        .value("ALTERNATE_HALF_WIDTHS", Text::Feature::AlternateHalfWidths)
        .value("HISTORICAL_FORMS", Text::Feature::HistoricalForms)
        .value("HORIZONTAL_KANA_ALTERNATES", Text::Feature::HorizontalKanaAlternates)
        .value("HISTORICAL_LIGATURES", Text::Feature::HistoricalLigatures)
        .value("HANGUL", Text::Feature::Hangul)
        .value("HOJO_KANJI_FORMS", Text::Feature::HojoKanjiForms)
        .value("HALF_WIDTHS", Text::Feature::HalfWidths)
        .value("INITIAL_FORMS", Text::Feature::InitialForms)
        .value("ISOLATED_FORMS", Text::Feature::IsolatedForms)
        .value("ITALICS", Text::Feature::Italics)
        .value("JUSTIFICATION_ALTERNATES", Text::Feature::JustificationAlternates)
        .value("JIS78_FORMS", Text::Feature::Jis78Forms)
        .value("JIS83_FORMS", Text::Feature::Jis83Forms)
        .value("JIS90_FORMS", Text::Feature::Jis90Forms)
        .value("JIS2004_FORMS", Text::Feature::Jis2004Forms)
        .value("KERNING", Text::Feature::Kerning)
        .value("LEFT_BOUNDS", Text::Feature::LeftBounds)
        .value("STANDARD_LIGATURES", Text::Feature::StandardLigatures)
        .value("LEADING_JAMO_FORMS", Text::Feature::LeadingJamoForms)
        .value("LINING_FIGURES", Text::Feature::LiningFigures)
        .value("LOCALIZED_FORMS", Text::Feature::LocalizedForms)
        .value("LEFT_TO_RIGHT_ALTERNATES", Text::Feature::LeftToRightAlternates)
        .value("LEFT_TO_RIGHT_MIRRORED_FORMS", Text::Feature::LeftToRightMirroredForms)
        .value("MARK_POSITIONING", Text::Feature::MarkPositioning)
        .value("MEDIAL_FORMS", Text::Feature::MedialForms)
        .value("MEDIAL_FORMS2", Text::Feature::MedialForms2)
        .value("MATHEMATICAL_GREEK", Text::Feature::MathematicalGreek)
        .value("MARK_TO_MARK_POSITIONING", Text::Feature::MarkToMarkPositioning)
        .value("MARK_POSITIONING_VIA_SUBSTITUTION", Text::Feature::MarkPositioningViaSubstitution)
        .value("ALTERNATE_ANNOTATION_FORMS", Text::Feature::AlternateAnnotationForms)
        .value("NLC_KANJI_FORMS", Text::Feature::NlcKanjiForms)
        .value("NUKTA_FORMS", Text::Feature::NuktaForms)
        .value("NUMERATORS", Text::Feature::Numerators)
        .value("OLDSTYLE_FIGURES", Text::Feature::OldstyleFigures)
        .value("OPTICAL_BOUNDS", Text::Feature::OpticalBounds)
        .value("ORDINALS", Text::Feature::Ordinals)
        .value("ORNAMENTS", Text::Feature::Ornaments)
        .value("PROPORTIONAL_ALTERNATE_WIDTHS", Text::Feature::ProportionalAlternateWidths)
        .value("PETITE_CAPITALS", Text::Feature::PetiteCapitals)
        .value("PROPORTIONAL_KANA", Text::Feature::ProportionalKana)
        .value("PROPORTIONAL_FIGURES", Text::Feature::ProportionalFigures)
        .value("PRE_BASE_FORMS", Text::Feature::PreBaseForms)
        .value("PRE_BASE_SUBSTITUTIONS", Text::Feature::PreBaseSubstitutions)
        .value("POST_BASE_FORMS", Text::Feature::PostBaseForms)
        .value("POST_BASE_SUBSTITUTIONS", Text::Feature::PostBaseSubstitutions)
        .value("PROPORTIONAL_WIDTHS", Text::Feature::ProportionalWidths)
        .value("QUARTER_WIDTHS", Text::Feature::QuarterWidths)
        .value("RANDOMIZE", Text::Feature::Randomize)
        .value("REQUIRED_CONTEXTUAL_ALTERNATES", Text::Feature::RequiredContextualAlternates)
        .value("RAKAR_FORMS", Text::Feature::RakarForms)
        .value("REQUIRED_LIGATURES", Text::Feature::RequiredLigatures)
        .value("REPH_FORMS", Text::Feature::RephForms)
        .value("RIGHT_BOUNDS", Text::Feature::RightBounds)
        .value("RIGHT_TO_LEFT_ALTERNATES", Text::Feature::RightToLeftAlternates)
        .value("RIGHT_TO_LEFT_MIRRORED_FORMS", Text::Feature::RightToLeftMirroredForms)
        .value("RUBY_NOTATION_FORMS", Text::Feature::RubyNotationForms)
        .value("REQUIRED_VARIATION_ALTERNATES", Text::Feature::RequiredVariationAlternates)
        .value("STYLISTIC_ALTERNATES", Text::Feature::StylisticAlternates)
        .value("SCIENTIFIC_INFERIORS", Text::Feature::ScientificInferiors)
        .value("OPTICAL_SIZE", Text::Feature::OpticalSize)
        .value("SMALL_CAPITALS", Text::Feature::SmallCapitals)
        .value("SIMPLIFIED_FORMS", Text::Feature::SimplifiedForms)
        .value("STYLISTIC_SET1", Text::Feature::StylisticSet1)
        .value("STYLISTIC_SET2", Text::Feature::StylisticSet2)
        .value("STYLISTIC_SET3", Text::Feature::StylisticSet3)
        .value("STYLISTIC_SET4", Text::Feature::StylisticSet4)
        .value("STYLISTIC_SET5", Text::Feature::StylisticSet5)
        .value("STYLISTIC_SET6", Text::Feature::StylisticSet6)
        .value("STYLISTIC_SET7", Text::Feature::StylisticSet7)
        .value("STYLISTIC_SET8", Text::Feature::StylisticSet8)
        .value("STYLISTIC_SET9", Text::Feature::StylisticSet9)
        .value("STYLISTIC_SET10", Text::Feature::StylisticSet10)
        .value("STYLISTIC_SET11", Text::Feature::StylisticSet11)
        .value("STYLISTIC_SET12", Text::Feature::StylisticSet12)
        .value("STYLISTIC_SET13", Text::Feature::StylisticSet13)
        .value("STYLISTIC_SET14", Text::Feature::StylisticSet14)
        .value("STYLISTIC_SET15", Text::Feature::StylisticSet15)
        .value("STYLISTIC_SET16", Text::Feature::StylisticSet16)
        .value("STYLISTIC_SET17", Text::Feature::StylisticSet17)
        .value("STYLISTIC_SET18", Text::Feature::StylisticSet18)
        .value("STYLISTIC_SET19", Text::Feature::StylisticSet19)
        .value("STYLISTIC_SET20", Text::Feature::StylisticSet20)
        .value("MATH_SCRIPT_STYLE_ALTERNATES", Text::Feature::MathScriptStyleAlternates)
        .value("STRETCHING_GLYPH_DECOMPOSITION", Text::Feature::StretchingGlyphDecomposition)
        .value("SUBSCRIPT", Text::Feature::Subscript)
        .value("SUPERSCRIPT", Text::Feature::Superscript)
        .value("SWASH", Text::Feature::Swash)
        .value("TITLING", Text::Feature::Titling)
        .value("TRAILING_JAMO_FORMS", Text::Feature::TrailingJamoForms)
        .value("TRADITIONAL_NAME_FORMS", Text::Feature::TraditionalNameForms)
        .value("TABULAR_FIGURES", Text::Feature::TabularFigures)
        .value("TRADITIONAL_FORMS", Text::Feature::TraditionalForms)
        .value("THIRD_WIDTHS", Text::Feature::ThirdWidths)
        .value("UNICASE", Text::Feature::Unicase)
        .value("ALTERNATE_VERTICAL_METRICS", Text::Feature::AlternateVerticalMetrics)
        .value("VATTU_VARIANTS", Text::Feature::VattuVariants)
        .value("KERNING_FOR_ALTERNATE_PROPORTIONAL_VERTICAL_METRICS", Text::Feature::KerningForAlternateProportionalVerticalMetrics)
        .value("VERTICAL_CONTEXTUAL_HALF_WIDTH_SPACING", Text::Feature::VerticalContextualHalfWidthSpacing)
        .value("VERTICAL_WRITING", Text::Feature::VerticalWriting)
        .value("ALTERNATE_VERTICAL_HALF_METRICS", Text::Feature::AlternateVerticalHalfMetrics)
        .value("VOWEL_JAMO_FORMS", Text::Feature::VowelJamoForms)
        .value("VERTICAL_KANA_ALTERNATES", Text::Feature::VerticalKanaAlternates)
        .value("VERTICAL_KERNING", Text::Feature::VerticalKerning)
        .value("PROPORTIONAL_ALTERNATE_VERTICAL_METRICS", Text::Feature::ProportionalAlternateVerticalMetrics)
        .value("VERTICAL_ALTERNATES_AND_ROTATION", Text::Feature::VerticalAlternatesAndRotation)
        .value("VERTICAL_ALTERNATES_FOR_ROTATION", Text::Feature::VerticalAlternatesForRotation)
        .value("SLASHED_ZERO", Text::Feature::SlashedZero)
        /** @todo drop std::string in favor of our own string caster */
        .def(py::init([](const std::string& fourCC) {
            if(fourCC.size() != 4) {
                PyErr_Format(PyExc_AssertionError, "expected a four-character code, got %s", fourCC.data());
                throw py::error_already_set{};
            }
            return Text::feature(fourCC);
        }));

    py::class_<Text::FeatureRange>{m, "FeatureRange", "OpenType feature for a text range"}
        /** @todo add a begin/end variant once it's clear whether a byte index
            or a "python char" index would be more useful */
        .def(py::init<Text::Feature, UnsignedInt>(), "Construct for the whole text", py::arg("feature"), py::arg("value") = true)
        /* To support the implicit conversion below */
        .def(py::init([](const std::pair<Text::Feature, UnsignedInt>& value){
            return Text::FeatureRange{value.first, value.second};
        }), "Construct for the whole text")
        .def_property_readonly("feature", &Text::FeatureRange::feature, "Feature to control")
        .def_property_readonly("is_enabled", &Text::FeatureRange::isEnabled, "Whether to enable the feature")
        .def_property_readonly("value", &Text::FeatureRange::value, "Feature value to set");
    /* For convenient passing as a list to renderer.add() and render() */
    py::implicitly_convertible<Text::Feature, Text::FeatureRange>();
    py::implicitly_convertible<std::pair<Text::Feature, UnsignedInt>, Text::FeatureRange>();

    /* Last updated for Unicode 17.0 */
    py::enum_<Text::Script>{m, "Script", "Script a text is written in"}
        .value("UNSPECIFIED", Text::Script::Unspecified)
        .value("INHERITED", Text::Script::Inherited)
        .value("MATH", Text::Script::Math)
        .value("COMMON", Text::Script::Common)
        .value("UNKNOWN", Text::Script::Unknown)
        .value("ADLAM", Text::Script::Adlam)
        .value("CAUCASIAN_ALBANIAN", Text::Script::CaucasianAlbanian)
        .value("AHOM", Text::Script::Ahom)
        .value("ARABIC", Text::Script::Arabic)
        .value("IMPERIAL_ARAMAIC", Text::Script::ImperialAramaic)
        .value("ARMENIAN", Text::Script::Armenian)
        .value("AVESTAN", Text::Script::Avestan)
        .value("BALINESE", Text::Script::Balinese)
        .value("BAMUM", Text::Script::Bamum)
        .value("BASSA_VAH", Text::Script::BassaVah)
        .value("BATAK", Text::Script::Batak)
        .value("BENGALI", Text::Script::Bengali)
        .value("BERIA_ERFE", Text::Script::BeriaErfe)
        .value("BHAIKSUKI", Text::Script::Bhaiksuki)
        .value("BOPOMOFO", Text::Script::Bopomofo)
        .value("BRAHMI", Text::Script::Brahmi)
        .value("BRAILLE", Text::Script::Braille)
        .value("BUGINESE", Text::Script::Buginese)
        .value("BUHID", Text::Script::Buhid)
        .value("CHAKMA", Text::Script::Chakma)
        .value("CANADIAN_ABORIGINAL", Text::Script::CanadianAboriginal)
        .value("CARIAN", Text::Script::Carian)
        .value("CHAM", Text::Script::Cham)
        .value("CHEROKEE", Text::Script::Cherokee)
        .value("CHORASMIAN", Text::Script::Chorasmian)
        .value("COPTIC", Text::Script::Coptic)
        .value("CYPRO_MINOAN", Text::Script::CyproMinoan)
        .value("CYPRIOT", Text::Script::Cypriot)
        .value("CYRILLIC", Text::Script::Cyrillic)
        .value("DEVANAGARI", Text::Script::Devanagari)
        .value("DIVES_AKURU", Text::Script::DivesAkuru)
        .value("DOGRA", Text::Script::Dogra)
        .value("DESERET", Text::Script::Deseret)
        .value("DUPLOYAN", Text::Script::Duployan)
        .value("EGYPTIAN_HIEROGLYPHS", Text::Script::EgyptianHieroglyphs)
        .value("ELBASAN", Text::Script::Elbasan)
        .value("ELYMAIC", Text::Script::Elymaic)
        .value("ETHIOPIC", Text::Script::Ethiopic)
        .value("GARAY", Text::Script::Garay)
        .value("GEORGIAN", Text::Script::Georgian)
        .value("GLAGOLITIC", Text::Script::Glagolitic)
        .value("GUNJALA_GONDI", Text::Script::GunjalaGondi)
        .value("MASARAM_GONDI", Text::Script::MasaramGondi)
        .value("GOTHIC", Text::Script::Gothic)
        .value("GRANTHA", Text::Script::Grantha)
        .value("GREEK", Text::Script::Greek)
        .value("GUJARATI", Text::Script::Gujarati)
        .value("GURUNG_KHEMA", Text::Script::GurungKhema)
        .value("GURMUKHI", Text::Script::Gurmukhi)
        .value("HANGUL", Text::Script::Hangul)
        .value("HAN", Text::Script::Han)
        .value("HANUNOO", Text::Script::Hanunoo)
        .value("HATRAN", Text::Script::Hatran)
        .value("HEBREW", Text::Script::Hebrew)
        .value("HIRAGANA", Text::Script::Hiragana)
        .value("ANATOLIAN_HIEROGLYPHS", Text::Script::AnatolianHieroglyphs)
        .value("PAHAWH_HMONG", Text::Script::PahawhHmong)
        .value("NYIAKENG_PUACHUE_HMONG", Text::Script::NyiakengPuachueHmong)
        .value("OLD_HUNGARIAN", Text::Script::OldHungarian)
        .value("OLD_ITALIC", Text::Script::OldItalic)
        .value("JAVANESE", Text::Script::Javanese)
        .value("KAYAH_LI", Text::Script::KayahLi)
        .value("KATAKANA", Text::Script::Katakana)
        .value("KAWI", Text::Script::Kawi)
        .value("KHAROSHTHI", Text::Script::Kharoshthi)
        .value("KHMER", Text::Script::Khmer)
        .value("KHOJKI", Text::Script::Khojki)
        .value("KHITAN_SMALL_SCRIPT", Text::Script::KhitanSmallScript)
        .value("KANNADA", Text::Script::Kannada)
        .value("KIRAT_RAI", Text::Script::KiratRai)
        .value("KAITHI", Text::Script::Kaithi)
        .value("TAI_THAM", Text::Script::TaiTham)
        .value("LAO", Text::Script::Lao)
        .value("LATIN", Text::Script::Latin)
        .value("LEPCHA", Text::Script::Lepcha)
        .value("LIMBU", Text::Script::Limbu)
        .value("LINEARA", Text::Script::LinearA)
        .value("LINEARB", Text::Script::LinearB)
        .value("LISU", Text::Script::Lisu)
        .value("LYCIAN", Text::Script::Lycian)
        .value("LYDIAN", Text::Script::Lydian)
        .value("MAHAJANI", Text::Script::Mahajani)
        .value("MAKASAR", Text::Script::Makasar)
        .value("MANDAIC", Text::Script::Mandaic)
        .value("MANICHAEAN", Text::Script::Manichaean)
        .value("MARCHEN", Text::Script::Marchen)
        .value("MEDEFAIDRIN", Text::Script::Medefaidrin)
        .value("MENDE_KIKAKUI", Text::Script::MendeKikakui)
        .value("MEROITIC_CURSIVE", Text::Script::MeroiticCursive)
        .value("MEROITIC_HIEROGLYPHS", Text::Script::MeroiticHieroglyphs)
        .value("MALAYALAM", Text::Script::Malayalam)
        .value("MODI", Text::Script::Modi)
        .value("MONGOLIAN", Text::Script::Mongolian)
        .value("MRO", Text::Script::Mro)
        .value("MEETEI_MAYEK", Text::Script::MeeteiMayek)
        .value("MULTANI", Text::Script::Multani)
        .value("MYANMAR", Text::Script::Myanmar)
        .value("NAG_MUNDARI", Text::Script::NagMundari)
        .value("NANDINAGARI", Text::Script::Nandinagari)
        .value("OLD_NORTH_ARABIAN", Text::Script::OldNorthArabian)
        .value("NABATAEAN", Text::Script::Nabataean)
        .value("NEWA", Text::Script::Newa)
        .value("N_KO", Text::Script::NKo)
        .value("NUSHU", Text::Script::Nushu)
        .value("OGHAM", Text::Script::Ogham)
        .value("OL_CHIKI", Text::Script::OlChiki)
        .value("OL_ONAL", Text::Script::OlOnal)
        .value("OLD_TURKIC", Text::Script::OldTurkic)
        .value("ORIYA", Text::Script::Oriya)
        .value("OSAGE", Text::Script::Osage)
        .value("OSMANYA", Text::Script::Osmanya)
        .value("OLD_UYGHUR", Text::Script::OldUyghur)
        .value("PALMYRENE", Text::Script::Palmyrene)
        .value("PAU_CIN_HAU", Text::Script::PauCinHau)
        .value("OLD_PERMIC", Text::Script::OldPermic)
        .value("PHAGS_PA", Text::Script::PhagsPa)
        .value("INSCRIPTIONAL_PAHLAVI", Text::Script::InscriptionalPahlavi)
        .value("PSALTER_PAHLAVI", Text::Script::PsalterPahlavi)
        .value("PHOENICIAN", Text::Script::Phoenician)
        .value("MIAO", Text::Script::Miao)
        .value("INSCRIPTIONAL_PARTHIAN", Text::Script::InscriptionalParthian)
        .value("REJANG", Text::Script::Rejang)
        .value("HANIFI_ROHINGYA", Text::Script::HanifiRohingya)
        .value("RUNIC", Text::Script::Runic)
        .value("SAMARITAN", Text::Script::Samaritan)
        .value("OLD_SOUTH_ARABIAN", Text::Script::OldSouthArabian)
        .value("SAURASHTRA", Text::Script::Saurashtra)
        .value("SIGN_WRITING", Text::Script::SignWriting)
        .value("SHAVIAN", Text::Script::Shavian)
        .value("SHARADA", Text::Script::Sharada)
        .value("SIDDHAM", Text::Script::Siddham)
        .value("SIDETIC", Text::Script::Sidetic)
        .value("KHUDAWADI", Text::Script::Khudawadi)
        .value("SINHALA", Text::Script::Sinhala)
        .value("SOGDIAN", Text::Script::Sogdian)
        .value("OLD_SOGDIAN", Text::Script::OldSogdian)
        .value("SORA_SOMPENG", Text::Script::SoraSompeng)
        .value("SOYOMBO", Text::Script::Soyombo)
        .value("SUNDANESE", Text::Script::Sundanese)
        .value("SUNUWAR", Text::Script::Sunuwar)
        .value("SYLOTI_NAGRI", Text::Script::SylotiNagri)
        .value("SYRIAC", Text::Script::Syriac)
        .value("TAGBANWA", Text::Script::Tagbanwa)
        .value("TAKRI", Text::Script::Takri)
        .value("TAI_LE", Text::Script::TaiLe)
        .value("NEW_TAI_LUE", Text::Script::NewTaiLue)
        .value("TAMIL", Text::Script::Tamil)
        .value("TANGUT", Text::Script::Tangut)
        .value("TAI_VIET", Text::Script::TaiViet)
        .value("TAI_YO", Text::Script::TaiYo)
        .value("TELUGU", Text::Script::Telugu)
        .value("TIFINAGH", Text::Script::Tifinagh)
        .value("TAGALOG", Text::Script::Tagalog)
        .value("THAANA", Text::Script::Thaana)
        .value("THAI", Text::Script::Thai)
        .value("TIBETAN", Text::Script::Tibetan)
        .value("TIRHUTA", Text::Script::Tirhuta)
        .value("TANGSA", Text::Script::Tangsa)
        .value("TODHRI", Text::Script::Todhri)
        .value("TOLONG_SIKI", Text::Script::TolongSiki)
        .value("TOTO", Text::Script::Toto)
        .value("TULU_TIGALARI", Text::Script::TuluTigalari)
        .value("UGARITIC", Text::Script::Ugaritic)
        .value("VAI", Text::Script::Vai)
        .value("VITHKUQI", Text::Script::Vithkuqi)
        .value("WARANG_CITI", Text::Script::WarangCiti)
        .value("WANCHO", Text::Script::Wancho)
        .value("OLD_PERSIAN", Text::Script::OldPersian)
        .value("CUNEIFORM", Text::Script::Cuneiform)
        .value("YEZIDI", Text::Script::Yezidi)
        .value("YI", Text::Script::Yi)
        .value("ZANABAZAR_SQUARE", Text::Script::ZanabazarSquare)
        /** @todo drop std::string in favor of our own string caster */
        .def(py::init([](const std::string& fourCC) {
            if(fourCC.size() != 4) {
                PyErr_Format(PyExc_AssertionError, "expected a four-character code, got %s", fourCC.data());
                throw py::error_already_set{};
            }
            return Text::script(fourCC);
        }));

    py::enum_<Text::ShapeDirection>{m, "ShapeDirection", "Direction a text is shaped in"}
        .value("UNSPECIFIED", Text::ShapeDirection::Unspecified)
        .value("LEFT_TO_RIGHT", Text::ShapeDirection::LeftToRight)
        .value("RIGHT_TO_LEFT", Text::ShapeDirection::RightToLeft)
        .value("TOP_TO_BOTTOM", Text::ShapeDirection::TopToBottom)
        .value("BOTTOM_TO_TOP", Text::ShapeDirection::BottomToTop);

    /* Font. Returned by AbstractShaper, so has to be declared before, but the
       font is returning AbstractShaper as well, so the method definitions are
       after AbstractShaper. */
    py::class_<Text::AbstractFont, PluginManager::PyPluginHolder<Text::AbstractFont>, PluginManager::AbstractPlugin> abstractFont{m, "AbstractFont", "Interface for font plugins"};

    /* Shaper */
    py::class_<Text::AbstractShaper>{m, "AbstractShaper", "Base for text shapers"}
        .def_property_readonly("font", static_cast<Text::AbstractFont&(Text::AbstractShaper::*)()>(&Text::AbstractShaper::font), "Font owning this shaper instance")
        /* Not using a property for these because it may be useful to know
           whether setting these actually did anything in given plugin */
        .def("set_script", &Text::AbstractShaper::setScript, "Set text script", py::arg("script"))
        .def("set_language", [](Text::AbstractShaper& self, const std::string& language) {
            return self.setLanguage(language);
        }, "Set text language", py::arg("language"))
        .def("set_direction", &Text::AbstractShaper::setDirection, "Set direction the text is meant to be shaped in", py::arg("direction"))
        /** @todo glyph count, script, language getters together with glyph
            data getters once it makes  sense to use shape() and such
            directly */
        ;

    /* Font methods */
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
            if(self.openFile(
                #ifdef CORRADE_TARGET_WINDOWS
                /* To allow people to conveniently use Python's os.path, we
                   need to convert backslashes to forward slashes as all
                   Corrade and Magnum APIs expect forward */
                Utility::Path::fromNativeSeparators(filename)
                #else
                filename
                #endif
                , size)) return;

            PyErr_Format(PyExc_RuntimeError, "opening %s failed", filename.data());
            throw py::error_already_set{};
        }, "Open a file", py::arg("filename"), py::arg("size"))
        .def("close", &Text::AbstractFont::close, "Close currently opened file")

        .def_property_readonly("size", checkOpened<Float, &Text::AbstractFont::size>, "Font size")
        .def_property_readonly("ascent", checkOpened<Float, &Text::AbstractFont::ascent>, "Font ascent")
        .def_property_readonly("descent", checkOpened<Float, &Text::AbstractFont::descent>, "Font descent")
        .def_property_readonly("line_height", checkOpened<Float, &Text::AbstractFont::lineHeight>, "Line height")
        .def_property_readonly("glyph_count", checkOpened<UnsignedInt, &Text::AbstractFont::glyphCount>, "Total count of glyphs in the font")
        .def("glyph_id", checkOpened<UnsignedInt, char32_t, &Text::AbstractFont::glyphId>, "Glyph ID for given character", py::arg("character"))
        .def("glyph_size", checkOpenedBounds<Vector2, &Text::AbstractFont::glyphSize>, "Glyph size in pixels", py::arg("glyph"))
        .def("glyph_advance", checkOpenedBounds<Vector2, &Text::AbstractFont::glyphAdvance>, "Glyph advance in pixels", py::arg("glyph"))
        .def("fill_glyph_cache", [](Text::AbstractFont& self, Text::AbstractGlyphCache& cache, const std::string& characters) {
            if(!self.isOpened()) {
                PyErr_SetString(PyExc_AssertionError, "no file opened");
                throw py::error_already_set{};
            }
            return self.fillGlyphCache(cache, characters);
        }, "Fill glyph cache with given character set", py::arg("cache"), py::arg("characters"))
        /** @todo createGlyphCache() */
        .def("create_shaper", [](Text::AbstractFont& self) {
            if(!self.isOpened()) {
                PyErr_SetString(PyExc_AssertionError, "no file opened");
                throw py::error_already_set{};
            }

            return std::unique_ptr<Text::AbstractShaper>(self.createShaper());
        }, "Create an instance of this font shaper implementation",
            /* Keeps the font (index 1) alive for as long as the return value
               (index 0) exists */
            py::keep_alive<0, 1>{});
    corrade::plugin(abstractFont);

    py::class_<PluginManager::Manager<Text::AbstractFont>, PluginManager::AbstractManager> fontManager{m, "FontManager", "Manager for font plugins"};
    corrade::manager(fontManager);

    py::enum_<Text::Alignment>{m, "Alignment", "Text rendering alignment"}
        .value("LINE_LEFT", Text::Alignment::LineLeft)
        .value("LINE_LEFT_GLYPH_BOUNDS", Text::Alignment::LineLeftGlyphBounds)
        .value("LINE_CENTER", Text::Alignment::LineCenter)
        .value("LINE_CENTER_INTEGRAL", Text::Alignment::LineCenterIntegral)
        .value("LINE_CENTER_GLYPH_BOUNDS", Text::Alignment::LineCenterGlyphBounds)
        .value("LINE_CENTER_GLYPH_BOUNDS_INTEGRAL", Text::Alignment::LineCenterGlyphBoundsIntegral)
        .value("LINE_RIGHT", Text::Alignment::LineRight)
        .value("LINE_RIGHT_GLYPH_BOUNDS", Text::Alignment::LineRightGlyphBounds)
        .value("LINE_BEGIN", Text::Alignment::LineBegin)
        .value("LINE_BEGIN_GLYPH_BOUNDS", Text::Alignment::LineBeginGlyphBounds)
        .value("LINE_END", Text::Alignment::LineEnd)
        .value("LINE_END_GLYPH_BOUNDS", Text::Alignment::LineEndGlyphBounds)
        .value("BOTTOM_LEFT", Text::Alignment::BottomLeft)
        .value("BOTTOM_LEFT_GLYPH_BOUNDS", Text::Alignment::BottomLeftGlyphBounds)
        .value("BOTTOM_CENTER", Text::Alignment::BottomCenter)
        .value("BOTTOM_CENTER_INTEGRAL", Text::Alignment::BottomCenterIntegral)
        .value("BOTTOM_CENTER_GLYPH_BOUNDS", Text::Alignment::BottomCenterGlyphBounds)
        .value("BOTTOM_CENTER_GLYPH_BOUNDS_INTEGRAL", Text::Alignment::BottomCenterGlyphBoundsIntegral)
        .value("BOTTOM_RIGHT", Text::Alignment::BottomRight)
        .value("BOTTOM_RIGHT_GLYPH_BOUNDS", Text::Alignment::BottomRightGlyphBounds)
        .value("BOTTOM_BEGIN", Text::Alignment::BottomBegin)
        .value("BOTTOM_BEGIN_GLYPH_BOUNDS", Text::Alignment::BottomBeginGlyphBounds)
        .value("BOTTOM_END", Text::Alignment::BottomEnd)
        .value("BOTTOM_END_GLYPH_BOUNDS", Text::Alignment::BottomEndGlyphBounds)
        .value("MIDDLE_LEFT", Text::Alignment::MiddleLeft)
        .value("MIDDLE_LEFT_INTEGRAL", Text::Alignment::MiddleLeftIntegral)
        .value("MIDDLE_LEFT_GLYPH_BOUNDS", Text::Alignment::MiddleLeftGlyphBounds)
        .value("MIDDLE_LEFT_GLYPH_BOUNDS_INTEGRAL", Text::Alignment::MiddleLeftGlyphBoundsIntegral)
        .value("MIDDLE_CENTER", Text::Alignment::MiddleCenter)
        .value("MIDDLE_CENTER_INTEGRAL", Text::Alignment::MiddleCenterIntegral)
        .value("MIDDLE_CENTER_GLYPH_BOUNDS", Text::Alignment::MiddleCenterGlyphBounds)
        .value("MIDDLE_CENTER_GLYPH_BOUNDS_INTEGRAL", Text::Alignment::MiddleCenterGlyphBoundsIntegral)
        .value("MIDDLE_RIGHT", Text::Alignment::MiddleRight)
        .value("MIDDLE_RIGHT_INTEGRAL", Text::Alignment::MiddleRightIntegral)
        .value("MIDDLE_RIGHT_GLYPH_BOUNDS", Text::Alignment::MiddleRightGlyphBounds)
        .value("MIDDLE_RIGHT_GLYPH_BOUNDS_INTEGRAL", Text::Alignment::MiddleRightGlyphBoundsIntegral)
        .value("MIDDLE_BEGIN", Text::Alignment::MiddleBegin)
        .value("MIDDLE_BEGIN_INTEGRAL", Text::Alignment::MiddleBeginIntegral)
        .value("MIDDLE_BEGIN_GLYPH_BOUNDS", Text::Alignment::MiddleBeginGlyphBounds)
        .value("MIDDLE_BEGIN_GLYPH_BOUNDS_INTEGRAL", Text::Alignment::MiddleBeginGlyphBoundsIntegral)
        .value("MIDDLE_END", Text::Alignment::MiddleEnd)
        .value("MIDDLE_END_INTEGRAL", Text::Alignment::MiddleEndIntegral)
        .value("MIDDLE_END_GLYPH_BOUNDS", Text::Alignment::MiddleEndGlyphBounds)
        .value("MIDDLE_END_GLYPH_BOUNDS_INTEGRAL", Text::Alignment::MiddleEndGlyphBoundsIntegral)
        .value("TOP_LEFT", Text::Alignment::TopLeft)
        .value("TOP_LEFT_GLYPH_BOUNDS", Text::Alignment::TopLeftGlyphBounds)
        .value("TOP_CENTER", Text::Alignment::TopCenter)
        .value("TOP_CENTER_INTEGRAL", Text::Alignment::TopCenterIntegral)
        .value("TOP_CENTER_GLYPH_BOUNDS", Text::Alignment::TopCenterGlyphBounds)
        .value("TOP_CENTER_GLYPH_BOUNDS_INTEGRAL", Text::Alignment::TopCenterGlyphBoundsIntegral)
        .value("TOP_RIGHT", Text::Alignment::TopRight)
        .value("TOP_RIGHT_GLYPH_BOUNDS", Text::Alignment::TopRightGlyphBounds)
        .value("TOP_BEGIN", Text::Alignment::TopBegin)
        .value("TOP_BEGIN_GLYPH_BOUNDS", Text::Alignment::TopBeginGlyphBounds)
        .value("TOP_END", Text::Alignment::TopEnd)
        .value("TOP_END_GLYPH_BOUNDS", Text::Alignment::TopEndGlyphBounds);

    /* RendererCore, Renderer, RendererGL */
    py::class_<Text::RendererCore>{m, "RendererCore", "Text renderer core"}
        /** @todo expose constructors once the class is directly useful for
            anything */
        /* In this case the glyph cache isn't owned by the renderer so the
           returned object doesn't increase the renderer refcount. This is
           verified in test_text_gl.py to be extra sure. */
        .def_property_readonly("glyph_cache", &Text::RendererCore::glyphCache, "Glyph cache associated with the renderer")
        /** @todo expose flags once accessing the glyph data is useful for
            anything */
        .def_property_readonly("glyph_count", &Text::RendererCore::glyphCount, "Total count of rendered glyphs")
        .def_property_readonly("glyph_capacity", &Text::RendererCore::glyphCapacity, "Glyph capacity")
        .def_property_readonly("run_count", &Text::RendererCore::runCount, "Total count of rendered runs")
        .def_property_readonly("run_capacity", &Text::RendererCore::runCapacity, "Run capacity")
        .def_property_readonly("is_rendering", &Text::RendererCore::isRendering, "Whether text rendering is currently in progress")
        .def_property_readonly("rendering_glyph_count", &Text::RendererCore::renderingGlyphCount, "Total count of glyphs including current in-progress rendering")
        .def_property_readonly("rendering_run_count", &Text::RendererCore::renderingRunCount, "Total count of runs including current in-progress rendering")
        .def_property("cursor", &Text::RendererCore::cursor, [](Text::RendererCore& self, const Vector2& cursor) {
            if(self.isRendering()) {
                PyErr_SetString(PyExc_AssertionError, "rendering in progress");
                throw py::error_already_set{};
            }
            self.setCursor(cursor);
        }, "Cursor position")
        .def_property("alignment", &Text::RendererCore::alignment, [](Text::RendererCore& self, Text::Alignment alignment) {
            if(self.isRendering()) {
                PyErr_SetString(PyExc_AssertionError, "rendering in progress");
                throw py::error_already_set{};
            }
            self.setAlignment(alignment);
        }, "Alignment")
        .def_property("line_advance", &Text::RendererCore::lineAdvance, [](Text::RendererCore& self, const Float advance) {
            if(self.isRendering()) {
                PyErr_SetString(PyExc_AssertionError, "rendering in progress");
                throw py::error_already_set{};
            }
            self.setLineAdvance(advance);
        }, "Cursor position")
        /** @todo layout direction once there's more than one value allowed */
        /** @todo reserve, clear, reset once it's possible to use RendererCore
            directly */
        /** @todo add a begin/end variant once it's clear whether a byte index
            or a "python char" index would be more useful */
        /** @todo drop std::string in favor of our own string caster */
        /** @todo drop std::vector in favor of our own list caster */
        .def("add", [](Text::RendererCore& self, Text::AbstractShaper& shaper, Float size, const std::string& text, const std::vector<Text::FeatureRange>& features) {
            if(!self.glyphCache().findFont(shaper.font())) {
                PyErr_Format(PyExc_AssertionError, "shaper font not found among %u fonts in associated glyph cache", self.glyphCache().fontCount());
                throw py::error_already_set{};
            }
            self.add(shaper, size, text, features);
        }, "Add a whole string to the currently rendered text", py::arg("shaper"), py::arg("size"), py::arg("text"), py::arg("features") = std::vector<Text::FeatureRange>{})
        /** @todo render once it's possible to use RendererCore directly */
        ;

    py::class_<Text::Renderer, Text::RendererCore>{m, "Renderer", "Text renderer"}
        /** @todo expose flags once accessing the glyph data is useful for
            anything */
        .def_property_readonly("glyph_index_capacity", &Text::Renderer::glyphIndexCapacity, "Glyph index capacity")
        .def_property_readonly("glyph_vertex_capacity", &Text::Renderer::glyphVertexCapacity, "Glyph vertex capacity")
        /** @todo index_type, reserve, clear, reset, render once it's possible
            to use Renderer directly */
        ;

    py::class_<Text::RendererGL, Text::Renderer> rendererGL{m, "RendererGL", "OpenGL text renderer"};
    rendererGL
        /** @todo expose flags once accessing the glyph data is useful for
            anything */
        .def(py::init<const Text::AbstractGlyphCache&>(), "Constructor", py::arg("cache"),
            /* Keeps the cache (index 2) alive for as long as the renderer
               (index 1) exists */
            py::keep_alive<1, 2>{})
        /* The default behavior when returning a reference seems to be that it
           increfs the originating instance and decrefs it again after the
           variable gets deleted. This is verified in test_text_gl.py to be
           extra sure. */
        .def_property_readonly("mesh", static_cast<GL::Mesh&(Text::RendererGL::*)()>(&Text::RendererGL::mesh), "Mesh containing the rendered index and vertex data")
        .def_property("index_type", &Text::RendererGL::indexType, [](Text::RendererGL& self, MeshIndexType atLeast) {
            if(self.isRendering()) {
                PyErr_SetString(PyExc_AssertionError, "rendering in progress");
                throw py::error_already_set{};
            }
            self.setIndexType(atLeast);
        }, "Index type")
        .def("reserve", [](Text::RendererGL& self, UnsignedInt glyphCapacity, UnsignedInt runCapacity) {
            /* Using a lambda to ignore the method chaining return type */
            self.reserve(glyphCapacity, runCapacity);
        }, "Reserve capacity for given glyph and run count", py::arg("glyph_capacity"), py::arg("run_capacity"))
        .def("clear", [](Text::RendererGL& self) {
            /* Using a lambda to ignore the method chaining return type */
            self.clear();
        }, "Clear rendered glyphs, runs and vertices")
        .def("reset", [](Text::RendererGL& self) {
            /* Using a lambda to ignore the method chaining return type */
            self.reset();
        }, "Reset internal renderer state")
        /** @todo drop std::pair in favor of our own string caster */
        .def("render", [](Text::RendererGL& self) {
            return std::pair<Range2D, Range1Dui>(self.render());
        }, "Wrap up rendering of all text added so far")
        .def("render", [](Text::RendererGL& self, Text::AbstractShaper& shaper, Float size, const std::string& text, const std::vector<Text::FeatureRange>& features) {
            if(!self.glyphCache().findFont(shaper.font())) {
                PyErr_Format(PyExc_AssertionError, "shaper font not found among %u fonts in associated glyph cache", self.glyphCache().fontCount());
                throw py::error_already_set{};
            }
            return std::pair<Range2D, Range1Dui>(self.render(shaper, size, text, features));
        }, "Render a whole text at once", py::arg("shaper"), py::arg("size"), py::arg("text"), py::arg("features") = std::vector<Text::FeatureRange>{});
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
