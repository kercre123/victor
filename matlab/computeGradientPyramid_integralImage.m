% function pyramid = computeGradientPyramid_integralImage(image, filterHalfWidths)

% pyramid = computeGradientPyramid_integralImage(im1, [1,5,9,14]);

function pyramid = computeGradientPyramid_integralImage(image, filterHalfWidths)

% image(:,:) = 1;
% image(:, 1:120) = 1;
% image(:, 121:end) = 0;

integralImage = integralimage(image);

pyramid = cell(length(filterHalfWidths), 2);

for k = 1:length(filterHalfWidths)
    curHalfWidth = filterHalfWidths(k);
    curDivisor = ((2*curHalfWidth+1)*curHalfWidth);
    
    dx = [-curHalfWidth, -curHalfWidth, curHalfWidth, -1, -1 / ((2*curHalfWidth+1)*curHalfWidth);
          -curHalfWidth, 1, curHalfWidth, curHalfWidth, 1 / ((2*curHalfWidth+1)*curHalfWidth)];
      
    dy = [-curHalfWidth, -curHalfWidth, -1, curHalfWidth, -1 / ((2*curHalfWidth+1)*curHalfWidth);
          1, -curHalfWidth, curHalfWidth, curHalfWidth, 1 / ((2*curHalfWidth+1)*curHalfWidth)];

%     dx = [-curHalfWidth, -curHalfWidth, curHalfWidth, -1, 1;
%           -curHalfWidth, 1, curHalfWidth, curHalfWidth, 0];
%           
%     dy = [-curHalfWidth, -curHalfWidth, -1, curHalfWidth, 1;
%           1, -curHalfWidth, curHalfWidth, curHalfWidth, 0];

%     dx1 = [-curHalfWidth, -curHalfWidth, curHalfWidth, -1, 1;
%            -curHalfWidth, 1, curHalfWidth, curHalfWidth, 0];
%        
%     dx2 = [-curHalfWidth, -curHalfWidth, curHalfWidth, -1, 0;
%            -curHalfWidth, 1, curHalfWidth, curHalfWidth, 1];
%       
%     dy = [-curHalfWidth, -curHalfWidth, -1, curHalfWidth, -1;
%           1, -curHalfWidth, curHalfWidth, curHalfWidth, 1];
    
%     disp(sprintf('%d) curHalfWidth=%d sum=%d', k, curHalfWidth, (2*curHalfWidth+1)*curHalfWidth));
    
%     pyramid{k,1} = (integralfilter(integralImage, dx1) - integralfilter(integralImage, dx2)) / curDivisor;

    pyramid{k,1} = integralfilter(integralImage, dx);
    pyramid{k,2} = integralfilter(integralImage, dy);
end

figure(1); 
subplot(1,2,1); imshow(uint8(image));
subplot(1,2,2); imshow(uint8(image));

for k = 1:length(filterHalfWidths)
    figure(k+1); 
    subplot(1,2,1); imshow(abs(pyramid{k,1})/100);
    subplot(1,2,2); imshow(abs(pyramid{k,2})/100);
end

keyboard