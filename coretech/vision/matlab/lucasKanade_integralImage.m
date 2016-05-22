% function lucasKanade_integralImage()
% Copies some things from EstimateDominantMotion.m

% im = double(imread('C:\Anki\blockImages\docking\blurredDots.png'));
% template = im(100:150, 100:150);
% initialTranslation = [80,80]; % a bit off

% lucasKanade_integralImage(im, template, initialTranslation , 2.^[0:5]);

% ?? initialTranslation is [x,y]

function lucasKanade_integralImage(image, template, initialTranslation, filterHalfWidths)

translation = initialTranslation; %zeros(2,1);

filterHalfWidths = sort(filterHalfWidths);

% Compute the derivatives (and uniform blur) at different scales
pyramid = computeGradientPyramid_integralImage(image, filterHalfWidths);

figure(100); imshow(uint8(image));
for pyramidLevel = length(filterHalfWidths):-1:1
   
    % Find local maxima
    % Later, this could be extended to any area with strong gradient
    horizontalExtrema = findLocalExtrema(pyramid{pyramidLevel,2}, true, .0001);
    verticalExtrema = findLocalExtrema(pyramid{pyramidLevel,3}, false, .0001);
    
%     extrema = [horizontalExtrema, verticalExtrema];
    extrema = [horizontalExtrema];

    binaryImage = zeros(size(image));
    for i = 1:size(extrema,2)
        binaryImage(extrema(2,i), extrema(1,i)) = 1;
    end
    
    figure(pyramidLevel); imshow(binaryImage);
    
%     keyboard
    
end % for pyramidLevel = length(filterHalfWidths):-1:1

keyboard

end % function lucasKanade_integralImage(image, template, initialTranslation, filterHalfWidths)

function extrema = findLocalExtrema(image, findHorizontal, maximaValueThreshold)

    extrema = zeros(2,0);
    
    if findHorizontal
        for y = 1:size(image,1)
            for x = 2:(size(image,2)-1)
                if image(y,x) > (image(y,x-1)+maximaValueThreshold) && image(y,x) > (image(y,x+1)+maximaValueThreshold) ||...
                   (image(y,x)+maximaValueThreshold) < image(y,x-1) && (image(y,x)+maximaValueThreshold) < image(y,x+1)
                    extrema(:,end+1) = [x,y];                
                end
            end % for x = 1:size(image,2)
        end % for y = 1:size(image,1)
    else
        for y = 2:(size(image,1)-1)
            for x = 1:size(image,2)
                if image(y,x) > (image(y-1,x)+maximaValueThreshold) && image(y,x) > (image(y+1,x)+maximaValueThreshold) ||...
                   (image(y,x)+maximaValueThreshold) < image(y-1,x) && (image(y,x)+maximaValueThreshold) < image(y+1,x)
                    extrema(:,end+1) = [x,y];                
                end
            end % for x = 1:size(image,2)
        end % for y = 1:size(image,1)
    end
end % function extrema = findLocalExtrema()

