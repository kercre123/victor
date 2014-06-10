% function [allQuads, markers] = extractMarkers_matlabOriginal_noRefinement(image)

function [allQuads, quadValidity, markers] = extractMarkers_matlabOriginal_noRefinement(image)
    allQuads = simpleDetector(image, 'decodeMarkers', false, 'quadRefinementIterations', 0);
    quadValidity = 100000 * ones(length(allQuads), 1, 'int32');
    markers = simpleDetector(image, 'decodeMarkers', true, 'quadRefinementIterations', 0);        