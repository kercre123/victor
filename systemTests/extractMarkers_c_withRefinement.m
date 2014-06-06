% function [allQuads, markers] = extractMarkers_c_withRefinement(image)

function [allQuads, markers] = extractMarkers_c_withRefinement(image)
    [allQuads, markers] = extractMarkers_c_total(image, true);    