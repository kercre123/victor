function [markers, validQuads] = simpleDetector_step5_decodeMarkers(img, quads, quadTforms, minQuadArea, quadRefinementIterations, embeddedConversions, showTiming, DEBUG_DISPLAY)

markers = {};
validQuads = false(size(quads));

% namedFigure('InitialFiltering')
% subplot(2,2,4)
% imshow(label2rgb(regionImg, 'jet', 'k', 'shuffle'))

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
                'CornerRefinementIterations', quadRefinementIterations);  %#ok<AGROW>
        elseif strcmp(embeddedConversions.extractFiducialMethod, 'matlab_exhaustive')
            markers{end+1} = VisionMarkerTrained(img, 'Corners', corners, ...
                'CornerRefinementIterations', quadRefinementIterations, 'exhaustiveSearchMethod', {1, embeddedConversions.extractFiducialMethodType});  %#ok<AGROW>
        elseif strcmp(embeddedConversions.extractFiducialMethod, 'c_exhaustive')
            markers{end+1} = VisionMarkerTrained(img, 'Corners', corners, ...
                'CornerRefinementIterations', quadRefinementIterations, 'exhaustiveSearchMethod', {2, ''});  %#ok<AGROW>            
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
