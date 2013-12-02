#include "anki/cozmo/robot/hal.h"

#if ANKICORETECH_USE_MATLAB
#include "engine.h"
#include "anki/common/robot/matlabInterface.h"
extern Engine *matlabEngine_; // global matlab engine pointer
#endif

namespace Anki
{
  namespace Cozmo
  {
    namespace HAL
    {

      void UARTInit()
      {
#if ANKICORETECH_USE_MATLAB
        assert(matlabEngine_ != NULL);
        engEvalString(matlabEngine_, "SerialBuffer = SimulatedSerial();");
#endif
      }
      
      int USBPutChar(int c)
      {
#if ANKICORETECH_USE_MATLAB
        const char temp = static_cast<char>(c);
        mxArray *mxChar = mxCreateString(&c);
        engPutVariable(matlabEngine_, "USBChar", mxChar);
        engEvalString(matlabEngine_, "SerialBuffer.putChar(USBChar);");
        mxDestroyArray(mxChar);
#endif
        return c;
      }
      
      
      s32 USBGetChar(u32 timeout, bool peek)
      {
        s32 retVal = -1;
#if ANKICORETECH_USE_MATLAB
        if(peek) {
          engEvalString(matlabEngine_, "USBChar = SerialBuffer.peekChar();");
        } else {
          engEvalString(matlabEngine_, "USBChar = SerialBuffer.getChar();");
        }
        mxArray *mxChar = engGetVariable(matlabEngine_, "USBChar");
        if(!mxIsEmpty(mxChar)) {
          retVal = static_cast<s32>(static_cast<char*>(mxGetData(mxChar))[0]);
        }
        mxDestroyArray(mxChar);
#endif
        
        return retVal;
      }
      
      s32 USBGetChar(u32 timeout) {
        USBGetChar(timeout, false);
      }
      
      void USBSendBuffer(u8* buffer, u32 size)
      {
#if ANKICORETECH_USE_MATLAB
        mxArray *mxMessage = mxCreateNumericMatrix(1, size, mxUINT8_CLASS, mxREAL);
        memcpy(mxGetData(mxMessage), buffer, size*sizeof(u8));
        
        engPutVariable(matlabEngine_, "USBmessage", mxMessage);
        engEvalString(matlabEngine_, "SerialBuffer.putChar(char(USBmessage));");
        mxDestroyArray(mxMessage);
#endif
      }
      
    } // namespace HAL
  } // namespace Cozmo
} // namespace Anki