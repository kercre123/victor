function [stereoProcessor, undistortMaps, masks] = initStereo(numDisparities, disparityType)
    
    if strcmpi(disparityType, 'bm')
        disparityWindowSize = 51;
        
        % TODO: add window size
        stereoProcessor = cv.StereoBM('Preset', 'Basic', 'NDisparities', numDisparities);
    else
        assert(false);
    end
    
    % Calibration for the Spynet stereo pair
     
    distCoeffs1 = [0.182998, -0.417100, 0.005281, -0.004976, 0.000000];
    distCoeffs2 = [0.165733, -0.391832, 0.002453, -0.011111, 0.000000];

    cameraMatrix1 = ...
        [726.606787, 0.000000, 321.470791;
         0.000000, 724.172303, 293.913773;
         0.000000, 0.000000, 1.000000];

    cameraMatrix2 = ...
        [743.337415, 0.000000, 308.270936;
         0.000000, 741.060548, 254.206903;
         0.000000, 0.000000, 1.000000];

    R = [0.993527, -0.023641, 0.111109;
         0.021796, 0.999604, 0.017785;
         -0.111486, -0.015248, 0.993649];

    T = [-13.201223, -0.708310, 0.961289]';
    
    imageSize = [640, 480];

    rectifyStruct = cv.stereoRectify(cameraMatrix1, distCoeffs1, cameraMatrix2, distCoeffs2, imageSize, R, T);
    
    [leftUndistortMapX, leftUndistortMapY] = cv.initUndistortRectifyMap(cameraMatrix1, distCoeffs1, rectifyStruct.P1, imageSize, 'R', rectifyStruct.R1);
    
    [rightUndistortMapX, rightUndistortMapY] = cv.initUndistortRectifyMap(cameraMatrix2, distCoeffs2, rectifyStruct.P2, imageSize, 'R', rectifyStruct.R2);
    
    undistortMaps = cell(2,1);
    undistortMaps{1} = struct('mapX', leftUndistortMapX, 'mapY', leftUndistortMapY);
    undistortMaps{2} = struct('mapX', rightUndistortMapX, 'mapY', rightUndistortMapY);
    
    masks = cell(2,1);
    
    masks{1} = ones(imageSize(2:-1:1), 'uint8');
    masks{1}(leftUndistortMapX(:,:,1) < 0) = 0;
    masks{1}(leftUndistortMapX(:,:,1) >= imageSize(1)) = 0;
    masks{1}(leftUndistortMapX(:,:,2) < 0) = 0;
    masks{1}(leftUndistortMapX(:,:,2) >= imageSize(2)) = 0;
    
    masks{2} = ones(imageSize(2:-1:1), 'uint8');
    masks{2}(rightUndistortMapX(:,:,1) < 0) = 0;
    masks{2}(rightUndistortMapX(:,:,1) >= imageSize(1)) = 0;
    masks{2}(rightUndistortMapX(:,:,2) < 0) = 0;
    masks{2}(rightUndistortMapX(:,:,2) >= imageSize(2)) = 0;    
end % function undistortMaps = computeUndistortMaps()

