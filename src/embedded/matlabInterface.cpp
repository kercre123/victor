#include "anki/embeddedCommon.h"

#include <stdarg.h>

namespace Anki
{
  namespace Embedded
  {
#if defined(ANKICORETECHEMBEDDED_USE_MATLAB)

#define TEXT_BUFFER_SIZE 1024

    mxClassID ConvertToMatlabType(const char *typeName, size_t byteDepth)
    {
      if(typeName[0] == 'u') { //unsigned
        if(byteDepth == 1) {
          return mxUINT8_CLASS;
        } else if(byteDepth == 2) {
          return mxUINT16_CLASS;
        }else if(byteDepth == 4) {
          return mxUINT32_CLASS;
        }else if(byteDepth == 8) {
          return mxUINT64_CLASS;
        }
      } else if(typeName[0] == 'f' && byteDepth == 4) { //float
        return mxSINGLE_CLASS;
      } else if(typeName[0] == 'd' && byteDepth == 8) { //double
        return mxDOUBLE_CLASS;
      } else { // signed
        if(byteDepth == 1) {
          return mxINT8_CLASS;
        } else if(byteDepth == 2) {
          return mxINT16_CLASS;
        }else if(byteDepth == 4) {
          return mxINT32_CLASS;
        }else if(byteDepth == 8) {
          return mxINT64_CLASS;
        }
      }

      return mxUNKNOWN_CLASS;
    }

    const char* ConvertToMatlabTypeString(const char *typeName, size_t byteDepth)
    {
      if(typeName[0] == 'u') { //unsigned
        if(byteDepth == 1) {
          return "uint8";
        } else if(byteDepth == 2) {
          return "uint16";
        }else if(byteDepth == 4) {
          return "uint32";
        }else if(byteDepth == 8) {
          return "uint64";
        }
      } else if(typeName[0] == 'f' && byteDepth == 4) { //float
        return "single";
      } else if(typeName[0] == 'd' && byteDepth == 8) { //double
        return "double";
      } else { // signed
        if(byteDepth == 1) {
          return "int8";
        } else if(byteDepth == 2) {
          return "int16";
        }else if(byteDepth == 4) {
          return "int32";
        }else if(byteDepth == 8) {
          return "int64";
        }
      }

      return "unknown";
    }

    Matlab::Matlab(bool clearWorkspace)
    {
      // Multithreading under Windows requires this command
      // CoInitializeEx(NULL, COINIT_MULTITHREADED);

      this->ep = engOpen(NULL);

      if(clearWorkspace){
        EvalString("clear all");
      }

      EvalString("lastAnkiCommandBuffer=cell(0, 1);");
    }

    std::string Matlab::EvalString(const char * const format, ...)
    {
      if(!this->ep) {
        DASError("Anki.", "Matlab engine is not started/connected");
        return "";
      }

      va_list args;
      char *buffer;

      va_start( args, format );

#ifdef _MSC_VER
      const u32 len = _vscprintf( format, args ) + 1;
#else
      const u32 len = 1024;
#endif

      buffer = (char*) malloc( len * sizeof(char) );
      vsnprintf( buffer, len, format, args );

      engEvalString(this->ep, buffer);

      std::string toReturn = std::string(buffer);
      free( buffer );

      return toReturn;
    }

    std::string Matlab::EvalStringEcho(const char * const format, ...)
    {
      if(!this->ep) {
        DASError("Anki.", "Matlab engine is not started/connected");
        return "";
      }

      va_list args;
      char *buffer;

      va_start( args, format );

#ifdef _MSC_VER
      const u32 len = _vscprintf( format, args ) + 1;
#else
      const u32 len = 1024;
#endif

      buffer = (char*) malloc( len * sizeof(char) );
      vsnprintf( buffer, len, format, args );

      engEvalString(this->ep, buffer);

      std::string toReturn = std::string(buffer);
      PutString(buffer, (int)toReturn.size(), "lastAnkiCommand");

      EvalString("if length(lastAnkiCommandBuffer)==%d lastAnkiCommandBuffer=lastAnkiCommandBuffer(2:end); end; lastAnkiCommandBuffer{end+1}=lastAnkiCommand;", Matlab::COMMAND_BUFFER_SIZE);

      free( buffer );

      return toReturn;
    }

    std::string Matlab::EvalStringExplicit(const char * buffer)
    {
      if(!this->ep) {
        DASError("Anki.", "Matlab engine is not started/connected");
        return "";
      }

      engEvalString(this->ep, buffer);

      std::string toReturn = std::string(buffer);

      return toReturn;
    }

    std::string Matlab::EvalStringExplicitEcho(const char * buffer)
    {
      if(!this->ep) {
        DASError("Anki.", "Matlab engine is not started/connected");
        return "";
      }

      engEvalString(this->ep, buffer);

      std::string toReturn = std::string(buffer);
      PutString(buffer, (int)toReturn.size(), "lastAnkiCommand");

      return toReturn;
    }

    mxArray* Matlab::GetArray(const std::string name)
    {
      if(!this->ep) {
        DASError("Anki.", "Matlab engine is not started/connected");
        return NULL;
      }

      mxArray *arr = engGetVariable(this->ep, name.data());
      return arr;
    }

