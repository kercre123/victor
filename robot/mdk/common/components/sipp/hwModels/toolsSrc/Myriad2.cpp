//--------------------------------------------
//File Name   : Myriad2.cpp
//Description : Myriad 2 archiecture specific common functions
//Author      : Alin Dobre (alin.dobre@movidius.com)
//Date        : 28.02.2013
//Updates     : (date, author, description)
//--------------------------------------------

#include <string.h>
#include "Myriad2.h"
#include "fragrakRegisters.h"

using namespace std;
//Cores: parameter specified in the command line
//Entry format in the table = { coreName, coreTag }
Myriad2::Myriad2()
{
    coresMyriad2 = new CoreMyriad2[70];
    coresMyriad2[0] = CoreMyriad2("SHAVE0", MYRIAD2_CORE_SHAVE0);
    coresMyriad2[1] = CoreMyriad2("SVU0", MYRIAD2_CORE_SHAVE0);
    coresMyriad2[2] = CoreMyriad2("SHV0", MYRIAD2_CORE_SHAVE0);
    coresMyriad2[3] = CoreMyriad2("S0", MYRIAD2_CORE_SHAVE0);
    coresMyriad2[4] = CoreMyriad2("0", MYRIAD2_CORE_SHAVE0);
    coresMyriad2[5] = CoreMyriad2("SHAVE1", MYRIAD2_CORE_SHAVE1);
    coresMyriad2[6] = CoreMyriad2("SVU1", MYRIAD2_CORE_SHAVE1);
    coresMyriad2[7] = CoreMyriad2("SHV1", MYRIAD2_CORE_SHAVE1);
    coresMyriad2[8] = CoreMyriad2("S1", MYRIAD2_CORE_SHAVE1);
    coresMyriad2[9] = CoreMyriad2("1", MYRIAD2_CORE_SHAVE1);
    coresMyriad2[10] = CoreMyriad2("SHAVE2", MYRIAD2_CORE_SHAVE2);
    coresMyriad2[11] = CoreMyriad2("SVU2", MYRIAD2_CORE_SHAVE2);
    coresMyriad2[12] = CoreMyriad2("SHV2", MYRIAD2_CORE_SHAVE2);
    coresMyriad2[13] = CoreMyriad2("S2", MYRIAD2_CORE_SHAVE2);
    coresMyriad2[14] = CoreMyriad2("2", MYRIAD2_CORE_SHAVE2);
    coresMyriad2[15] = CoreMyriad2("SHAVE3", MYRIAD2_CORE_SHAVE3);
    coresMyriad2[16] = CoreMyriad2("SVU3", MYRIAD2_CORE_SHAVE3);
    coresMyriad2[17] = CoreMyriad2("SHV3", MYRIAD2_CORE_SHAVE3);
    coresMyriad2[18] = CoreMyriad2("S3", MYRIAD2_CORE_SHAVE3);
    coresMyriad2[19] = CoreMyriad2("3", MYRIAD2_CORE_SHAVE3);
    coresMyriad2[20] = CoreMyriad2("SHAVE4", MYRIAD2_CORE_SHAVE4);
    coresMyriad2[21] = CoreMyriad2("SVU4", MYRIAD2_CORE_SHAVE4);
    coresMyriad2[22] = CoreMyriad2("SHV4", MYRIAD2_CORE_SHAVE4);
    coresMyriad2[23] = CoreMyriad2("S4", MYRIAD2_CORE_SHAVE4);
    coresMyriad2[24] = CoreMyriad2("4", MYRIAD2_CORE_SHAVE4);
    coresMyriad2[25] = CoreMyriad2("SHAVE5", MYRIAD2_CORE_SHAVE5);
    coresMyriad2[26] = CoreMyriad2("SVU5", MYRIAD2_CORE_SHAVE5);
    coresMyriad2[27] = CoreMyriad2("SHV5", MYRIAD2_CORE_SHAVE5);
    coresMyriad2[28] = CoreMyriad2("S5", MYRIAD2_CORE_SHAVE5);
    coresMyriad2[29] = CoreMyriad2("5", MYRIAD2_CORE_SHAVE5);
    coresMyriad2[30] = CoreMyriad2("SHAVE6", MYRIAD2_CORE_SHAVE6);
    coresMyriad2[31] = CoreMyriad2("SVU6", MYRIAD2_CORE_SHAVE6);
    coresMyriad2[32] = CoreMyriad2("SHV6", MYRIAD2_CORE_SHAVE6);
    coresMyriad2[33] = CoreMyriad2("S6", MYRIAD2_CORE_SHAVE6);
    coresMyriad2[34] = CoreMyriad2("6", MYRIAD2_CORE_SHAVE6);
    coresMyriad2[35] = CoreMyriad2("SHAVE7", MYRIAD2_CORE_SHAVE7);
    coresMyriad2[36] = CoreMyriad2("SVU7", MYRIAD2_CORE_SHAVE7);
    coresMyriad2[37] = CoreMyriad2("SHV7", MYRIAD2_CORE_SHAVE7);
    coresMyriad2[38] = CoreMyriad2("S7", MYRIAD2_CORE_SHAVE7);
    coresMyriad2[39] = CoreMyriad2("7", MYRIAD2_CORE_SHAVE7);
    coresMyriad2[40] = CoreMyriad2("SHAVE8", MYRIAD2_CORE_SHAVE8);
    coresMyriad2[41] = CoreMyriad2("SVU8", MYRIAD2_CORE_SHAVE8);
    coresMyriad2[42] = CoreMyriad2("SHV8", MYRIAD2_CORE_SHAVE8);
    coresMyriad2[43] = CoreMyriad2("S8", MYRIAD2_CORE_SHAVE8);
    coresMyriad2[44] = CoreMyriad2("8", MYRIAD2_CORE_SHAVE8);
    coresMyriad2[45] = CoreMyriad2("SHAVE9", MYRIAD2_CORE_SHAVE9);
    coresMyriad2[46] = CoreMyriad2("SVU9", MYRIAD2_CORE_SHAVE9);
    coresMyriad2[47] = CoreMyriad2("SHV9", MYRIAD2_CORE_SHAVE9);
    coresMyriad2[48] = CoreMyriad2("S9", MYRIAD2_CORE_SHAVE9);
    coresMyriad2[49] = CoreMyriad2("9", MYRIAD2_CORE_SHAVE9);
    coresMyriad2[50] = CoreMyriad2("SHAVE10", MYRIAD2_CORE_SHAVE10);
    coresMyriad2[51] = CoreMyriad2("SVU10", MYRIAD2_CORE_SHAVE10);
    coresMyriad2[52] = CoreMyriad2("SHV10", MYRIAD2_CORE_SHAVE10);
    coresMyriad2[53] = CoreMyriad2("S10", MYRIAD2_CORE_SHAVE10);
    coresMyriad2[54] = CoreMyriad2("10", MYRIAD2_CORE_SHAVE10);
    coresMyriad2[55] = CoreMyriad2("SHAVE11", MYRIAD2_CORE_SHAVE11);
    coresMyriad2[56] = CoreMyriad2("SVU11", MYRIAD2_CORE_SHAVE11);
    coresMyriad2[57] = CoreMyriad2("SHV11", MYRIAD2_CORE_SHAVE11);
    coresMyriad2[58] = CoreMyriad2("S11", MYRIAD2_CORE_SHAVE11);
    coresMyriad2[59] = CoreMyriad2("11", MYRIAD2_CORE_SHAVE11);
    coresMyriad2[60] = CoreMyriad2("LEONOS", MYRIAD2_CORE_LEON_OS);
    coresMyriad2[61] = CoreMyriad2("LOS", MYRIAD2_CORE_LEON_OS);
    coresMyriad2[62] = CoreMyriad2("L", MYRIAD2_CORE_LEON_OS);
    coresMyriad2[63] = CoreMyriad2("LEONRT", MYRIAD2_CORE_LEON_RT);
    coresMyriad2[64] = CoreMyriad2("LRT", MYRIAD2_CORE_LEON_RT);
    coresMyriad2[65] = CoreMyriad2("GLOBAL", MYRIAD2_CORE_GLOBAL);
    coresMyriad2[66] = CoreMyriad2("G", MYRIAD2_CORE_GLOBAL);
    coresMyriad2[67] = CoreMyriad2("ANYSHAVE", MYRIAD2_CORE_ANY_SHAVE);
    coresMyriad2[68] = CoreMyriad2("UNASSIGNED", MYRIAD2_CORE_UNASSIGNED);
    coresMyriad2[69] = CoreMyriad2("END", 0);

    numberOfEntryPoints = 14;
}

