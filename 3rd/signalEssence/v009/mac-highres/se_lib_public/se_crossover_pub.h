/**  Public definition of 
 *   EQ and Crossover configuration.
 *   Actual data is contained in a project specific directory.
 */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef SE_CROSSOVER_PUB
#define SE_CROSSOVER_PUB
#include "se_types.h"

//
// see notes about these config parameters in se_crossover.c::SeCrossoverInit
typedef struct {
    int32   num_outputs;  // Number of output channels in the EQ/Crossover.
    int16 **coef_array_per_chan;
    int32  *ntaps_per_chan;   // A List of the number of coefficients, per filter. Specify 0 for pass-thru.
} SeCrossoverConfig_t;
extern SeCrossoverConfig_t crossoverConfig;

#endif // SE_CROSSOVER_PUB

#ifdef __cplusplus
}
#endif
