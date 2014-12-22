% function [labelName, labelID, minDifference] = visionMarker_exhaustiveMatch_search(allImages, img, tform, threshold)

% AllMarkerImages = VisionMarkerTrained.LoadAllMarkerImages(VisionMarkerTrained.TrainingImageDir);

% im = im2double(imread('C:/Anki/products-cozmo-large-files/systemTestsData/scripts/../images/cozmo_date2014_06_04_time16_55_15_frame0.png'));
% corners = [43.7794,10.0951; 43.0506, 33.4150; 67.0992, 10.3381; 66.1275, 33.4150];
% [labelName, labelID, minDifference] = visionMarker_exhaustiveMatch_search(VisionMarkerTrained.AllMarkerImages, im, corners, 'forward')

function [labelName, labelID, minDifference] = visionMarker_exhaustiveMatch_search(allImages, img, corners, matchType)
    allImageSize = size(allImages{1}{1});
    
    tforms = cell(4,1);
    warpedImages = cell(4,1);
    
    img = im2double(img);
    
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
                
                [warpedImages{iRotation}, ~, ~] = warpProjective(allImages{iImage}{1}, tforms{iRotation}.tdata.T', size(img));
                
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
            
            [warpedImages{iRotation}, ~, ~] = warpProjective(img, tforms{iRotation}.tdata.Tinv', size(allImages{1}{1}));
            
            allImagesCorners = allImagesCorners([3,1,4,2],:);
        end
        
        for iImage = 1:length(allImages)
            databaseImage = allImages{iImage}{1};
            databaseImage(isnan(databaseImage)) = 0;
            databaseImage(databaseImage < 0.5) = 0;
            databaseImage(databaseImage > 0) = 1;
            
            for iRotation = 1:4
                warpedBinary = warpedImages{iRotation}(:);
                
                meanValue = mean(warpedBinary);
                
                warpedBinary(warpedBinary < meanValue) = 0;
                warpedBinary(warpedBinary > 0) = 1;
                
                diff = mean(abs(databaseImage(:) - warpedBinary));
                
                diffs(iImage, iRotation) = diff;
            end
        end
    elseif strcmpi(matchType, 'backward_noEdges')
        warpedInds = cell(4,1);
        
        imgU8 = im2uint8(img);
        
        lt = 0.2;
        ht = 0.8;
        h = hist(imgU8(imgMask), 0:255);
        c = cumsum(h);
        c = c/max(c);
        
        low = find(c<=lt);
        low = low(end);
        
        high = find(c>=ht);
        high = high(1);
        
        imgNoEdges = nan * zeros(size(img));
        imgNoEdges(imgU8 <= low) = 0;
        imgNoEdges(imgU8 >= high) = 1;
        
        for iRotation = 1:4
            tforms{iRotation} = cp2tform(allImagesCorners, corners, 'projective');
            
            [warpedImages{iRotation}, ~, ~] = warpProjective(imgNoEdges, tforms{iRotation}.tdata.Tinv', size(allImages{1}{1}), [], 'nearest');
            warpedInds{iRotation} = find(~isnan(warpedImages{iRotation}));
            
            allImagesCorners = allImagesCorners([3,1,4,2],:);
        end
        
        for iImage = 1:length(allImages)
            databaseImage = allImages{iImage}{1};
            databaseImage(isnan(databaseImage)) = 0;
            databaseImage(databaseImage < 0.5) = 0;
            databaseImage(databaseImage > 0) = 1;
            
            for iRotation = 1:4
                databaseSelected = databaseImage(warpedInds{iRotation});
                warpedSelected = warpedImages{iRotation}(warpedInds{iRotation});
                
                diff = mean(abs(databaseSelected - warpedSelected));
                
                diffs(iImage, iRotation) = diff;
            end
        end
    else
        assert(false);
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
    
    %     imshows(allImages{iy}{1}, warpedImages{ix})
    
    