    MatlabVariableType Matlab::GetType(const std::string name)
    {
      if(!this->ep) {
        DASError("Anki.", "Matlab engine is not started/connected");
        return TYPE_UNKNOWN;
      }

      char typeName[TEXT_BUFFER_SIZE];
      snprintf(typeName, TEXT_BUFFER_SIZE, "%s_types", name.data());
      EvalStringEcho("%s=int32([isa(%s, 'int8'), isa(%s, 'u8'), isa(%s, 'int16'), isa(%s, 'u16'), isa(%s, 'int32'), isa(%s, 'u32'), isa(%s, 'int64'), isa(%s, 'u64'), isa(%s, 'single'), isa(%s, 'double')]);", typeName, name.data(), name.data(), name.data(), name.data(), name.data(), name.data(), name.data(), name.data(), name.data(), name.data());
      int *types = Get<s32>(typeName);
      EvalStringEcho("clear %s;", typeName);

      MatlabVariableType type;

      if(types[0] == 1){type=TYPE_INT8;}
      else if(types[1] == 1){type=TYPE_UINT8;}
      else if(types[2] == 1){type=TYPE_INT16;}
      else if(types[3] == 1){type=TYPE_UINT16;}
      else if(types[4] == 1){type=TYPE_INT32;}
      else if(types[5] == 1){type=TYPE_UINT32;}
      else if(types[6] == 1){type=TYPE_INT64;}
      else if(types[7] == 1){type=TYPE_UINT64;}
      else if(types[8] == 1){type=TYPE_SINGLE;}
      else if(types[9] == 1){type=TYPE_DOUBLE;}
      else{type = TYPE_UNKNOWN;}

      types = 0; free(types);

      return type;
    }

#if defined(ANKI_USE_OPENCV)
    s32 Matlab::GetCvMat(const CvMat *matrix, std::string name)
    {
      if(!this->ep) {
        DASError("Anki.", "Matlab engine is not started/connected");
        return -1;
      }

      if(matrix == 0)  {
        printf("Error: CvMat is not initialized for %s\n", name.data());
        return -1;
      }

      char tmpName[TEXT_BUFFER_SIZE];
      snprintf(tmpName, TEXT_BUFFER_SIZE, "%s_AnkiTMP", name.data());
      EvalString("%s=%s';", tmpName, name.data());
      mxArray *arrayTmp = GetArray(tmpName);

      mxClassID classId = mxGetClassID(arrayTmp);

      mwSize size = mxGetNumberOfElements(arrayTmp);

      bool mismatch = true;
      if(classId == mxDOUBLE_CLASS) {
        //if(mat->type == CV_64F)
        {
          double *valTmp = (double*)mxGetPr(arrayTmp);
          for(s32 i = 0; i<size; i++) {
            matrix->data.db[i] = valTmp[i];
          }
          mismatch = false;
        }
      }else if(classId == mxSINGLE_CLASS) {
        //if(mat->type == CV_32F)
        {
          float *valTmp = (float*)mxGetPr(arrayTmp);
          for(int i = 0; i<(s32)size; i++) {
            matrix->data.fl[i] = valTmp[i];
          }
          mismatch = false;
        }
      }else if(classId == mxINT16_CLASS) {
        //if(mat->type == CV_16S)
        {
          s16 *valTmp = (s16*)mxGetPr(arrayTmp);
          for(int i = 0; i<(s32)size; i++) {
            matrix->data.s[i] = valTmp[i];
          }
          mismatch = false;
        }
      }else if(classId == mxUINT16_CLASS) {
        //if(mat->type == CV_16U)
        {
          u16 *valTmp = (u16*)mxGetPr(arrayTmp);
          for(int i = 0; i<(s32)size; i++) {
            matrix->data.s[i] = valTmp[i];
          }
          mismatch = false;
        }
      }else if(classId == mxINT32_CLASS) {
        //if(mat->type == CV_32S)
        {
          s32 *valTmp = (s32*)mxGetPr(arrayTmp);
          for(int i = 0; i<(s32)size; i++) {
            matrix->data.i[i] = valTmp[i];
          }
          mismatch = false;
        }
      } else {
        printf("Error: Class ID not supported for %s\n", name.data());
        EvalString("clear %s;", tmpName);
        return -1;
      }

      mxDestroyArray(arrayTmp);

      if(mismatch) {
        printf("Error: Class mismatch for %s\n", name.data());
        EvalString("clear %s;", tmpName);
        return -1;
      }

      EvalString("clear %s;", tmpName);
      return 0;
    }

    //like GetCvMat, but also allocates the CvMat* from the heap
    CvMat* Matlab::GetCvMat(const std::string name)
    {
      CvMat* matrix = 0;

      char tmpName[TEXT_BUFFER_SIZE];
      snprintf(tmpName, TEXT_BUFFER_SIZE, "%s_AnkiTMP", name.data());
      EvalString("%s=%s';", tmpName, name.data());
      mxArray *arrayTmp = GetArray(tmpName);

      mxClassID classId = mxGetClassID(arrayTmp);
      mwSize nDims = mxGetNumberOfDimensions(arrayTmp);

      if(nDims != 2) {
        printf("Error: Matrix %s must be 2D\n", name.data());
        return matrix;
      }

      const mwSize *dims = mxGetDimensions(arrayTmp);
      mwSize size = mxGetNumberOfElements(arrayTmp);

      if(classId == mxDOUBLE_CLASS) {
        double *valTmp = (double*)mxGetPr(arrayTmp);
        matrix = cvCreateMat((int)dims[1], (int)dims[0], CV_64F);
        for(int i = 0; i<(int)size; i++) { matrix->data.db[i] = valTmp[i]; }
      }else if(classId == mxSINGLE_CLASS) {
        float *valTmp = (float*)mxGetPr(arrayTmp);
        matrix = cvCreateMat((int)dims[1], (int)dims[0], CV_32F);
        for(int i = 0; i<(int)size; i++) { matrix->data.fl[i] = valTmp[i]; }
      }else if(classId == mxINT16_CLASS) {
        s16 *valTmp = (s16*)mxGetPr(arrayTmp);
        matrix = cvCreateMat((int)dims[1], (int)dims[0], CV_16S);
        for(int i = 0; i<(int)size; i++) { matrix->data.s[i] = valTmp[i]; }
      }else if(classId == mxUINT16_CLASS) {
        u16 *valTmp = (u16*)mxGetPr(arrayTmp);
        matrix = cvCreateMat((int)dims[1], (int)dims[0], CV_16U);
        for(int i = 0; i<(int)size; i++) {matrix->data.s[i] = valTmp[i]; }
      }else if(classId == mxINT32_CLASS) {
        s32 *valTmp = (s32*)mxGetPr(arrayTmp);
        matrix = cvCreateMat((int)dims[1], (int)dims[0], CV_32S);
        for(int i = 0; i<(int)size; i++) { matrix->data.i[i] = valTmp[i]; }
      } else {
        printf("Error: Class ID not supported for %s\n", name.data());
        EvalString("clear %s;", tmpName);
        return matrix;
      }

      mxDestroyArray(arrayTmp);

      EvalString("clear %s;", tmpName);
      return matrix;
    }

