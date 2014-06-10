% function [allQuads, markers] = extractMarkers_matlabOriginal_noRefinement(image)

function [allQuads, markers] = extractMarkers_matlabOriginal_noRefinement(image)
    allQuads = simpleDetector(image, 'decodeMarkers', false, 'quadRefinementIterations', 0);
    markers = simpleDetector(image, 'decodeMarkers', true, 'quadRefinementIterations', 0);        