Myriad2::~Myriad2()
{

}
//Tokenize the specified core name
unsigned int Myriad2::GetCoreToken(char *coreName)
{
    int currentIndex = MYRIAD2_CORE_SHAVE0;
    CoreMyriad2 coreMyriad2;
    char savedcoreName[100];
    strcpy(savedcoreName, coreName);
    char* nameStr = &savedcoreName[0];
    while(*nameStr)
    {
        *nameStr =(char)toupper(*nameStr);
        nameStr ++;
    }
    do
    {
        coreMyriad2 = coresMyriad2[currentIndex];
        if(strcmp(coreMyriad2.coreName, coreName) == 0)
        {
            return coreMyriad2.coreTag;
        }
        currentIndex ++;
    }while(strcmp(coreMyriad2.coreName, "END") != 0);
    return MYRIAD2_CORE_UNASSIGNED;
}
#pragma region Detect address ranges
bool Myriad2::IsCmxAddress(unsigned int address)
{
    if((address & MYRIAD2_CMX_ADDRESS_MASK) == MYRIAD2_CMX_ADDRESS_MXI)
    {
        return true;
    }
    return false;
}

bool Myriad2::IsDdrAddress(unsigned int address)
{
    if((address & MYRIAD2_DDR_ADDRESS_MASK) == MYRIAD2_DDR_ADDRESS)
    {
        return true;
    }
    return false;
}

