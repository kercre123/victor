% function [allQuads, markers] = extractMarkers_c_noRefinement(image)

function [allQuads, markers] = extractMarkers_c_noRefinement(image)
    [allQuads, markers] = extractMarkers_c_total(image, false);