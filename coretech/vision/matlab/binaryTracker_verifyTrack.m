% function binaryTracker_verifyTrack()

function numTemplatePixelsMatched = binaryTracker_verifyTrack(...
  homography, verifyRange,...
  xDecreasingTemplateList_x, xDecreasingTemplateList_y, yDecreasingTemplateList_x, yDecreasingTemplateList_y, xIncreasingTemplateList_x, xIncreasingTemplateList_y, yIncreasingTemplateList_x, yIncreasingTemplateList_y,...
  xDecreasingNew, xIncreasingNew, yDecreasingNew, yIncreasingNew)

imageHeight = size(xDecreasingNew,1);
imageWidth = size(xDecreasingNew,2);

homographyOffset = [(imageWidth-1)/2, (imageHeight-1)/2];

numTemplatePixelsMatched_xDecreasing = computeNumTemplateMatches(homography, homographyOffset, xDecreasingTemplateList_x, xDecreasingTemplateList_y, xDecreasingNew, verifyRange, false);
numTemplatePixelsMatched_xIncreasing = computeNumTemplateMatches(homography, homographyOffset, xIncreasingTemplateList_x, xIncreasingTemplateList_y, xIncreasingNew, verifyRange, false);
numTemplatePixelsMatched_yDecreasing = computeNumTemplateMatches(homography, homographyOffset, yDecreasingTemplateList_x, yDecreasingTemplateList_y, yDecreasingNew, verifyRange, true);
numTemplatePixelsMatched_yIncreasing = computeNumTemplateMatches(homography, homographyOffset, yIncreasingTemplateList_x, yIncreasingTemplateList_y, yIncreasingNew, verifyRange, true);

numTemplatePixelsMatched = numTemplatePixelsMatched_xDecreasing + numTemplatePixelsMatched_xIncreasing + numTemplatePixelsMatched_yDecreasing + numTemplatePixelsMatched_yIncreasing;


function numTemplatePixelsMatched = computeNumTemplateMatches(homography, homographyOffset, templateList_x, templateList_y, newImage, verifyRange, isVerticalCheck)
    templatePoints = [templateList_x, templateList_y, ones(size(templateList_y))]';
    warpedPoints = homography * templatePoints;
    warpedPoints = warpedPoints ./ repmat(warpedPoints(3,:), [3,1]);
    warpedPoints = warpedPoints(1:2,:);
    warpedPoints(1,:) = warpedPoints(1,:) + homographyOffset(1);
    warpedPoints(2,:) = warpedPoints(2,:) + homographyOffset(2);    
    
    numTemplatePixelsMatched = 0;
    for iPoint = 1:size(warpedPoints,2)
        if isVerticalCheck
            x =  round(warpedPoints(1,iPoint));
            ys = ((-verifyRange):verifyRange) + round(warpedPoints(2,iPoint));
            
            ys = ys(ys>=1 & ys<=size(newImage,1));
            
            if x >= 1 && x <= size(newImage,2)
                for y = ys
                    if newImage(y,x) ~= 0
                        numTemplatePixelsMatched = numTemplatePixelsMatched + 1;
                        break;
                    end
                end
            end
        else % if isVerticalCheck
            xs = ((-verifyRange):verifyRange) + round(warpedPoints(1,iPoint));
            y = round(warpedPoints(2,iPoint));
            
            xs = xs(xs>=1 & xs<=size(newImage,2));
            
            if y >= 1 && y <= size(newImage,1)
                for x = xs
                    if newImage(y,x) ~= 0
                        numTemplatePixelsMatched = numTemplatePixelsMatched + 1;
                        break;
                    end
                end
            end
        end % if isVerticalCheck ... else
    end % for iPoint = 1:size(warpedPoints,2)
    
