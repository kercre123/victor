% function [allQuads, markers] = extractMarkers_matlabOriginal_withRefinement(image)

function [allQuads, markers] = extractMarkers_matlabOriginal_withRefinement(image)
    allQuads = simpleDetector(image, 'decodeMarkers', false, 'quadRefinementIterations', 25);
    markers = simpleDetector(image, 'decodeMarkers', true, 'quadRefinementIterations', 25);