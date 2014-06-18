% function [allQuads, markers] = extractMarkers_matlabOriginal_noRefinement(image)

function [allQuads, quadValidity, markers] = extractMarkers_matlabOriginalQuads_cExtraction_noRefinement(image)
    [allQuads, quadValidity, markers] = extractMarkers_c_total(image, false, true);