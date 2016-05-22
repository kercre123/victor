%function images = captureImages(videoCaptures, undistortMaps)

function images = captureImages(videoCaptures, undistortMaps)
    images =  {};
    for i = 1:length(videoCaptures)
        image = videoCaptures{i}.read;
        image = rgb2gray2(image);
        
        if exist('undistortMaps', 'var') && ~isempty(undistortMaps)
            image = cv.remap(image, undistortMaps{i}.mapX, undistortMaps{i}.mapY);
            
            %             if (i == 1) && (rightImageWarpHomography is not None)
            %                 image = cv2.warpPerspective(image, rightImageWarpHomography, image.shape[::-1])
            %             end
        end
        
        images{end+1} = image; %#ok<AGROW>
    end % for i = 1:length(videoCaptures)
end % function images = captureImages()