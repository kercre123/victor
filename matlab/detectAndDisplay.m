function detectAndDisplay(img, h_axes, h_img, varargin)

markerLibrary = [];

simpleDetectorArgs = parseVarargin(varargin{:});

%persistent h_detections;

if isempty(img) || any(size(img)==0)
    warning('Empty image passed in!');
    return
end

%t = tic;
detections = simpleDetector(img, simpleDetectorArgs{:});
%fprintf('Detection processing took %.2f seconds.\n', toc(t));

%t = tic;
numDetections = length(detections);

axis(h_axes, 'on');
set(h_axes, 'LineWidth', 5, 'Box', 'on', 'XTick', [], 'YTick', []);
set(h_img, 'CData', img);

h_detections = findobj(h_axes, 'Tag', 'BlockMarker2D');
if ~isempty(h_detections)
    delete(h_detections)
end

if numDetections > 0
    hold(h_axes, 'on')
    for i = 1:numDetections
        switch(class(detections{i}))
            case 'VisionMarker'
                if isempty(markerLibrary)
                    detections{i}.DrawProbes('Tag', 'BlockMarker2D', 'Parent', h_axes);
                else
                    match = markerLibrary.IdentifyMarker(detections{i});
                    if isempty(match)
                        detections{i}.DrawProbes('Tag', 'BlockMarker2D', 'Parent', h_axes);
                    else
                        detections{i}.Draw('Tag', 'BlockMarker2D', 'Parent', h_axes);
                    end
                end
                %draw(detections{i}, 'where', h_axes, 'drawTextLabels', 'short');
            case 'VisionMarkerTrained'
                detections{i}.Draw('Tag', 'BlockMarker2D', 'Parent', h_axes);
            otherwise
                error('Unrecognized detection type.');
        end
    end
    
    set(h_axes, 'XColor', 'g', 'YColor', 'g');
    hold(h_axes, 'off')
    drawnow
else
    set(h_axes, 'XColor', 'r', 'YColor', 'r');
end

%fprintf('Rest of detectAndDisplay took %.2f seconds\n', toc(t));


end
