#
#   This file is part of Magnum.
#
#   Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
#               2020, 2021, 2022, 2023 Vladimír Vondruš <mosra@centrum.cz>
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

import array
import sys
import unittest

from corrade import containers
import test_stridedarrayview
import test_optional

class ArrayView(unittest.TestCase):
    def test_init(self):
        a = containers.ArrayView()
        b = containers.MutableArrayView()
        self.assertIs(a.owner, None)
        self.assertIs(b.owner, None)
        self.assertEqual(len(a), 0)
        self.assertEqual(len(b), 0)
        self.assertEqual(bytes(a), b'')
        self.assertEqual(bytes(b), b'')

    def test_init_buffer(self):
        a = b'hello'
        a_refcount = sys.getrefcount(a)
        # Verify that bytes has the same semantics, i.e. not possible to
        # directly get a character
        self.assertEqual(a[2], ord('l'))

        b = containers.ArrayView(a)
        self.assertIs(b.owner, a)
        self.assertEqual(len(b), 5)
        self.assertEqual(bytes(b), b'hello')
        self.assertEqual(b[2], ord('l'))
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        # Not mutable
        with self.assertRaisesRegex(TypeError, "object does not support item assignment"):
            b[4] = ord('!')

        # b should keep a reference to a, so deleting the local reference
        # shouldn't affect it
        del a
        self.assertTrue(sys.getrefcount(b.owner), a_refcount)
        self.assertEqual(b[2], ord('l'))

        # Now, if we delete b, a should not be referenced by anything anymore
        a = b.owner
        del b
        self.assertTrue(sys.getrefcount(a), a_refcount)

    def test_init_buffer_empty(self):
        a = b''
        a_refcount = sys.getrefcount(a)

        b = containers.ArrayView(a)
        self.assertIs(b.owner, None)
        self.assertEqual(len(b), 0)
        self.assertEqual(sys.getrefcount(a), a_refcount)

    def test_init_buffer_memoryview_obj(self):
        a = b'hello'
        v = memoryview(a)
        b = containers.ArrayView(v)
        # memoryview's buffer protocol returns itself, not the underlying
        # bytes, as it manages the Py_buffer instance. So this is expected.
        self.assertIs(b.owner, v)

    def test_init_buffer_mutable(self):
        a = bytearray(b'hello')
        # Verify that the bytearray has the same semantics, i.e. not possible
        # to directly assign a character
        a[4] = ord('?')
        self.assertEqual(a[4], ord('?'))

        b = containers.MutableArrayView(a)
        b[4] = ord('!')
        self.assertEqual(b[4], ord('!'))
        self.assertEqual(bytes(b), b'hell!')

    def test_init_array(self):
        a = array.array('f', [1.0, 4.5, 7.9])
        b = containers.ArrayView(a)
        self.assertIs(b.owner, a)
        self.assertEqual(len(b), 3*4)

    def test_init_buffer_unexpected_stride(self):
        a = memoryview(b'hello')[::2]
        self.assertEqual(bytes(a), b'hlo')
        # Error emitted by memoryview, not us
        with self.assertRaisesRegex(BufferError, "memoryview: underlying buffer is not C-contiguous"):
            b = containers.ArrayView(a)

    def test_init_buffer_mutable_from_immutable(self):
        a = b'hello'
        with self.assertRaisesRegex(BufferError, "Object is not writable."):
            b = containers.MutableArrayView(a)

    def test_slice(self):
        a = b'World is hell!'
        a_refcount = sys.getrefcount(a)

        b = containers.ArrayView(a)
        b_refcount = sys.getrefcount(b)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        # When slicing, b's refcount should not change but a's refcount should
        # increase
        c = b[4:-4]
        self.assertIsInstance(c, containers.ArrayView)
        self.assertEqual(bytes(c), b'd is h')
        self.assertEqual(sys.getrefcount(b), b_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)

        # Deleting a slice should reduce a's refcount again, keep b's unchanged
        del c
        self.assertEqual(sys.getrefcount(b), b_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

    def test_slice_empty(self):
        data = b'hello'
        data_refcount = sys.getrefcount(data)

        # slice.start = slice.stop
        a = containers.ArrayView(data)[7:8]
        self.assertEqual(len(a), 0)

        # Empty view, original data not referenced at all
        self.assertIs(a.owner, None)
        self.assertEqual(sys.getrefcount(data), data_refcount)

    def test_slice_invalid(self):
        with self.assertRaisesRegex(ValueError, "slice step cannot be zero"):
            containers.ArrayView()[::0]

    def test_slice_stride(self):
        a = b'World_ _i_s_ _hell!'
        a_refcount = sys.getrefcount(a)

        b = containers.ArrayView(a)
        b_refcount = sys.getrefcount(b)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        # When slicing to a strided array view, b's refcount should not change
        # but a's refcount should increase. Check consistency with slices on
        # bytes, slicing bytes will make a copy so it doesn't affect the
        # refcount.
        c1 = a[4:-4:2]
        c2 = b[4:-4:2]
        self.assertIsInstance(c2, containers.StridedArrayView1D)
        self.assertEqual(len(c1), 6)
        self.assertEqual(len(c2), 6)
        self.assertEqual(bytes(c1), b'd is h')
        self.assertEqual(bytes(c2), b'd is h')
        self.assertEqual(c2.size, (6,))
        self.assertEqual(c2.stride, (2,))
        self.assertEqual(sys.getrefcount(b), b_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)

        # Deleting a slice should reduce a's refcount again, keep b's unchanged
        del c2
        self.assertEqual(sys.getrefcount(b), b_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

    def test_slice_stride_empty(self):
        data = b'hello'
        data_refcount = sys.getrefcount(data)

        # slice.start = slice.stop
        a = containers.ArrayView(data)[7:8:2]
        self.assertEqual(len(a), 0)

        # Empty view, original data not referenced at all
        self.assertIs(a.owner, None)
        self.assertEqual(sys.getrefcount(data), data_refcount)

    def test_slice_stride_negative(self):
        a = b'World_ _i_s_ _hell!'
        b = containers.ArrayView(a)

        # Check consistency with slices on bytes
        c1 = a[-5:3:-2] # like [4:-4:2] above, but reverted
        c2 = b[-5:3:-2]
        self.assertEqual(len(c1), 6)
        self.assertEqual(len(c2), 6)
        self.assertEqual(bytes(c1), b'h si d') # like b'd is h' but reverted
        self.assertEqual(bytes(c2), b'h si d')
        self.assertEqual(c2.size, (6,))
        self.assertEqual(c2.stride, (-2,))

    def test_slice_stride_reverse(self):
        # slice.stop = -1
        a = containers.ArrayView(b'hello')[::-1]
        self.assertEqual(len(a), 5)
        self.assertEqual(bytes(a), b'olleh')

    def test_convert_memoryview(self):
        a = b'World is hell!'
        a_refcount = sys.getrefcount(a)

        b = containers.ArrayView(a)
        b_refcount = sys.getrefcount(b)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        c = memoryview(b)
        # Unlike slicing, ArrayView's buffer protocol returns a reference to
        # itself -- it needs to be kept around because the Py_buffer refers to
        # its internals for size. Also returning a reference to the underlying
        # buffer would mean the underlying buffer's releasebuffer function gets
        # called instead of ours which is *not* wanted.
        self.assertIs(c.obj, b)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        with self.assertRaisesRegex(TypeError, "cannot modify read-only memory"):
            c[-1] = ord('?')

    def test_convert_mutable_memoryview(self):
        a = bytearray(b'World is hell!')
        b = memoryview(containers.MutableArrayView(a))
        b[-1] = ord('?')
        self.assertEqual(a, b'World is hell?')

class StridedArrayView1D(unittest.TestCase):
    def test_init(self):
        a = containers.StridedArrayView1D()
        b = containers.MutableStridedArrayView1D()
        self.assertIs(a.owner, None)
        self.assertIs(b.owner, None)
        self.assertEqual(len(a), 0)
        self.assertEqual(len(b), 0)
        self.assertEqual(bytes(a), b'')
        self.assertEqual(bytes(b), b'')
        self.assertEqual(a.size, (0, ))
        self.assertEqual(b.size, (0, ))
        self.assertEqual(a.stride, (0, ))
        self.assertEqual(b.stride, (0, ))
        # By default the format is unspecified
        self.assertEqual(b.format, None)
        self.assertEqual(a.dimensions, 1)
        self.assertEqual(b.dimensions, 1)

    def test_init_buffer(self):
        a = b'hello'
        a_refcount = sys.getrefcount(a)
        # Verify that bytes has the same semantics, i.e. not possible to
        # directly get a character
        self.assertEqual(a[2], ord('l'))

        b = containers.StridedArrayView1D(a)
        self.assertIs(b.owner, a)
        self.assertEqual(len(b), 5)
        self.assertEqual(bytes(b), b'hello')
        self.assertEqual(b.size, (5, ))
        self.assertEqual(b.stride, (1, ))
        # We don't provide typed access for views created from buffers, so the
        # format is unspecified to convey "generic data"
        self.assertEqual(b.format, None)
        self.assertEqual(b[2], ord('l'))
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        # Not mutable
        with self.assertRaisesRegex(TypeError, "object does not support item assignment"):
            b[4] = ord('!')

        # b should keep a reference to a, so deleting the local reference
        # shouldn't affect it
        del a
        self.assertTrue(sys.getrefcount(b.owner), a_refcount)
        self.assertEqual(b[2], ord('l'))

        # Now, if we delete b, a should not be referenced by anything anymore
        a = b.owner
        del b
        self.assertTrue(sys.getrefcount(a), a_refcount)

    def test_init_buffer_empty(self):
        a = b''
        a_refcount = sys.getrefcount(a)

        b = containers.StridedArrayView1D(a)
        self.assertIs(b.owner, None)
        self.assertEqual(len(b), 0)
        self.assertEqual(b.size, (0, ))
        self.assertEqual(b.stride, (1, ))
        self.assertEqual(b.format, None)
        self.assertEqual(sys.getrefcount(a), a_refcount)

    def test_init_buffer_memoryview_obj(self):
        a = b'hello'
        v = memoryview(a)
        b = containers.StridedArrayView1D(v)
        # memoryview's buffer protocol returns itself, not the underlying
        # bytes, as it manages the Py_buffer instance. So this is expected.
        self.assertIs(b.owner, v)

    def test_init_buffer_mutable(self):
        a = bytearray(b'hello')
        # Verify that the bytearray has the same semantics, i.e. not possible
        # to directly assign a character
        a[4] = ord('?')
        self.assertEqual(a[4], ord('?'))

        b = containers.MutableStridedArrayView1D(a)
        self.assertEqual(b.size, (5, ))
        self.assertEqual(b.stride, (1, ))
        # We don't provide typed access for views created from buffers, so the
        # format is unspecified to convey "generic data"
        self.assertEqual(b.format, None)
        self.assertEqual(b[4], ord('?'))
        b[4] = ord('!')
        self.assertEqual(b[4], ord('!'))
        self.assertEqual(bytes(b), b'hell!')

    def test_init_buffer_unexpected_dimensions(self):
        a = memoryview(b'123456').cast('b', shape=[2, 3])
        self.assertEqual(bytes(a), b'123456')
        with self.assertRaisesRegex(BufferError, "expected 1 dimensions but got 2"):
            b = containers.StridedArrayView1D(a)

    def test_init_buffer_stride(self):
        a = memoryview(b'hello')[::2]
        self.assertEqual(bytes(a), b'hlo')
        b = containers.StridedArrayView1D(a)
        self.assertEqual(len(b), 3)
        self.assertEqual(bytes(b), b'hlo')
        self.assertEqual(b.size, (3, ))
        self.assertEqual(b.stride, (2, ))
        self.assertEqual(b[2], ord('o'))

    def test_init_buffer_mutable_from_immutable(self):
        a = b'hello'
        with self.assertRaisesRegex(BufferError, "Object is not writable."):
            b = containers.MutableStridedArrayView1D(a)

    def test_slice(self):
        a = b'World is hell!'
        a_refcount = sys.getrefcount(a)

        b = containers.StridedArrayView1D(a)
        b_refcount = sys.getrefcount(b)
        self.assertIs(b.owner, a)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        # When slicing, b's refcount should not change but a's refcount should
        # increase
        c = b[4:-4]
        self.assertEqual(c.size, (6,))
        self.assertEqual(c.stride, (1,))
        self.assertIs(c.owner, a)
        self.assertIsInstance(c, containers.StridedArrayView1D)
        self.assertEqual(bytes(c), b'd is h')
        self.assertEqual(sys.getrefcount(b), b_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)

        # Deleting a slice should reduce a's refcount again, keep b's unchanged
        del c
        self.assertEqual(sys.getrefcount(b), b_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

    def test_slice_empty(self):
        data = b'hello'
        data_refcount = sys.getrefcount(data)

        # Because this is out of range, slice.start = slice.stop
        a = containers.StridedArrayView1D(data)[7:8]
        self.assertEqual(a.size, (0, ))

        # Empty view, original data not referenced at all
        self.assertIs(a.owner, None)
        self.assertEqual(sys.getrefcount(data), data_refcount)

    def test_slice_invalid(self):
        with self.assertRaisesRegex(TypeError, "indices must be integers"):
            containers.StridedArrayView1D()[-5:3:"boo"]

    def test_slice_stride(self):
        a = b'World_ _i_s_ _hell!'
        b = containers.StridedArrayView1D(a)

        # Check consistency with slices on bytes
        c1 = a[4:-4:2]
        c2 = b[4:-4:2]
        self.assertIsInstance(c2, containers.StridedArrayView1D)
        self.assertEqual(len(c1), 6)
        self.assertEqual(len(c2), 6)
        self.assertEqual(bytes(c1), b'd is h')
        self.assertEqual(bytes(c2), b'd is h')
        self.assertEqual(c2.size, (6,))
        self.assertEqual(c2.stride, (2,))

    def test_slice_stride_negative(self):
        a = b'World_ _i_s_ _hell!'
        b = containers.StridedArrayView1D(a)

        # Check consistency with slices on bytes
        c1 = a[-5:3:-2] # like [4:-4:2] above, but reverted
        c2 = b[-5:3:-2]
        self.assertEqual(len(c1), 6)
        self.assertEqual(len(c2), 6)
        self.assertEqual(bytes(c1), b'h si d') # like b'd is h' but reverted
        self.assertEqual(bytes(c2), b'h si d')
        self.assertEqual(c2.size, (6,))
        self.assertEqual(c2.stride, (-2,))

    def test_ops(self):
        a = b'01234567'

        # slice_bit() tested extensively in StridedBitArrayView tests

        b = containers.StridedArrayView1D(a).flipped(0)
        self.assertEqual(b.size, (8,))
        self.assertEqual(b.stride, (-1,))
        self.assertEqual(bytes(b), b'76543210')

        c = containers.StridedArrayView1D(a)[3:4].broadcasted(0, 5)
        self.assertEqual(c.size, (5,))
        self.assertEqual(c.stride, (0,))
        self.assertEqual(bytes(c), b'33333')

        d2 = containers.StridedArrayView1D(a).expanded(0, (2, 4))
        self.assertIsInstance(d2, containers.StridedArrayView2D)
        self.assertEqual(d2.size, (2, 4))
        self.assertEqual(d2.stride, (4, 1))
        self.assertEqual(bytes(d2), b'01234567')

        d3 = containers.StridedArrayView1D(a).expanded(0, (2, 2, 2))
        self.assertIsInstance(d3, containers.StridedArrayView3D)
        self.assertEqual(d3.size, (2, 2, 2))
        self.assertEqual(d3.stride, (4, 2, 1))
        self.assertEqual(bytes(d3), b'01234567')

        d4 = containers.StridedArrayView1D(a).expanded(0, (2, 1, 2, 2))
        self.assertIsInstance(d4, containers.StridedArrayView4D)
        self.assertEqual(d4.size, (2, 1, 2, 2))
        self.assertEqual(d4.stride, (4, 4, 2, 1))
        self.assertEqual(bytes(d4), b'01234567')

    def test_ops_invalid(self):
        a = b'00'

        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 bits"):
            containers.StridedArrayView1D(a).slice_bit(8)
        with self.assertRaisesRegex(IndexError, "dimension 1 out of range for a 1D view"):
            containers.StridedArrayView1D().flipped(1)
        with self.assertRaisesRegex(IndexError, "dimension 1 out of range for a 1D view"):
            containers.StridedArrayView1D(a).broadcasted(1, 3)
        with self.assertRaisesRegex(IndexError, "dimension 1 out of range for a 1D view"):
            containers.StridedArrayView1D(a).expanded(1, (1, 2))
        with self.assertRaisesRegex(ValueError, "can't broadcast dimension 0 with 2 elements"):
            containers.StridedArrayView1D(a).broadcasted(0, 3)
        with self.assertRaisesRegex(ValueError, "total size 3 doesn't match dimension 0 with 2 elements"):
            containers.StridedArrayView1D(a).expanded(0, (1, 3))

    def test_convert_memoryview(self):
        a = b'World is hell!'
        a_refcount = sys.getrefcount(a)

        b = containers.StridedArrayView1D(a)
        b_refcount = sys.getrefcount(b)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        c = memoryview(b)
        self.assertEqual(c.ndim, 1)
        self.assertEqual(len(c), len(a))
        self.assertEqual(bytes(c), a)
        # Unlike slicing, StridedArrayView's buffer protocol returns a
        # reference to itself and not the underlying buffer -- it needs to be
        # kept around because the Py_buffer refers to its internals for size.
        # Also returning a reference to the underlying buffer would mean the
        # underlying buffer's releasebuffer function gets called instead of
        # ours which is *not* wanted.
        self.assertIs(c.obj, b)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        with self.assertRaisesRegex(TypeError, "cannot modify read-only memory"):
            c[-1] = ord('?')

    def test_convert_mutable_memoryview(self):
        a = bytearray(b'World is hell!')
        b = memoryview(containers.MutableStridedArrayView1D(a))
        b[-1] = ord('?')
        self.assertEqual(a, b'World is hell?')

class StridedArrayView2D(unittest.TestCase):
    def test_init(self):
        a = containers.StridedArrayView2D()
        b = containers.MutableStridedArrayView2D()
        self.assertIs(a.owner, None)
        self.assertIs(b.owner, None)
        self.assertEqual(len(a), 0)
        self.assertEqual(len(b), 0)
        self.assertEqual(bytes(a), b'')
        self.assertEqual(bytes(b), b'')
        self.assertEqual(a.size, (0, 0))
        self.assertEqual(b.size, (0, 0))
        self.assertEqual(a.stride, (0, 0))
        self.assertEqual(b.stride, (0, 0))
        # By default the format is unspecified
        self.assertEqual(b.format, None)
        self.assertEqual(a.dimensions, 2)
        self.assertEqual(b.dimensions, 2)

    def test_init_buffer(self):
        a = (b'01234567'
             b'456789ab'
             b'89abcdef')
        a_refcount = sys.getrefcount(a)

        v = memoryview(a).cast('b', shape=[3, 8])
        b = containers.StridedArrayView2D(v)
        self.assertEqual(len(b), 3)
        self.assertEqual(bytes(b), b'01234567'
                                   b'456789ab'
                                   b'89abcdef')
        self.assertEqual(b.size, (3, 8))
        self.assertEqual(b.stride, (8, 1))
        self.assertIsInstance(b[1], containers.StridedArrayView1D)
        self.assertEqual(bytes(b[1]), b'456789ab')
        self.assertEqual(b[1, 2], ord('6'))
        self.assertEqual(b[1][2], ord('6'))
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        # Not mutable
        with self.assertRaisesRegex(TypeError, "object does not support item assignment"):
            b[1, 2] = ord('!')

        # b should keep a reference to a, so deleting the local reference
        # shouldn't affect it
        del a
        self.assertTrue(sys.getrefcount(b.owner), a_refcount)
        self.assertEqual(b[1][2], ord('6'))

        # Now, if we delete b, a should not be referenced by anything anymore
        a = b.owner
        del b
        self.assertTrue(sys.getrefcount(a), a_refcount)

    def test_init_buffer_mutable(self):
        a = bytearray(b'01234567'
                      b'456789ab'
                      b'89abcdef')
        b = containers.MutableStridedArrayView2D(memoryview(a).cast('b', shape=[3, 8]))
        b[0, 7] = ord('!')
        b[1, 7] = ord('!')
        b[2, 7] = ord('!')
        self.assertEqual(b[0][7], ord('!'))
        self.assertEqual(bytes(b), b'0123456!'
                                   b'456789a!'
                                   b'89abcde!')

    def test_init_buffer_unexpected_dimensions(self):
        a = b'123456'
        with self.assertRaisesRegex(BufferError, "expected 2 dimensions but got 1"):
            b = containers.StridedArrayView2D(a)

    def test_init_buffer_stride(self):
        a = memoryview(b'01234567'
                       b'456789ab'
                       b'89abcdef').cast('b', shape=[3, 8])[::2]
        self.assertEqual(bytes(a), b'0123456789abcdef')
        b = containers.StridedArrayView2D(a)
        self.assertEqual(len(b), 2)
        self.assertEqual(bytes(b), b'0123456789abcdef')
        self.assertEqual(b.size, (2, 8))
        self.assertEqual(b.stride, (16, 1))
        self.assertEqual(bytes(b[1]), b'89abcdef')
        self.assertEqual(b[1][3], ord('b'))

    def test_init_buffer_mutable_from_immutable(self):
        a = memoryview(b'01234567'
                       b'456789ab'
                       b'89abcdef').cast('b', shape=[3, 8])
        with self.assertRaisesRegex(BufferError, "underlying buffer is not writable"):
            b = containers.MutableStridedArrayView2D(a)

    def test_slice(self):
        a = memoryview(b'01234567'
                       b'456789ab'
                       b'89abcdef').cast('b', shape=[3, 8])
        a_refcount = sys.getrefcount(a)

        b = containers.StridedArrayView2D(a)
        b_refcount = sys.getrefcount(b)
        # memoryview's buffer protocol returns itself, not the underlying
        # bytes, as it manages the Py_buffer instance. So this is expected.
        self.assertIs(b.owner, a)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        # When slicing, b's refcount should not change but a's refcount should
        # increase
        c = b[0:-1]
        self.assertIsInstance(c, containers.StridedArrayView2D)
        self.assertEqual(c.size, (2, 8))
        self.assertEqual(c.stride, (8, 1))
        self.assertEqual(bytes(c), b'01234567456789ab')
        self.assertEqual(sys.getrefcount(b), b_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)

        # Deleting a slice should reduce a's refcount again, keep b's unchanged
        del c
        self.assertEqual(sys.getrefcount(b), b_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

    def test_slice_empty(self):
        a = memoryview(b'01234567'
                       b'456789ab'
                       b'89abcdef').cast('b', shape=[3, 8])
        a_refcount = sys.getrefcount(a)

        b = containers.StridedArrayView2D(a)[1:1]
        self.assertEqual(b.size, (0, 8))

        # Empty view, original data not referenced at all
        self.assertIs(b.owner, None)
        self.assertEqual(sys.getrefcount(a), a_refcount)

    def test_slice_multidimensional(self):
        a = memoryview(b'01234567'
                       b'456789ab'
                       b'89abcdef').cast('b', shape=[3, 8])
        a_refcount = sys.getrefcount(a)

        b = containers.StridedArrayView2D(a)
        b_refcount = sys.getrefcount(b)
        # memoryview's buffer protocol returns itself, not the underlying
        # bytes, as it manages the Py_buffer instance. So this is expected.
        self.assertIs(b.owner, a)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        # When slicing, b's refcount should not change but a's refcount should
        # increase
        c = b[1:3,4:7]
        self.assertIsInstance(c, containers.StridedArrayView2D)
        self.assertEqual(c.size, (2, 3))
        self.assertEqual(c.stride, (8, 1))
        self.assertEqual(bytes(c[0]), b'89a')
        self.assertEqual(bytes(c[1]), b'cde')
        self.assertEqual(sys.getrefcount(b), b_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 2)

        # Deleting a slice should reduce a's refcount again, keep b's unchanged
        del c
        self.assertEqual(sys.getrefcount(b), b_refcount)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

    def test_slice_multidimensional_empty(self):
        a = memoryview(b'01234567'
                       b'456789ab'
                       b'89abcdef').cast('b', shape=[3, 8])
        a_refcount = sys.getrefcount(a)

        b = containers.StridedArrayView2D(a)[1:1,2:2]
        self.assertEqual(b.size, (0, 0))

        # Empty view, original data not referenced at all
        self.assertIs(b.owner, None)
        self.assertEqual(sys.getrefcount(a), a_refcount)

    def test_slice_invalid(self):
        with self.assertRaisesRegex(ValueError, "slice step cannot be zero"):
            containers.StridedArrayView1D()[-5:3:0]

    def test_slice_stride(self):
        a = (b'01234567'
             b'456789ab'
             b'89abcdef')
        v = memoryview(a).cast('b', shape=[3, 8])
        b = containers.StridedArrayView2D(v)

        # Check consistency with slices on memoryview
        c1 = v[0:3:2]
        c2 = b[0:3:2]
        self.assertEqual(len(c1), 2)
        self.assertEqual(len(c2), 2)
        self.assertIsInstance(c2, containers.StridedArrayView2D)
        self.assertEqual(bytes(c1), b'0123456789abcdef')
        self.assertEqual(bytes(c2), b'0123456789abcdef')
        self.assertEqual(c2.size, (2, 8))
        self.assertEqual(c2.stride, (16, 1))
        self.assertEqual(bytes(c2[1]), b'89abcdef')

    def test_slice_stride_negative(self):
        a = (b'01234567'
             b'456789ab'
             b'89abcdef')
        v = memoryview(a).cast('b', shape=[3, 8])
        b = containers.StridedArrayView2D(v)

        # Check consistency with slices on memoryview
        self.assertEqual(v.shape, (3, 8))
        self.assertEqual(b.size, (3, 8))
        self.assertEqual(v.strides, (8, 1))
        self.assertEqual(b.stride, (8, 1))
        c1 = v[-1:None:-2] # like [0:3:2] above, but reverted
        c2 = b[-1:None:-2]
        self.assertEqual(len(c1), 2)
        self.assertEqual(len(c2), 2)
        self.assertEqual(bytes(c1), b'89abcdef01234567') # like above but reverted
        self.assertEqual(bytes(c2), b'89abcdef01234567')
        self.assertEqual(c1.shape, (2, 8))
        self.assertEqual(c2.size, (2, 8))
        self.assertEqual(c1.strides, (-16, 1))
        self.assertEqual(c2.stride, (-16, 1))

    def test_slice_stride_negative_multidimensional(self):
        a = (b'01234567'
             b'456789ab'
             b'89abcdef')
        v = memoryview(a).cast('b', shape=[3, 8])
        b = containers.StridedArrayView2D(v)

        # Check consistency with slices on memoryview
        self.assertEqual(v.shape, (3, 8))
        self.assertEqual(b.size, (3, 8))
        self.assertEqual(v.strides, (8, 1))
        self.assertEqual(b.stride, (8, 1))

        with self.assertRaises(NotImplementedError):
            c1 = v[-1:None:-2, -2:2:-3] # HAH!

        c2 = b[-1:None:-2, -2:2:-3]
        self.assertEqual(len(c2), 2)
        self.assertEqual(bytes(c2), b'eb63')
        self.assertEqual(c2.size, (2, 2))
        self.assertEqual(c2.stride, (-16, -3))

    def test_ops(self):
        a = (b'01234567'
             b'456789ab'
             b'89abcdef')
        v = memoryview(a).cast('b', shape=[3, 8])

        # slice_bit() tested extensively in StridedBitArrayView tests

        b = containers.StridedArrayView2D(v).transposed(0, 1).flipped(0)
        self.assertEqual(b.size, (8, 3))
        self.assertEqual(b.stride, (-1, 8))
        self.assertEqual(bytes(b), b'7bf6ae59d48c37b26a159048')

        c = containers.StridedArrayView2D(v).transposed(1, 0).flipped(1)
        self.assertEqual(c.size, (8, 3))
        self.assertEqual(c.stride, (1, -8))
        self.assertEqual(bytes(c), b'840951a62b73c84d95ea6fb7')

        d = containers.StridedArrayView2D(v).transposed(0, 1)[3:4].broadcasted(0, 5)
        self.assertEqual(d.size, (5, 3))
        self.assertEqual(d.stride, (0, 8))
        self.assertEqual(bytes(d), b'37b37b37b37b37b')

        d = containers.StridedArrayView2D(v)[:, 3:4].broadcasted(1, 2)
        self.assertEqual(d.size, (3, 2))
        self.assertEqual(d.stride, (8, 0))
        self.assertEqual(bytes(d), b'3377bb')

        e3 = containers.StridedArrayView2D(v).expanded(1, (2, 4))
        self.assertIsInstance(e3, containers.StridedArrayView3D)
        self.assertEqual(e3.size, (3, 2, 4))
        self.assertEqual(e3.stride, (8, 4, 1))
        self.assertEqual(bytes(e3), b'01234567456789ab89abcdef')

        e4 = containers.StridedArrayView2D(v).expanded(0, (1, 3, 1))
        self.assertIsInstance(e4, containers.StridedArrayView4D)
        self.assertEqual(e4.size, (1, 3, 1, 8))
        self.assertEqual(e4.stride, (24, 8, 8, 1))
        self.assertEqual(bytes(e4), b'01234567456789ab89abcdef')

    def test_ops_invalid(self):
        a = b'00'
        v = memoryview(a).cast('b', shape=[1, 2])

        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 bits"):
            containers.StridedArrayView2D(v).slice_bit(8)
        with self.assertRaisesRegex(IndexError, "dimension 2 out of range for a 2D view"):
            containers.StridedArrayView2D().flipped(2)
        with self.assertRaisesRegex(IndexError, "dimension 2 out of range for a 2D view"):
            containers.StridedArrayView2D(v).broadcasted(2, 3)
        with self.assertRaisesRegex(IndexError, "dimension 2 out of range for a 2D view"):
            containers.StridedArrayView2D(v).expanded(2, (1, 2))
        with self.assertRaisesRegex(ValueError, "can't broadcast dimension 1 with 2 elements"):
            containers.StridedArrayView2D(v).broadcasted(1, 3)
        with self.assertRaisesRegex(IndexError, "dimensions 0, 2 can't be transposed in a 2D view"):
            containers.StridedArrayView2D().transposed(0, 2)
        with self.assertRaisesRegex(ValueError, "total size 2 doesn't match dimension 0 with 1 elements"):
            containers.StridedArrayView2D(v).expanded(0, (1, 2))
        with self.assertRaisesRegex(ValueError, "total size 4 doesn't match dimension 1 with 2 elements"):
            containers.StridedArrayView2D(v).expanded(1, (2, 1, 2))

    def test_convert_memoryview(self):
        a = memoryview(b'01234567'
                       b'456789ab'
                       b'89abcdef').cast('b', shape=[3, 8])
        a_refcount = sys.getrefcount(a)

        b = containers.StridedArrayView2D(a)
        b_refcount = sys.getrefcount(b)
        # memoryview's buffer protocol returns itself, not the underlying
        # bytes, as it manages the Py_buffer instance. So this is expected.
        self.assertIs(b.owner, a)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        c = memoryview(b)
        self.assertEqual(c.ndim, 2)
        self.assertEqual(c.shape, (3, 8))
        self.assertEqual(c.strides, (8, 1))
        self.assertIs(c.obj, b)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)
        self.assertEqual(sys.getrefcount(b), b_refcount + 1)

# Slicing is tested extensively for StridedArrayView2D, this checks just what
# differs, like constructors and fancy operations
class StridedArrayView3D(unittest.TestCase):
    def test_init_buffer(self):
        a = (b'01234567'
             b'456789ab'
             b'89abcdef'

             b'cdef0123'
             b'01234567'
             b'456789ab')
        b = containers.StridedArrayView3D(memoryview(a).cast('b', shape=[2, 3, 8]))
        self.assertEqual(len(b), 2)
        self.assertEqual(bytes(b), b'01234567456789ab89abcdefcdef012301234567456789ab')
        self.assertEqual(b.size, (2, 3, 8))
        self.assertEqual(b.stride, (24, 8, 1))
        self.assertEqual(b[1, 2, 3], ord('7'))
        self.assertEqual(b[1][2][3], ord('7'))

    def test_init_buffer_mutable(self):
        a = bytearray(b'01234567'
                      b'456789ab'
                      b'89abcdef'

                      b'cdef0123'
                      b'01234567'
                      b'456789ab')
        b = containers.MutableStridedArrayView3D(memoryview(a).cast('b', shape=[2, 3, 8]))
        b[0, 0, 7] = ord('!')
        b[0, 1, 7] = ord('!')
        b[0, 2, 7] = ord('!')
        b[1, 0, 7] = ord('!')
        b[1, 1, 7] = ord('!')
        b[1, 2, 7] = ord('!')
        self.assertEqual(b[1][1][7], ord('!'))
        self.assertEqual(bytes(b), b'0123456!'
                                   b'456789a!'
                                   b'89abcde!'

                                   b'cdef012!'
                                   b'0123456!'
                                   b'456789a!')

    def test_ops(self):
        a = (b'01234567'
             b'456789ab'
             b'89abcdef'

             b'cdef0123'
             b'01234567'
             b'456789ab')
        v = memoryview(a).cast('b', shape=[2, 3, 8])

        # slice_bit() tested extensively in StridedBitArrayView tests

        b = containers.StridedArrayView3D(v).transposed(0, 1).flipped(0)
        self.assertEqual(b.size, (3, 2, 8))
        self.assertEqual(b.stride, (-8, 24, 1))
        self.assertEqual(bytes(b), b'89abcdef456789ab456789ab0123456701234567cdef0123')

        c = containers.StridedArrayView3D(v).transposed(2, 0).flipped(1)
        self.assertEqual(c.size, (8, 3, 2))
        self.assertEqual(c.stride, (1, -8, 24))
        self.assertEqual(bytes(c), b'84400c95511da6622eb7733fc88440d99551eaa662fbb773')

        d = containers.StridedArrayView3D(v).transposed(1, 2)[0:1, 3:5, :].broadcasted(0, 5)
        self.assertEqual(d.size, (5, 2, 3))
        self.assertEqual(d.stride, (0, 1, 8))
        self.assertEqual(bytes(d), b'37b48c37b48c37b48c37b48c37b48c')

        e = containers.StridedArrayView3D(v)[:, 1:2, 3:4].flipped(2).broadcasted(1, 2)
        self.assertEqual(e.size, (2, 2, 1))
        self.assertEqual(e.stride, (24, 0, -1))
        self.assertEqual(bytes(e), b'7733')

        f = containers.StridedArrayView3D(v)[:, :, 0:1].broadcasted(2, 5)
        self.assertEqual(f.size, (2, 3, 5))
        self.assertEqual(f.stride, (24, 8, 0))
        self.assertEqual(bytes(f), b'000004444488888ccccc0000044444')

        g4 = containers.StridedArrayView3D(v).expanded(2, (2, 4))
        self.assertIsInstance(g4, containers.StridedArrayView4D)
        self.assertEqual(g4.size, (2, 3, 2, 4))
        self.assertEqual(g4.stride, (24, 8, 4, 1))
        self.assertEqual(bytes(g4), b'01234567456789ab89abcdefcdef012301234567456789ab')

    def test_ops_invalid(self):
        a = b'00'
        v = memoryview(a).cast('b', shape=[1, 1, 2])

        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 bits"):
            containers.StridedArrayView3D(v).slice_bit(8)
        with self.assertRaisesRegex(IndexError, "dimension 3 out of range for a 3D view"):
            containers.StridedArrayView3D().flipped(3)
        with self.assertRaisesRegex(IndexError, "dimension 3 out of range for a 3D view"):
            containers.StridedArrayView3D(v).broadcasted(3, 3)
        with self.assertRaisesRegex(IndexError, "dimension 3 out of range for a 3D view"):
            containers.StridedArrayView3D(v).expanded(3, (1, 2))
        with self.assertRaisesRegex(ValueError, "can't broadcast dimension 2 with 2 elements"):
            containers.StridedArrayView3D(v).broadcasted(2, 3)
        with self.assertRaisesRegex(IndexError, "dimensions 1, 3 can't be transposed in a 3D view"):
            containers.StridedArrayView3D().transposed(1, 3)
        with self.assertRaisesRegex(ValueError, "total size 2 doesn't match dimension 0 with 1 elements"):
            containers.StridedArrayView3D(v).expanded(0, (1, 2))
        with self.assertRaisesRegex(ValueError, "total size 3 doesn't match dimension 2 with 2 elements"):
            containers.StridedArrayView3D(v).expanded(2, (3, 1))

# This is just a dumb copy of the above with one dimension inserted at the
# second place.
class StridedArrayView4D(unittest.TestCase):
    def test_init_buffer(self):
        a = (b'01234567'
             b'456789ab'
             b'89abcdef'

             b'cdef0123'
             b'01234567'
             b'456789ab')
        b = containers.StridedArrayView4D(memoryview(a).cast('b', shape=[2, 1, 3, 8]))
        self.assertEqual(len(b), 2)
        self.assertEqual(bytes(b), b'01234567456789ab89abcdefcdef012301234567456789ab')
        self.assertEqual(b.size, (2, 1, 3, 8))
        self.assertEqual(b.stride, (24, 24, 8, 1))
        self.assertEqual(b[1, 0, 2, 3], ord('7'))
        self.assertEqual(b[1][0][2][3], ord('7'))

    def test_init_buffer_mutable(self):
        a = bytearray(b'01234567'
                      b'456789ab'
                      b'89abcdef'

                      b'cdef0123'
                      b'01234567'
                      b'456789ab')
        b = containers.MutableStridedArrayView4D(memoryview(a).cast('b', shape=[2, 1, 3, 8]))
        b[0, 0, 0, 7] = ord('!')
        b[0, 0, 1, 7] = ord('!')
        b[0, 0, 2, 7] = ord('!')
        b[1, 0, 0, 7] = ord('!')
        b[1, 0, 1, 7] = ord('!')
        b[1, 0, 2, 7] = ord('!')
        self.assertEqual(b[1][0][1][7], ord('!'))
        self.assertEqual(bytes(b), b'0123456!'
                                   b'456789a!'
                                   b'89abcde!'

                                   b'cdef012!'
                                   b'0123456!'
                                   b'456789a!')

    def test_ops(self):
        a = (b'01234567'
             b'456789ab'
             b'89abcdef'

             b'cdef0123'
             b'01234567'
             b'456789ab')
        v = memoryview(a).cast('b', shape=[2, 1, 3, 8])

        # slice_bit() tested extensively in StridedBitArrayView tests

        b = containers.StridedArrayView4D(v).transposed(0, 2).flipped(0)
        self.assertEqual(b.size, (3, 1, 2, 8))
        self.assertEqual(b.stride, (-8, 24, 24, 1))
        self.assertEqual(bytes(b), b'89abcdef456789ab456789ab0123456701234567cdef0123')

        c = containers.StridedArrayView4D(v).transposed(3, 0).flipped(2)
        self.assertEqual(c.size, (8, 1, 3, 2))
        self.assertEqual(c.stride, (1, 24, -8, 24))
        self.assertEqual(bytes(c), b'84400c95511da6622eb7733fc88440d99551eaa662fbb773')

        d = containers.StridedArrayView4D(v).transposed(2, 3)[0:1, :, 3:5, :].broadcasted(0, 5)
        self.assertEqual(d.size, (5, 1, 2, 3))
        self.assertEqual(d.stride, (0, 24, 1, 8))
        self.assertEqual(bytes(d), b'37b48c37b48c37b48c37b48c37b48c')

        e = containers.StridedArrayView4D(v)[:, :, 1:2, 3:4].flipped(3).broadcasted(2, 2)
        self.assertEqual(e.size, (2, 1, 2, 1))
        self.assertEqual(e.stride, (24, 24, 0, -1))
        self.assertEqual(bytes(e), b'7733')

        f = containers.StridedArrayView4D(v)[:, :, :, 0:1].broadcasted(3, 5)
        self.assertEqual(f.size, (2, 1, 3, 5))
        self.assertEqual(f.stride, (24, 24, 8, 0))
        self.assertEqual(bytes(f), b'000004444488888ccccc0000044444')

    def test_ops_invalid(self):
        a = b'00'
        v = memoryview(a).cast('b', shape=[1, 1, 1, 2])

        with self.assertRaisesRegex(IndexError, "index 8 out of range for 8 bits"):
            containers.StridedArrayView4D(v).slice_bit(8)
        with self.assertRaisesRegex(IndexError, "dimension 4 out of range for a 4D view"):
            containers.StridedArrayView4D().flipped(4)
        with self.assertRaisesRegex(IndexError, "dimension 4 out of range for a 4D view"):
            containers.StridedArrayView4D(v).broadcasted(4, 3)
        with self.assertRaisesRegex(ValueError, "can't broadcast dimension 3 with 2 elements"):
            containers.StridedArrayView4D(v).broadcasted(3, 3)
        with self.assertRaisesRegex(IndexError, "dimensions 4, 3 can't be transposed in a 4D view"):
            containers.StridedArrayView4D().transposed(4, 3)

class StridedArrayViewCustomType(unittest.TestCase):
    def test_short(self):
        a = test_stridedarrayview.get_containers()
        self.assertEqual(type(a.view), containers.StridedArrayView2D)
        self.assertEqual(a.view.size, (2, 3))
        self.assertEqual(a.view.stride, (3*2, 2))
        self.assertEqual(a.view.format, 'h')
        self.assertEqual(a.list, [3, -17565, 5, 3, -17565, 5])
        self.assertEqual(a.view[0][0], 3)
        self.assertEqual(a.view[0][1], -17565)
        self.assertEqual(a.view[0][2], 5)
        self.assertEqual(a.view[1][0], 3)
        self.assertEqual(a.view[1][1], -17565)
        self.assertEqual(a.view[1][2], 5)

        with self.assertRaisesRegex(TypeError, "object does not support item assignment"):
            a.view[1][1] = 15

        # Test that memoryview understands the type
        av = memoryview(a.view[0])
        self.assertEqual(av[0], 3)
        self.assertEqual(av[1], -17565)
        self.assertEqual(av[2], 5)

    def test_mutable_int(self):
        a = test_stridedarrayview.MutableContaineri()
        self.assertEqual(type(a.view), containers.MutableStridedArrayView2D)
        self.assertEqual(a.view.format, 'i')
        self.assertEqual(a.list, [0, 0, 0, 0, 0, 0])
        a.view[0][1] = -7656581
        a.view[1][2] = 4666
        self.assertEqual(a.list, [0, -7656581, 0, 0, 0, 4666])

        # Test that memoryview understands the type and has changes reflected
        av = memoryview(a.view[1])
        a.view[1][0] = -333
        self.assertEqual(av[0], -333)
        self.assertEqual(av[1], 0)
        self.assertEqual(av[2], 4666)

        # And the other way around as well
        av[1] = 11111
        self.assertEqual(a.list, [0, -7656581, 0, -333, 11111, 4666])

    # mutable_vector3d and mutable_long_float tested in test_containers_numpy
    # as memoryview can't handle their types

class StridedArrayViewCustomDynamicType(unittest.TestCase):
    # TODO test construction from a (typed) array or memory view, should work
    #   and now it doesn't

    def test_float(self):
        a = test_stridedarrayview.MutableContainerDynamicType('f')
        self.assertEqual(a.view.size, (2, 3))
        self.assertEqual(a.view.stride, (12, 4))
        self.assertEqual(a.view.format, 'f')
        a.view[0][1] = 15.0
        a.view[1][0] = -22.0
        self.assertEqual(a.view[0][0], 0.0)
        self.assertEqual(a.view[0][1], 15.0)
        self.assertEqual(a.view[0][2], 0.0)
        self.assertEqual(a.view[1][0], -22.0)
        self.assertEqual(a.view[1][1], 0.0)
        self.assertEqual(a.view[1][2], 0.0)

    def test_int(self):
        a = test_stridedarrayview.MutableContainerDynamicType('i')
        self.assertEqual(a.view.size, (2, 3))
        self.assertEqual(a.view.stride, (12, 4))
        self.assertEqual(a.view.format, 'i')
        a.view[0][2] = 15
        a.view[1][1] = -773
        self.assertEqual(a.view[0][0], 0)
        self.assertEqual(a.view[0][1], 0)
        self.assertEqual(a.view[0][2], 15)
        self.assertEqual(a.view[1][0], 0)
        self.assertEqual(a.view[1][1], -773)
        self.assertEqual(a.view[1][2], 0)

class BitArray(unittest.TestCase):
    def test_init(self):
        a = containers.BitArray()
        self.assertEqual(len(a), 0)
        self.assertEqual(a.offset, 0)

    def test_value_init(self):
        a = containers.BitArray.value_init(3)
        self.assertEqual(len(a), 3)
        self.assertEqual(a.offset, 0)
        self.assertEqual(a[0], False)
        self.assertEqual(a[1], False)
        self.assertEqual(a[2], False)

    def test_no_init(self):
        a = containers.BitArray.no_init(3)
        self.assertEqual(len(a), 3)
        self.assertEqual(a.offset, 0)
        # Values can be anything

    def test_direct_init(self):
        a = containers.BitArray.direct_init(3, False)
        b = containers.BitArray.direct_init(3, True)
        self.assertEqual(len(a), 3)
        self.assertEqual(len(b), 3)
        self.assertEqual(a.offset, 0)
        self.assertEqual(b.offset, 0)
        self.assertEqual(a[0], False)
        self.assertEqual(a[1], False)
        self.assertEqual(a[2], False)
        self.assertEqual(b[0], True)
        self.assertEqual(b[1], True)
        self.assertEqual(b[2], True)

    def test_mutable_access(self):
        a = containers.BitArray.value_init(5)
        a[2] = True
        a[4] = True
        self.assertEqual(a[0], False)
        self.assertEqual(a[1], False)
        self.assertEqual(a[2], True)
        self.assertEqual(a[3], False)
        self.assertEqual(a[4], True)

    def test_access_invalid(self):
        a = containers.BitArray.value_init(3)

        with self.assertRaises(IndexError):
            a[3]
        with self.assertRaises(IndexError):
            a[3] = True

    def test_slice(self):
        a = containers.BitArray.value_init(10)
        a[3] = True
        a[5] = True
        a[6] = True
        a_refcount = sys.getrefcount(a)

        b = a[3:-2]
        self.assertIsInstance(b, containers.MutableBitArrayView)
        self.assertIs(b.owner, a)
        self.assertEqual(len(b), 5)
        self.assertEqual(b.offset, 3)
        self.assertEqual(b[0], True)
        self.assertEqual(b[1], False)
        self.assertEqual(b[2], True)
        self.assertEqual(b[3], True)
        self.assertEqual(b[4], False)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        # Deleting a slice should reduce a's refcount again
        del b
        self.assertEqual(sys.getrefcount(a), a_refcount)

    def test_slice_empty(self):
        a = containers.BitArray.value_init(5)
        a_refcount = sys.getrefcount(a)

        # Because this is out of range, slice.start = slice.stop
        b = a[7:8]
        self.assertEqual(len(b), 0)
        self.assertEqual(b.offset, 5)

        # Empty view, original data not referenced at all
        self.assertIs(b.owner, None)
        self.assertEqual(sys.getrefcount(a), a_refcount)

    def test_slice_invalid(self):
        with self.assertRaisesRegex(ValueError, "slice step cannot be zero"):
            containers.BitArray()[::0]

    def test_slice_stride(self):
        a = containers.BitArray.value_init(15)
        a[2] = True
        a[5] = True
        a[7] = False
        a[11] = True
        a_refcount = sys.getrefcount(a)

        b = a[2:-3:3]
        self.assertIsInstance(b, containers.MutableStridedBitArrayView1D)
        self.assertIs(b.owner, a)
        self.assertEqual(len(b), 4)
        self.assertEqual(b.offset, 2)
        self.assertEqual(b.size, (4,))
        self.assertEqual(b.stride, (3,))
        self.assertEqual(b[0], True)
        self.assertEqual(b[1], True)
        self.assertEqual(b[2], False)
        self.assertEqual(b[3], True)
        self.assertEqual(sys.getrefcount(a), a_refcount + 1)

        # Deleting a slice should reduce a's refcount again
        del b
        self.assertEqual(sys.getrefcount(a), a_refcount)

    def test_slice_stride_empty(self):
        a = containers.BitArray.value_init(5)
        a_refcount = sys.getrefcount(a)

        # Because this is out of range, slice.start = slice.stop
        b = a[7:8:2]
        self.assertEqual(len(b), 0)
        self.assertEqual(b.offset, 5)

        # Empty view, original data not referenced at all
        self.assertIs(b.owner, None)
        self.assertEqual(sys.getrefcount(a), a_refcount)

    def test_slice_stride_negative(self):
        a = containers.BitArray.value_init(15)
        a[2] = True
        a[5] = True
        a[7] = False
        a[11] = True

        # Check consistency with slices on lists
        b1 = list(a)[-4:1:-3] # Like [2:-4:3] above, but reverted
        self.assertEqual(len(b1), 4)
        self.assertEqual(b1[0], True)
        self.assertEqual(b1[1], False)
        self.assertEqual(b1[2], True)
        self.assertEqual(b1[3], True)

        b2 = a[-4:1:-3]
        self.assertIsInstance(b2, containers.MutableStridedBitArrayView1D)
        self.assertEqual(len(b2), 4)
        self.assertEqual(b2.offset, 3)
        self.assertEqual(b2.size, (4,))
        self.assertEqual(b2.stride, (-3,))
        self.assertEqual(b2[0], True)
        self.assertEqual(b2[1], False)
        self.assertEqual(b2[2], True)
        self.assertEqual(b2[3], True)

    def test_slice_stride_reverse(self):
        a = containers.BitArray.value_init(5)
        a[0] = True
        a[1] = True
        a[2] = False
        a[3] = True
        a[4] = False

        # slice.stop = -1
        b = a[::-1]
        self.assertEqual(len(b), 5)
        self.assertEqual(b[0], False)
        self.assertEqual(b[1], True)
        self.assertEqual(b[2], False)
        self.assertEqual(b[3], True)
        self.assertEqual(b[4], True)

class BitArrayView(unittest.TestCase):
    def test_init(self):
        a = containers.BitArrayView()
        b = containers.MutableBitArrayView()
        self.assertIs(a.owner, None)
        self.assertIs(b.owner, None)
        self.assertEqual(len(a), 0)
        self.assertEqual(len(b), 0)
        self.assertEqual(a.offset, 0)
        self.assertEqual(b.offset, 0)

    def test_init_from_array(self):
        data = containers.BitArray.value_init(6)
        data[3] = True
        data_refcount = sys.getrefcount(data)

        a = containers.BitArrayView(data)
        b = containers.MutableBitArrayView(data)
        self.assertIs(a.owner, data)
        self.assertIs(b.owner, data)
        self.assertEqual(len(a), 6)
        self.assertEqual(len(b), 6)
        self.assertEqual(a.offset, 0)
        self.assertEqual(b.offset, 0)
        self.assertEqual(a[3], True)
        self.assertEqual(b[3], True)
        self.assertEqual(sys.getrefcount(data), data_refcount + 2)

        self.assertEqual(data[4], False)
        b[4] = True
        self.assertEqual(data[3], True)

        # Deleting the views should reduce array's refcount again
        del a
        del b
        self.assertEqual(sys.getrefcount(data), data_refcount)

    def test_init_from_empty_array(self):
        data = containers.BitArray()
        data_refcount = sys.getrefcount(data)

        # Empty view, original data not referenced at all
        a = containers.BitArrayView(data)
        self.assertIs(a.owner, None)
        self.assertEqual(sys.getrefcount(data), data_refcount)

    def test_init_mutable_from_immutable(self):
        # It's possible to construct only from a BitArray that's always
        # mutable, nothing to test
        pass

    def test_access_invalid(self):
        data = containers.BitArray.value_init(3)
        a = containers.BitArrayView(data)
        b = containers.MutableBitArrayView(data)

        with self.assertRaises(IndexError):
            a[3]
        with self.assertRaises(IndexError):
            b[3] = True

    def test_mutable_access_invalid(self):
        data = containers.BitArray.value_init(3)
        a = containers.BitArrayView(data)

        # This is Python's own exception
        with self.assertRaises(TypeError):
            a[0] = True

    # Mostly the same as BitArray.test_slice() except for the additional
    # referencing logic that skips intermediate views
    def test_slice(self):
        data = containers.BitArray.value_init(10)
        data[3] = True
        data[5] = True
        data[6] = True
        data_refcount = sys.getrefcount(data)

        a = containers.BitArrayView(data)
        a_refcount = sys.getrefcount(a)
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)

        # When slicing, a's refcount should not change but data's refcount
        # should increase
        b = a[3:-2]
        self.assertIsInstance(b, containers.BitArrayView)
        self.assertIs(b.owner, data)
        self.assertEqual(len(b), 5)
        self.assertEqual(b.offset, 3)
        self.assertEqual(b[0], True)
        self.assertEqual(b[1], False)
        self.assertEqual(b[2], True)
        self.assertEqual(b[3], True)
        self.assertEqual(b[4], False)
        self.assertEqual(sys.getrefcount(data), data_refcount + 2)
        self.assertEqual(sys.getrefcount(a), a_refcount)

        # Deleting a slice should reduce data's refcount again, keep a's
        # unchanged
        del b
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)
        self.assertEqual(sys.getrefcount(a), a_refcount)

    # Mostly the same as BitArray.test_slice_empty()
    def test_slice_empty(self):
        data = containers.BitArray.value_init(5)
        data_refcount = sys.getrefcount(data)

        # Because this is out of range, slice.start = slice.stop
        b = containers.BitArrayView(data)[7:8]
        self.assertEqual(len(b), 0)
        self.assertEqual(b.offset, 5)

        # Empty view, original data not referenced at all
        self.assertIs(b.owner, None)
        self.assertEqual(sys.getrefcount(data), data_refcount)

    def test_slice_invalid(self):
        with self.assertRaisesRegex(ValueError, "slice step cannot be zero"):
            containers.BitArrayView()[::0]

    # Mostly the same as BitArray.test_slice_stride() except for the additional
    # referencing logic that skips intermediate views
    def test_slice_stride(self):
        data = containers.BitArray.value_init(15)
        data[2] = True
        data[5] = True
        data[7] = False
        data[11] = True
        data_refcount = sys.getrefcount(data)

        a = containers.BitArrayView(data)
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)

        b = a[2:-3:3]
        self.assertIsInstance(b, containers.StridedBitArrayView1D)
        self.assertIs(b.owner, data)
        self.assertEqual(len(b), 4)
        self.assertEqual(b.offset, 2)
        self.assertEqual(b.size, (4,))
        self.assertEqual(b.stride, (3,))
        self.assertEqual(b[0], True)
        self.assertEqual(b[1], True)
        self.assertEqual(b[2], False)
        self.assertEqual(b[3], True)
        self.assertEqual(sys.getrefcount(data), data_refcount + 2)

        # Deleting the original view should reduce data's refcount again -- b
        # shouldn't depend on the view but on the original array
        del a
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)
        del b
        self.assertEqual(sys.getrefcount(data), data_refcount)

    # Mostly the same as BitArray.test_slice_stride_empty()
    def test_slice_stride_empty(self):
        a = containers.BitArray.value_init(5)
        a_refcount = sys.getrefcount(a)

        # Because this is out of range, slice.start = slice.stop
        b = containers.BitArrayView(a)[7:8:2]
        self.assertEqual(len(b), 0)
        self.assertEqual(b.offset, 5)

        # Empty view, original data not referenced at all
        self.assertIs(b.owner, None)
        self.assertEqual(sys.getrefcount(a), a_refcount)

    # Mostly the same as BitArray.test_slice_stride_negative()
    def test_slice_stride_negative(self):
        data = containers.BitArray.value_init(15)
        data[2] = True
        data[5] = True
        data[7] = False
        data[11] = True

        # Check consistency with slices on lists
        b1 = list(data)[-4:1:-3] # Like [2:-4:3] above, but reverted
        self.assertEqual(len(b1), 4)
        self.assertEqual(b1[0], True)
        self.assertEqual(b1[1], False)
        self.assertEqual(b1[2], True)
        self.assertEqual(b1[3], True)

        b2 = containers.BitArrayView(data)[-4:1:-3]
        self.assertIsInstance(b2, containers.StridedBitArrayView1D)
        self.assertEqual(len(b2), 4)
        self.assertEqual(b2.offset, 3)
        self.assertEqual(b2.size, (4,))
        self.assertEqual(b2.stride, (-3,))
        self.assertEqual(b2[0], True)
        self.assertEqual(b2[1], False)
        self.assertEqual(b2[2], True)
        self.assertEqual(b2[3], True)

    # Mostly the same as BitArray.test_slice_stride_reverse()
    def test_slice_stride_reverse(self):
        data = containers.BitArray.value_init(5)
        data[0] = True
        data[1] = True
        data[2] = False
        data[3] = True
        data[4] = False

        # slice.stop = -1
        b = containers.BitArrayView(data)[::-1]
        self.assertEqual(len(b), 5)
        self.assertEqual(b[0], False)
        self.assertEqual(b[1], True)
        self.assertEqual(b[2], False)
        self.assertEqual(b[3], True)
        self.assertEqual(b[4], True)

