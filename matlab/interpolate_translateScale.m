% function interpolate_translateScale()

% The inputImage will be scaled about its center by imageScale, then
% translated by imageTranslation. It will be placed in an image of size
% outputImageSize (which has a top-left coordinate of (1,1)).

% all scale and translation parameters are [y,x]

% im = double(imread('C:\Anki\blockImages\docking\blurredDots.png'));
% output = interpolate_translateScale(ones(17,19), [100,100], [1.0,1.0], [200,200]);
% output = interpolate_translateScale(im, [120,160], [.5,.5], [240,320]);
% output = interpolate_translateScale(im, [10,10], [.25,.25], [240,320]);

% inputImage = zeros([5,5]); for y=1:5 inputImage(y,:)=y*(1:5); end
% output = interpolate_translateScale(inputImage, [2.5,2.5], [0.5,0.5], [11,11])
% output = interpolate_translateScale(inputImage, [2.5,2.5], [1.0,1.0], [11,11])
% output = interpolate_translateScale(inputImage, [2.5,2.5], [2.0,2.0], [11,11])
% output = interpolate_translateScale(inputImage, [2.75,2.75], [1.0,1.0], [11,11])
% output = interpolate_translateScale(inputImage, [2.25,2.25], [1.0,1.0], [11,11])

function output = interpolate_translateScale(inputImage, imageTranslation, imageScale, outputImageSize)

% All coordinates treat the upper left as (0,0), not (1,1) as in normal in
% Matlab.

inputImageSize = size(inputImage);

output = zeros(outputImageSize);

% These coordinates are the minimum and maximum input coordinates that are
% possible to interpolate the output image. If the topLeft is negative or
% the bottomRight is larger than outputImageSize, then the whole input
% image won't be used.
minOutputCoordinates = [(-(inputImageSize(1)/2-0.5)*imageScale(1)) + imageTranslation(1),...
                        (-(inputImageSize(2)/2-0.5)*imageScale(2)) + imageTranslation(2)];

maxOutputCoordinates = [((inputImageSize(1)/2-0.5)*imageScale(1)) + imageTranslation(1),...
                             ((inputImageSize(2)/2-0.5)*imageScale(2)) + imageTranslation(2)];

% Check if the interpolated image is inside the boundaries of the
% outputImage. Clip the coordinates if not.
if minOutputCoordinates(1) < 0.5
    minOutputCoordinates(1) = 0.5;
end

if minOutputCoordinates(2) < 0.5
    minOutputCoordinates(2) = 0.5;
end

if maxOutputCoordinates(1) > (outputImageSize(1)-0.5)
    maxOutputCoordinates(1) = outputImageSize(1)-0.5;
end

if maxOutputCoordinates(2) > (outputImageSize(2)-0.5)
    maxOutputCoordinates(2) = outputImageSize(2)-0.5;
end

% We can't interpolate based on a partial pixel, so add another pixel of
% padding, if needed
remainderTopLeft = minOutputCoordinates - floor(minOutputCoordinates) - 0.5;
minOutputCoordinates = minOutputCoordinates - remainderTopLeft + ceil(remainderTopLeft);

remainderBottomRight = maxOutputCoordinates - floor(maxOutputCoordinates) - 0.5;
maxOutputCoordinates = maxOutputCoordinates - remainderBottomRight - ceil(-remainderBottomRight);


% Compute the looping limits

% Map the output coordinates to the input coordinates
curInputCoordinatesRaw = ((minOutputCoordinates - imageTranslation) ./ imageScale) + inputImageSize/2;

% This is the shift in the input coordinates, for a one-pixel shift in the
% output coordinates
dy = 1 / imageScale(1);
dx = 1 / imageScale(2);

% We will loop on integer indices, so remove the extra 0.5 offset
minOutputCoordinates = minOutputCoordinates - 0.5;
maxOutputCoordinates = maxOutputCoordinates - 0.5;

% Remove the extra 0.5 offset, to save doing it every step
%
% The integer part of curInputCoordinates is the upper-left pixel to
% interpolate. The fractional part is the percent of the lower-right
% pixel's interpolation weight. For example, [1.25, 1.25] means to use pixels at
% coordinates [1,1], [1,2], [2,1], and [2,2]. The weights are 75% on the
% upper-left pixels, and 25% on the lower right pixels.
curInputCoordinates = curInputCoordinatesRaw - 0.5;