    s32 Matlab::PutCvMat(const CvMat *matrix, std::string name)
    {
      if(!this->ep) {
        DASError("Anki.", "Matlab engine is not started/connected");
        return -1;
      }

      if(matrix == 0) {
        printf("Error: CvMat is not initialized for %s\n", name.data());
        return -1;
      }

      char tmpName[TEXT_BUFFER_SIZE];
      snprintf(tmpName, TEXT_BUFFER_SIZE, "%s_AnkiTMP", name.data());

      bool mismatch = true;
      if((matrix->type&CV_MAT_DEPTH_MASK) == CV_64F) {
        //if(mat->type == CV_64F)
        {
          Put<double>(matrix->data.db, matrix->rows*matrix->cols, tmpName);
          mismatch = false;
        }
      }else if((matrix->type&CV_MAT_DEPTH_MASK) == CV_32F) {
        //if(mat->type == CV_32F)
        {
          Put<float>(matrix->data.fl, matrix->rows*matrix->cols, tmpName);
          mismatch = false;
        }
      }else if((matrix->type&CV_MAT_DEPTH_MASK) == CV_16S) {
        //if(mat->type == CV_16S)
        {
          Put<s16>(matrix->data.s, matrix->rows*matrix->cols, tmpName);
          mismatch = false;
        }
      }else if((matrix->type&CV_MAT_DEPTH_MASK) == CV_16U) {
        //if(mat->type == CV_16U)
        {
          Put<u16>((const unsigned short*)matrix->data.s, matrix->rows*matrix->cols, tmpName);
          mismatch = false;
        }
      }else if((matrix->type&CV_MAT_DEPTH_MASK) == CV_32S) {
        //if(mat->type == CV_32S)
        {
          Put<s32>(matrix->data.i, matrix->rows*matrix->cols, tmpName);
          mismatch = false;
        }
      } else {
        printf("Error: Class ID not supported for %s\n", name.data());
        EvalString("clear %s;", tmpName);
        return -1;
      }

      if(mismatch) {
        printf("Error: Class mismatch for %s\n", name.data());
        EvalString("clear %s;", tmpName);
        return -1;
      }

      EvalString("%s=reshape(%s, [%d, %d])';", name.data(), tmpName, matrix->cols, matrix->rows);
      EvalString("clear %s;", tmpName);

      return 0;
    }
#endif // #if defined(ANKI_USE_OPENCV)

