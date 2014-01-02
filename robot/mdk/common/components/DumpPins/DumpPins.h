///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @defgroup DumpPins DumpPins component
/// @{
/// @brief     Component used for debugging slow-changing signals on the myriad GPIOs.
///
/// This component can be used for debugging
/// and it's like a digital analyzer for slow signals (sample rate is up to 1MHz).
///
/// To use it, include the component, and start it like this:
///
/// @code
/// #include "DumpPins.h"
///
/// // 20 Megs
/// #define DUMPBUFSIZE 5242880
/// int __attribute__((section(".ddr.bss"))) dumpBuf[DUMPBUFSIZE]; 
///
/// int pins[] = {74, 75, 67, 68};
/// DumpPins(pins, sizeof(pins)/sizeof(int), dumpBuf, DUMPBUFSIZE, /* period is microsec: */ 1);
/// @endcode
///
/// The recording of pin state will stop after the buffer is full, or
/// StopDumpingPins(); is called.
/// It will tell you what command to use to save the
/// the dump buffer into a file.
/// This file does not have a standard format, but in can be converted
/// into a standard LXT format, using the small command line application:
/// dumpy_for_gtkwave_val_2_lxt, which you can find in this folder.
///
/// The waveform can be opened by any waveform viewer which supports
/// the LXT file format, such as, gtkwave.
///
/// GTKWave for windows:
/// http://www.dspia.com/gtkwave.html
/// GTKWave for linux is usually found in distribution repositories.
///
/// Executables for the conversion command line app are available.
///
/// To compile the sources for dumpy_for_gtkwave_val_2_lxt,
/// you need the zlib, and libbz2 development libraries.
///
/// specific examples:
///
/// for cygwin:
/// libbz2-devel zlib-devel
///
/// for mingw under cygwin:
/// mingw-libbz2-devel mingw-zlib-devel

#ifndef DUMP_PINS_H
#define DUMP_PINS_H

/// @brief Start dumping pin values into buffer.
/// @param[in] pins An array of pin numbers that you want to save
/// @param[in] pinNo The number of pins in the array
/// @param[out] buffer Pointer to the buffer where you want to save pin data
/// @param[in] bufferSize The size in words of the buffer.
/// @param[in] periodMicroSeconds The period of the timer that is used to sample the pins. 
///            A period of 1 will give you a maximum sample rate of 1 MHz, or for example a period of 7 will give you ~143 KHz sample rate.
void DumpPins(int *pins, int pinNo, int *buffer, int bufferSize, int periodMicroSeconds);

/// @brief Stop dumping pins into the data buffer.
///
/// This stops dumping pins into the data buffer, sooner then when the buffer becomes full.
/// You don't need to call this if your buffer fills up quickly, because the dumping is auto-
/// matically stopped when the buffer fills up.
void StopDumpingPins();

/// @}
#endif

