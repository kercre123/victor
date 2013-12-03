#include "anki/cozmo/robot/hal.h"


#if !ANKICORETECH_USE_MATLAB
#warning sim_uart.cpp currently requires USE_MATLAB=1
#else
#include "engine.h"
#include "anki/common/robot/matlabInterface.h"
extern Engine *matlabEngine_; // global matlab engine pointer

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {
      namespace {
        const u32 USB_BUFFER_SIZE = 4096;
        mxArray *mxBuffer_ = mxCreateNumericMatrix(1, USB_BUFFER_SIZE,
                                                   mxUINT8_CLASS, mxREAL);
        u8* USBbuffer_ = static_cast<u8*>(mxGetData(mxBuffer_));
        u32 USBbufferIndex_ = 0;
      }

      void UARTInit()
      {
        assert(matlabEngine_ != NULL);
        engEvalString(matlabEngine_, "SerialBuffer = SimulatedSerial();");
      }
      
      void USBFlush(void)
      {
        // Only copy the portion of the buffer in use to Matlab:
        // (Don't need to actually deallocate the buffer.)
        mxSetN(mxBuffer_, USBbufferIndex_);
        engPutVariable(matlabEngine_, "USBbuffer", mxBuffer_);
        engEvalString(matlabEngine_, "SerialBuffer.sendCharTo(USBbuffer);");
        USBbufferIndex_ = 0;
      }
      
      int USBPutChar(int c)
      {
        USBbuffer_[USBbufferIndex_++] = static_cast<u8>(c);
        
        // If buffer is full, send it off to Matlab
        if(USBbufferIndex_ == USB_BUFFER_SIZE) {
          USBFlush();
        }
        
        return c;
      }
      
      s32 USBGetChar(u32 timeout, bool peek)
      {
        s32 retVal = -1;
        
        if(peek) {
          engEvalString(matlabEngine_, "USBChar = SerialBuffer.getCharFrom(true);");
        } else {
          engEvalString(matlabEngine_, "USBChar = SerialBuffer.getCharFrom(false);");
        }
        mxArray *mxChar = engGetVariable(matlabEngine_, "USBChar");
        if(!mxIsEmpty(mxChar)) {
          retVal = static_cast<s32>(static_cast<char*>(mxGetData(mxChar))[0]);
        }
        mxDestroyArray(mxChar);
        
        return retVal;
      }
      
      s32 USBGetChar(u32 timeout)
      {
        return USBGetChar(timeout, false);
      }
      
      s32 USBPeekChar()
      {
        return USBGetChar(0, true);
      }
    
      void USBSendBuffer(u8* buffer, u32 size)
      {
        mxArray *mxMessage = mxCreateNumericMatrix(1, size, mxUINT8_CLASS, mxREAL);
        memcpy(mxGetData(mxMessage), buffer, size*sizeof(u8));
        
        engPutVariable(matlabEngine_, "USBmessage", mxMessage);
        engEvalString(matlabEngine_, "SerialBuffer.putChar(char(USBmessage));");
        mxDestroyArray(mxMessage);
      }
      
      u32 USBGetNumBytesToRead(void)
      {
        engEvalString(matlabEngine_, "numBytes = SerialBuffer.TxBytesAvailable;");
        mxArray* mxNumBytes = engGetVariable(matlabEngine_, "numBytes");
        u32 retval = static_cast<u32>(mxGetScalar(mxNumBytes));
        mxDestroyArray(mxNumBytes);
        return retval;
      }
      
      
    } // namespace HAL
  } // namespace Cozmo
} // namespace Anki

#endif // !ANKICORETECH_USE_MATLAB
