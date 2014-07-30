% function [labelName, labelID, minDifference] = TestExhaustiveSearch(allImages, img, tform, threshold)

function [labelName, labelID, minDifference] = TestExhaustiveSearch(allImages, img, corners)
    allImageSize = size(allImages{1}{1});
    
    tforms = cell(4,1);
    warpedImages = cell(4,1);
    warpedMasks = cell(4,1);
    
    img = double(img);
    
    imgMask = roipoly(img, corners([1,2,4,3],1), corners([1,2,4,3],2));
    
    meanValue = mean(img(imgMask > 0.5));
    
    % TODO: is width and height swapped?
    allImagesCorners = [0 0 allImageSize(2) allImageSize(2); 0 allImageSize(1) 0 allImageSize(1)]';
    
    originalPixels = img(imgMask);
    originalPixels(originalPixels < meanValue) = 0;
    originalPixels(originalPixels > 0) = 1;
    
    diffs = zeros(length(allImages), 4);
    
    %     tic
    for iImage = 1:length(allImages)
        for iRotation = 1:4
            tforms{iRotation} = cp2tform(allImagesCorners, corners, 'projective');
            
            allImagesCorners = allImagesCorners([3,1,4,2],:);
            
            [warpedImages{iRotation}, ~, ~] = lucasKande_warpGroundTruth(allImages{iImage}{1}, tforms{iRotation}.tdata.T', size(img));
            %             figure(1 + iRotation); imshow(warpedImages{iRotation});
            
            warpedPixels = warpedImages{iRotation}(imgMask);
            warpedPixels(isnan(warpedPixels)) = 0;
            warpedPixels(warpedPixels < 0.5) = 0;
            warpedPixels(warpedPixels > 0) = 1;
            
            diff = mean(abs(originalPixels - warpedPixels));
            
%             if diff < 0.03
%                 keyboard
%             end
            
            diffs(iImage, iRotation) = diff;
        end
        %         pause(.01);
    end
    %     toc
    
    minDifference = min(diffs(:));
    
    [iy, ix] = find(diffs == minDifference, 1);
        
    if ix == 1
        rotationSuffix = '_000';
    elseif ix == 2
        rotationSuffix = '_090';
    elseif ix == 3
        rotationSuffix = '_180';
    elseif ix == 4
        rotationSuffix = '_270';
    end
        
    labelName = [allImages{iy}{2}, rotationSuffix];
    labelID = 1;
        
