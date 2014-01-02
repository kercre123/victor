#include <sipp.h>


//define custom filters param if needed
// ...none in this case...

//define the aggregate Filter that contains 3 chained convolutions
  typedef struct
  {
     SippFilter *c1; //1st convolution
     SippFilter *c2; //2nd convolution
     SippFilter *c3; //3rd convolution
  }TripleConv3x3;


//define a constructor 
 TripleConv3x3* createTripleConv3x3(SippPipeline *pl, UInt32 width, UInt32 height);
 
