% function [labelName, labelID, minDifference] = TestExhaustiveSearch(allImages, img, tform, threshold)

% AllMarkerImages = VisionMarkerTrained.LoadAllMarkerImages(VisionMarkerTrained.TrainingImageDir);

% im = im2double(imread('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/../images/cozmo_date2014_06_04_time16_55_15_frame0.png'));
% corners = [43.7794,10.0951; 43.0506, 33.4150; 67.0992, 10.3381; 66.1275, 33.4150];
% [labelName, labelID, minDifference] = TestExhaustiveSearch(VisionMarkerTrained.AllMarkerImages, im, corners, 'forward')

function [labelName, labelID, minDifference] = TestExhaustiveSearch(allImages, img, corners, matchType)
    allImageSize = size(allImages{1}{1});
    
    tforms = cell(4,1);
    warpedImages = cell(4,1);
    
    img = double(img);
    
    imgMask = roipoly(img, corners([1,2,4,3],1), corners([1,2,4,3],2));
        
    % TODO: is width and height swapped?
    allImagesCorners = [0 0 allImageSize(2) allImageSize(2); 0 allImageSize(1) 0 allImageSize(1)]';
    
    diffs = zeros(length(allImages), 4);
    
    if strcmpi(matchType, 'forward')
        meanValue = mean(img(imgMask > 0.5));
        
        % TODO: is width and height swapped?
        allImagesCorners = [0 0 allImageSize(2) allImageSize(2); 0 allImageSize(1) 0 allImageSize(1)]';
        
        originalPixels = img(imgMask);
        originalPixels(originalPixels < meanValue) = 0;
        originalPixels(originalPixels > 0) = 1;
        
        for iImage = 1:length(allImages)
            for iRotation = 1:4
                tforms{iRotation} = cp2tform(allImagesCorners, corners, 'projective');
                
                allImagesCorners = allImagesCorners([3,1,4,2],:);
                
                [warpedImages{iRotation}, ~, ~] = lucasKande_warpGroundTruth(allImages{iImage}{1}, tforms{iRotation}.tdata.T', size(img));
                
                warpedPixels = warpedImages{iRotation}(imgMask);
                warpedPixels(isnan(warpedPixels)) = 0;
                warpedPixels(warpedPixels < 0.5) = 0;
                warpedPixels(warpedPixels > 0) = 1;
                
                diff = mean(abs(originalPixels - warpedPixels));
                
                diffs(iImage, iRotation) = diff;
            end
        end
    elseif strcmpi(matchType, 'backward')
        for iRotation = 1:4
            tforms{iRotation} = cp2tform(allImagesCorners, corners, 'projective');
            
            [warpedImages{iRotation}, ~, ~] = lucasKande_warpGroundTruth(img, tforms{iRotation}.tdata.Tinv', size(allImages{1}{1}));
            
            meanValue = mean(warpedImages{iRotation}(:));
            warpedImages{iRotation}(warpedImages{iRotation} < meanValue) = 0;
            warpedImages{iRotation}(warpedImages{iRotation} > 0) = 1;
            
            allImagesCorners = allImagesCorners([3,1,4,2],:);
        end
        
        for iImage = 1:length(allImages)
            databaseImage = allImages{iImage}{1};
            databaseImage(isnan(databaseImage)) = 0;
            databaseImage(databaseImage < 0.5) = 0;
            databaseImage(databaseImage > 0) = 1;
            
            for iRotation = 1:4
                diff = mean(abs(databaseImage(:) - warpedImages{iRotation}(:)));
                
                diffs(iImage, iRotation) = diff;
            end
        end
    end
    
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
    
    imshows(allImages{iy}{1}, warpedImages{ix})
    
    