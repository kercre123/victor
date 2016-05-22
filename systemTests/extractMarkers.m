% function [allQuads, markers] = extractMarkers()

function [allQuads, quadValidity, markers, allQuads_pixelValues, markers_pixelValues] = extractMarkers(image, algorithmParameters)
    allQuads_pixelValues = {};
    markers_pixelValues = {};
    
    image = rgb2gray2(image);
    
    if algorithmParameters.useMatlabForAll
        convertFromCToMatlab = false;
        
        allQuadsRaw = simpleDetector(image,...
            'returnInvalid', true,...
            'quadRefinementIterations', 25,...
            'thresholdFraction', algorithmParameters.scaleImage_thresholdMultiplier,...
            'embeddedConversions', algorithmParameters.matlab_embeddedConversions,...
            'cornerMethod', algorithmParameters.cornerMethod);
        
        quadValidity = zeros([length(allQuadsRaw), 1], 'int32');
        
        allQuads = cell(length(allQuadsRaw), 1);
        markers = {};
        allQuads_pixelValues = cell(length(allQuadsRaw), 1);
        markers_pixelValues = {};
        
        for iQuad = 1:length(allQuadsRaw)
            allQuads{iQuad} = allQuadsRaw{iQuad}.corners;
            [allQuads_pixelValues{iQuad}, ~, ~] = allQuadsRaw{iQuad}.GetProbeValues(image, maketform('projective', allQuadsRaw{iQuad}.H'));
            
            if allQuadsRaw{iQuad}.isValid
                quadValidity(iQuad) = 0;
                markers{end+1} = allQuadsRaw{iQuad}; %#ok<AGROW>
                markers_pixelValues{end+1} = allQuads_pixelValues{iQuad}; %#ok<AGROW>
            else
                quadValidity(iQuad) = 9;
            end
        end
    elseif algorithmParameters.useMatlabForQuadExtraction
        convertFromCToMatlab = true;
        
        allQuadsMatlabRaw = simpleDetector(image,...
            'returnInvalid', true,...
            'quadRefinementIterations', algorithmParameters.refine_quadRefinementIterations,...
            'thresholdFraction', algorithmParameters.scaleImage_thresholdMultiplier,...
            'embeddedConversions', algorithmParameters.matlab_embeddedConversions,...
            'cornerMethod', algorithmParameters.cornerMethod);
        
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
        
        % TODO: fix mexDetectFiducialMarkers_quadInput
        assert(false);
        
%         returnInvalidMarkers = 1;
%         [allQuads, ~, ~, quadValidity] = mexDetectFiducialMarkers_quadInput(image, allQuadsMatlab, algorithmParameters.quads_minQuadArea, algorithmParameters.quads_quadSymmetryThreshold, algorithmParameters.quads_minDistanceFromImageEdge, algorithmParameters.decode_minContrastRatio, 0, algorithmParameters.refine_numRefinementSamples, algorithmParameters.refine_quadRefinementMaxCornerChange, algorithmParameters.refine_quadRefinementMinCornerChange, returnInvalidMarkers);
%         
%         returnInvalidMarkers = 0;
%         [goodQuads, ~, markerNames] = mexDetectFiducialMarkers_quadInput(image, allQuadsMatlab, algorithmParameters.quads_minQuadArea, algorithmParameters.quads_quadSymmetryThreshold, algorithmParameters.quads_minDistanceFromImageEdge, algorithmParameters.decode_minContrastRatio, 0, algorithmParameters.refine_numRefinementSamples, algorithmParameters.refine_quadRefinementMaxCornerChange, algorithmParameters.refine_quadRefinementMinCornerChange, returnInvalidMarkers);
    else 
        convertFromCToMatlab = true;
        
        if strcmp(algorithmParameters.cornerMethod, 'laplacianPeaks')
            cornerMethodIndex = 0;
        elseif strcmp(algorithmParameters.cornerMethod, 'lineFits')
            cornerMethodIndex = 1;
        else
            keyboard
            assert(false);
        end
        
        returnInvalidMarkers = 1;
        [allQuads, ~, ~, quadValidity] = mexDetectFiducialMarkers(image, algorithmParameters.useIntegralImageFiltering, algorithmParameters.scaleImage_numPyramidLevels, algorithmParameters.scaleImage_thresholdMultiplier, algorithmParameters.component1d_minComponentWidth, algorithmParameters.component1d_maxSkipDistance, algorithmParameters.component_minimumNumPixels, algorithmParameters.component_maximumNumPixels, algorithmParameters.component_sparseMultiplyThreshold, algorithmParameters.component_solidMultiplyThreshold, algorithmParameters.component_minHollowRatio, algorithmParameters.quads_minLaplacianPeakRatio, algorithmParameters.quads_minQuadArea, algorithmParameters.quads_quadSymmetryThreshold, algorithmParameters.quads_minDistanceFromImageEdge, algorithmParameters.decode_minContrastRatio, algorithmParameters.refine_quadRefinementIterations, algorithmParameters.refine_numRefinementSamples, algorithmParameters.refine_quadRefinementMaxCornerChange, algorithmParameters.refine_quadRefinementMinCornerChange, returnInvalidMarkers, cornerMethodIndex);
        
        returnInvalidMarkers = 0;
        [goodQuads, ~, markerNames] = mexDetectFiducialMarkers(image, algorithmParameters.useIntegralImageFiltering, algorithmParameters.scaleImage_numPyramidLevels, algorithmParameters.scaleImage_thresholdMultiplier, algorithmParameters.component1d_minComponentWidth, algorithmParameters.component1d_maxSkipDistance, algorithmParameters.component_minimumNumPixels, algorithmParameters.component_maximumNumPixels, algorithmParameters.component_sparseMultiplyThreshold, algorithmParameters.component_solidMultiplyThreshold, algorithmParameters.component_minHollowRatio, algorithmParameters.quads_minLaplacianPeakRatio, algorithmParameters.quads_minQuadArea, algorithmParameters.quads_quadSymmetryThreshold, algorithmParameters.quads_minDistanceFromImageEdge, algorithmParameters.decode_minContrastRatio, algorithmParameters.refine_quadRefinementIterations, algorithmParameters.refine_numRefinementSamples, algorithmParameters.refine_quadRefinementMaxCornerChange, algorithmParameters.refine_quadRefinementMinCornerChange, returnInvalidMarkers, cornerMethodIndex);
    end
    
    if convertFromCToMatlab
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
    end
    
