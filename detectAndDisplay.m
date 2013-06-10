function detectAndDisplay(img, h_axes, h_img)

%persistent h_detections;

if isempty(img) || any(size(img)==0)
    warning('Empty image passed in!');
    return
end

t = tic;
detections = simpleDetector(img, 'downsampleFactor', 1.5);
fprintf('Detection processing took %.2f seconds.\n', toc(t));

%t = tic;
numDetections = length(detections);

axis(h_axes, 'on');
set(h_axes, 'LineWidth', 5, 'Box', 'on', 'XTick', [], 'YTick', []);
set(h_img, 'CData', img);

h_detections = findobj(h_axes, 'Tag', 'BlockMarker');
if ~isempty(h_detections)
    delete(h_detections)
end

if numDetections > 0
    hold(h_axes, 'on')
    for i = 1:numDetections
        draw(detections{i}, 'where', h_axes);
    end
    
    set(h_axes, 'XColor', 'g', 'YColor', 'g');
    hold(h_axes, 'off')
else
    set(h_axes, 'XColor', 'r', 'YColor', 'r');
end

%fprintf('Rest of detectAndDisplay took %.2f seconds\n', toc(t));


end