class StridedBitArrayView1D(unittest.TestCase):
    def test_init(self):
        a = containers.StridedBitArrayView1D()
        b = containers.MutableStridedBitArrayView1D()
        self.assertIs(a.owner, None)
        self.assertIs(b.owner, None)
        self.assertEqual(len(a), 0)
        self.assertEqual(len(b), 0)
        self.assertEqual(a.offset, 0)
        self.assertEqual(b.offset, 0)
        self.assertEqual(a.size, (0,))
        self.assertEqual(b.size, (0,))
        self.assertEqual(a.stride, (0,))
        self.assertEqual(b.stride, (0,))
        self.assertEqual(a.dimensions, 1)
        self.assertEqual(b.dimensions, 1)

    def test_init_from_array(self):
        data = containers.BitArray.value_init(6)
        data[3] = True
        data_refcount = sys.getrefcount(data)

        a = containers.StridedBitArrayView1D(data)
        b = containers.MutableStridedBitArrayView1D(data)
        self.assertIs(a.owner, data)
        self.assertIs(b.owner, data)
        self.assertEqual(len(a), 6)
        self.assertEqual(len(b), 6)
        self.assertEqual(a.offset, 0)
        self.assertEqual(b.offset, 0)
        self.assertEqual(a.size, (6,))
        self.assertEqual(b.size, (6,))
        self.assertEqual(a.stride, (1,))
        self.assertEqual(b.stride, (1,))
        self.assertEqual(a[3], True)
        self.assertEqual(b[3], True)
        self.assertEqual(sys.getrefcount(data), data_refcount + 2)

        self.assertEqual(data[4], False)
        b[4] = True
        self.assertEqual(data[3], True)

        # Deleting the views should reduce array's refcount again
        del a
        del b
        self.assertEqual(sys.getrefcount(data), data_refcount)

    def test_init_from_empty_array(self):
        data = containers.BitArray()
        data_refcount = sys.getrefcount(data)

        # Empty view, original data not referenced at all
        a = containers.StridedBitArrayView1D(data)
        self.assertIs(a.owner, None)
        self.assertEqual(sys.getrefcount(data), data_refcount)

    def test_init_from_arrayview(self):
        data = containers.BitArray.value_init(6)
        data[3] = True
        data_refcount = sys.getrefcount(data)

        view = containers.BitArrayView(data)
        mutable_view = containers.MutableBitArrayView(data)

        a = containers.StridedBitArrayView1D(view)
        b = containers.StridedBitArrayView1D(mutable_view)
        c = containers.MutableStridedBitArrayView1D(mutable_view)
        self.assertIs(a.owner, data)
        self.assertIs(b.owner, data)
        self.assertIs(c.owner, data)
        self.assertEqual(len(a), 6)
        self.assertEqual(len(b), 6)
        self.assertEqual(len(c), 6)
        self.assertEqual(a.offset, 0)
        self.assertEqual(b.offset, 0)
        self.assertEqual(c.offset, 0)
        self.assertEqual(a.size, (6,))
        self.assertEqual(b.size, (6,))
        self.assertEqual(c.size, (6,))
        self.assertEqual(a.stride, (1,))
        self.assertEqual(b.stride, (1,))
        self.assertEqual(c.stride, (1,))
        self.assertEqual(a[3], True)
        self.assertEqual(b[3], True)
        self.assertEqual(c[3], True)
        self.assertEqual(sys.getrefcount(data), data_refcount + 5)

        self.assertEqual(data[4], False)
        c[4] = True
        self.assertEqual(data[3], True)

        # Deleting the original view should reduce data's refcount again -- a,
        # b, c shouldn't depend on the view but on the original array
        del view
        del mutable_view
        self.assertEqual(sys.getrefcount(data), data_refcount + 3)
        del a
        del b
        del c
        self.assertEqual(sys.getrefcount(data), data_refcount)

    def test_init_from_slice(self):
        self.assertEqual(ord('0') % 2, 0)
        self.assertEqual(ord('1') % 2, 1)

        data = b'11110010'
        data_refcount = sys.getrefcount(data)

        view = containers.StridedArrayView1D(data)
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)

        a = view.slice_bit(0)
        self.assertIs(a.owner, data)
        self.assertEqual(len(a), 8)
        self.assertEqual(a.offset, 0)
        self.assertEqual(a.size, (8,))
        self.assertEqual(a.stride, (8,))
        self.assertEqual(a[0], True)
        self.assertEqual(a[1], True)
        self.assertEqual(a[2], True)
        self.assertEqual(a[3], True)
        self.assertEqual(a[4], False)
        self.assertEqual(a[5], False)
        self.assertEqual(a[6], True)
        self.assertEqual(a[7], False)
        self.assertEqual(sys.getrefcount(data), data_refcount + 2)

        # Deleting the original view should reduce data's refcount again -- a
        # shouldn't depend on the view but on the original array
        del a
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)
        del view
        self.assertEqual(sys.getrefcount(data), data_refcount)

    def test_init_mutable_from_immutable(self):
        # It's possible to construct only via slice_bit() from a
        # StridedArrayView and the mutability is then implicit, nothing to test
        pass

    def test_access_invalid(self):
        data = containers.BitArray.value_init(3)
        a = containers.StridedBitArrayView1D(data)
        b = containers.MutableStridedBitArrayView1D(data)

        with self.assertRaises(IndexError):
            a[3]
        with self.assertRaises(IndexError):
            b[3] = True

    def test_mutable_access_invalid(self):
        data = containers.BitArray.value_init(3)
        a = containers.StridedBitArrayView1D(data)

        # This is Python's own exception
        with self.assertRaises(TypeError):
            a[0] = True

    # Mostly the same as BitArray.test_slice() except for the additional
    # referencing logic that skips intermediate views
    def test_slice(self):
        data = containers.BitArray.value_init(10)
        data[3] = True
        data[5] = True
        data[6] = True
        data_refcount = sys.getrefcount(data)

        a = containers.StridedBitArrayView1D(data)
        a_refcount = sys.getrefcount(a)
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)

        # When slicing, a's refcount should not change but data's refcount
        # should increase
        b = a[3:-2]
        self.assertIsInstance(b, containers.StridedBitArrayView1D)
        self.assertIs(b.owner, data)
        self.assertEqual(len(b), 5)
        self.assertEqual(b.offset, 3)
        self.assertEqual(b.size, (5,))
        self.assertEqual(b.stride, (1,))
        self.assertEqual(b[0], True)
        self.assertEqual(b[1], False)
        self.assertEqual(b[2], True)
        self.assertEqual(b[3], True)
        self.assertEqual(b[4], False)
        self.assertEqual(sys.getrefcount(data), data_refcount + 2)
        self.assertEqual(sys.getrefcount(a), a_refcount)

        # Deleting a slice should reduce data's refcount again, keep a's
        # unchanged
        del b
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)
        self.assertEqual(sys.getrefcount(a), a_refcount)

    # Mostly the same as BitArray.test_slice_empty()
    def test_slice_empty(self):
        data = containers.BitArray.value_init(5)
        data_refcount = sys.getrefcount(data)

        # Because this is out of range, slice.start = slice.stop
        b = containers.StridedBitArrayView1D(data)[7:8]
        self.assertEqual(len(b), 0)
        self.assertEqual(b.offset, 5)
        self.assertEqual(b.size, (0,))
        self.assertEqual(b.stride, (1,))

        # Empty view, original data not referenced at all
        self.assertIs(b.owner, None)
        self.assertEqual(sys.getrefcount(data), data_refcount)

    def test_slice_invalid(self):
        with self.assertRaisesRegex(ValueError, "slice step cannot be zero"):
            containers.StridedBitArrayView1D()[::0]

    # Mostly the same as BitArray.test_slice_stride() except for the additional
    # referencing logic that skips intermediate views
    def test_slice_stride(self):
        data = containers.BitArray.value_init(15)
        data[2] = True
        data[5] = True
        data[7] = False
        data[11] = True
        data_refcount = sys.getrefcount(data)

        a = containers.StridedBitArrayView1D(data)
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)

        b = a[2:-3:3]
        self.assertIsInstance(b, containers.StridedBitArrayView1D)
        self.assertIs(b.owner, data)
        self.assertEqual(len(b), 4)
        self.assertEqual(b.offset, 2)
        self.assertEqual(b.size, (4,))
        self.assertEqual(b.stride, (3,))
        self.assertEqual(b[0], True)
        self.assertEqual(b[1], True)
        self.assertEqual(b[2], False)
        self.assertEqual(b[3], True)
        self.assertEqual(sys.getrefcount(data), data_refcount + 2)

        # Deleting the original view should reduce data's refcount again -- b
        # shouldn't depend on the view but on the original array
        del a
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)
        del b
        self.assertEqual(sys.getrefcount(data), data_refcount)

    # Mostly the same as BitArray.test_slice_stride_negative()
    def test_slice_stride_negative(self):
        data = containers.BitArray.value_init(15)
        data[2] = True
        data[5] = True
        data[7] = False
        data[11] = True

        # Check consistency with slices on lists
        b1 = list(data)[-4:1:-3] # Like [2:-4:3] above, but reverted
        self.assertEqual(len(b1), 4)
        self.assertEqual(b1[0], True)
        self.assertEqual(b1[1], False)
        self.assertEqual(b1[2], True)
        self.assertEqual(b1[3], True)

        b2 = containers.StridedBitArrayView1D(data)[-4:1:-3]
        self.assertIsInstance(b2, containers.StridedBitArrayView1D)
        self.assertEqual(len(b2), 4)
        self.assertEqual(b2.offset, 3)
        self.assertEqual(b2.size, (4,))
        self.assertEqual(b2.stride, (-3,))
        self.assertEqual(b2[0], True)
        self.assertEqual(b2[1], False)
        self.assertEqual(b2[2], True)
        self.assertEqual(b2[3], True)

    def test_ops(self):
        a = b'11110010'
        v = containers.StridedArrayView1D(a).slice_bit(0)

        b = v.flipped(0)
        self.assertEqual(b.offset, 0)
        self.assertEqual(b.size, (8,))
        self.assertEqual(b.stride, (-8,))
        self.assertEqual(b[0], False)
        self.assertEqual(b[1], True)
        self.assertEqual(b[2], False)
        self.assertEqual(b[3], False)
        self.assertEqual(b[4], True)
        self.assertEqual(b[5], True)
        self.assertEqual(b[6], True)
        self.assertEqual(b[7], True)

        c = v[3:4].broadcasted(0, 5)
        self.assertEqual(c.offset, 0)
        self.assertEqual(c.size, (5,))
        self.assertEqual(c.stride, (0,))
        self.assertEqual(c[0], True)
        self.assertEqual(c[1], True)
        self.assertEqual(c[2], True)
        self.assertEqual(c[3], True)
        self.assertEqual(c[4], True)

        d2 = v.expanded(0, (2, 4))
        self.assertIsInstance(d2, containers.StridedBitArrayView2D)
        self.assertEqual(d2.size, (2, 4))
        self.assertEqual(d2.stride, (32, 8))
        self.assertEqual(d2[0][0], True)
        self.assertEqual(d2[0][1], True)
        self.assertEqual(d2[0][2], True)
        self.assertEqual(d2[0][3], True)
        self.assertEqual(d2[1][0], False)
        self.assertEqual(d2[1][1], False)
        self.assertEqual(d2[1][2], True)
        self.assertEqual(d2[1][3], False)

        d3 = v.expanded(0, (2, 2, 2))
        self.assertIsInstance(d3, containers.StridedBitArrayView3D)
        self.assertEqual(d3.size, (2, 2, 2))
        self.assertEqual(d3.stride, (32, 16, 8))
        self.assertEqual(d3[0][0][0], True)
        self.assertEqual(d3[0][0][1], True)
        self.assertEqual(d3[0][1][0], True)
        self.assertEqual(d3[0][1][1], True)
        self.assertEqual(d3[1][0][0], False)
        self.assertEqual(d3[1][0][1], False)
        self.assertEqual(d3[1][1][0], True)
        self.assertEqual(d3[1][1][1], False)

        d4 = v.expanded(0, (2, 1, 2, 2))
        self.assertIsInstance(d4, containers.StridedBitArrayView4D)
        self.assertEqual(d4.size, (2, 1, 2, 2))
        self.assertEqual(d4.stride, (32, 32, 16, 8))
        self.assertEqual(d4[0][0][0][0], True)
        self.assertEqual(d4[0][0][0][1], True)
        self.assertEqual(d4[0][0][1][0], True)
        self.assertEqual(d4[0][0][1][1], True)
        self.assertEqual(d4[1][0][0][0], False)
        self.assertEqual(d4[1][0][0][1], False)
        self.assertEqual(d4[1][0][1][0], True)
        self.assertEqual(d4[1][0][1][1], False)

    def test_ops_invalid(self):
        v = containers.StridedArrayView1D(b'00').slice_bit(0)

        with self.assertRaisesRegex(IndexError, "dimension 1 out of range for a 1D view"):
            containers.StridedBitArrayView1D().flipped(1)
        with self.assertRaisesRegex(IndexError, "dimension 1 out of range for a 1D view"):
            v.broadcasted(1, 3)
        with self.assertRaisesRegex(IndexError, "dimension 1 out of range for a 1D view"):
            v.expanded(1, (1, 2))
        with self.assertRaisesRegex(ValueError, "can't broadcast dimension 0 with 2 elements"):
            v.broadcasted(0, 3)
        with self.assertRaisesRegex(ValueError, "total size 3 doesn't match dimension 0 with 2 elements"):
            v.expanded(0, (1, 3))

