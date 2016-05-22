% function pyramid = computeGradientPyramid_integralImage(image, filterHalfWidths)

% pyramid = computeGradientPyramid_integralImage(im, [1,5,9,14]);

function pyramid = computeGradientPyramid_integralImage(image, filterHalfWidths)

% showResults = true;
showResults = false;

integralImage = integralimage(image);

pyramid = cell(length(filterHalfWidths), 3);

useLargeFilter = false;

for k = 1:length(filterHalfWidths)
    curHalfWidth = filterHalfWidths(k);
    
    
    blur = [-curHalfWidth, -curHalfWidth, curHalfWidth, curHalfWidth, 1 / ((2*curHalfWidth+1)^2)];
    
    if useLargeFilter
        dx = [-curHalfWidth, -curHalfWidth, curHalfWidth, -1, -1 / ((2*curHalfWidth+1)*curHalfWidth);
              -curHalfWidth, 1, curHalfWidth, curHalfWidth, 1 / ((2*curHalfWidth+1)*curHalfWidth)];

        dy = [-curHalfWidth, -curHalfWidth, -1, curHalfWidth, -1 / ((2*curHalfWidth+1)*curHalfWidth);
              1, -curHalfWidth, curHalfWidth, curHalfWidth, 1 / ((2*curHalfWidth+1)*curHalfWidth)];
    else
        filterSize = 3;
        
        dx = [-filterSize, -curHalfWidth, filterSize, -1, -1 / ((2*curHalfWidth+1)*curHalfWidth);
              -filterSize, 1,             filterSize, curHalfWidth, 1 / ((2*curHalfWidth+1)*curHalfWidth)];

        dy = [-curHalfWidth, -filterSize, -1,           filterSize, -1 / ((2*curHalfWidth+1)*curHalfWidth);
              1,             -filterSize, curHalfWidth, filterSize, 1 / ((2*curHalfWidth+1)*curHalfWidth)];
    end

    pyramid{k,1} = integralfilter(integralImage, blur);
    pyramid{k,2} = integralfilter(integralImage, dx);
    pyramid{k,3} = integralfilter(integralImage, dy);
end

if showResults
    figure(1); 
    subplot(1,3,1); imshow(uint8(image));
    subplot(1,3,2); imshow(uint8(image));
    subplot(1,3,3); imshow(uint8(image));

    for k = 1:length(filterHalfWidths)
        figure(k+1); 
        subplot(1,3,1); imshow(abs(pyramid{k,1})/255);
        subplot(1,3,2); imshow(abs(pyramid{k,2})/255);
        subplot(1,3,3); imshow(abs(pyramid{k,3})/255);
    end
end