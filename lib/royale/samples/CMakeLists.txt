#****************************************************************************
# Copyright (C) 2018 pmdtechnologies ag & Infineon Technologies
#
# THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
# KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
# PARTICULAR PURPOSE.
#
#****************************************************************************

cmake_minimum_required(VERSION 2.8)

project (royale_samples)

# C++ samples
add_subdirectory(cpp/sampleCameraInfo)
add_subdirectory(cpp/sampleRetrieveData)
add_subdirectory(cpp/sampleOpenCV)
add_subdirectory(cpp/sampleSlave)
add_subdirectory(cpp/sampleExportPLY)
add_subdirectory(cpp/sampleOpenNI2Driver)
add_subdirectory(cpp/sampleProcessingParameters)
add_subdirectory(cpp/sampleRawData)
add_subdirectory(cpp/sampleRecordRRF)
add_subdirectory(cpp/sampleMultiCamera)

# C sample
add_subdirectory(c/sampleCAPI)

# C# sample
if(MSVC)
    add_subdirectory(csharp/sampleDotNetCamInfo)
    add_subdirectory(csharp/sampleDotNetRetrieveData)
endif()
