% function [allQuads, markers] = extractMarkers_c_withRefinement(image)

function [allQuads, quadValidity, markers] = extractMarkers_c_withRefinement(image)
    [allQuads, quadValidity, markers] = extractMarkers_c_total(image, true, false);    