bool Myriad2::IsLramAddress(unsigned int address)
{
    return false;
}

bool Myriad2::IsL2Address(unsigned int address)
{
    if((address & MYRIAD2_L2_ADDRESS_MASK) == MYRIAD2_L2_ADDRESS)
    {
        return true;
    }
    return false;
}

bool Myriad2::IsWindowedAddress(unsigned int address)
{
    if((address & MYRIAD2_WINDOW_MASK) == MYRIAD2_WINDOW_ADDRESS)
    {
        return true;
    }
    return false;
}

bool Myriad2::IsCmxBlock(unsigned int address, unsigned int size)
{
    if(!IsCmxAddress(address))
    {
        return false;
    }
    if(((address & 0x00FFFFFF) + size) > MYRIAD2_CMX_SIZE)
    {
        return false;
    }
    return true;
}

bool Myriad2::IsConfigSection(unsigned int address)
{
    if((IsCmxAddress(address)) ||
        (IsWindowedAddress(address)) ||
        (IsDdrAddress(address)) ||
        (IsL2Address(address)))
    {
        return false;
    }
    return true;
}

bool Myriad2::IsShaveWindowConfig(unsigned int address)
{
    if (((address & 0xFFFF0000) >= MYRIAD2_SHAVE0_ADDR) && ((address & 0xFFFF0000) <= MYRIAD2_SHAVE11_ADDR))
    {
        if((address & 0x0000FFFF) == MYRIAD2_SHAVE_WIN_CONFIG)
        {
            return true;
        }
    }
    return false;
}

