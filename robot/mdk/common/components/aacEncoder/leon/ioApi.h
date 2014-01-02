///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @addtogroup aacEncoderApi
/// @{
/// @brief     I/O api that should be rewritten by the user
///

#ifndef IOAPI_H_
#define IOAPI_H_


/// Read data from the various input source
///
/// This should be implemented in the application.
/// It depends on the input stream source (read from file or microphone, etc).
/// To see a example, check the example application.
///
/// @param buffer    The location where data should be written
/// @param bytes     The size of the buffer that should be written in the buffer
/// @param bytesRead The size of the buffer that was written
int getStream (void *buffer, unsigned int bytes, unsigned int *bytesRead);

/// Write data to various outputs (Usually to a file)
///
/// This should be implemented in the application.
/// It depends on the output stream destination (written to a file, etc).
/// To see a example, check the example application.
///
/// @param buffer       The location where data should be read from
/// @param bytes        The size of the buffer that should be read from the buffer
/// @param bytesWritten The size of the buffer that should be read
int writeStream (void *buffer,	unsigned int bytes,	unsigned int *bytesWritten);
/// @}
#endif /* IOAPI_H_ */
