% function [allQuads, markers] = extractMarkers_matlabOriginal_withRefinement(image)

function [allQuads, quadValidity, markers] = extractMarkers_matlabOriginalQuads_cExtraction_withRefinement(image)
    [allQuads, quadValidity, markers] = extractMarkers_c_total(image, true, true);