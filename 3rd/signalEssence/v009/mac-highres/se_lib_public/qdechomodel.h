#ifdef __cplusplus
extern "C" {
#endif

/* (C) Copyright 2012 Signal Essence LLC; All Rights Reserved

 Module Name  - qdechomodel.h

 Author: Hugh McLaughlin

 Description:

     quick, not so dirty, full band echo modelling function.
 
 History:    hjm - Hugh McLaughlin

    08-14-12 hjm created

 Machine/Compiler: ANSI C 

**************************************************************************/

#ifndef ___qdechomodel_h
#define ___qdechomodel_h

#include "se_types.h"


typedef struct 
{
    // configuration parameters
    int16    Alpha;                // direct, holdover coefficient
    int16    Beta;                 // indirect, leak coefficient
    int16    OneMinusBeta;         // 1 - Beta
    int16    IndirectFactor_q15;
    float    IndirectErleLogFactor;
    int16    GErleMin_q12;

    // variables, states
    int32    RefState;
    int32    IndirectState;

    // echo states
    int32 DirectResidualEcho;
    int32 IndirectResidualEcho;
    int32 EchoEst;
} QdEchoModel_t;


/*=======================================================================
  Function Name:   InitFreqDomainEchoModeling()

  Description:     Initializes the frequency domain echo modelling algorithm.  
  
  Returns:    0 if successful    

  Notes:      Must allocate SenrPrivateObj prior to calling.
======================================================================*/

void InitQdEchoModeling( QdEchoModel_t *sp,
                         int16 modelIndex );   // ECHO_MODEL_HANDHELD = 0, ECHO_MODEL_DESKTOP_VIDEO = 1, ECHO_MODEL_BIG_BOARD_SHORT_AEC = 2,

/*=======================================================================
  Function Name:   DoQdEchoModeling()

  Description:  see above description in the header

  Returns:    void         
======================================================================*/

int32 DoQdEchoModeling( 
            QdEchoModel_t *sp,
            int16 gErl_q12,
            int16 gErle_q12,
            int16 padFactor_q10,
            int32 routPower );


#endif

#ifdef __cplusplus
}
#endif

