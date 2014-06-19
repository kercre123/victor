% function [allQuads, markers] = extractMarkers_c_total(image, useRefinement)

function [allQuads, quadValidity, markers] = extractMarkers_c_total(image, useRefinement, useMatlabForQuadExtraction)
    imageSize = size(image);
    scaleImage_thresholdMultiplier = 1.0;
    scaleImage_numPyramidLevels = 3;
    component1d_minComponentWidth = 0;
    component1d_maxSkipDistance = 0;
    minSideLength = round(0.03*max(imageSize(1),imageSize(2)));
    maxSideLength = round(0.97*min(imageSize(1),imageSize(2)));
    component_minimumNumPixels = round(minSideLength*minSideLength - (0.8*minSideLength)*(0.8*minSideLength));
    component_maximumNumPixels = round(maxSideLength*maxSideLength - (0.8*maxSideLength)*(0.8*maxSideLength));
    component_sparseMultiplyThreshold = 1000.0;
    component_solidMultiplyThreshold = 2.0;
    component_minHollowRatio = 1.0;
    quads_minQuadArea = 100 / 4;
    quads_quadSymmetryThreshold = 2.0;
    quads_minDistanceFromImageEdge = 2;
    decode_minContrastRatio = 1.25;
    refine_quadRefinementMaxCornerChange = 2;
    refine_numRefinementSamples = 100;
    
    if useRefinement
        refine_quadRefinementIterations = 25;
    else
        refine_quadRefinementIterations = 0;
    end
    
    if useMatlabForQuadExtraction
        allQuadsMatlabRaw = simpleDetector(image, 'returnInvalid', true, 'quadRefinementIterations', quadRefinementIterations);
        
        if isempty(allQuadsMatlabRaw)
            allQuads = {};
            quadValidity = int32([]);
            markers = {};
            return;           
        end

        allQuadsMatlab = cell(length(allQuadsMatlabRaw), 1);
        for iQuad = 1:length(allQuadsMatlabRaw)
            allQuadsMatlab{iQuad} = allQuadsMatlabRaw{iQuad}.corners;
        end
        
        for i = 1:length(allQuadsMatlab)
            allQuadsMatlab{i} = allQuadsMatlab{i} - 1;
        end
        
        returnInvalidMarkers = 1;
        [allQuads, ~, ~, quadValidity] = mexDetectFiducialMarkers_quadInput(image, allQuadsMatlab, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, 0, refine_numRefinementSamples, quadRefinementMaxCornerChange, returnInvalidMarkers);
        
        returnInvalidMarkers = 0;
        [goodQuads, ~, markerNames] = mexDetectFiducialMarkers_quadInput(image, allQuadsMatlab, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, 0, refine_numRefinementSamples, quadRefinementMaxCornerChange, returnInvalidMarkers);
    else
        returnInvalidMarkers = 1;
        [allQuads, ~, ~, quadValidity] = mexDetectFiducialMarkers(image, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_minHollowRatio, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, refine_quadRefinementIterations, refine_numRefinementSamples, refine_quadRefinementMaxCornerChange, returnInvalidMarkers);
        
        returnInvalidMarkers = 0;
        [goodQuads, ~, markerNames] = mexDetectFiducialMarkers(image, scaleImage_numPyramidLevels, scaleImage_thresholdMultiplier, component1d_minComponentWidth, component1d_maxSkipDistance, component_minimumNumPixels, component_maximumNumPixels, component_sparseMultiplyThreshold, component_solidMultiplyThreshold, component_minHollowRatio, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, decode_minContrastRatio, refine_quadRefinementIterations, refine_numRefinementSamples, refine_quadRefinementMaxCornerChange, returnInvalidMarkers);
    end
    
    markers = cell(length(markerNames), 1);
    
    for i = 1:length(markerNames)
        markers{i}.name = markerNames{i};
        markers{i}.corners = goodQuads{i};
    end
    
    % Convert from c to matlab coordinates
    for i = 1:length(allQuads)
        allQuads{i} = allQuads{i} + 1;
    end
    
    for i = 1:length(markers)
        markers{i}.corners = markers{i}.corners + 1;
    end
    