    s32 Matlab::PutString(const char * characters, s32 nValues, const std::string name)
    {
      if(!this->ep) {
        DASError("Anki.", "Matlab engine is not started/connected");
        return -1;
      }

      s32 returnVal = Put<s8>((s8*)characters, nValues, name);
      EvalString("%s=char(%s');", name.data(), name.data());

      return returnVal;
    }

#if defined(ANKI_USE_OPENCV)
    s32 Matlab::GetIplImage(IplImage *im, const std::string name)
    {
      if(!this->ep) {
        DASError("Anki.", "Matlab engine is not started/connected");
        return -1;
      }

      if(!im) {
        printf("Error: im is not initialized for %s.\n", name.data());
        return -1;
      }

      char nameTmp[TEXT_BUFFER_SIZE];
      snprintf(nameTmp, TEXT_BUFFER_SIZE, "%s_AnkiTMP", name.data());

      if(im->nChannels == 1) {
        EvalString("%s=permute(%s, [2, 1]);", nameTmp, name.data());
        //EvalString("%s=%s;", nameTmp, name);
      }else if(im->nChannels == 3) {
        EvalString("%s_AnkiTMP(:, :, 3)=%s(:, :, 1);\
                   %s_AnkiTMP(:, :, 2)=%s(:, :, 2);\
                   %s_AnkiTMP(:, :, 1)=%s(:, :, 3);\
                   %s_AnkiTMP=permute(%s_AnkiTMP, [3, 2, 1]);\
                   %s_AnkiTMP=%s_AnkiTMP(:);",
                   name.data(), name.data(),
                   name.data(), name.data(),
                   name.data(), name.data(),
                   name.data(), name.data(),
                   name.data(), name.data(),
                   name.data());
      } else {
        printf("Error: %d channels not supported for %s.\n", im->nChannels, name.data());
        return -1;
      }

      mxArray *arrayTmp = GetArray(nameTmp);
      if(!arrayTmp) {
        printf("Error: no variable named %s exists on the workspace.\n", name.data());
        EvalString("clear %s_AnkiTMP;", name.data());
        return -1;
      }
      mwSize size = mxGetNumberOfElements(arrayTmp);

      if(size != im->width*im->height*im->nChannels) {
        printf("Error: Matlab %s and IplImage sizes don't match.\n", name.data());
        EvalString("clear %s_AnkiTMP;", name.data());
        return -1;
      }

      if(im->depth == IPL_DEPTH_8U) {
        if(!mxIsClass(arrayTmp, "u8")) {
          printf("Error: Matlab %s and IplImage depths don't match.\n", name.data());
          EvalString("clear %s_AnkiTMP;", name.data());
          return -1;
        }

        u8 *imageData = (u8*)im->imageData;
        u8 *matlabData;
        matlabData = Get<u8>(nameTmp);
        int ciO = 0, ciM = 0;
        for(s32 j = 0; j<im->height; j++) {
          ciO = j*im->widthStep;
          for(s32 i = 0; i<im->width*im->nChannels; i++) {
            imageData[ciO] = matlabData[ciM];
            ciO++;
            ciM++;
          }
        }
        free(matlabData);
      }else if(im->depth == IPL_DEPTH_32F) {
        if(!mxIsClass(arrayTmp, "single")) {
          printf("Error: Matlab %s and IplImage depths don't match.\n", name.data());
          EvalString("clear %s_AnkiTMP;", name.data());
          return -1;
        }

        float *imageData = (float*)im->imageData;
        float *matlabData;
        matlabData = Get<float>(nameTmp);
        int ciO = 0, ciM = 0;
        for(s32 j = 0; j<im->height; j++) {
          ciO = j*im->widthStep/4;
          for(s32 i = 0; i<im->width*im->nChannels; i++) {
            imageData[ciO] = matlabData[ciM];
            ciO++;
            ciM++;
          }
        }
        /*for(s32 i = 0; i<(s32)size; i++)
        {
        imageData[i] = matlabData[i];
        }*/
        free(matlabData);
      }else if(im->depth == IPL_DEPTH_32S) {
        if(!mxIsClass(arrayTmp, "int32")) {
          printf("Error: Matlab %s and IplImage depths don't match.\n", name.data());
          EvalString("clear %s_AnkiTMP;", name.data());
          return -1;
        }

        s32 *imageData = (s32*)im->imageData;
        s32 *matlabData;
        matlabData = Get<s32>(nameTmp);
        int ciO = 0, ciM = 0;
        for(s32 j = 0; j<im->height; j++) {
          ciO = j*im->widthStep/4;
          for(s32 i = 0; i<(s32)(im->width*im->nChannels; i++) {
            imageData[ciO] = matlabData[ciM];
              ciO++;
            ciM++;
          }
        }
        /*for(s32 i = 0; i<(s32)size; i++)
        {
        imageData[i] = matlabData[i];
        }*/
        free(matlabData);
      } else {
        printf("Error: depth not supported for %s.\n", name.data());
        EvalString("clear %s_AnkiTMP;", name.data());
        return -1;
      }

      EvalString("clear %s_AnkiTMP;", name.data());

      return 0;
    }

    s32 Matlab::PutIplImage(const IplImage *im, const std::string name, bool flipRedBlue)
    {
      if(!this->ep) {
        DASError("Anki.", "Matlab engine is not started/connected");
        return -1;
      }

      if(!im) {
        printf("Error: image is not initialized for %s\n", name.data());
        return -1;
      }

      s32 width = im->width, height = im->height, nChannels = im->nChannels;

      char nameTmp[TEXT_BUFFER_SIZE];
      snprintf(nameTmp, TEXT_BUFFER_SIZE, "%s_AnkiTMP", name.data());

      if(im->depth == IPL_DEPTH_8U)
        Put<u8>((u8*)im->imageData, width*height*nChannels, nameTmp);
      else if(im->depth == IPL_DEPTH_32F)
        Put<float>((float*)im->imageData, width*height*nChannels, nameTmp);
      else if(im->depth == IPL_DEPTH_32S)
        Put<s32>((s32*)im->imageData, width*height*nChannels, nameTmp);
      else{
        printf("Error: Dthis->epth not supported for %s.\n", name.data());
        return -1;
      }

      if(nChannels == 1) {
        EvalString("%s=permute(reshape(%s_AnkiTMP, [%d, %d]), [2, 1]); clear %s_AnkiTMP;", name.data(), name.data(), width, height, name.data());
      }else if(nChannels == 3) {
        if(flipRedBlue) {
          EvalString("%s_AnkiTMP=permute(reshape(%s_AnkiTMP, [3, %d, %d]), [3, 2, 1]);\
                     %s(:, :, 3)=%s_AnkiTMP(:, :, 1);\
                     %s(:, :, 2)=%s_AnkiTMP(:, :, 2);\
                     %s(:, :, 1)=%s_AnkiTMP(:, :, 3);\
                     clear %s_AnkiTMP",
                     name.data(), name.data(), width, height,
                     name.data(), name.data(),
                     name.data(), name.data(),
                     name.data(), name.data(),
                     name.data());
        } else {
          EvalString("%s=permute(reshape(%s_AnkiTMP, [3, %d, %d]), [3, 2, 1]);\
                     clear %s_AnkiTMP",
                     name.data(), name.data(), width, height, name.data());
        }
      } else {
        printf("Error: %d channels not supported for %s.\n", im->nChannels, name.data());
        return -1;
      }

      return 0;
    }
#endif // #if defined(ANKI_USE_OPENCV)

    s32 Matlab::SetVisible(bool isVisible)
    {
      if(!this->ep) {
        DASError("Anki.", "Matlab engine is not started/connected");
        return -1;
      }

      s32 returnVal = engSetVisible(this->ep, isVisible);

      return returnVal;
    }

    bool Matlab::DoesVariableExist(const std::string name)
    {
      if(!this->ep) {
        DASError("Anki.", "Matlab engine is not started/connected");
        return false;
      }

      EvalString("ans=exist('%s', 'var');", name.data());

      double *ans = Get<double>("ans");
      double ansVal = *ans;
      free(ans); ans = 0;
      if(ansVal<.5)
        return false;
      else
        return true;
    }

    s32 Matlab::PutArray_u8(const Array_u8 &matrix, const std::string name)
    {
      if(!ep) {
        DASError("Anki.PutArray", "Matlab engine is not started/connected");
        return -1;
      }

      const std::string tmpName = name + std::string("_AnkiTMP");
      const std::string matlabTypeName = Anki::Embedded::ConvertToMatlabTypeString(typeid(u8).name(), sizeof(u8));

      EvalStringEcho("%s=zeros([%d,%d],'%s');", name.data(), matrix.get_size(0), matrix.get_size(1), matlabTypeName.data());

      for(s32 y=0; y<matrix.get_size(0); y++) {
        Put<u8>(matrix.Pointer(y,0), matrix.get_size(1), tmpName);
        EvalStringEcho("%s(%d,:)=%s;", name.data(), y+1, tmpName.data());
      }

      EvalString("clear %s;", tmpName.data());

      return 0;
    }

    s32 Matlab::PutArray_s8(const Array_s8 &matrix, const std::string name)
    {
      if(!ep) {
        DASError("Anki.PutArray", "Matlab engine is not started/connected");
        return -1;
      }

      const std::string tmpName = name + std::string("_AnkiTMP");
      const std::string matlabTypeName = Anki::Embedded::ConvertToMatlabTypeString(typeid(s8).name(), sizeof(s8));

      EvalStringEcho("%s=zeros([%d,%d],'%s');", name.data(), matrix.get_size(0), matrix.get_size(1), matlabTypeName.data());

      for(s32 y=0; y<matrix.get_size(0); y++) {
        Put<s8>(matrix.Pointer(y,0), matrix.get_size(1), tmpName);
        EvalStringEcho("%s(%d,:)=%s;", name.data(), y+1, tmpName.data());
      }

      EvalString("clear %s;", tmpName.data());

      return 0;
    }

    s32 Matlab::PutArray_u16(const Array_u16 &matrix, const std::string name)
    {
      if(!ep) {
        DASError("Anki.PutArray", "Matlab engine is not started/connected");
        return -1;
      }

      const std::string tmpName = name + std::string("_AnkiTMP");
      const std::string matlabTypeName = Anki::Embedded::ConvertToMatlabTypeString(typeid(u16).name(), sizeof(u16));

      EvalStringEcho("%s=zeros([%d,%d],'%s');", name.data(), matrix.get_size(0), matrix.get_size(1), matlabTypeName.data());

      for(s32 y=0; y<matrix.get_size(0); y++) {
        Put<u16>(matrix.Pointer(y,0), matrix.get_size(1), tmpName);
        EvalStringEcho("%s(%d,:)=%s;", name.data(), y+1, tmpName.data());
      }

      EvalString("clear %s;", tmpName.data());

      return 0;
    }
    s32 Matlab::PutArray_s16(const Array_s16 &matrix, const std::string name)
    {
      if(!ep) {
        DASError("Anki.PutArray", "Matlab engine is not started/connected");
        return -1;
      }

      const std::string tmpName = name + std::string("_AnkiTMP");
      const std::string matlabTypeName = Anki::Embedded::ConvertToMatlabTypeString(typeid(s16).name(), sizeof(s16));

      EvalStringEcho("%s=zeros([%d,%d],'%s');", name.data(), matrix.get_size(0), matrix.get_size(1), matlabTypeName.data());

      for(s32 y=0; y<matrix.get_size(0); y++) {
        Put<s16>(matrix.Pointer(y,0), matrix.get_size(1), tmpName);
        EvalStringEcho("%s(%d,:)=%s;", name.data(), y+1, tmpName.data());
      }

      EvalString("clear %s;", tmpName.data());

      return 0;
    }

    s32 Matlab::PutArray_u32(const Array_u32 &matrix, const std::string name)
    {
      if(!ep) {
        DASError("Anki.PutArray", "Matlab engine is not started/connected");
        return -1;
      }

      const std::string tmpName = name + std::string("_AnkiTMP");
      const std::string matlabTypeName = Anki::Embedded::ConvertToMatlabTypeString(typeid(u32).name(), sizeof(u32));

      EvalStringEcho("%s=zeros([%d,%d],'%s');", name.data(), matrix.get_size(0), matrix.get_size(1), matlabTypeName.data());

      for(s32 y=0; y<matrix.get_size(0); y++) {
        Put<u32>(matrix.Pointer(y,0), matrix.get_size(1), tmpName);
        EvalStringEcho("%s(%d,:)=%s;", name.data(), y+1, tmpName.data());
      }

      EvalString("clear %s;", tmpName.data());

      return 0;
    }
    s32 Matlab::PutArray_s32(const Array_s32 &matrix, const std::string name)
    {
      if(!ep) {
        DASError("Anki.PutArray", "Matlab engine is not started/connected");
        return -1;
      }

      const std::string tmpName = name + std::string("_AnkiTMP");
      const std::string matlabTypeName = Anki::Embedded::ConvertToMatlabTypeString(typeid(s32).name(), sizeof(s32));

      EvalStringEcho("%s=zeros([%d,%d],'%s');", name.data(), matrix.get_size(0), matrix.get_size(1), matlabTypeName.data());

      for(s32 y=0; y<matrix.get_size(0); y++) {
        Put<s32>(matrix.Pointer(y,0), matrix.get_size(1), tmpName);
        EvalStringEcho("%s(%d,:)=%s;", name.data(), y+1, tmpName.data());
      }

      EvalString("clear %s;", tmpName.data());

      return 0;
    }

    s32 Matlab::PutArray_f32(const Array_f32 &matrix, const std::string name)
    {
      if(!ep) {
        DASError("Anki.PutArray", "Matlab engine is not started/connected");
        return -1;
      }

      const std::string tmpName = name + std::string("_AnkiTMP");
      const std::string matlabTypeName = Anki::Embedded::ConvertToMatlabTypeString(typeid(f32).name(), sizeof(f32));

      EvalStringEcho("%s=zeros([%d,%d],'%s');", name.data(), matrix.get_size(0), matrix.get_size(1), matlabTypeName.data());

      for(s32 y=0; y<matrix.get_size(0); y++) {
        Put<f32>(matrix.Pointer(y,0), matrix.get_size(1), tmpName);
        EvalStringEcho("%s(%d,:)=%s;", name.data(), y+1, tmpName.data());
      }

      EvalString("clear %s;", tmpName.data());

      return 0;
    }

    s32 Matlab::PutArray_f64(const Array_f64 &matrix, const std::string name)
    {
      if(!ep) {
        DASError("Anki.PutArray", "Matlab engine is not started/connected");
        return -1;
      }

      const std::string tmpName = name + std::string("_AnkiTMP");
      const std::string matlabTypeName = Anki::Embedded::ConvertToMatlabTypeString(typeid(f64).name(), sizeof(f64));

      EvalStringEcho("%s=zeros([%d,%d],'%s');", name.data(), matrix.get_size(0), matrix.get_size(1), matlabTypeName.data());

      for(s32 y=0; y<matrix.get_size(0); y++) {
        Put<f64>(matrix.Pointer(y,0), matrix.get_size(1), tmpName);
        EvalStringEcho("%s(%d,:)=%s;", name.data(), y+1, tmpName.data());
      }

      EvalString("clear %s;", tmpName.data());

      return 0;
    }

    Array_u8 Matlab::GetArray_u8(const std::string name)
    {
      if(!ep) {
        DASError("Anki.GetArray_u8", "Matlab engine is not started/connected");
        return Array_u8(0, 0, NULL, 0, false);
      }

      const std::string tmpName = name + std::string("_AnkiTMP");

      EvalString("%s=%s';", tmpName.data(), name.data());
      mxArray *arrayTmp = GetArray(tmpName.data());

      if(!arrayTmp) {
        DASError("Anki.GetArray_u8", "%s could not be got from Matlab", tmpName.data());
        return Array_u8(0, 0, NULL, 0, false);
      }

      const mxClassID ankiVisionClassId = Anki::Embedded::ConvertToMatlabType(typeid(u8).name(), sizeof(u8));

      const mxClassID matlabClassId = mxGetClassID(arrayTmp);

      if(matlabClassId != ankiVisionClassId) {
        DASError("Anki.GetArray_u8", "matlabClassId != ankiVisionClassId");
        return Array_u8(0, 0, NULL, 0, false);
      }

      const size_t numCols = mxGetM(arrayTmp);
      const size_t numRows = mxGetN(arrayTmp);
      const s32 stride = Array_u8::ComputeRequiredStride(static_cast<s32>(numCols),false);

      Array_u8 ankiArray_u8(static_cast<s32>(numRows), static_cast<s32>(numCols), reinterpret_cast<u8*>(calloc(stride*numCols,1)), stride*static_cast<s32>(numCols), false);

      u8 *matlabArrayTmp = reinterpret_cast<u8*>(mxGetPr(arrayTmp));
      s32 matlabIndex = 0;
      for(s32 y=0; y<ankiArray_u8.get_size(0); y++) {
        u8 * rowPointer = ankiArray_u8.Pointer(y, 0);
        for(s32 x=0; x<ankiArray_u8.get_size(1); x++) {
          rowPointer[x] = matlabArrayTmp[matlabIndex++];
        }
      }

      mxDestroyArray(arrayTmp);

      EvalString("clear %s;", tmpName.data());

      return ankiArray_u8;
    }

    Array_s8 Matlab::GetArray_s8(const std::string name)
    {
      if(!ep) {
        DASError("Anki.GetArray_s8", "Matlab engine is not started/connected");
        return Array_s8(0, 0, NULL, 0, false);
      }

      const std::string tmpName = name + std::string("_AnkiTMP");

      EvalString("%s=%s';", tmpName.data(), name.data());
      mxArray *arrayTmp = GetArray(tmpName.data());

      if(!arrayTmp) {
        DASError("Anki.GetArray_s8", "%s could not be got from Matlab", tmpName.data());
        return Array_s8(0, 0, NULL, 0, false);
      }

      const mxClassID ankiVisionClassId = Anki::Embedded::ConvertToMatlabType(typeid(s8).name(), sizeof(s8));

      const mxClassID matlabClassId = mxGetClassID(arrayTmp);

      if(matlabClassId != ankiVisionClassId) {
        DASError("Anki.GetArray_s8", "matlabClassId != ankiVisionClassId");
        return Array_s8(0, 0, NULL, 0, false);
      }

      const size_t numCols = mxGetM(arrayTmp);
      const size_t numRows = mxGetN(arrayTmp);
      const s32 stride = Array_s8::ComputeRequiredStride(static_cast<s32>(numCols),false);

      Array_s8 ankiArray_s8(static_cast<s32>(numRows), static_cast<s32>(numCols), reinterpret_cast<s8*>(calloc(stride*numCols,1)), stride*static_cast<s32>(numCols), false);

      s8 *matlabArrayTmp = reinterpret_cast<s8*>(mxGetPr(arrayTmp));
      s32 matlabIndex = 0;
      for(s32 y=0; y<ankiArray_s8.get_size(0); y++) {
        s8 * rowPointer = ankiArray_s8.Pointer(y, 0);
        for(s32 x=0; x<ankiArray_s8.get_size(1); x++) {
          rowPointer[x] = matlabArrayTmp[matlabIndex++];
        }
      }

      mxDestroyArray(arrayTmp);

      EvalString("clear %s;", tmpName.data());

      return ankiArray_s8;
    }

    Array_u16 Matlab::GetArray_u16(const std::string name)
    {
      if(!ep) {
        DASError("Anki.GetArray_u16", "Matlab engine is not started/connected");
        return Array_u16(0, 0, NULL, 0, false);
      }

      const std::string tmpName = name + std::string("_AnkiTMP");

      EvalString("%s=%s';", tmpName.data(), name.data());
      mxArray *arrayTmp = GetArray(tmpName.data());

      if(!arrayTmp) {
        DASError("Anki.GetArray_u16", "%s could not be got from Matlab", tmpName.data());
        return Array_u16(0, 0, NULL, 0, false);
      }

      const mxClassID ankiVisionClassId = Anki::Embedded::ConvertToMatlabType(typeid(u16).name(), sizeof(u16));

      const mxClassID matlabClassId = mxGetClassID(arrayTmp);

      if(matlabClassId != ankiVisionClassId) {
        DASError("Anki.GetArray_u16", "matlabClassId != ankiVisionClassId");
        return Array_u16(0, 0, NULL, 0, false);
      }

      const size_t numCols = mxGetM(arrayTmp);
      const size_t numRows = mxGetN(arrayTmp);
      const s32 stride = Array_u16::ComputeRequiredStride(static_cast<s32>(numCols),false);

      Array_u16 ankiArray_u16(static_cast<s32>(numRows), static_cast<s32>(numCols), reinterpret_cast<u16*>(calloc(stride*numCols,1)), stride*static_cast<s32>(numCols), false);

      u16 *matlabArrayTmp = reinterpret_cast<u16*>(mxGetPr(arrayTmp));
      s32 matlabIndex = 0;
      for(s32 y=0; y<ankiArray_u16.get_size(0); y++) {
        u16 * rowPointer = ankiArray_u16.Pointer(y, 0);
        for(s32 x=0; x<ankiArray_u16.get_size(1); x++) {
          rowPointer[x] = matlabArrayTmp[matlabIndex++];
        }
      }

      mxDestroyArray(arrayTmp);

      EvalString("clear %s;", tmpName.data());

      return ankiArray_u16;
    }

    Array_s16 Matlab::GetArray_s16(const std::string name)
    {
      if(!ep) {
        DASError("Anki.GetArray_s16", "Matlab engine is not started/connected");
        return Array_s16(0, 0, NULL, 0, false);
      }

      const std::string tmpName = name + std::string("_AnkiTMP");

      EvalString("%s=%s';", tmpName.data(), name.data());
      mxArray *arrayTmp = GetArray(tmpName.data());

      if(!arrayTmp) {
        DASError("Anki.GetArray_s16", "%s could not be got from Matlab", tmpName.data());
        return Array_s16(0, 0, NULL, 0, false);
      }

      const mxClassID ankiVisionClassId = Anki::Embedded::ConvertToMatlabType(typeid(s16).name(), sizeof(s16));

      const mxClassID matlabClassId = mxGetClassID(arrayTmp);

      if(matlabClassId != ankiVisionClassId) {
        DASError("Anki.GetArray_s16", "matlabClassId != ankiVisionClassId");
        return Array_s16(0, 0, NULL, 0, false);
      }

      const size_t numCols = mxGetM(arrayTmp);
      const size_t numRows = mxGetN(arrayTmp);
      const s32 stride = Array_s16::ComputeRequiredStride(static_cast<s32>(numCols),false);

      Array_s16 ankiArray_s16(static_cast<s32>(numRows), static_cast<s32>(numCols), reinterpret_cast<s16*>(calloc(stride*numCols,1)), stride*static_cast<s32>(numCols), false);

      s16 *matlabArrayTmp = reinterpret_cast<s16*>(mxGetPr(arrayTmp));
      s32 matlabIndex = 0;
      for(s32 y=0; y<ankiArray_s16.get_size(0); y++) {
        s16 * rowPointer = ankiArray_s16.Pointer(y, 0);
        for(s32 x=0; x<ankiArray_s16.get_size(1); x++) {
          rowPointer[x] = matlabArrayTmp[matlabIndex++];
        }
      }

      mxDestroyArray(arrayTmp);

      EvalString("clear %s;", tmpName.data());

      return ankiArray_s16;
    }

    Array_u32 Matlab::GetArray_u32(const std::string name)
    {
      if(!ep) {
        DASError("Anki.GetArray_u32", "Matlab engine is not started/connected");
        return Array_u32(0, 0, NULL, 0, false);
      }

      const std::string tmpName = name + std::string("_AnkiTMP");

      EvalString("%s=%s';", tmpName.data(), name.data());
      mxArray *arrayTmp = GetArray(tmpName.data());

      if(!arrayTmp) {
        DASError("Anki.GetArray_u32", "%s could not be got from Matlab", tmpName.data());
        return Array_u32(0, 0, NULL, 0, false);
      }

      const mxClassID ankiVisionClassId = Anki::Embedded::ConvertToMatlabType(typeid(u32).name(), sizeof(u32));

      const mxClassID matlabClassId = mxGetClassID(arrayTmp);

      if(matlabClassId != ankiVisionClassId) {
        DASError("Anki.GetArray_u32", "matlabClassId != ankiVisionClassId");
        return Array_u32(0, 0, NULL, 0, false);
      }

      const size_t numCols = mxGetM(arrayTmp);
      const size_t numRows = mxGetN(arrayTmp);
      const s32 stride = Array_u32::ComputeRequiredStride(static_cast<s32>(numCols),false);

      Array_u32 ankiArray_u32(static_cast<s32>(numRows), static_cast<s32>(numCols), reinterpret_cast<u32*>(calloc(stride*numCols,1)), stride*static_cast<s32>(numCols), false);

      u32 *matlabArrayTmp = reinterpret_cast<u32*>(mxGetPr(arrayTmp));
      s32 matlabIndex = 0;
      for(s32 y=0; y<ankiArray_u32.get_size(0); y++) {
        u32 * rowPointer = ankiArray_u32.Pointer(y, 0);
        for(s32 x=0; x<ankiArray_u32.get_size(1); x++) {
          rowPointer[x] = matlabArrayTmp[matlabIndex++];
        }
      }

      mxDestroyArray(arrayTmp);

      EvalString("clear %s;", tmpName.data());

      return ankiArray_u32;
    }

    Array_s32 Matlab::GetArray_s32(const std::string name)
    {
      if(!ep) {
        DASError("Anki.GetArray_s32", "Matlab engine is not started/connected");
        return Array_s32(0, 0, NULL, 0, false);
      }

      const std::string tmpName = name + std::string("_AnkiTMP");

      EvalString("%s=%s';", tmpName.data(), name.data());
      mxArray *arrayTmp = GetArray(tmpName.data());

      if(!arrayTmp) {
        DASError("Anki.GetArray_s32", "%s could not be got from Matlab", tmpName.data());
        return Array_s32(0, 0, NULL, 0, false);
      }

      const mxClassID ankiVisionClassId = Anki::Embedded::ConvertToMatlabType(typeid(s32).name(), sizeof(s32));

      const mxClassID matlabClassId = mxGetClassID(arrayTmp);

      if(matlabClassId != ankiVisionClassId) {
        DASError("Anki.GetArray_s32", "matlabClassId != ankiVisionClassId");
        return Array_s32(0, 0, NULL, 0, false);
      }

      const size_t numCols = mxGetM(arrayTmp);
      const size_t numRows = mxGetN(arrayTmp);
      const s32 stride = Array_s32::ComputeRequiredStride(static_cast<s32>(numCols),false);

      Array_s32 ankiArray_s32(static_cast<s32>(numRows), static_cast<s32>(numCols), reinterpret_cast<s32*>(calloc(stride*numCols,1)), stride*static_cast<s32>(numCols), false);

      s32 *matlabArrayTmp = reinterpret_cast<s32*>(mxGetPr(arrayTmp));
      s32 matlabIndex = 0;
      for(s32 y=0; y<ankiArray_s32.get_size(0); y++) {
        s32 * rowPointer = ankiArray_s32.Pointer(y, 0);
        for(s32 x=0; x<ankiArray_s32.get_size(1); x++) {
          rowPointer[x] = matlabArrayTmp[matlabIndex++];
        }
      }

      mxDestroyArray(arrayTmp);

      EvalString("clear %s;", tmpName.data());

      return ankiArray_s32;
    }

    Array_f32 Matlab::GetArray_f32(const std::string name)
    {
      if(!ep) {
        DASError("Anki.GetArray_f32", "Matlab engine is not started/connected");
        return Array_f32(0, 0, NULL, 0, false);
      }

      const std::string tmpName = name + std::string("_AnkiTMP");

      EvalString("%s=%s';", tmpName.data(), name.data());
      mxArray *arrayTmp = GetArray(tmpName.data());

      if(!arrayTmp) {
        DASError("Anki.GetArray_f32", "%s could not be got from Matlab", tmpName.data());
        return Array_f32(0, 0, NULL, 0, false);
      }

      const mxClassID ankiVisionClassId = Anki::Embedded::ConvertToMatlabType(typeid(f32).name(), sizeof(f32));

      const mxClassID matlabClassId = mxGetClassID(arrayTmp);

      if(matlabClassId != ankiVisionClassId) {
        DASError("Anki.GetArray_f32", "matlabClassId != ankiVisionClassId");
        return Array_f32(0, 0, NULL, 0, false);
      }

      const size_t numCols = mxGetM(arrayTmp);
      const size_t numRows = mxGetN(arrayTmp);
      const s32 stride = Array_f32::ComputeRequiredStride(static_cast<s32>(numCols),false);

      Array_f32 ankiArray_f32(static_cast<s32>(numRows), static_cast<s32>(numCols), reinterpret_cast<f32*>(calloc(stride*numCols,1)), stride*static_cast<s32>(numCols), false);

      f32 *matlabArrayTmp = reinterpret_cast<f32*>(mxGetPr(arrayTmp));
      s32 matlabIndex = 0;
      for(s32 y=0; y<ankiArray_f32.get_size(0); y++) {
        f32 * rowPointer = ankiArray_f32.Pointer(y, 0);
        for(s32 x=0; x<ankiArray_f32.get_size(1); x++) {
          rowPointer[x] = matlabArrayTmp[matlabIndex++];
        }
      }

      mxDestroyArray(arrayTmp);

      EvalString("clear %s;", tmpName.data());

      return ankiArray_f32;
    }

    Array_f64 Matlab::GetArray_f64(const std::string name)
    {
      if(!ep) {
        DASError("Anki.GetArray_f64", "Matlab engine is not started/connected");
        return Array_f64(0, 0, NULL, 0, false);
      }

      const std::string tmpName = name + std::string("_AnkiTMP");

      EvalString("%s=%s';", tmpName.data(), name.data());
      mxArray *arrayTmp = GetArray(tmpName.data());

      if(!arrayTmp) {
        DASError("Anki.GetArray_f64", "%s could not be got from Matlab", tmpName.data());
        return Array_f64(0, 0, NULL, 0, false);
      }

      const mxClassID ankiVisionClassId = Anki::Embedded::ConvertToMatlabType(typeid(f64).name(), sizeof(f64));

      const mxClassID matlabClassId = mxGetClassID(arrayTmp);

      if(matlabClassId != ankiVisionClassId) {
        DASError("Anki.GetArray_f64", "matlabClassId != ankiVisionClassId");
        return Array_f64(0, 0, NULL, 0, false);
      }

      const size_t numCols = mxGetM(arrayTmp);
      const size_t numRows = mxGetN(arrayTmp);
      const s32 stride = Array_f64::ComputeRequiredStride(static_cast<s32>(numCols),false);

      Array_f64 ankiArray_f64(static_cast<s32>(numRows), static_cast<s32>(numCols), reinterpret_cast<f64*>(calloc(stride*numCols,1)), stride*static_cast<s32>(numCols), false);

      f64 *matlabArrayTmp = reinterpret_cast<f64*>(mxGetPr(arrayTmp));
      s32 matlabIndex = 0;
      for(s32 y=0; y<ankiArray_f64.get_size(0); y++) {
        f64 * rowPointer = ankiArray_f64.Pointer(y, 0);
        for(s32 x=0; x<ankiArray_f64.get_size(1); x++) {
          rowPointer[x] = matlabArrayTmp[matlabIndex++];
        }
      }

      mxDestroyArray(arrayTmp);

      EvalString("clear %s;", tmpName.data());

      return ankiArray_f64;
    }

#endif // #if defined(ANKICORETECHEMBEDDED_USE_MATLAB)
  } // namespace Embedded
} // namespace Anki