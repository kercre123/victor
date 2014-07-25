typedef unsigned short char16_t;
#include <mex.h>

#include <cmath>

void mexFunction(int nlhs, mxArray* plhs[], int nrhs, const mxArray* prhs[])
{
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
    
    const double eps = 1e-16;
    
    const mxArray* labelsArray = prhs[0];
    const mwSize   numLabels   = static_cast<mwSize>(mxGetScalar(prhs[1]));
    const mxArray* probeValues = prhs[2];
    
    const mwSize numProbes   = mxGetN(probeValues);
    const mwSize numExamples = mxGetM(probeValues);
    
    const double oneOverNumExamples = 1. / static_cast<double>(numExamples);
    
    const double* labels = mxGetPr(labelsArray);
    mxAssert(mxGetNumberOfElements(labelsArray) == numExamples, 
            "Number of elements in labels should match number of rows in probeValues.");
    
    double* markerProb_on  = (double*) mxMalloc(numLabels*numProbes*sizeof(double));
    double* markerProb_off = (double*) mxMalloc(numLabels*numProbes*sizeof(double));
    
    // Initialize with small values to avoid zero probabilities (and log of zero)
    for(mwSize i=0; i<numLabels*numProbes; ++i) {
        markerProb_on[i]  = eps;
        markerProb_off[i] = eps;
    }
    
    double* sum_on  = (double*) mxCalloc(numProbes, sizeof(double));
    double* sum_off = (double*) mxCalloc(numProbes, sizeof(double));
    
    double* p_on = (double*) mxCalloc(numProbes, sizeof(double));
    
    for(mwSize iProbe=0; iProbe<numProbes; ++iProbe) {
        const double* currentProbe  = mxGetPr(probeValues) + iProbe*numExamples;
        
        double* currentMarkerProb_on  = markerProb_on  + iProbe*numLabels;
        double* currentMarkerProb_off = markerProb_off + iProbe*numLabels;
                
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
        const double* currentMarkerProb_on  = markerProb_on  + iProbe*numLabels;
        const double* currentMarkerProb_off = markerProb_off + iProbe*numLabels;
             
        const double normalizer_on  = 1. / sum_on[iProbe];
        const double normalizer_off = 1. / sum_off[iProbe];
        
        // Accumulate probes into the right place based on each example's label
        double onTerm = 0.f, offTerm = 0.f;
        for(mwSize iLabel=0; iLabel<numLabels; ++iLabel) {
            const double probOn  = currentMarkerProb_on[iLabel]*normalizer_on;
            const double probOff = currentMarkerProb_off[iLabel]*normalizer_off;
            
            onTerm  += probOn  * log2(probOn);
            offTerm += probOff * log2(probOff);
        }
        
        infoGain[iProbe] = p_on[iProbe]*onTerm + (1.-p_on[iProbe])*offTerm;
    }
    
    mxFree(markerProb_on);
    mxFree(markerProb_off);
    mxFree(p_on);
    mxFree(sum_on);
    mxFree(sum_off);
    
    return;

} // mexFunction()