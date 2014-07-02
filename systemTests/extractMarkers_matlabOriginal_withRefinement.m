% function [allQuads, markers] = extractMarkers_matlabOriginal_withRefinement(image)

function [allQuads, quadValidity, markers] = extractMarkers_matlabOriginal_withRefinement(image)
    allQuadsRaw = simpleDetector(image, 'returnInvalid', true, 'quadRefinementIterations', 25);
    
    quadValidity = zeros([length(allQuadsRaw), 1], 'int32');
    
    allQuads = cell(length(allQuadsRaw), 1);
    markers = {};
    for iQuad = 1:length(allQuadsRaw)
        allQuads{iQuad} = allQuadsRaw{iQuad}.corners;
        
        if allQuadsRaw{iQuad}.isValid
            quadValidity(iQuad) = 0;
            markers{end+1} = allQuadsRaw{iQuad}; %#ok<AGROW>
        else
            quadValidity(iQuad) = 9;
        end
    end