int Myriad2::GetShaveNumberFromWinConfig(unsigned int address)
{
    unsigned int shaveNumber = MYRIAD2_CORE_UNASSIGNED;
    if (((address & 0xFFFF0000) >= MYRIAD2_SHAVE0_ADDR) && ((address & 0xFFFF0000) <= MYRIAD2_SHAVE11_ADDR))
    {
        if((address & 0x0000FFFF) == MYRIAD2_SHAVE_WIN_CONFIG)
        {
            unsigned int number = (((address & 0x000F0000) >> 16) & 0xF);
            if((number >= 0) && (number < 12))
            {
                shaveNumber = number;
            }
        }
    }
    return shaveNumber;
}
#pragma endregion
#pragma region GetCoreName
string Myriad2::GetCoreName(unsigned int coreID)
{
    switch(coreID)
    {
        case MYRIAD2_CORE_SHAVE0:
            return "SHAVE0";
        case MYRIAD2_CORE_SHAVE1:
            return "SHAVE1";
        case MYRIAD2_CORE_SHAVE2:
            return "SHAVE2";
        case MYRIAD2_CORE_SHAVE3:
            return "SHAVE3";
        case MYRIAD2_CORE_SHAVE4:
            return "SHAVE4";
        case MYRIAD2_CORE_SHAVE5:
            return "SHAVE5";
        case MYRIAD2_CORE_SHAVE6:
            return "SHAVE6";
        case MYRIAD2_CORE_SHAVE7:
            return "SHAVE7";
        case MYRIAD2_CORE_SHAVE8:
            return "SHAVE8";
        case MYRIAD2_CORE_SHAVE9:
            return "SHAVE9";
        case MYRIAD2_CORE_SHAVE10:
            return "SHAVE10";
        case MYRIAD2_CORE_SHAVE11:
            return "SHAVE11";
        case MYRIAD2_CORE_LEON_OS:
            return "LEONOS";
        case MYRIAD2_CORE_LEON_RT:
            return "LEONRT";
        case MYRIAD2_CORE_GLOBAL:
            return "GLOBAL";
        case MYRIAD2_CORE_ALL_SHAVES:
            return "ALLSHAVES";
        case MYRIAD2_CORE_ANY_SHAVE:
            return "ANYSHAVE";
        case MYRIAD2_CORE_UNASSIGNED:
            return "UNASSIGNED";
        default:
            break;
    }
    return "INVALIDCORE";
}
#pragma endregion
#pragma region GetShortCoreName
string Myriad2::GetShortCoreName(unsigned int coreID)
{
    switch(coreID)
    {
        case MYRIAD2_CORE_SHAVE0:
            return "S0";
        case MYRIAD2_CORE_SHAVE1:
            return "S1";
        case MYRIAD2_CORE_SHAVE2:
            return "S2";
        case MYRIAD2_CORE_SHAVE3:
            return "S3";
        case MYRIAD2_CORE_SHAVE4:
            return "S4";
        case MYRIAD2_CORE_SHAVE5:
            return "S5";
        case MYRIAD2_CORE_SHAVE6:
            return "S6";
        case MYRIAD2_CORE_SHAVE7:
            return "S7";
        case MYRIAD2_CORE_SHAVE8:
            return "S8";
        case MYRIAD2_CORE_SHAVE9:
            return "S9";
        case MYRIAD2_CORE_SHAVE10:
            return "S10";
        case MYRIAD2_CORE_SHAVE11:
            return "S11";
        case MYRIAD2_CORE_LEON_OS:
            return "LOS";
        case MYRIAD2_CORE_LEON_RT:
            return "LRT";
        case MYRIAD2_CORE_GLOBAL:
            return "G";
        case MYRIAD2_CORE_ALL_SHAVES:
            return "SA";
        case MYRIAD2_CORE_ANY_SHAVE:
            return "AS";
        case MYRIAD2_CORE_UNASSIGNED:
            return "U";
        default:
            break;
    }
    return "I ";
}
#pragma endregion
#pragma region Get Core Symbol Prefix
string Myriad2::GetCoreSymbolPrefix(unsigned int coreID)
{
    switch(coreID)
    {
        case MYRIAD2_CORE_SHAVE0:
            return "SVE0";
        case MYRIAD2_CORE_SHAVE1:
            return "SVE1";
        case MYRIAD2_CORE_SHAVE2:
            return "SVE2";
        case MYRIAD2_CORE_SHAVE3:
            return "SVE3";
        case MYRIAD2_CORE_SHAVE4:
            return "SVE4";
        case MYRIAD2_CORE_SHAVE5:
            return "SVE5";
        case MYRIAD2_CORE_SHAVE6:
            return "SVE6";
        case MYRIAD2_CORE_SHAVE7:
            return "SVE7";
        case MYRIAD2_CORE_SHAVE8:
            return "SVE8";
        case MYRIAD2_CORE_SHAVE9:
            return "SVE9";
        case MYRIAD2_CORE_SHAVE10:
            return "SVE10";
        case MYRIAD2_CORE_SHAVE11:
            return "SVE11";
        case MYRIAD2_CORE_LEON_OS:
            return "LEONOS";
        case MYRIAD2_CORE_LEON_RT:
            return "LEONRT";
        case MYRIAD2_CORE_GLOBAL:
            return "GLOBAL";
        default:
            break;
    }
    return "UNASSIGNED";
}
#pragma endregion

unsigned int Myriad2::GetNumberOfEntryPoints()
{
    return numberOfEntryPoints;
}

unsigned int Myriad2::GetLowShaveToken()
{
    return MYRIAD2_CORE_SHAVE0;
}

unsigned int Myriad2::GetHighShaveToken()
{
    return MYRIAD2_CORE_SHAVE11;
}

bool Myriad2::IsLeon(unsigned int token)
{
    bool retVal = false;
    if ((token == MYRIAD2_CORE_LEON_OS) || (token == MYRIAD2_CORE_LEON_RT))
    {
        retVal = true;
    }
    return retVal;
}

unsigned int Myriad2::GetDefaultEntryPoint(unsigned int token)
{
    unsigned int retVal = 0;
    if ((token == MYRIAD2_CORE_LEON_OS) || (token == MYRIAD2_CORE_LEON_RT))
    {
        retVal = 0x70000000;
    }
    else
    {
        retVal = 0x1D000000;
    }
    return retVal;
}

unsigned int Myriad2::GetDefaultLocalCodeAddress()
{
    return MYRIAD2_LOCAL_CODE_DEFAULT_ADDRESS;
}

unsigned int Myriad2::GetDefaultLocalDataAddress()
{
    return MYRIAD2_LOCAL_DATA_DEFAULT_ADDRESS;
}

