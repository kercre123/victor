% function image = drawExampleSquaresImage()
% Draws a simple pattern of big hollow rectangles and small solid rectangles

function image = drawExampleSquaresImage()

lineColor = 0;
backgroundColor = 128;

image = backgroundColor * ones(480, 640, 'uint8'); 

boxWidth = 60;

upperLeft = [20,20];
for i = 1:5
    image = drawBox(image, upperLeft + [0, boxWidth*2*(i-1)], upperLeft + boxWidth + [0, boxWidth*2*(i-1)], i, lineColor, backgroundColor);
end

upperLeft = upperLeft + [2*boxWidth,0];
for i = 1:5
    image = drawBox(image, upperLeft + [0, boxWidth*2*(i-1)], upperLeft + boxWidth + [0, boxWidth*2*(i-1)], i+5, lineColor, backgroundColor);
end

upperLeft = upperLeft + [2*boxWidth,0];
for i = 1:5
    image = drawBox(image, upperLeft + [0, boxWidth*2*(i-1)], upperLeft + boxWidth + [0, boxWidth*2*(i-1)], i+10, lineColor, backgroundColor);
end

upperLeft = upperLeft + [2*boxWidth,0];
speckWidths = 1:22;
for i = speckWidths
    image = drawBox(image, upperLeft + [0, 2*sum(speckWidths(1:i))], upperLeft + [0, i + 2*sum(speckWidths(1:i))], -1, lineColor, backgroundColor);
end

upperLeft = upperLeft + [10,0];
for i = speckWidths
    image = drawBox(image, upperLeft + [0, 2*sum(speckWidths(1:i))], upperLeft + [1, i + 2*sum(speckWidths(1:i))], -1, lineColor, backgroundColor);
end

upperLeft = upperLeft + [10,0];
for i = speckWidths
    image = drawBox(image, upperLeft + [0, 2*sum(speckWidths(1:i))], upperLeft + [2, i + 2*sum(speckWidths(1:i))], -1, lineColor, backgroundColor);
end

upperLeft = upperLeft + [10,0];
for i = speckWidths
    image = drawBox(image, upperLeft + [0, 2*sum(speckWidths(1:i))], upperLeft + [4, i + 2*sum(speckWidths(1:i))], -1, lineColor, backgroundColor);
end

upperLeft = upperLeft + [13,0];
for i = speckWidths
    image = drawBox(image, upperLeft + [0, 2*sum(speckWidths(1:i))], upperLeft + [8, i + 2*sum(speckWidths(1:i))], -1, lineColor, backgroundColor);
end

end % function image = drawExampleSquaresImage()

function image = drawBox(image, upperLeft, lowerRight, lineWidth, lineColor, backgroundColor)
    image(upperLeft(1):lowerRight(1), upperLeft(2):lowerRight(2)) = lineColor; 
    
    if lineWidth > 0
        image((upperLeft(1)+lineWidth):(lowerRight(1)-lineWidth), (upperLeft(2)+lineWidth):(lowerRight(2)-lineWidth)) = backgroundColor; 
    end
end % function image = drawBox(image, upperLeft, lowerRight, lineWidth, lineColor)