#!/usr/bin/env python3
"""Simple test script for validating ROI calculation algorithm."""
__author__ = "Daniel Casner <daniel@anki.com>"

SPAD_COLS = 16  ##< What's in the sensor
SPAD_ROWS = 16  ##< What's in the sensor
SPAD_MIN_ROI = 4  ##< Minimum ROI size in spads
MAX_ROWS = (SPAD_ROWS / SPAD_MIN_ROI)
MAX_COLS = (SPAD_COLS / SPAD_MIN_ROI)


def calc_rois(rows, cols):
    "Tabulate ROIs for a given number of ros and columns filling the view"

    col_step = SPAD_COLS // cols
    row_step = SPAD_ROWS // rows

    rois = []

    for row in range(rows):
        for col in range(cols):
            tlx = col * col_step
            tly = (row + 1) * row_step - 1
            brx = (col + 1) * col_step - 1
            bry = row * row_step
            rois.append((tlx, tly, brx, bry))
    return rois


def main():
    "Main method running a script"
    parser = argparse.ArgumentParser()
    parser.add_argument("rows", type=int, help="Number of rows of ROIs")
    parser.add_argument("cols", type=int, help="Number of columns of ROIs")
    args = parser.parse_args()
    print(calc_rois(args.rows, args.cols))


if __name__ == '__main__':
    import argparse
    main()
