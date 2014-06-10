% function [allQuads, markers] = extractMarkers_c_noRefinement(image)

function [allQuads, quadValidity, markers] = extractMarkers_c_noRefinement(image)
    [allQuads, quadValidity, markers] = extractMarkers_c_total(image, false);