keyboard 

% % These coordinates are the outer border of the scaled and translated
% % inputImage.
% % minOutputCoordinates = [(-imageScale(1)*(inputImageSize(1)/2)) + imageTranslation(1),...
% %                         (-imageScale(2)*(inputImageSize(2)/2)) + imageTranslation(2)];
% %
% % maxOutputCoordinates = [(imageScale(1)*(inputImageSize(1)/2)) + imageTranslation(1),...
% %                              (imageScale(2)*(inputImageSize(2)/2)) + imageTranslation(2)];
%
% % These coordinates are the minimum and maximum input coordinates that are
% % required to interpolate the output image. If the topLeft is negative or
% % the bottomRight is larger than outputImageSize, then the whole input
% % image won't be used.
% minOutputCoordinates = [(-imageScale(1)*(inputImageSize(1)/2-0.5)) + imageTranslation(1),...
%                         (-imageScale(2)*(inputImageSize(2)/2-0.5)) + imageTranslation(2)];
%
% maxOutputCoordinates = [(imageScale(1)*(inputImageSize(1)/2-0.5)) + imageTranslation(1),...
%                              (imageScale(2)*(inputImageSize(2)/2-0.5)) + imageTranslation(2)];
%
% % maxOutputCoordinates = floor(maxOutputCoordinates);
%
% dy = 1 / imageScale(1);
% dx = 1 / imageScale(2);
%
% curInputY = 0;
% curInputX = 0;
%
% if minOutputCoordinates(1) < 0
%     % If the start pixel is out of bounds, move it in bounds
%     curInputY = curInputY - dy*minOutputCoordinates(1) + 0.5;
%     minOutputCoordinates(1) = 0;
% else
%     % If the start pixel is not an integer, round it up
%     remainder1 = ceil(minOutputCoordinates(1)) - 0.5 - minOutputCoordinates(1);
%     curInputY = curInputY + dy*remainder1;
%     minOutputCoordinates(1) = ceil(minOutputCoordinates(1)) - 0.5;
% end
%
% if minOutputCoordinates(2) < 0
%     % If the start pixel is out of bounds, move it in bounds
%     curInputX = curInputX - dx*minOutputCoordinates(2) - 0.5;
%     minOutputCoordinates(2) = 0;
% else
%     % If the start pixel is not an integer, round it up
%     remainder2 = ceil(minOutputCoordinates(2)) - 0.5 - minOutputCoordinates(2);
%     curInputX = curInputX + dx*remainder2;
%     minOutputCoordinates(2) = ceil(minOutputCoordinates(2)) + 0.5;
% end
%
% if maxOutputCoordinates(1) > (outputImageSize(1)-1)
%     % If the final pixel is out of bounds, move it in bounds
%     maxOutputCoordinates(1) = floor(outputImageSize(1)-1);
% end
%
% if maxOutputCoordinates(2) > (outputImageSize(2)-1)
%     % If the final pixel is out of bounds, move it in bounds
%     maxOutputCoordinates(2) = floor(outputImageSize(2)-1);
% end
%
% for y = minOutputCoordinates(1):maxOutputCoordinates(1)
%     curInputY0 = floor(curInputY);
%     alphaYinverse = curInputY - curInputY0;
%     alphaY = 1 - alphaYinverse;
%
%     for x = minOutputCoordinates(2):maxOutputCoordinates(2)
%         curInputX0 = floor(curInputX);
%         alphaXinverse = curInputX - curInputX0;
%         alphaX = 1 - alphaXinverse;
%
%         pixel00 = inputImage(curInputY0+1, curInputX0+1);
%         pixel01 = inputImage(curInputY0+1, curInputX0+2);
%         pixel10 = inputImage(curInputY0+2, curInputX0+1);
%         pixel11 = inputImage(curInputY0+2, curInputX0+2);
%
%         interpolatedTop = alphaXinverse*pixel00 + alphaX*pixel01;
%         interpolatedBottom = alphaXinverse*pixel10 + alphaX*pixel11;
%         interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;
%
%         output(y+1, x+1) = interpolatedPixel;
%
%         curInputX = curInputX + dx;
%     end
%     curInputY = curInputY + dy;
% end
%
% keyboard
