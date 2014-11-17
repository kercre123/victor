#if !defined(__APPLE_CC__)
#error This file only builds in XCode
#endif

//typedef unsigned short char16_t;
typedef unsigned char u8;

#include <mex.h>

#include <cmath>

#define USE_PERSISTENT_MEMORY 0

#define USE_DOUBLE_PRECISION 0

#if USE_DOUBLE_PRECISION
typedef double Precision;
#else
typedef float Precision;
#endif
        

#if USE_PERSISTENT_MEMORY     
static PRECISION* markerProb_on  = NULL;
static PRECISION* markerProb_off = NULL;
static PRECISION* sum_on         = NULL;
static PRECISION* sum_off        = NULL;
static PRECISION* p_on           = NULL;

static mwSize numProbesCached=0, numExamplesCached=0, numLabelsCached=0;

void freeMemory(void)
{
    mexPrintf("Freeing mexComputeInfoGain memory.\n");
    if(markerProb_on != NULL) {
        mxFree(markerProb_on);
        markerProb_on = NULL;
    }
    
    if(markerProb_off != NULL) {
        mxFree(markerProb_off);
        markerProb_off = NULL;
    }
    
    if(sum_on != NULL) {
        mxFree(sum_on);
        sum_on = NULL;
    }
    
    if(sum_off != NULL) {
        mxFree(sum_off);
        sum_off = NULL;
    }
    
    if(p_on != NULL) {
        mxFree(p_on);
        p_on = NULL;
    }
}
#endif // USE_PERSISTENT_MEMORY 


void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
#if USE_PERSISTENT_MEMORY
    mexAtExit(freeMemory);
#endif
        
    /*
    function infoGain = computeInfoGain(labels, numLabels, probeValues)
    
    % Since we are just looking for max info gain, we don't actually need to
     * % compute the currentEntropy, since it's the same for all probes.  We just
     * % need the probe with the lowest conditional entropy, since that will be
     * % the one with the highest infoGain
     * %markerProb = max(eps,hist(labels, 1:numLabels));
     * %markerProb = markerProb/max(eps,sum(markerProb));
     * %currentEntropy = -sum(markerProb.*log2(markerProb));
     * currentEntropy = 0;
     *
     * numProbes = size(probeValues,1);
     *
     * % Use the (blurred) image values directly as indications of the
     * % probability a probe is "on" or "off" instead of thresholding.
     * % The hope is that this discourages selecting probes near
     * % edges, which will be gray ("uncertain"), instead of those far
     * % from any edge in any image, since those will still be closer
     * % to black or white even after blurring.
     *
     * probeIsOff = probeValues;
     * probeIsOn  = 1 - probeValues;
     * % probeIsOn  = max(0, 1 - 2*probeValues);
     * % probeIsOff = max(0, 2*probeValues - 1);
     *
     * markerProb_on  = zeros(numProbes,numLabels);
     * markerProb_off = zeros(numProbes,numLabels);
     *
     * for i_probe = 1:numProbes
     * markerProb_on(i_probe,:)  = max(eps, accumarray(labels(:), probeIsOn(i_probe,:)', [numLabels 1])');
     * markerProb_off(i_probe,:) = max(eps, accumarray(labels(:), probeIsOff(i_probe,:)', [numLabels 1])');
     * end
     *
     * markerProb_on  = markerProb_on  ./ (sum(markerProb_on,2)*ones(1,numLabels));
     * markerProb_off = markerProb_off ./ (sum(markerProb_off,2)*ones(1,numLabels));
     *
     * p_on = sum(probeIsOn,2)/size(probeIsOn,2);
     * p_off = 1 - p_on;
     *
     * conditionalEntropy = - (p_on.*sum(markerProb_on.*log2(max(eps,markerProb_on)), 2) + ...
     * p_off.*sum(markerProb_off.*log2(max(eps,markerProb_off)), 2));
     *
     * infoGain = currentEntropy - conditionalEntropy;
     **/
    if(mxGetClassID(prhs[0]) != mxUINT32_CLASS) {
        mexErrMsgTxt("labelsArray should be UINT32.");
    }
    
    if(mxGetClassID(prhs[2]) != mxUINT8_CLASS) {
        mexErrMsgTxt("probeValues should be UINT8.");
    }
    
