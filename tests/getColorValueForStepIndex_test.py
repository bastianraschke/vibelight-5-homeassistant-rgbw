#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import unittest

def constrain(x, a, b):
    if (x < a):
        return a
    elif (b < x):
        return b
    else:
        return x

# def mapValue(x, in_min, in_max, out_min, out_max):
#     print("({} - {}) * ({} - {}) / ({} - {}) + {}".format(x ,in_min, out_max, out_min, in_max, in_min, out_min))
#     return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

# def getColorValueForStepIndex(i, s, e):
#     lowerLimit = 0
#     upperLimit = 255
#     contrainedIndex = constrain(i, lowerLimit, upperLimit)

#     ## Prevent division-by-zero
#     if (s != e):
#         return mapValue(contrainedIndex, lowerLimit, upperLimit, s, e)
#     else:
#         return lowerLimit

def getColorValueForStepIndex(i, s, e):
    lowerLimit = 0
    upperLimit = 255
    contrainedIndex = constrain(i, lowerLimit, upperLimit)

    if (e > s):
        return ((e - s) / upperLimit) * contrainedIndex
    elif (s > e):
        return ((s - e) / upperLimit) * (upperLimit - contrainedIndex)
    else:
        return s

class GetColorValueForStepIndexTest(unittest.TestCase):

    def test_indices_realworld_ranges(self):

        ## Normal 0..255 range
        self.assertEqual(0, getColorValueForStepIndex(i = 0, s = 0, e = 255))
        self.assertEqual(128, getColorValueForStepIndex(i = 128, s = 0, e = 255))
        self.assertEqual(255, getColorValueForStepIndex(i = 255, s = 0, e = 255))

        ## Inverted 255..0 range
        self.assertEqual(255, getColorValueForStepIndex(i = 0, s = 255, e = 0))
        self.assertEqual(127, getColorValueForStepIndex(i = 128, s = 255, e = 0))
        self.assertEqual(0, getColorValueForStepIndex(i = 255, s = 255, e = 0))

    def test_index_out_of_range(self):

        ## Index out of range should return a value never greater than [start..end] limits
        self.assertEqual(224, getColorValueForStepIndex(i = 255, s = 0, e = 224))
        self.assertEqual(20, getColorValueForStepIndex(i = 255, s = 0, e = 20))
        self.assertEqual(130, getColorValueForStepIndex(i = 255, s = 0, e = 130))

    def test_linear_ranging(self):

        ## The values must scale linear according to 0..255
        self.assertEqual(0, getColorValueForStepIndex(i = 0, s = 0, e = 20))
        self.assertAlmostEqual(10.039, getColorValueForStepIndex(i = 128, s = 0, e = 20), 2)
        self.assertEqual(20, getColorValueForStepIndex(i = 255, s = 0, e = 20))

if __name__ == '__main__':
    unittest.main()
