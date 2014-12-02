function [stereoProcessor, undistortMaps, masks] = initStereo(numDisparities, disparityType)
    
    if strcmpi(disparityType, 'bm')
        disparityWindowSize = 51;
        
        % TODO: add window size
        stereoProcessor = cv.StereoBM('Preset', 'Basic', 'NDisparities', numDisparities);
    else
        assert(false);
    end
    
    % Calibration for the Spynet stereo pair
     
    distCoeffs1 = [0.164272, -0.376660, 0.000841, -0.013974, 0.000000];
    distCoeffs2 = [0.168726, -0.365303, 0.006350, -0.007782, 0.000000];

    cameraMatrix1 = [740.272685, 0.000000, 303.320456;
        0.000000, 737.724452, 250.412135;
        0.000000, 0.000000, 1.000000];

    cameraMatrix2 = [723.767887, 0.000000, 317.103693;
        0.000000, 721.290002, 295.642274;
        0.000000, 0.000000, 1.000000];

    R = [0.999395, 0.030507, -0.016679;
        -0.030129, 0.999293, 0.022470;
        0.017353, -0.021954, 0.999608];

    T = [-13.399603, 0.297130, 0.749115];

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

