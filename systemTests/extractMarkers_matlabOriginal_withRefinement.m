% function [allQuads, markers] = extractMarkers_matlabOriginal_withRefinement(image)

function [allQuads, quadValidity, markers] = extractMarkers_matlabOriginal_withRefinement(image)
    allQuads = simpleDetector(image, 'decodeMarkers', false, 'quadRefinementIterations', 25);
    quadValidity = 100000 * ones(length(allQuads), 1, 'int32');
    markers = simpleDetector(image, 'decodeMarkers', true, 'quadRefinementIterations', 25);