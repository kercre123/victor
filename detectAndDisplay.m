function detectAndDisplay(img)

if isempty(img) || any(size(img)==0)
    warning('Empty image passed in!');
    return
end

detections = simpleDetector(img, [], 'thresholdFraction', 1);

numDetections = length(detections);
if numDetections > 0

    namedFigure('LiveDetections');
    hold off
    h_img = imagesc(img);
    axis image 
    hold on
    
    h_axes = get(h_img, 'Parent');

    for i = 1:numDetections
        draw(detections{i}, h_axes);
    end
    
    set(h_axes, 'XColor', 'g', 'YColor', 'g', 'LineWidth', 5, ...
        'Box', 'on', 'XTick', [], 'YTick', []);
else
    h_axes = findobj(namedFigure('LiveDetections'), 'Type', 'axes');
    if ~isempty(h_axes)
        set(h_axes, 'XColor', 'r', 'YColor', 'r');
    end
end




end
