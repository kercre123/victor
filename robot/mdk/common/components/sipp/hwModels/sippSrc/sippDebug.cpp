// -----------------------------------------------------------------------------
// Copyright (C) 2012 Movidius Ltd. All rights reserved
//
// Company          : Movidius
// Author           : Rick Richmond (richard.richmond@movidius.com)
// Description      : SIPP Accelerator DEBUG functions
//
// This is the place to put any common debug code
//
// -----------------------------------------------------------------------------
#include "sippDebug.h"

#include <iostream>
#include <iomanip>

using namespace std;

void showKernel(uint8_t *kernel, int kw, int kh, bool fliph, bool flipv) {
    streamsize w = std::cout.width();
    char f = cout.fill();

    cout << setbase(16);
    cout << right;
    cout << setw(2);
    cout << setfill('0');
	cout << noshowbase;

    for (int i = 0; i < kh; i++) {
        cout << "    ";
        for (int j = 0; j < kw; j++) {
            int v = i;
            int h = j;

            // Print out may be flipped in either direction
            if (flipv == true)
                v = kh - (i + 1);

            if (fliph == true)
                h = kw - (j + 1);

            uint8_t d = kernel[v*kw + h];
            if (d != 0)
                cout << "0x" << setw(2) << (unsigned int)d;
            else
                cout << "0x00";
            cout << "  ";
        }
        cout << endl;
    }
    cout << endl;

    cout << setbase(10);
    cout.unsetf(ios::right);
    cout << setw(w);
    cout << setfill(f);
}

void showKernel(uint16_t *kernel, int kw, int kh, bool fliph, bool flipv) {
    streamsize w = std::cout.width();
    char f = cout.fill();

    cout << setbase(16);
    cout << right;
    cout << setw(4);
    cout << setfill('0');
	cout << noshowbase;

    for (int i = 0; i < kh; i++) {
        cout << "    ";
        for (int j = 0; j < kw; j++) {
            int v = i;
            int h = j;

            // Print out may be flipped in either direction
            if (flipv == true)
                v = kh - (i + 1);

            if (fliph == true)
                h = kw - (j + 1);

            uint16_t d = kernel[v*kw + h];
            if (d != 0)
                cout << "0x" << setw(4) << (unsigned int)d;
            else
                cout << "0x0000";
            cout << "  ";
        }
        cout << endl;
    }
    cout << endl;

    cout << setbase(10);
    cout.unsetf(ios::right);
    cout << setw(w);
    cout << setfill(f);
}

void showKernel(int *kernel, int kw, int kh, bool fliph, bool flipv) {
    streamsize w = std::cout.width();
    char f = cout.fill();

    cout << setbase(16);
    cout << right;
    cout << setw(8);
    cout << setfill('0');
	cout << noshowbase;

    for (int i = 0; i < kh; i++) {
        cout << "    ";
        for (int j = 0; j < kw; j++) {
            int v = i;
            int h = j;

            // Print out may be flipped in either direction
            if (flipv == true)
                v = kh - (i + 1);

            if (fliph == true)
                h = kw - (j + 1);

            int d = kernel[v*kw + h];
            if (d != 0)
                cout << "0x" << setw(8) << (unsigned int)d;
            else
                cout << "0x00000000";
            cout << "  ";
        }
        cout << endl;
    }
    cout << endl;

    cout << setbase(10);
    cout.unsetf(ios::right);
    cout << setw(w);
    cout << setfill(f);
}

void showKernel(fp16 *kernel, int kw, int kh, bool fliph, bool flipv) {
    streamsize w = std::cout.width();
    char f = cout.fill();

    cout << setbase(16);
    cout << right;
    cout << setw(4);
    cout << setfill('0');
	cout << noshowbase;

    for (int i = 0; i < kh; i++) {
        cout << "    ";
        for (int j = 0; j < kw; j++) {
            int v = i;
            int h = j;

            // Print out may be flipped in either direction
            if (flipv == true)
                v = kh - (i + 1);

            if (fliph == true)
                h = kw - (j + 1);

            fp16 d = kernel[v*kw + h];
            unsigned short half = d.getPackedValue();
            if (half != 0)
                cout << "0x" << setw(4) << half;
            else
                cout << "0x0000";
            cout << "  ";
        }
        cout << endl;
    }
    cout << endl;

    cout << setbase(10);
    cout.unsetf(ios::right);
    cout << setw(w);
    cout << setfill(f);
}
