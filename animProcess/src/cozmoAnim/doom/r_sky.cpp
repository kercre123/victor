// Needed for FRACUNIT.
#include "cozmoAnim/doom/m_fixed.h"

// Needed for Flat retrieval.
#include "cozmoAnim/doom/r_data.h"

#include "cozmoAnim/doom/r_sky.h"

//
// sky mapping
//
int			skyflatnum;
int			skytexture;
int			skytexturemid;



//
// R_InitSkyMap
// Called whenever the view size changes.
//
void R_InitSkyMap (void)
{
  // skyflatnum = R_FlatNumForName ( SKYFLATNAME );
    skytexturemid = 100*FRACUNIT;
}