#if USE_DOUBLE_PRECISION
    if(mxGetClassID(prhs[3]) != mxDOUBLE_CLASS) {
        mexErrMsgTxt("weights should be DOUBLE.");
    }
#else    
    if(mxGetClassID(prhs[3]) != mxSINGLE_CLASS) {
        mexErrMsgTxt("weights should be SINGLE.");
    }
#endif
    
    if(mxGetNumberOfElements(prhs[1]) != 1) {
        mexErrMsgTxt("numLabels should be a scalar.");
    }
    
    const Precision eps = 1e-9;
    
    const mxArray* labelsArray = prhs[0];
    const mwSize   numLabels   = static_cast<mwSize>(mxGetScalar(prhs[1]));
    const mxArray* probeValues = prhs[2];
    
    const mwSize numProbes   = mxGetN(probeValues);
    const mwSize numExamples = mxGetM(probeValues);
    
    //const Precision oneOverNumExamples = 1. / static_cast<Precision>(numExamples);
    const double oneOver255 = 1. / 255.;
    
    const unsigned int* labels = (unsigned int*) mxGetData(labelsArray);
    if(mxGetNumberOfElements(labelsArray) != numExamples) {
        mexErrMsgTxt("Number of elements in labels should match number of rows in probeValues.");
    }
    
    const mxArray* weightsArray = prhs[3];
    if(mxGetNumberOfElements(weightsArray) != numExamples) {
        mexErrMsgTxt("Number of weights should match number of rows in probeValues.");
    }
    
    
#if USE_PERSISTENT_MEMORY
    if(numProbes != numProbesCached || numExamples != numExamplesCached || numLabels != numLabelsCached) {
        mexPrintf("Allocating memory for mexComputeInfoGain.\n");
        numProbesCached   = numProbes;
        numExamplesCached = numExamples;
        numLabelsCached   = numLabels;
        mexMakeMemoryPersistent(&numProbesCached);
        mexMakeMemoryPersistent(&numExamplesCached);
        mexMakeMemoryPersistent(&numLabelsCached);
                        
        freeMemory();
        
        markerProb_on  = (Precision*) mxMalloc(numLabels*numProbes*sizeof(Precision));
        mexMakeMemoryPersistent(markerProb_on);
        
        markerProb_off = (Precision*) mxMalloc(numLabels*numProbes*sizeof(Precision));
        mexMakeMemoryPersistent(markerProb_off);
        
        sum_on  = (Precision*) mxMalloc(numProbes*sizeof(Precision));
        mexMakeMemoryPersistent(sum_on);
        
        sum_off = (Precision*) mxMalloc(numProbes*sizeof(Precision));
        mexMakeMemoryPersistent(sum_off);
        
        p_on = (Precision*) mxMalloc(numProbes*sizeof(Precision));
        mexMakeMemoryPersistent(p_on);
    } else {
        mexPrintf("Re-using existing mexComputeInfoGain allocation.\n");
        mxAssert(markerProb_on  != NULL, "markerProb_on should not be NULL.");
        mxAssert(markerProb_off != NULL, "markerProb_off should not be NULL.");
        mxAssert(sum_on         != NULL, "sum_on should not be NULL.");
        mxAssert(sum_off        != NULL, "sum_off should not be NULL.");
        mxAssert(p_on           != NULL, "p_on should not be NULL.");        
    }
#else
    
    Precision* markerProb_on  = (Precision*) mxMalloc(numLabels*numProbes*sizeof(Precision));
        
    Precision* markerProb_off = (Precision*) mxMalloc(numLabels*numProbes*sizeof(Precision));
    
    Precision* sum_on  = (Precision*) mxMalloc(numProbes*sizeof(Precision));
    
    Precision* sum_off = (Precision*) mxMalloc(numProbes*sizeof(Precision));
    
    Precision* p_off = (Precision*) mxMalloc(numProbes*sizeof(Precision));
    
