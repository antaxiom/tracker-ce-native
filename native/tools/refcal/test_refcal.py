import unittest

import numpy as np

import refcal


class RefcalTests(unittest.TestCase):
    def test_calibrate_stretches_per_channel(self):
        rgb = np.zeros((20, 20, 3), dtype=np.uint8)
        rgb[:10] = (10, 20, 30)
        rgb[10:] = (200, 210, 220)
        out, info = refcal.calibrate(rgb)
        self.assertLessEqual(int(out.min()), 2)
        self.assertGreaterEqual(int(out.max()), 253)
        self.assertEqual(len(info["black"]), 3)
        self.assertEqual(len(info["white"]), 3)

    def test_detect_lcd_finds_header_line(self):
        tile = np.full((120, 300, 3), 5, dtype=np.uint8)
        tile[30, 20:281] = 220
        # Text-like bright rows above the separator must not win.
        tile[10:16, 40:140] = 200
        found = refcal.detect_lcd(tile)
        self.assertEqual(found["header_y"], 30)
        self.assertLessEqual(abs(found["left"] - 20), 1)
        self.assertLessEqual(abs(found["right"] - 281), 1)
        self.assertGreater(found["scale"], 0.3)

    def test_groups_merge_by_gap(self):
        mask = np.array([False, True, True, False, True, False, True, True])
        self.assertEqual(refcal._groups(mask, max_gap=0), [(1, 1), (2, 2), (4, 4), (6, 6), (7, 7)])
        self.assertEqual(refcal._groups(mask, max_gap=1), [(1, 2), (4, 4), (6, 7)])


if __name__ == "__main__":
    unittest.main()
