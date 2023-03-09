#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020, 2021, 2022 Vladimír Vondruš <mosra@centrum.cz>
#
#   Permission is hereby granted, free of charge, to any person obtaining a
#   copy of this software and associated documentation files (the "Software"),
#   to deal in the Software without restriction, including without limitation
#   the rights to use, copy, modify, merge, publish, distribute, sublicense,
#   and/or sell copies of the Software, and to permit persons to whom the
#   Software is furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be included
#   in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
#   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#

import os
import sys
import tempfile
import unittest

from corrade import utility

# Tests also the ConfigurationGroup bindings, as a ConfigurationGroup cannot be
# constructed as a standalone type
class Configuration(unittest.TestCase):
    def test_open(self):
        a = utility.Configuration(os.path.join(os.path.dirname(__file__), "file.conf"))
        a_refcount = sys.getrefcount(a)
        self.assertEqual(a['someKey'], '42')
        self.assertEqual(a['nonexistent'], '')

        b = a.group('someGroup')
        b_refcount = sys.getrefcount(b)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)
        self.assertEqual(b['value'], 'hello')

        c = b.group('subgroup')
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)
        self.assertEqual(c['anotherValue'], 'another')

        del c
        self.assertEqual(sys.getrefcount(b), b_refcount)

        del b
        self.assertEqual(sys.getrefcount(a), a_refcount)

    def test_nonexistent_group(self):
        a = utility.Configuration(os.path.join(os.path.dirname(__file__), "file.conf"))
        a_refcount = sys.getrefcount(a)

        with self.assertRaises(KeyError):
            a.group('nonexistent')

        self.assertEqual(sys.getrefcount(a), a_refcount)

    def test_save(self):
        with tempfile.TemporaryDirectory() as tmp:
            filename = os.path.join(tmp, "file.conf")

            a = utility.Configuration(filename)
            a['value'] = 'hello'
            a['int'] = -168454764726
            a['float'] = 3.14
            a['bool'] = True

            a_refcount = sys.getrefcount(a)
            b = a.add_group('someGroup')
            self.assertEqual(sys.getrefcount(a), a_refcount + 1)

            b['someKey'] = '42'

            # This should not delete the group from the configuration
            del b
            self.assertEqual(sys.getrefcount(a), a_refcount)

            a.save()

            with open(filename, 'r') as f:
                self.assertEqual(f.read(), "value=hello\nint=-168454764726\nfloat=3.14\nbool=1\n[someGroup]\nsomeKey=42\n")

    def test_save_different_filename(self):
        a = utility.Configuration()
        a['value'] = 'hello'

        with tempfile.TemporaryDirectory() as tmp:
            filename = os.path.join(tmp, "file.conf")
            a.save(filename)

            with open(filename, 'r') as f:
                self.assertEqual(f.read(), "value=hello\n")

    def test_save_implicit(self):
        with tempfile.TemporaryDirectory() as tmp:
            filename = os.path.join(tmp, "file.conf")

            a = utility.Configuration(filename)
            a['value'] = 'hello'
            self.assertFalse(os.path.exists(filename))

            del a
            self.assertTrue(os.path.exists(filename))

            with open(filename, 'r') as f:
                self.assertEqual(f.read(), "value=hello\n")

    def test_open_nonexistent(self):
        # This should not raise any exception as it's a valid use case (i.e,
        # opening an app for the first time)
        a = utility.Configuration("nonexistent.conf")
        self.assertFalse(a.has_groups)
        self.assertFalse(a.has_values)

    def test_open_failed(self):
        with self.assertRaises(IOError):
            utility.Configuration(os.path.join(os.path.dirname(__file__), "broken.conf"))

    @unittest.skipIf(os.access("/", os.W_OK), "root dir is writable")
    def test_save_failed(self):
        # The file doesn't exist, which means nothing is parsed during
        # construction. But saving will fail as the directory is not writable.
        a = utility.Configuration("/nonexistent.conf")
        self.assertFalse(a.has_groups)
        self.assertFalse(a.has_values)

        with self.assertRaises(IOError):
            a.save()

    def test_save_different_filename_failed(self):
        a = utility.Configuration()

        with self.assertRaises(IOError):
            a.save("/some/path/that/does/not/exist.conf")