class StridedBitArrayView2D(unittest.TestCase):
    def test_init(self):
        a = containers.StridedBitArrayView2D()
        b = containers.MutableStridedBitArrayView2D()
        self.assertIs(a.owner, None)
        self.assertIs(b.owner, None)
        self.assertEqual(len(a), 0)
        self.assertEqual(len(b), 0)
        self.assertEqual(a.offset, 0)
        self.assertEqual(b.offset, 0)
        self.assertEqual(a.size, (0, 0))
        self.assertEqual(b.size, (0, 0))
        self.assertEqual(a.stride, (0, 0))
        self.assertEqual(b.stride, (0, 0))
        self.assertEqual(a.dimensions, 2)
        self.assertEqual(b.dimensions, 2)

    def test_init_from_slice(self):
        self.assertEqual(ord('0') % 2, 0)
        self.assertEqual(ord('1') % 2, 1)
        data = memoryview(bytearray(
            b'1111'
            b'0010')).cast('b', shape=[2, 4])
        data_refcount = sys.getrefcount(data)

        view = containers.MutableStridedArrayView2D(data)
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)

        a = view.slice_bit(0)
        self.assertIsInstance(a, containers.MutableStridedBitArrayView2D)
        self.assertIs(a.owner, data)
        self.assertEqual(len(a), 2)
        self.assertEqual(a.offset, 0)
        self.assertEqual(a.size, (2, 4))
        self.assertEqual(a.stride, (32, 8))
        self.assertEqual(a[0, 0], True)
        self.assertEqual(a[0, 1], True)
        self.assertEqual(a[0, 2], True)
        self.assertEqual(a[0, 3], True)
        self.assertEqual(a[1, 0], False)
        self.assertEqual(a[1, 1], False)
        self.assertEqual(a[1, 2], True)
        self.assertEqual(a[1, 3], False)
        self.assertEqual(sys.getrefcount(data), data_refcount + 2)

        a[0, 2] = False
        self.assertEqual(bytes(data), b'11010010') # haha!

        # Deleting the original view should reduce data's refcount again -- a
        # shouldn't depend on the view but on the original array
        del a
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)
        del view
        self.assertEqual(sys.getrefcount(data), data_refcount)

    def test_init_mutable_from_immutable(self):
        # It's possible to construct only via slice_bit() from a
        # StridedArrayView and the mutability is then implicit, nothing to test
        pass

    def test_slice(self):
        data = memoryview(
            b'10'
            b'10'
            b'11'
            b'10').cast('b', shape=[4, 2])
        data_refcount = sys.getrefcount(data)

        a = containers.StridedArrayView2D(data).slice_bit(0)
        a_refcount = sys.getrefcount(a)
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)

        # When slicing, a's refcount should not change but data's refcount
        # should increase
        b = a[1:-1]
        self.assertIsInstance(b, containers.StridedBitArrayView2D)
        self.assertIs(a.owner, data)
        self.assertEqual(len(b), 2)
        self.assertEqual(b.offset, 0)
        self.assertEqual(b.size, (2, 2))
        self.assertEqual(b.stride, (16, 8))
        self.assertEqual(b[0, 0], True)
        self.assertEqual(b[0, 1], False)
        self.assertEqual(b[1, 0], True)
        self.assertEqual(b[1, 1], True)
        self.assertEqual(sys.getrefcount(data), data_refcount + 2)
        self.assertEqual(sys.getrefcount(a), a_refcount)

        # Deleting a slice should reduce data's refcount again, keep a's
        # unchanged
        del b
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)
        self.assertEqual(sys.getrefcount(a), a_refcount)

    def test_slice_empty(self):
        data = memoryview(
            b'10'
            b'10'
            b'11'
            b'10').cast('b', shape=[4, 2])
        data_refcount = sys.getrefcount(data)

        b = containers.StridedArrayView2D(data).slice_bit(0)[1:1]

        # Empty view, original data not referenced at all
        self.assertIs(b.owner, None)
        self.assertEqual(sys.getrefcount(data), data_refcount)

    def test_slice_multidimensional(self):
        data = memoryview(
            b'111000'
            b'111000'
            b'111100'
            b'111000').cast('b', shape=[4, 6])
        data_refcount = sys.getrefcount(data)

        view = containers.StridedArrayView2D(data)
        self.assertEqual(sys.getrefcount(data), data_refcount + 1)

        a = view.slice_bit(0)
        a_refcount = sys.getrefcount(a)
        self.assertEqual(sys.getrefcount(data), data_refcount + 2)

        # When slicing, a's refcount should not change but data's refcount
        # should increase
        b = a[1:3,2:4]
        self.assertIsInstance(b, containers.StridedBitArrayView2D)
        self.assertIs(a.owner, data)
        self.assertEqual(len(b), 2)
        self.assertEqual(b.offset, 0)
        self.assertEqual(b.size, (2, 2))
        self.assertEqual(b.stride, (48, 8))
        self.assertEqual(b[0, 0], True)
        self.assertEqual(b[0, 1], False)
        self.assertEqual(b[1, 0], True)
        self.assertEqual(b[1, 1], True)
        self.assertEqual(sys.getrefcount(data), data_refcount + 3)
        self.assertEqual(sys.getrefcount(a), a_refcount)

        # Deleting a slice should reduce data's refcount again, keep a's
        # unchanged
        del b
        self.assertEqual(sys.getrefcount(data), data_refcount + 2)
        self.assertEqual(sys.getrefcount(a), a_refcount)

    def test_slice_multidimensional_empty(self):
        data = memoryview(
            b'10'
            b'10'
            b'11'
            b'10').cast('b', shape=[4, 2])
        data_refcount = sys.getrefcount(data)

        b = containers.StridedArrayView2D(data).slice_bit(0)[2:2,1:1]

        # Empty view, original data not referenced at all
        self.assertIs(b.owner, None)
        self.assertEqual(sys.getrefcount(data), data_refcount)

    def test_slice_invalid(self):
        with self.assertRaisesRegex(ValueError, "slice step cannot be zero"):
            containers.StridedBitArrayView2D()[-5:3:0]

    def test_slice_stride(self):
        data = memoryview(
            b'10'
            b'00'
            b'10'
            b'00'
            b'11'
            b'00'
            b'10').cast('b', shape=[7, 2])
        a = containers.StridedArrayView2D(data).slice_bit(0)

        b = a[2:5:2]
        self.assertEqual(b.offset, 0)
        self.assertEqual(b.size, (2, 2))
        self.assertEqual(b.stride, (32, 8))
        self.assertEqual(b[0, 0], True)
        self.assertEqual(b[0, 1], False)
        self.assertEqual(b[1, 0], True)
        self.assertEqual(b[1, 1], True)

    def test_slice_stride_negative(self):
        data = memoryview(
            b'10'
            b'00'
            b'10'
            b'00'
            b'11'
            b'00'
            b'10').cast('b', shape=[7, 2])
        a = containers.StridedArrayView2D(data).slice_bit(0)

        b = a[4:1:-2] # like [2:5:2] above, but reverted
        self.assertEqual(b.offset, 0)
        self.assertEqual(b.size, (2, 2))
        self.assertEqual(b.stride, (-32, 8))
        self.assertEqual(b[0, 0], True)
        self.assertEqual(b[0, 1], True)
        self.assertEqual(b[1, 0], True)
        self.assertEqual(b[1, 1], False)

    def test_slice_stride_negative_multidimensional(self):
        data = memoryview(
            b'110000'
            b'100000'
            b'110000'
            b'100000'
            b'110001'
            b'100000'
            b'110000').cast('b', shape=[7, 6])
        a = containers.StridedArrayView2D(data).slice_bit(0)

        # first dimension is like [2:5:2] above, but reverted,
        # second like [1:6:4] but reverted
        b = a[4:1:-2, 5:0:-4]
        self.assertEqual(b.offset, 0)
        self.assertEqual(b.size, (2, 2))
        self.assertEqual(b.stride, (-96, -32))
        self.assertEqual(b[0, 0], True)
        self.assertEqual(b[0, 1], True)
        self.assertEqual(b[1, 0], False)
        self.assertEqual(b[1, 1], True)

    def test_ops(self):
        data = memoryview(
            b'1111'
            b'0010').cast('b', shape=[2, 4])
        a = containers.StridedArrayView2D(data).slice_bit(0)

        b = a.transposed(0, 1)
        self.assertEqual(b.offset, 0)
        self.assertEqual(b.size, (4, 2))
        self.assertEqual(b.stride, (8, 32))
        self.assertEqual(b[0, 0], True)
        self.assertEqual(b[1, 0], True)
        self.assertEqual(b[2, 0], True)
        self.assertEqual(b[3, 0], True)
        self.assertEqual(b[0, 1], False)
        self.assertEqual(b[1, 1], False)
        self.assertEqual(b[2, 1], True)
        self.assertEqual(b[3, 1], False)

        c = a.flipped(1)
        self.assertEqual(c.offset, 0)
        self.assertEqual(c.size, (2, 4))
        self.assertEqual(c.stride, (32, -8))
        self.assertEqual(c[0, 0], True)
        self.assertEqual(c[0, 1], True)
        self.assertEqual(c[0, 2], True)
        self.assertEqual(c[0, 3], True)
        self.assertEqual(c[1, 0], False)
        self.assertEqual(c[1, 1], True)
        self.assertEqual(c[1, 2], False)
        self.assertEqual(c[1, 3], False)

        d = a.transposed(0, 1)[0:1].broadcasted(0, 4)
        self.assertEqual(d.offset, 0)
        self.assertEqual(d.size, (4, 2))
        self.assertEqual(d.stride, (0, 32))
        self.assertEqual(d[0, 0], True)
        self.assertEqual(d[1, 0], True)
        self.assertEqual(d[2, 0], True)
        self.assertEqual(d[3, 0], True)
        self.assertEqual(d[0, 1], False)
        self.assertEqual(d[1, 1], False)
        self.assertEqual(d[2, 1], False)
        self.assertEqual(d[3, 1], False)

        d3 = a.expanded(1, (2, 2))
        self.assertIsInstance(d3, containers.StridedBitArrayView3D)
        self.assertEqual(d3.size, (2, 2, 2))
        self.assertEqual(d3.stride, (32, 16, 8))
        self.assertEqual(d3[0][0][0], True)
        self.assertEqual(d3[0][0][1], True)
        self.assertEqual(d3[0][1][0], True)
        self.assertEqual(d3[0][1][1], True)
        self.assertEqual(d3[1][0][0], False)
        self.assertEqual(d3[1][0][1], False)
        self.assertEqual(d3[1][1][0], True)
        self.assertEqual(d3[1][1][1], False)

        d4 = a.expanded(1, (1, 2, 2))
        self.assertIsInstance(d4, containers.StridedBitArrayView4D)
        self.assertEqual(d4.size, (2, 1, 2, 2))
        self.assertEqual(d4.stride, (32, 32, 16, 8))
        self.assertEqual(d4[0][0][0][0], True)
        self.assertEqual(d4[0][0][0][1], True)
        self.assertEqual(d4[0][0][1][0], True)
        self.assertEqual(d4[0][0][1][1], True)
        self.assertEqual(d4[1][0][0][0], False)
        self.assertEqual(d4[1][0][0][1], False)
        self.assertEqual(d4[1][0][1][0], True)
        self.assertEqual(d4[1][0][1][1], False)

    def test_ops_invalid(self):
        v = containers.StridedArrayView2D(memoryview(b'00').cast('b', shape=[1, 2])).slice_bit(0)

        with self.assertRaisesRegex(IndexError, "dimension 2 out of range for a 2D view"):
            containers.StridedBitArrayView2D().flipped(2)
        with self.assertRaisesRegex(IndexError, "dimensions 2, 1 can't be transposed in a 2D view"):
            containers.StridedBitArrayView2D().transposed(2, 1)
        with self.assertRaisesRegex(IndexError, "dimension 2 out of range for a 2D view"):
            v.broadcasted(2, 3)
        with self.assertRaisesRegex(IndexError, "dimension 2 out of range for a 2D view"):
            v.expanded(2, (1, 2))
        with self.assertRaisesRegex(ValueError, "can't broadcast dimension 1 with 2 elements"):
            v.broadcasted(1, 3)
        with self.assertRaisesRegex(ValueError, "total size 2 doesn't match dimension 0 with 1 elements"):
            v.expanded(0, (1, 2))
        with self.assertRaisesRegex(ValueError, "total size 4 doesn't match dimension 1 with 2 elements"):
            v.expanded(1, (2, 1, 2))

