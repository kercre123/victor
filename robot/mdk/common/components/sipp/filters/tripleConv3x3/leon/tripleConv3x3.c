#include <sipp.h>
#include <sippMacros.h>
#include <filters/conv3x3/conv3x3.h>
#include <filters/tripleConv3x3/tripleConv3x3.h>

//##########################################################################################
// Here just define the filters that make up the aggregate filter and build internal links
// the links to outer system must be build at top-level
TripleConv3x3* createTripleConv3x3(SippPipeline *pl, UInt32 width, UInt32 height)
{
    TripleConv3x3 *ret = (TripleConv3x3 *)sippMemAlloc(mempool_sipp, sizeof(TripleConv3x3));

    ret->c1 = sippCreateFilter(pl, 0x00, width, height, N_PL(1), SZ(UInt8), SZ(Conv3x3Param), SVU_SYM(svuConv3x3), 0);
    ret->c2 = sippCreateFilter(pl, 0x00, width, height, N_PL(1), SZ(UInt8), SZ(Conv3x3Param), SVU_SYM(svuConv3x3), 0);
    ret->c3 = sippCreateFilter(pl, 0x00, width, height, N_PL(1), SZ(UInt8), SZ(Conv3x3Param), SVU_SYM(svuConv3x3), 0);

    sippLinkFilter(ret->c2, ret->c1, 3, 3);
    sippLinkFilter(ret->c3, ret->c2, 3, 3);

    return ret;
}
