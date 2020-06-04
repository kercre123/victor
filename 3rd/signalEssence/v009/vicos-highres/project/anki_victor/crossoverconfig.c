/** Crossover and equalization for the loudspeaker. */
#include "se_crossover_pub.h"
#include "mmglobalsizes.h"

//
// number of output channels in receive path rout
#define ROUT_EQ_CROSSOVER_NUM_CHANS  1

int16  *ppEqCrossoverCoefs[3]   = {0, 0, 0};  // the 0 channel will simply be a pass-through.
int32   pEqCrossoverNumCoefs[3] = {0, 0, 0};

SeCrossoverConfig_t crossoverConfig = {
    ROUT_EQ_CROSSOVER_NUM_CHANS,
    ppEqCrossoverCoefs, 
    pEqCrossoverNumCoefs, 
};
