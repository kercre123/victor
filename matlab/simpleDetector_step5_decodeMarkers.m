function [markers, validQuads] = simpleDetector_step5_decodeMarkers(img, quads, quadTforms, minQuadArea, quadRefinementIterations, embeddedConversions, showTiming, NearestNeighborLibrary, CNN, DEBUG_DISPLAY)
    
    markers = {};
    validQuads = false(size(quads));
    
    % namedFigure('InitialFiltering')
    % subplot(2,2,4)
    % imshow(label2rgb(regionImg, 'jet', 'k', 'shuffle'))
    
    persistent allMarkerImages;
    persistent tree;
    persistent treeFilename;
    
    if strcmp(embeddedConversions.extractFiducialMethod, 'matlab_exhaustive') || strcmp(embeddedConversions.extractFiducialMethod, 'c_exhaustive')
        if isempty(allMarkerImages)
            allMarkerImages = visionMarker_exhaustiveMatch_loadallMarkerImages(VisionMarkerTrained.TrainingImageDir);
        end
    end
    
    if ~isempty(quads)
        if showTiming, t_quads = tic; end
        
        numQuads = length(quads);
        
        if DEBUG_DISPLAY
            namedFigure('SimpleDetector')
            h_quadAxes(1) = subplot(223);
            hold off, imshow(img), hold on
            title(sprintf('%d Quadrilaterals Returned', length(quads)))
            
            if numQuads==1
                quadColors = [1 0 0];
            else
                quadColors = im2double(squeeze(label2rgb((1:numQuads)', ...
                    jet(numQuads), 'k', 'shuffle')));
            end
            
            h_quadAxes(2) = subplot(224);
            hold off, imshow(img), hold on
        end
        
        numMarkers = 0;
        numInvalidMarkers = 0;
        
        for i_quad = 1:numQuads
            if DEBUG_DISPLAY
                plot(quads{i_quad}([1 2 4 3 1],1), quads{i_quad}([1 2 4 3 1],2), ...
                    'Color', quadColors(i_quad,:), 'LineWidth', 1, ...
                    'Parent', h_quadAxes(1));
            end
            
            % Sanity check:
            corners = quads{i_quad};
            A = [corners(2,:) - corners(1,:);
                corners(3,:) - corners(1,:)];
            assert(abs(det(A)) >= minQuadArea, 'Area of quad too small.');
            
            %marker = BlockMarker2D(img, quads{i_quad}, quadTforms{i_quad});
            %
            %         validQuads(i_quad) = marker.isValid;
            %         if marker.isValid
            %             markers{end+1} = marker; %#ok<AGROW>
            %         end
            
            %marker = VisionMarker(img, 'Corners', corners);
            if strcmp(embeddedConversions.extractFiducialMethod, 'matlab_original')
                markers{end+1} = VisionMarkerTrained(img, 'Corners', corners, ...
                    'CornerRefinementIterations', quadRefinementIterations, ...
                    'NearestNeighborLibrary', NearestNeighborLibrary, ...
                    'CNN', CNN);  %#ok<AGROW>
            elseif strcmp(embeddedConversions.extractFiducialMethod, 'matlab_exhaustive')
                markerTmp = VisionMarkerTrained([], 'Corners', corners, 'Initialize', false);
                
                tform = cp2tform([0 0 1 1; 0 1 0 1]', corners, 'projective');
                
                markerTmp.H = tform.tdata.T';
                
                [~, bright, dark] = VisionMarkerTrained.ComputeThreshold(img, tform);
                
                if isempty(bright) || isempty(dark)
                    continue;
                end
                
                [newCorners, ~] = markerTmp.RefineCorners(...
                    img, 'NumSamples', 100, ...
                    'MaxIterations', quadRefinementIterations, ...
                    'DarkValue', dark, 'BrightValue', bright);
                
                newMarker = visionMarker_exhaustiveMatch(...
                    allMarkerImages,...
                    {1, embeddedConversions.extractFiducialMethodParameters.exhaustiveMethod},...
                    img,...
                    newCorners);
                
                if iscell(newMarker.codeName)
                    newMarker.codeName = 'MARKER_UNKNOWN';
                    newMarker.name = newMarker.codeName;
                else
                    newMarker = newMarker.ReorderCorners();
                end
                
                markers{end+1} = newMarker; %#ok<AGROW>
            elseif strcmp(embeddedConversions.extractFiducialMethod, 'c_exhaustive')
                markerTmp = VisionMarkerTrained([], 'Corners', corners, 'Initialize', false);
                
                tform = cp2tform([0 0 1 1; 0 1 0 1]', corners, 'projective');
                
                markerTmp.H = tform.tdata.T';
                
                [~, bright, dark] = VisionMarkerTrained.ComputeThreshold(img, tform);
                
                if isempty(bright) || isempty(dark)
                    continue;
                end
                
                [newCorners, ~] = markerTmp.RefineCorners(...
                    img, 'NumSamples', 100, ...
                    'MaxIterations', quadRefinementIterations, ...
                    'DarkValue', dark, 'BrightValue', bright);
                
                newMarker = visionMarker_exhaustiveMatch(...
                    allMarkerImages,...
                    {2, ''},...
                    img,...
                    newCorners);
                
                if iscell(newMarker.codeName)
                    newMarker.codeName = 'MARKER_UNKNOWN';
                    newMarker.name = newMarker.codeName;
                else
                    newMarker = newMarker.ReorderCorners();
                end
                
                markers{end+1} = newMarker; %#ok<AGROW>
            elseif strcmp(embeddedConversions.extractFiducialMethod, 'matlab_alternateTree')
                if isempty(tree) || ~strcmp(treeFilename, embeddedConversions.extractFiducialMethodParameters.treeFilename)
                    treeFilename = embeddedConversions.extractFiducialMethodParameters.treeFilename;
%                     load(embeddedConversions.extractFiducialMethodParameters.treeFilename, 'minimalTree');
                    load(embeddedConversions.extractFiducialMethodParameters.treeFilename, 'tree');
                end
                
                imgU8 = im2uint8(img);
                
                counts = integerCounts(imgU8, corners);
                
                countsSum = cumsum(counts);
                countsSum = countsSum / max(countsSum);
                blackValue = find(countsSum >= 0.01, 1) - 1;
                whiteValue = find(countsSum >= 0.99, 1) - 1;
                
                tform = cp2tform(corners, [0 0; 0 1; 1 0; 1 1], 'projective');
                
                [labelName, labelID] = decisionTree2_query(tree, imgU8, tform, blackValue, whiteValue);
                
                newMarker = VisionMarkerTrained([], 'Initialize', false);
                
                newMarker.codeID = labelID;
                newMarker.codeName = labelName;
                newMarker.corners = corners;
                newMarker.name = labelName;
                newMarker.fiducialSize = 1;
                newMarker.isValid = true;
                % newMarker.matchDistance = matchDistance;
                
                tform = cp2tform([0 0 1 1; 0 1 0 1]', corners, 'projective');
                newMarker.H = tform.tdata.T';
                
                if iscell(newMarker.codeName) || strcmp(newMarker.codeName, 'MARKER_UNKNOWN')
                    newMarker.codeName = 'MARKER_UNKNOWN';
                    newMarker.name = newMarker.codeName;
                else
                    newMarker = newMarker.ReorderCorners();
                end
                
                markers{end+1} = newMarker;  %#ok<AGROW>
            end
            
            if DEBUG_DISPLAY
                if markers{end}.isValid
                    draw(markers{end}, 'where', h_quadAxes(2));
                    numMarkers = numMarkers + 1;
                else
                    numInvalidMarkers = numInvalidMarkers + 1;
                end
                
                
                %{
            drawThisMarker = false;
            if isValid
                edgeColor = 'g';
                numValidMarkers = numValidMarkers + 1;
                drawThisMarker = true;
            elseif drawInvalidMarkers
                edgeColor = 'r';
                drawThisMarker = true;
            end
            
            if drawThisMarker
                patch(quads{i_quad}([1 2 4 3 1],1), quads{i_quad}([1 2 4 3 1],2), ...
                    'g', 'EdgeColor', edgeColor, 'LineWidth', 2, ...
                    'FaceColor', 'r', 'FaceAlpha', .3, ...
                    'Parent', h_quadAxes(2));
                if ~strcmp(keyOrient, 'none')
                    topSideLUT = struct('down', [2 4], 'up', [1 3], ...
                        'left', [1 2], 'right', [3 4]);
                    plot(quads{i_quad}(topSideLUT.(keyOrient),1), ...
                        quads{i_quad}(topSideLUT.(keyOrient),2), ...
                        'Color', 'b', 'LineWidth', 3, 'Parent', h_quadAxes(2));
                end
                text(mean(quads{i_quad}([1 4],1)), mean(quads{i_quad}([1 4],2)), ...
                    sprintf('Block %d, Face %d', blockType, faceType), ...
                    'Color', 'b', 'FontSize', 14, 'FontWeight', 'b', ...
                    'BackgroundColor', 'w', ...
                    'Hor', 'center', 'Parent', h_quadAxes(2));
                
                numMarkers = numMarkers + 1;
            end
                %}
            end % IF DEBUG_DISPLAY
            
        end % FOR each quad
        
        if DEBUG_DISPLAY
            title(h_quadAxes(2), sprintf('%d Markers Detected', numMarkers));
        end
        
        if showTiming
            fprintf('Marker decoding took %.2f seconds.\n', toc(t_quads));
        end
        
    end % IF any quads found
    
    
    if DEBUG_DISPLAY
        subplot_expand on
    end
