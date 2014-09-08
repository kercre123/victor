// Save a matlab array to an embedded Array file

// Example:
// mexSaveEmbeddedArray(array, filename);

#include "mex.h"

#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/serialize.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/matlab/mexUtilities.h"
#include "anki/common/shared/utilities_shared.h"

#include "anki/tools/threads/threadSafeUtilities.h"

using namespace Anki;
using namespace Anki::Embedded;

template<typename Type> Result AllocateAndSave(const mxArray *matlabArray, const char *filename, const s32 compressionLevel, MemoryStack scratch);
template<typename Type> Result AllocateAndSaveCell(const mxArray *matlabArray, const char *filename, const s32 compressionLevel, MemoryStack scratch);
template<> Result AllocateAndSaveCell<char*>(const mxArray *matlabArray, const char *filename, const s32 compressionLevel, MemoryStack scratch);

const s32 bufferSize = 50000000;

template<typename Type> Result AllocateAndSave(const mxArray *matlabArray, const char *filename, const s32 compressionLevel, MemoryStack scratch)
{
  AnkiAssert(!mxIsCell(matlabArray));

  Array<Type> ankiArray = mxArrayToArray<Type>(matlabArray, scratch);

  if(!ankiArray.IsValid())
    return RESULT_FAIL;

  return ankiArray.SaveBinary(filename, compressionLevel, scratch);
}

template<> Result AllocateAndSaveCell<char*>(const mxArray *matlabArray, const char *filename, const s32 compressionLevel, MemoryStack scratch)
{
  AnkiAssert(mxIsCell(matlabArray));

  Array<char *> ankiArray = mxCellArrayToStringArray(matlabArray, scratch);

  if(!ankiArray.IsValid())
    return RESULT_FAIL;

  return ankiArray.SaveBinary(filename, compressionLevel, scratch);
}

Result Save(const mxArray *matlabArray, const char *filename, const s32 compressionLevel, MemoryStack scratch)
{
  const mxClassID matlabClassId = mxGetClassID(matlabArray);

  if(mxIsCell(matlabArray)) {
    const mxArray * firstElement = mxGetCell(matlabArray, 0);
    const mxClassID matlabCellClassId = mxGetClassID(firstElement);
    //if(matlabCellClassId == mxDOUBLE_CLASS) {
    //  return AllocateAndSaveCell<f64>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxSINGLE_CLASS) {
    //  return AllocateAndSaveCell<f32>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxINT8_CLASS) {
    //  return AllocateAndSaveCell<s8>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxUINT8_CLASS) {
    //  return AllocateAndSaveCell<u8>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxINT16_CLASS) {
    //  return AllocateAndSaveCell<s16>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxUINT16_CLASS) {
    //  return AllocateAndSaveCell<u16>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxINT32_CLASS) {
    //  return AllocateAndSaveCell<s32>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxUINT32_CLASS) {
    //  return AllocateAndSaveCell<u32>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxINT64_CLASS) {
    //  return AllocateAndSaveCell<s64>(matlabArray, filename, scratch);
    //} else if(matlabCellClassId == mxUINT64_CLASS) {
    //  return AllocateAndSaveCell<u64>(matlabArray, filename, scratch);
    //} else
    if(matlabCellClassId == mxCHAR_CLASS) {
      return AllocateAndSaveCell<char *>(matlabArray, filename, compressionLevel, scratch);
    } else {
      AnkiAssert(false);
    }
  } else { // if(mxIsCell(matlabArray))
    if(matlabClassId == mxDOUBLE_CLASS) {
      return AllocateAndSave<f64>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxSINGLE_CLASS) {
      return AllocateAndSave<f32>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxINT8_CLASS) {
      return AllocateAndSave<s8>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxUINT8_CLASS) {
      return AllocateAndSave<u8>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxINT16_CLASS) {
      return AllocateAndSave<s16>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxUINT16_CLASS) {
      return AllocateAndSave<u16>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxINT32_CLASS) {
      return AllocateAndSave<s32>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxUINT32_CLASS) {
      return AllocateAndSave<u32>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxINT64_CLASS) {
      return AllocateAndSave<s64>(matlabArray, filename, compressionLevel, scratch);
    } else if(matlabClassId == mxUINT64_CLASS) {
      return AllocateAndSave<u64>(matlabArray, filename, compressionLevel, scratch);
    } else {
      AnkiAssert(false);
    }
  } // if(mxIsCell(matlabArray)) ... else

  return RESULT_FAIL;
}

