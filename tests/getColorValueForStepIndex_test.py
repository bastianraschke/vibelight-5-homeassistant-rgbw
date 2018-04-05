#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import unittest

TRANSITION_ANIMATION_STEPCOUNT = 256

def mapValue(x, in_min, in_max, out_min, out_max):
    print("({} - {}) * ({} - {}) / ({} - {}) + {}".format(x ,in_min, out_max, out_min, in_max, in_min, out_min))
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min

def getColorValueForStepIndex(stepIndex, startColorValue, endColorValue):
    return mapValue(stepIndex, 0, 255, startColorValue, endColorValue)


class GetColorValueForStepIndexTest(unittest.TestCase):

    def test_indices(self):
        # self.assertEqual(0, getColorValueForStepIndex(stepIndex = 0, startColorValue = 0, endColorValue = 255))
        self.assertEqual(128, getColorValueForStepIndex(stepIndex = 128, startColorValue = 0, endColorValue = 255))
        # self.assertEqual(255, getColorValueForStepIndex(stepIndex = 255, startColorValue = 0, endColorValue = 255))

        # self.assertEqual(255, getColorValueForStepIndex(stepIndex = 0, startColorValue = 255, endColorValue = 0))
        self.assertEqual(128, getColorValueForStepIndex(stepIndex = 128, startColorValue = 255, endColorValue = 0))
        # self.assertEqual(0, getColorValueForStepIndex(stepIndex = 255, startColorValue = 255, endColorValue = 0))

        # self.assertEqual(0, getColorValueForStepIndex(stepIndex = 0, startColorValue = 0, endColorValue = 0))
        # self.assertEqual(0, getColorValueForStepIndex(stepIndex = 128, startColorValue = 0, endColorValue = 0))
        # self.assertEqual(0, getColorValueForStepIndex(stepIndex = 255, startColorValue = 0, endColorValue = 0))

        # self.assertEqual(128, getColorValueForStepIndex(stepIndex = 0, startColorValue = 128, endColorValue = 128))
        # self.assertEqual(128, getColorValueForStepIndex(stepIndex = 128, startColorValue = 128, endColorValue = 128))
        # self.assertEqual(128, getColorValueForStepIndex(stepIndex = 255, startColorValue = 128, endColorValue = 128))

        # self.assertEqual(255, getColorValueForStepIndex(stepIndex = 0, startColorValue = 255, endColorValue = 255))
        # self.assertEqual(255, getColorValueForStepIndex(stepIndex = 128, startColorValue = 255, endColorValue = 255))
        # self.assertEqual(255, getColorValueForStepIndex(stepIndex = 255, startColorValue = 255, endColorValue = 255))

if __name__ == '__main__':
    unittest.main()