#endif // USE_PERSISTENT_MEMORY
    
    
            
    // Initialize with small values to avoid zero probabilities (and log of zero)
    for(mwSize i=0; i<numLabels*numProbes; ++i) {
        markerProb_on[i]  = eps;
        markerProb_off[i] = eps;
    }
    
    const Precision * weights = (const Precision*)mxGetData(weightsArray);
    Precision oneOverTotalWeight = eps;
    for(mwSize iExample=0; iExample<numExamples; ++iExample) {
        oneOverTotalWeight += weights[iExample]; // initially store total weight...
    }
    oneOverTotalWeight = Precision(1) / oneOverTotalWeight; // ... then invert.
    
    for(mwSize iProbe=0; iProbe<numProbes; ++iProbe) {
        //const Precision * currentProbe  = (const Precision*)mxGetData(probeValues) + iProbe*numExamples;
        const u8* currentProbe = static_cast<u8*>(mxGetData(probeValues)) + iProbe*numExamples;
        
        Precision * currentMarkerProb_on  = markerProb_on  + iProbe*numLabels;
        Precision * currentMarkerProb_off = markerProb_off + iProbe*numLabels;
           
        sum_on[iProbe]  = eps;
        sum_off[iProbe] = eps;
        p_off[iProbe]   = eps;
        
        // Accumulate probes into the right place based on each example's label
        for(mwSize iExample=0; iExample<numExamples; ++iExample) {
            const unsigned int label = labels[iExample]; 
            const double currentProbeVal = static_cast<double>(currentProbe[iExample])*oneOver255;
            
            //const Precision probeIsOn  = Precision(1) - currentProbe[iExample];
            //const Precision probeIsOff = (Precision)(currentProbe[iExample] >  Precision(0.5));
            //const Precision probeIsOn  = (Precision)(currentProbe[iExample] <= Precision(0.5));
          
            if( (Precision)(currentProbeVal <= Precision(0.5)) ) {
                currentMarkerProb_on[label] += weights[iExample];
                sum_on[iProbe] += weights[iExample];
            } else {
                currentMarkerProb_off[label] += weights[iExample];
                sum_off[iProbe] += weights[iExample];
            }
                
            p_off[iProbe] += currentProbeVal * weights[iExample];
            
            //currentMarkerProb_on[label]  += probeIsOn;
            //currentMarkerProb_off[label] += probeIsOff;
            
            // Keep track of per-probe totals for normalization
            //sum_on[iProbe]  += p_on[iProbe];
            //sum_off[iProbe] += probeIsOff;
        }
     
        p_off[iProbe] *= oneOverTotalWeight;
    }
    
    
    // Compute conditional entropy of each probe.  We'll just store infoGain as 
    // negative conditional entropy, since we are ignoring (constant) current
    // entropy.
#if USE_DOUBLE_PRECISION
    plhs[0] = mxCreateNumericMatrix(numProbes, 1, mxDOUBLE_CLASS, mxREAL);
#else
    plhs[0] = mxCreateNumericMatrix(numProbes, 1, mxSINGLE_CLASS, mxREAL);
#endif
    Precision* infoGain = (Precision*) mxGetData(plhs[0]);
    for(mwSize iProbe=0; iProbe<numProbes; ++iProbe) {        
        const Precision * currentMarkerProb_on  = markerProb_on  + iProbe*numLabels;
        const Precision * currentMarkerProb_off = markerProb_off + iProbe*numLabels;
             
        const Precision normalizer_on  = Precision(1) / sum_on[iProbe];
        const Precision normalizer_off = Precision(1) / sum_off[iProbe];
        
        // Accumulate probes into the right place based on each example's label
        Precision onTerm = eps, offTerm = eps;
        for(mwSize iLabel=0; iLabel<numLabels; ++iLabel) {
            const Precision probOn  = currentMarkerProb_on[iLabel]*normalizer_on;
            const Precision probOff = currentMarkerProb_off[iLabel]*normalizer_off;
            
            onTerm  += probOn  * log2(probOn);
            offTerm += probOff * log2(probOff);
        }
        
        infoGain[iProbe] = (Precision(1)-p_off[iProbe])*onTerm + p_off[iProbe]*offTerm;
    }
    
#if !USE_PERSISTENT_MEMORY
    mxFree(markerProb_on);
    mxFree(markerProb_off);
    mxFree(p_off);
    mxFree(sum_on);
    mxFree(sum_off);
#endif
    
    return;

} // mexFunction()