# Multi-dimensional behavior is tested extensively for StridedBitArrayView2D,
# this checks just what differs, like constructors and fancy operations
class StridedBitArrayView3D(unittest.TestCase):
    def test_init_from_slice(self):
        data = containers.StridedArrayView3D(memoryview(
            b'1111'
            b'0010').cast('b', shape=[2, 1, 4]))

        a = data.slice_bit(0)
        self.assertIsInstance(a, containers.StridedBitArrayView3D)
        self.assertEqual(a.offset, 0)
        self.assertEqual(a.size, (2, 1, 4))
        self.assertEqual(a.stride, (32, 32, 8))
        self.assertEqual(a[0, 0, 0], True)
        self.assertEqual(a[0, 0, 1], True)
        self.assertEqual(a[0, 0, 2], True)
        self.assertEqual(a[0, 0, 3], True)
        self.assertEqual(a[1, 0, 0], False)
        self.assertEqual(a[1, 0, 1], False)
        self.assertEqual(a[1, 0, 2], True)
        self.assertEqual(a[1, 0, 3], False)

    def test_ops(self):
        data = memoryview(
            b'1111'
            b'0010').cast('b', shape=[2, 1, 4])
        a = containers.StridedArrayView3D(data).slice_bit(0)

        b = a.transposed(2, 1)
        self.assertEqual(b.offset, 0)
        self.assertEqual(b.size, (2, 4, 1))
        self.assertEqual(b.stride, (32, 8, 32))
        self.assertEqual(b[0, 0, 0], True)
        self.assertEqual(b[0, 1, 0], True)
        self.assertEqual(b[0, 2, 0], True)
        self.assertEqual(b[0, 3, 0], True)
        self.assertEqual(b[1, 0, 0], False)
        self.assertEqual(b[1, 1, 0], False)
        self.assertEqual(b[1, 2, 0], True)
        self.assertEqual(b[1, 3, 0], False)

        c = a.flipped(2)
        self.assertEqual(c.offset, 0)
        self.assertEqual(c.size, (2, 1, 4))
        self.assertEqual(c.stride, (32, 32, -8))
        self.assertEqual(c[0, 0, 0], True)
        self.assertEqual(c[0, 0, 1], True)
        self.assertEqual(c[0, 0, 2], True)
        self.assertEqual(c[0, 0, 3], True)
        self.assertEqual(c[1, 0, 0], False)
        self.assertEqual(c[1, 0, 1], True)
        self.assertEqual(c[1, 0, 2], False)
        self.assertEqual(c[1, 0, 3], False)

        d = a.transposed(0, 2)[0:1].broadcasted(0, 4)
        self.assertEqual(d.offset, 0)
        self.assertEqual(d.size, (4, 1, 2))
        self.assertEqual(d.stride, (0, 32, 32))
        self.assertEqual(d[0, 0, 0], True)
        self.assertEqual(d[1, 0, 0], True)
        self.assertEqual(d[2, 0, 0], True)
        self.assertEqual(d[3, 0, 0], True)
        self.assertEqual(d[0, 0, 1], False)
        self.assertEqual(d[1, 0, 1], False)
        self.assertEqual(d[2, 0, 1], False)
        self.assertEqual(d[3, 0, 1], False)

        e4 = a.expanded(2, (2, 2))
        self.assertIsInstance(e4, containers.StridedBitArrayView4D)
        self.assertEqual(e4.size, (2, 1, 2, 2))
        self.assertEqual(e4.stride, (32, 32, 16, 8))
        self.assertEqual(e4[0][0][0][0], True)
        self.assertEqual(e4[0][0][0][1], True)
        self.assertEqual(e4[0][0][1][0], True)
        self.assertEqual(e4[0][0][1][1], True)
        self.assertEqual(e4[1][0][0][0], False)
        self.assertEqual(e4[1][0][0][1], False)
        self.assertEqual(e4[1][0][1][0], True)
        self.assertEqual(e4[1][0][1][1], False)

    def test_ops_invalid(self):
        v = containers.StridedArrayView3D(memoryview(b'00').cast('b', shape=[1, 1, 2])).slice_bit(0)

        with self.assertRaisesRegex(IndexError, "dimension 3 out of range for a 3D view"):
            containers.StridedBitArrayView3D().flipped(3)
        with self.assertRaisesRegex(IndexError, "dimensions 2, 3 can't be transposed in a 3D view"):
            containers.StridedBitArrayView3D().transposed(2, 3)
        with self.assertRaisesRegex(IndexError, "dimension 3 out of range for a 3D view"):
            v.broadcasted(3, 3)
        with self.assertRaisesRegex(IndexError, "dimension 3 out of range for a 3D view"):
            v.expanded(3, (1, 2))
        with self.assertRaisesRegex(ValueError, "can't broadcast dimension 2 with 2 elements"):
            v.broadcasted(2, 3)
        with self.assertRaisesRegex(ValueError, "total size 3 doesn't match dimension 0 with 1 elements"):
            v.expanded(0, (1, 3))
        with self.assertRaisesRegex(ValueError, "total size 6 doesn't match dimension 2 with 2 elements"):
            v.expanded(2, (2, 3))