typedef struct SaveThreadParams
{
  const mxArray * matlabArray;
  const char * filename;
  const s32 compressionLevel;
  MemoryStack scratch;

  SaveThreadParams(
    const mxArray * matlabArray,
    const char * filename,
    const s32 compressionLevel,
    MemoryStack scratch)
    : matlabArray(matlabArray), filename(filename), compressionLevel(compressionLevel), scratch(scratch)
  {
  }
} SaveThreadParams;

ThreadResult SaveThread(void * voidSaveThreadParams)
{
  SaveThreadParams * restrict saveThreadParams = reinterpret_cast<SaveThreadParams*>(voidSaveThreadParams);

  Save(saveThreadParams->matlabArray, saveThreadParams->filename, saveThreadParams->compressionLevel, saveThreadParams->scratch);

  return 0;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  s32 compressionLevel = 6;
  s32 numThreads = 1;
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  AnkiConditionalErrorAndReturn((nrhs >= 2 && nrhs <= 4) && nlhs == 0, "mexSaveEmbeddedArray", "Call this function as follows: mexSaveEmbeddedArray(array, filename, <compressionLeve>, <numThreads>); % array and filename may be cell arrays.");

  if(nrhs >= 3) {
    compressionLevel = MAX(0, MIN(9, saturate_cast<s32>(mxGetScalar(prhs[2]))));
  }

  if(nrhs >= 4) {
    numThreads = MAX(1, MIN(64, saturate_cast<s32>(mxGetScalar(prhs[3]))));
  }

  //
  // Note: This multithreading is a bit wierd, because Matlab doesn't like threads allocating their own memory. It could be improved.
  //

  if(mxIsCell(prhs[0])) {
    AnkiConditionalErrorAndReturn(mxIsCell(prhs[1]), "mexSaveEmbeddedArray", "If one input is a cell, both must be");

    MemoryStack memory(mxMalloc(100000), 100000);
    AnkiConditionalErrorAndReturn(memory.IsValid(), "mexSaveEmbeddedArray", "Memory could not be allocated");

    Array<char *> filenames = mxCellArrayToStringArray(prhs[1], memory);
    const s32 numFilenames = filenames.get_size(1);

    SaveThreadParams ** threadParams = reinterpret_cast<SaveThreadParams**>( mxMalloc(numThreads*sizeof(SaveThreadParams*)) );
    ThreadHandle * threadHandles = reinterpret_cast<ThreadHandle*>( mxMalloc(numThreads*sizeof(ThreadHandle)) );
    MemoryStack * memorys = reinterpret_cast<MemoryStack*>( mxMalloc(numThreads*sizeof(MemoryStack)) );

    for(s32 iThread=0; iThread<numThreads; iThread++) {
      memorys[iThread] = MemoryStack(mxMalloc(bufferSize), bufferSize);
      AnkiConditionalErrorAndReturn(memorys[iThread].IsValid(), "mexSaveEmbeddedArray", "Memory could not be allocated");
    }

    for(s32 iFile=0; iFile<numFilenames; iFile+=numThreads) {
      for(s32 iThread=0; iThread<numThreads; iThread++) {
        const s32 index = iFile + iThread;

        if(index >= numFilenames)
          break;

        mwIndex subs[2] = {0, index};

        const mwIndex cellIndex = mxCalcSingleSubscript(prhs[0], 2, &subs[0]);

        const mxArray * curMatlabArray = mxGetCell(prhs[0], cellIndex);

        // Each thread is responsible for deleting its own
        threadParams[iThread] = new SaveThreadParams(
          curMatlabArray,
          filenames[0][index],
          compressionLevel,
          memorys[iThread]);

        threadHandles[iThread] = CreateSimpleThread(SaveThread, threadParams[iThread]);
      }

      for(s32 iThread=0; iThread<numThreads; iThread++) {
        const s32 index = iFile + iThread;

        if(index >= numFilenames)
          break;

        WaitForSimpleThread(threadHandles[iThread]);

        delete(threadParams[iThread]);
      }
    }

    for(s32 iThread=0; iThread<numThreads; iThread++) {
      mxFree(memorys[iThread].get_buffer());
    }

    mxFree(memory.get_buffer());
    mxFree(threadParams);
    mxFree(threadHandles);
    mxFree(memorys);
  } else {
    AnkiConditionalErrorAndReturn(!mxIsCell(prhs[1]), "mexSaveEmbeddedArray", "If one input is a cell, both must be");

    MemoryStack memory(mxMalloc(bufferSize), bufferSize);
    AnkiConditionalErrorAndReturn(memory.IsValid(), "mexSaveEmbeddedArray", "Memory could not be allocated");

    char* filename = mxArrayToString(prhs[1]);

    Save(prhs[0], filename, compressionLevel, memory);

    mxFree(filename);
    mxFree(memory.get_buffer());
  }
} // mexFunction()
