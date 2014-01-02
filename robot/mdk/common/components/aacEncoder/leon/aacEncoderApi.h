///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @defgroup aacEncoderApi AAC Encoder component
/// @{
/// @brief     aac Encoder API
/// 

#ifndef AACENCODERAPI_H_
#define AACENCODERAPI_H_

/// Select shave core to do the encoding
/// @param target   This selects the variant of the encoder:
///                   0 = ASM optimized
///                   1 = no ASM optimizations
void shaveSelectEncoder(unsigned int target);

/// Configure aac encoder settings
///
/// This function transmits the parameters of the PCM stream to the shave core that does the processing
///
/// @param sampleRate    The stream sample rate (frequency) in Hz
/// @param channelNumber The stream channel number (1 = mono ; 2 = stereo)
/// @param bytesPerSec   The stream byte rate ( bitrate/8 )
/// return               void
void shaveConfigureEncoder(
		unsigned long sampleRate,
		unsigned short channelNumber,
		unsigned long bytesPerSec
		);

/// Start the encoder
///
/// Starts the shave core and provide data to the input stream and save the output stream.
///
/// @return Returns 1 if successfull or 0 if there were errors detected
int shaveRun();

/// Prints a short summary of the encoding speed
void showReport();
///@}
#endif /* AACENCODERAPI_H_ */
