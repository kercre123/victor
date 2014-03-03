% function binaryTracker_robustTranslation()

% im1 = imresize(imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_10.png'), [240,320]);
% im2 = imresize(imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_12.png'), [240,320]);
% mask1 = imresize(imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_12_mask.png'), [240,320]);
% maxSearchDistance = 20;
% maxScaleChangePercent = 0.1;
% [dx, dy] = binaryTracker_robustTranslation(im1, im2, mask1, 100, 2, maxSearchDistance, maxScaleChangePercent);


% mask1 = imread('C:\Anki\systemTestImages\cozmo_date2014_02_27_time14_56_37_frame0_mask.png');
% for i = 1:4:49
%     im1 = uint8(imresize(imread(sprintf('C:/Anki/systemTestImages/cozmo_date2014_02_27_time14_56_37_frame%d.png',i-1)), [240,320]));
%     im2 = uint8(imresize(imread(sprintf('C:/Anki/systemTestImages/cozmo_date2014_02_27_time14_56_37_frame%d.png',i)),   [240,320]));
%     [dx, dy] = binaryTracker_robustTranslation(im1, im2, mask1, 100, 2, 40, 0.1);
%     homography = eye(3,3); homography(1,3) = -dx; homography(2,3) = -dy;
%     warped2 = uint8(binaryTracker_warpWithHomography(im2, homography));
%     imshows(im2, im1, warped2, 'maximize');
%     figure(1); text(50,50,sprintf('image %d', i))
%     figure(2); text(50,50,sprintf('image %d', i))
%     figure(3); text(50,50,sprintf('image %d', i))
%     pause();
% end

function [dx, dy] = binaryTracker_robustTranslation(image1, image2, templateMask, grayvalueThreshold, minComponentWidth, maxSearchDistance, maxScaleChangePercent)

[xMinima1Image, xMaxima1Image, yMinima1Image, yMaxima1Image] = binaryTracker_binarizeWithAutomata(image1, grayvalueThreshold, minComponentWidth);
[xMinima2Image, xMaxima2Image, yMinima2Image, yMaxima2Image] = binaryTracker_binarizeWithAutomata(image2, grayvalueThreshold, minComponentWidth);

xMinima1Image(~templateMask) = 0;
yMinima1Image(~templateMask) = 0;
xMaxima1Image(~templateMask) = 0;
yMaxima1Image(~templateMask) = 0;

numMatches_xMinima = findMatches(xMinima1Image, xMinima2Image, maxSearchDistance);
numMatches_xMaxima = findMatches(xMaxima1Image, xMaxima2Image, maxSearchDistance);
numMatches_yMinima = findMatches(yMinima1Image, yMinima2Image, maxSearchDistance);
numMatches_yMaxima = findMatches(yMaxima1Image, yMaxima2Image, maxSearchDistance);

[maskIndsY, maskIndsX] = find(templateMask ~= 0);
maskSize = [max(maskIndsX(:))-min(maskIndsX(:)), max(maskIndsY(:))-min(maskIndsY(:))];

inlierDistance = round(mean(maskSize) * maxScaleChangePercent);

% maxMatches = max([max(numMatches_xMinima(:)), max(numMatches_xMaxima(:)), max(numMatches_yMinima(:)), max(numMatches_yMaxima(:))]);

blurSize = 2*inlierDistance + 1;

blurKernel1D = fspecial('gaussian', [blurSize,1], blurSize/2); 
blurKernel1D = blurKernel1D / max(blurKernel1D(:));

blurKernel2D = fspecial('gaussian', [blurSize,blurSize], blurSize/2); 
blurKernel2D = blurKernel2D / max(blurKernel2D(:));

numMatches_xMinimaBlur1 = imfilter(numMatches_xMinima, blurKernel1D');
numMatches_xMaximaBlur1 = imfilter(numMatches_xMaxima, blurKernel1D');
numMatches_yMinimaBlur1 = imfilter(numMatches_yMinima, blurKernel1D);
numMatches_yMaximaBlur1 = imfilter(numMatches_yMaxima, blurKernel1D);

totalMatches = numMatches_xMinima + numMatches_xMaxima + numMatches_yMinima + numMatches_yMaxima;

totalMatches1 = numMatches_xMinima + numMatches_yMinima;
totalMatches2 = numMatches_xMinima + numMatches_yMaxima;
totalMatches3 = numMatches_xMaxima + numMatches_yMinima;
totalMatches4 = numMatches_xMaxima + numMatches_yMaxima;

totalMatches_blur1 = numMatches_xMinimaBlur1 + numMatches_xMaximaBlur1 + numMatches_yMinimaBlur1 + numMatches_yMaximaBlur1;
totalMatchesFilt = imfilter(totalMatches, blurKernel2D);

% maxVal = max(totalMatchesFilt(:));
% [indsY, indsX] = find(totalMatchesFilt == maxVal);

maxVal = max(totalMatches_blur1(:));
[indsY, indsX] = find(totalMatches_blur1 == maxVal);

% imshows(numMatches_xMinima/maxMatches, numMatches_xMaxima/maxMatches, numMatches_yMinima/maxMatches, numMatches_yMaxima/maxMatches, totalMatches/max(totalMatches(:)));
% imshows(totalMatches/max(totalMatches(:)), totalMatchesFilt/max(totalMatchesFilt(:)));

offsets = (-maxSearchDistance):maxSearchDistance;
dy = offsets(indsY(1));
dx = offsets(indsX(1));

totalMatchesFiltL = totalMatchesFilt .^ 4;

% imshows(totalMatches1/max(totalMatches1(:)), totalMatches2/max(totalMatches2(:)), totalMatches3/max(totalMatches3(:)), totalMatches4/max(totalMatches4(:)), totalMatches/max(totalMatches(:)), totalMatchesFiltL/max(totalMatchesFiltL(:)), 'maximize')

disp(sprintf('Best offset is (%d,%d)', dx, dy))

% figure(1); 
% hold off
% % subplot(1,2,1); imshow(image1); hold on;
% % subplot(1,2,2); imshow(image2); hold on;
% imshow(image1);
% hold on;
% plot([size(image1,2)/2, dx + size(image1,2)/2], [size(image1,1)/2, dy + size(image1,1)/2]);
% 
% figure(2);
% imshow(image2);



% keyboard

function numMatches = findMatches(binaryImage1, binaryImage2, maxSearchDistance)
    [indsY, indsX] = find(binaryImage1 ~= 0);
    
    offsets = (-maxSearchDistance):maxSearchDistance;
    
    numMatches = zeros(length(offsets), length(offsets));
    
    for iDy = 1:length(offsets)
        dy = offsets(iDy);
        
        for iDx = 1:length(offsets)
            dx = offsets(iDx);
            
            offsetIndsY = indsY + dy;
            offsetIndsX = indsX + dx;
            
            validInds = ...
                (offsetIndsY>0) & (offsetIndsY<=size(binaryImage2,1)) &...
                (offsetIndsX>0) & (offsetIndsX<=size(binaryImage2,2));
            
            offsetIndsY = offsetIndsY(validInds);
            offsetIndsX = offsetIndsX(validInds);
            
            for i = 1:length(offsetIndsY)
                if binaryImage2(offsetIndsY(i), offsetIndsX(i)) ~= 0
                    numMatches(iDy, iDx) = numMatches(iDy, iDx) + 1;
                end
            end
        end
    end
    
    
    
    