unsigned int Myriad2::GetNumberOfShaves()
{
    return MYRIAD2_NUMBER_OF_SHAVES;
}

unsigned int Myriad2::GetCmxConfigAddress()
{
    return MYRIAD2_CMX_CONFIG_ADDR;
}

unsigned int Myriad2::GetL2ModeAddress()
{
    return MYRIAD2_L2C_MODE_ADR;
}

void Myriad2::UpdateWindowRegisters(WindowRegisters* shaveWinRegs, unsigned int addr, vector<char> data)
{
    unsigned int shaveIndex = (addr & 0xF0000) >> 16;
    if ((shaveIndex >= MYRIAD2_NUMBER_OF_SHAVES) || (!IsShaveWindowConfig(addr)))
    {
        return;
    }
    unsigned int dataSize = (unsigned int)data.size();
    char* dataPtr = new char[dataSize];
    //get data
    for (unsigned int index = 0; index < dataSize; index++)
    {
        dataPtr[index] = data[index];
    }
    //set register values
    if (dataSize >= 4)
    {
        //set winC
        memcpy((void*)(&shaveWinRegs[shaveIndex].window[0]), (void*)dataPtr, 4);
    }
    if ((dataSize >= 8))
    {
        //set winD
         memcpy((void*)(&shaveWinRegs[shaveIndex].window[1]), (void*)(dataPtr + 4), 4);
    }
    if (dataSize >= 12)
    {
        //set winE
         memcpy((void*)(&shaveWinRegs[shaveIndex].window[2]), (void*)(dataPtr + 8), 4);
    }
    if ((dataSize >= 16))
    {
        //set winF
         memcpy((void*)(&shaveWinRegs[shaveIndex].window[3]), (void*)(dataPtr + 12), 4);
    }
    delete [] dataPtr;
}

unsigned int Myriad2::GetDdrOffset(unsigned int addr)
{
     return addr - MYRIAD2_DDR_ADDRESS;
}

unsigned int Myriad2::GetCMXBase()
{
    return MYRIAD2_CMX_ADDRESS_LHB;
}

unsigned int Myriad2::GetCMXSize()
{
    return MYRIAD2_CMX_SIZE;
}

unsigned int Myriad2::GetDDRBase()
{
    return MYRIAD2_DDR_BASE_ADR;
}

unsigned int Myriad2::GetDDRSize()
{
    return MYRIAD2_DDR_SIZE;
}

unsigned int Myriad2::GetWindowRegistersBase(unsigned int shaveIndex)
{
    return MYRIAD2_SHAVE0_ADDR + MYRIAD2_SHAVE_OFFSET * shaveIndex + MYRIAD2_SHAVE_WIN_CONFIG;
}

bool Myriad2::IsValidCore(unsigned int token)
{
    bool retVal = true;
    if ((token > MYRIAD2_CORE_LEON_RT))
    {
        retVal = false;
    }
    return retVal;
}

unsigned int Myriad2::getAddressDCR(unsigned int core)
{
    return MYRIAD2_SVU_DCR(core);
}

unsigned int Myriad2::getDSUControlRegAddress(unsigned int core)
{
    if (core == MYRIAD2_CORE_LEON_OS)
    {
        return MYRIAD2_LEON_OS_DSU_CTRL_ADR;
    }
    else if (core == MYRIAD2_CORE_LEON_RT)
    {
        return MYRIAD2_LEON_RT_DSU_CTRL_ADR;
    }
    else
    {
        return MYRIAD2_SVU_DCU(core);
    }
}

unsigned int Myriad2::getCPRClkEn0Addr()
{
    return MYRIAD2_CPR_CLK_EN0_ADR;
}

unsigned int Myriad2::getCPRClkEn1Addr()
{
    return MYRIAD2_CPR_CLK_EN1_ADR;
}

bool Myriad2::IsShaveRegisterAddress(unsigned int address)
{
    if ((address >= MYRIAD2_SVU_BASE_ADDRESS) && (address < (MYRIAD2_SVU_BASE_ADDRESS + MYRIAD2_NUMBER_OF_SHAVES * MYRIAD2_SVU_SLICE_OFFSET)))
    {
        return true;
    }
    return false;
}

bool Myriad2::IsMemoryAddress(unsigned int address)
{
    bool retVal = false;
    if (IsCmxAddress(address) || IsDdrAddress(address) || IsWindowedAddress(address))
    {
        retVal = true;
    }
    return retVal;
}

int Myriad2::GetNumberOfProcessors()
{
    return MYRIAD2_CORES_NUMBER;
}
