typedef unsigned short char16_t;
#include <mex.h>

#include <cmath>

#define USE_PERSISTENT_MEMORY 0

#if USE_PERSISTENT_MEMORY     
static double* markerProb_on  = NULL;
static double* markerProb_off = NULL;
static double* sum_on         = NULL;
static double* sum_off        = NULL;
static double* p_on           = NULL;

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
    mxAssert(mxGetClassID(prhs[0]) == mxDOUBLE_CLASS, 
            "labelsArray should be DOUBLE.");
    
    mxAssert(mxGetClassID(prhs[2]) == mxDOUBLE_CLASS, 
            "probeValues should be DOUBLE.");
    
    mxAssert(mxGetNumberOfElements(prhs[1]) == 1, 
            "numLabels should be a scalar.");
    
    const double eps = 1e-9;
    
    const mxArray* labelsArray = prhs[0];
    const mwSize   numLabels   = static_cast<mwSize>(mxGetScalar(prhs[1]));
    const mxArray* probeValues = prhs[2];
    
    const mwSize numProbes   = mxGetN(probeValues);
    const mwSize numExamples = mxGetM(probeValues);
    
    const double oneOverNumExamples = 1. / static_cast<double>(numExamples);
    
    const double* labels = mxGetPr(labelsArray);
    mxAssert(mxGetNumberOfElements(labelsArray) == numExamples, 
            "Number of elements in labels should match number of rows in probeValues.");
    
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
        
        markerProb_on  = (double*) mxMalloc(numLabels*numProbes*sizeof(double));
        mexMakeMemoryPersistent(markerProb_on);
        
        markerProb_off = (double*) mxMalloc(numLabels*numProbes*sizeof(double));
        mexMakeMemoryPersistent(markerProb_off);
        
        sum_on  = (double*) mxMalloc(numProbes*sizeof(double));
        mexMakeMemoryPersistent(sum_on);
        
        sum_off = (double*) mxMalloc(numProbes*sizeof(double));
        mexMakeMemoryPersistent(sum_off);
        
        p_on = (double*) mxMalloc(numProbes*sizeof(double));
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
    
    double* markerProb_on  = (double*) mxMalloc(numLabels*numProbes*sizeof(double));
        
    double* markerProb_off = (double*) mxMalloc(numLabels*numProbes*sizeof(double));
    
    double* sum_on  = (double*) mxMalloc(numProbes*sizeof(double));
    
    double* sum_off = (double*) mxMalloc(numProbes*sizeof(double));
    
    double* p_on = (double*) mxMalloc(numProbes*sizeof(double));
    
#endif // USE_PERSISTENT_MEMORY
    
    
            
    // Initialize with small values to avoid zero probabilities (and log of zero)
    for(mwSize i=0; i<numLabels*numProbes; ++i) {
        markerProb_on[i]  = eps;
        markerProb_off[i] = eps;
    }
    
    for(mwSize iProbe=0; iProbe<numProbes; ++iProbe) {
        const double * currentProbe  = mxGetPr(probeValues) + iProbe*numExamples;
        
        double * currentMarkerProb_on  = markerProb_on  + iProbe*numLabels;
        double * currentMarkerProb_off = markerProb_off + iProbe*numLabels;
           
        sum_on[iProbe]  = eps;
        sum_off[iProbe] = eps;
        p_on[iProbe]    = eps;
        
        // Accumulate probes into the right place based on each example's label
        for(mwSize iExample=0; iExample<numExamples; ++iExample) {
            const int label = static_cast<int>(labels[iExample]);      
            const double probeIsOn  = 1. - currentProbe[iExample];
            const double probeIsOff = currentProbe[iExample];
            
            p_on[iProbe] += probeIsOn;
            
            currentMarkerProb_on[label]  += probeIsOn;
            currentMarkerProb_off[label] += probeIsOff;
            
            // Keep track of per-probe totals for normalization
            sum_on[iProbe]  += probeIsOn;
            sum_off[iProbe] += probeIsOff;
        }
     
        p_on[iProbe] *= oneOverNumExamples;
    }
    
    
    // Compute conditional entropy of each probe.  We'll just store infoGain as 
    // negative conditional entropy, since we are ignoring (constant) current
    // entropy.
    plhs[0] = mxCreateDoubleMatrix(numProbes,1,mxREAL);
    double* infoGain = mxGetPr(plhs[0]);
    for(mwSize iProbe=0; iProbe<numProbes; ++iProbe) {        
        const double * currentMarkerProb_on  = markerProb_on  + iProbe*numLabels;
        const double * currentMarkerProb_off = markerProb_off + iProbe*numLabels;
             
        const double normalizer_on  = 1. / sum_on[iProbe];
        const double normalizer_off = 1. / sum_off[iProbe];
        
        // Accumulate probes into the right place based on each example's label
        double onTerm = eps, offTerm = eps;
        for(mwSize iLabel=0; iLabel<numLabels; ++iLabel) {
            const double probOn  = currentMarkerProb_on[iLabel]*normalizer_on;
            const double probOff = currentMarkerProb_off[iLabel]*normalizer_off;
            
            onTerm  += probOn  * log2(probOn);
            offTerm += probOff * log2(probOff);
        }
        
        infoGain[iProbe] = p_on[iProbe]*onTerm + (1.-p_on[iProbe])*offTerm;
    }
    
#if !USE_PERSISTENT_MEMORY
    mxFree(markerProb_on);
    mxFree(markerProb_off);
    mxFree(p_on);
    mxFree(sum_on);
    mxFree(sum_off);
#endif
    
    return;

} // mexFunction()