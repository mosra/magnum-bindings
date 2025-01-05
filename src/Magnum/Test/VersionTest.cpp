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

#include <Corrade/TestSuite/Tester.h>
#include <Corrade/TestSuite/Compare/Numeric.h>

#include "Magnum/Magnum.h"
#include "Magnum/versionBindings.h"

namespace Magnum { namespace Test { namespace {

struct VersionTest: TestSuite::Tester {
    explicit VersionTest();

    void test();
};

VersionTest::VersionTest() {
    addTests({&VersionTest::test});
}

void VersionTest::test() {
    Debug{} << "MAGNUMBINDINGS_VERSION_YEAR:" << MAGNUMBINDINGS_VERSION_YEAR;
    Debug{} << "MAGNUMBINDINGS_VERSION_MONTH:" << MAGNUMBINDINGS_VERSION_MONTH;
    #ifdef MAGNUMBINDINGS_VERSION_COMMIT
    Debug{} << "MAGNUMBINDINGS_VERSION_COMMIT:" << MAGNUMBINDINGS_VERSION_COMMIT;
    Debug{} << "MAGNUMBINDINGS_VERSION_HASH:" << Debug::hex << MAGNUMBINDINGS_VERSION_HASH;
    Debug{} << "MAGNUMBINDINGS_VERSION_STRING:" << MAGNUMBINDINGS_VERSION_STRING;
    #else
    Debug{} << "No Git version information available.";
    #endif

    CORRADE_COMPARE_AS(MAGNUMBINDINGS_VERSION_YEAR, 2019, TestSuite::Compare::GreaterOrEqual);
    CORRADE_COMPARE_AS(MAGNUMBINDINGS_VERSION_YEAR, 2100, TestSuite::Compare::LessOrEqual);
    CORRADE_COMPARE_AS(MAGNUMBINDINGS_VERSION_MONTH, 0, TestSuite::Compare::Greater);
    CORRADE_COMPARE_AS(MAGNUMBINDINGS_VERSION_MONTH, 12, TestSuite::Compare::LessOrEqual);
    #ifdef MAGNUMBINDINGS_VERSION_COMMIT
    CORRADE_COMPARE_AS(MAGNUMBINDINGS_VERSION_COMMIT, 0, TestSuite::Compare::GreaterOrEqual);
    #endif
}

}}}

CORRADE_TEST_MAIN(Magnum::Test::VersionTest)
