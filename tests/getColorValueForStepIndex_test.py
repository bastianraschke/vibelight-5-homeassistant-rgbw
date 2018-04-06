#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import unittest

def getColorValueForStepIndex(i, s, e):
    upperLimit = max(s, e)

    if (e > s):
        return ((e - s) / upperLimit) * i
    elif (s > e):
        return ((s - e) / upperLimit) * (upperLimit - i)
    else:
        return s

class GetColorValueForStepIndexTest(unittest.TestCase):

    def test_indices_lower_range(self):

        ## Normal 0..9 range
        self.assertEqual(0, getColorValueForStepIndex(i = 0, s = 0, e = 9))
        self.assertEqual(5, getColorValueForStepIndex(i = 5, s = 0, e = 9))
        self.assertEqual(9, getColorValueForStepIndex(i = 9, s = 0, e = 9))

        ## Inverted 9..0 range
        self.assertEqual(9, getColorValueForStepIndex(i = 0, s = 9, e = 0))
        self.assertEqual(4, getColorValueForStepIndex(i = 5, s = 9, e = 0))
        self.assertEqual(0, getColorValueForStepIndex(i = 9, s = 9, e = 0))

        ## Index out of range 0..0
        self.assertEqual(0, getColorValueForStepIndex(i = 0, s = 0, e = 0))
        self.assertEqual(0, getColorValueForStepIndex(i = 5, s = 0, e = 0))
        self.assertEqual(0, getColorValueForStepIndex(i = 9, s = 0, e = 0))

        ## Index out of range 9..9
        self.assertEqual(9, getColorValueForStepIndex(i = 0, s = 9, e = 9))
        self.assertEqual(9, getColorValueForStepIndex(i = 5, s = 9, e = 9))
        self.assertEqual(9, getColorValueForStepIndex(i = 9, s = 9, e = 9))

    def test_indices_realworld_ranges(self):

        ## Normal 0..255 range
        self.assertEqual(0, getColorValueForStepIndex(i = 0, s = 0, e = 255))
        self.assertEqual(128, getColorValueForStepIndex(i = 128, s = 0, e = 255))
        self.assertEqual(255, getColorValueForStepIndex(i = 255, s = 0, e = 255))

        ## Inverted 255..0 range
        self.assertEqual(255, getColorValueForStepIndex(i = 0, s = 255, e = 0))
        self.assertEqual(127, getColorValueForStepIndex(i = 128, s = 255, e = 0))
        self.assertEqual(0, getColorValueForStepIndex(i = 255, s = 255, e = 0))

        ## Index out of range 0..0
        self.assertEqual(0, getColorValueForStepIndex(i = 0, s = 0, e = 0))
        self.assertEqual(0, getColorValueForStepIndex(i = 128, s = 0, e = 0))
        self.assertEqual(0, getColorValueForStepIndex(i = 255, s = 0, e = 0))

        ## Index out of range 128..128
        self.assertEqual(128, getColorValueForStepIndex(i = 0, s = 128, e = 128))
        self.assertEqual(128, getColorValueForStepIndex(i = 128, s = 128, e = 128))
        self.assertEqual(128, getColorValueForStepIndex(i = 255, s = 128, e = 128))

        ## Index out of range 255..255
        self.assertEqual(255, getColorValueForStepIndex(i = 0, s = 255, e = 255))
        self.assertEqual(255, getColorValueForStepIndex(i = 128, s = 255, e = 255))
        self.assertEqual(255, getColorValueForStepIndex(i = 255, s = 255, e = 255))

if __name__ == '__main__':
    unittest.main()