# This is just a dumb copy of the above with one dimension inserted at the
# second place.
class StridedBitArrayView4D(unittest.TestCase):
    def test_init_from_slice(self):
        data = containers.MutableStridedArrayView4D(memoryview(bytearray(
            b'1111'
            b'0010')).cast('b', shape=[1, 2, 1, 4]))

        a = data.slice_bit(0)
        self.assertIsInstance(a, containers.MutableStridedBitArrayView4D)
        self.assertEqual(a.offset, 0)
        self.assertEqual(a.size, (1, 2, 1, 4))
        self.assertEqual(a.stride, (64, 32, 32, 8))
        self.assertEqual(a[0, 0, 0, 0], True)
        self.assertEqual(a[0, 0, 0, 1], True)
        self.assertEqual(a[0, 0, 0, 2], True)
        self.assertEqual(a[0, 0, 0, 3], True)
        self.assertEqual(a[0, 1, 0, 0], False)
        self.assertEqual(a[0, 1, 0, 1], False)
        self.assertEqual(a[0, 1, 0, 2], True)
        self.assertEqual(a[0, 1, 0, 3], False)

        a[0, 0, 0, 2] = False
        self.assertEqual(bytes(data), b'11010010') # haha!

    def test_ops(self):
        data = memoryview(
            b'1111'
            b'0010').cast('b', shape=[1, 2, 1, 4])
        a = containers.StridedArrayView4D(data).slice_bit(0)

        b = a.transposed(3, 1)
        self.assertEqual(b.offset, 0)
        self.assertEqual(b.size, (1, 4, 1, 2))
        self.assertEqual(b.stride, (64, 8, 32, 32))
        self.assertEqual(b[0, 0, 0, 0], True)
        self.assertEqual(b[0, 1, 0, 0], True)
        self.assertEqual(b[0, 2, 0, 0], True)
        self.assertEqual(b[0, 3, 0, 0], True)
        self.assertEqual(b[0, 0, 0, 1], False)
        self.assertEqual(b[0, 1, 0, 1], False)
        self.assertEqual(b[0, 2, 0, 1], True)
        self.assertEqual(b[0, 3, 0, 1], False)

        c = a.flipped(3)
        self.assertEqual(c.offset, 0)
        self.assertEqual(c.size, (1, 2, 1, 4))
        self.assertEqual(c.stride, (64, 32, 32, -8))
        self.assertEqual(c[0, 0, 0, 0], True)
        self.assertEqual(c[0, 0, 0, 1], True)
        self.assertEqual(c[0, 0, 0, 2], True)
        self.assertEqual(c[0, 0, 0, 3], True)
        self.assertEqual(c[0, 1, 0, 0], False)
        self.assertEqual(c[0, 1, 0, 1], True)
        self.assertEqual(c[0, 1, 0, 2], False)
        self.assertEqual(c[0, 1, 0, 3], False)

        d = a.transposed(0, 3)[0:1].broadcasted(0, 4)
        self.assertEqual(d.offset, 0)
        self.assertEqual(d.size, (4, 2, 1, 1))
        self.assertEqual(d.stride, (0, 32, 32, 64))
        self.assertEqual(d[0, 0, 0, 0], True)
        self.assertEqual(d[1, 0, 0, 0], True)
        self.assertEqual(d[2, 0, 0, 0], True)
        self.assertEqual(d[3, 0, 0, 0], True)
        self.assertEqual(d[0, 1, 0, 0], False)
        self.assertEqual(d[1, 1, 0, 0], False)
        self.assertEqual(d[2, 1, 0, 0], False)
        self.assertEqual(d[3, 1, 0, 0], False)

    def test_ops_invalid(self):
        v = containers.StridedArrayView4D(memoryview(b'00').cast('b', shape=[1, 1, 1, 2])).slice_bit(0)

        with self.assertRaisesRegex(IndexError, "dimension 4 out of range for a 4D view"):
            containers.StridedBitArrayView4D().flipped(4)
        with self.assertRaisesRegex(IndexError, "dimensions 4, 3 can't be transposed in a 4D view"):
            containers.StridedBitArrayView4D().transposed(4, 3)
        with self.assertRaisesRegex(IndexError, "dimension 4 out of range for a 4D view"):
            v.broadcasted(4, 3)
        with self.assertRaisesRegex(ValueError, "can't broadcast dimension 3 with 2 elements"):
            v.broadcasted(3, 3)

class Optional(unittest.TestCase):
    def test_simple(self):
        self.assertIsNone(test_optional.simple_type(False))
        self.assertEqual(test_optional.simple_type(True), 5)
        self.assertEqual(test_optional.acquire_simple_type(None), -1)
        self.assertEqual(test_optional.acquire_simple_type(15), 15)

    def test_nested(self):
        self.assertIsNone(test_optional.nested_type(False))
        self.assertEqual(test_optional.nested_type(True).a, 15)
        self.assertEqual(test_optional.acquire_nested_type(None), -1)
        self.assertEqual(test_optional.acquire_nested_type(test_optional.Foo(25)), 25)
