///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Implemenation of stdio puts function
/// 
/// Implemented in a platform independent way using externally defined putchar function
/// 

// 1: Includes
// ----------------------------------------------------------------------------

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------


// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------

// 4: Static Local Data 
// ----------------------------------------------------------------------------

// 6: Functions Implementation
// ----------------------------------------------------------------------------

int systemPuts(int (*systemPutCharFn)(int character) ,const char * str)
{
    int i=0;
    while (str[i] != 0)
        (void)systemPutCharFn(str[i++]);
    return 0;
}
