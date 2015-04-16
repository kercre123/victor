
function simpleTrackingTest()
    
%     trackingDatabaseDirectory = '/Users/pbarnum/Documents/datasets/tracking/2015_04_14/';
%     filenameIncrement = 1;
%     firstFilename = 'cozmo1_img_53151.jpg';
%     lastFilename = 'cozmo1_img_57704.jpg';
%     templateQuad = [127.728426395939,43.7487309644670;173.048223350254,44.9670050761421;171.829949238579,89.3121827411168;127.728426395939,88.5812182741117];
    
%     trackingDatabaseDirectory = '/Users/pbarnum/Documents/datasets/tracking/2015_04_15KevinRotate/';
%     filenameIncrement = 1;

%     firstFilename = 'cozmo1_img_46168.jpg';
%     lastFilename = 'cozmo1_img_50012.jpg';
%     templateQuad = [51.0989847715736,64.7030456852792;183.403553299492,64.9467005076142;181.941624365482,186.774111675127;59.6269035532995,187.992385786802];
    
%     firstFilename = 'cozmo1_img_38656.jpg';
%     lastFilename = 'cozmo1_img_50012.jpg';
%     templateQuad = [93.9822335025381,66.4086294416243;229.454314720812,65.1903553299492;224.824873096447,190.916243654822;100.073604060914,194.814720812183];

%     trackingDatabaseDirectory = '/Users/pbarnum/Documents/datasets/tracking/2015_04_14/';
%     filenameIncrement = -1;
%     firstFilename = 'cozmo1_img_68059.jpg';
%     lastFilename = 'cozmo1_img_64874.jpg';
%     templateQuad = [129.068527918782,50.8147208121827;182.428934010152,51.5456852791878;179.992385786802,102.713197969543;129.068527918782,101.982233502538];
    
    trackingDatabaseDirectory = '/Users/pbarnum/Documents/datasets/tracking/2015_04_14/';
    filenameIncrement = 1;
    firstFilename = 'cozmo1_img_80567.jpg';
    lastFilename = 'cozmo1_img_83758.jpg';
    templateQuad = [129.555837563452,61.0482233502538;214.591370558376,61.0482233502538;212.154822335025,142.916243654822;130.530456852792,141.697969543147];

    %     radialDistortionCoeffs = [0.02865, 0.14764, -0.00074, 0.00046];
    focalLength_x = 374.98139;
    focalLength_y = 371.84817;
    camCenter_x = 155.83712;
    camCenter_y = 117.87848;
    templateWidth_mm = 25.0;
    
    parameters = cell(5,1);
    parameters{1}.fiducialSquareWidthFraction = 0.1;
    parameters{1}.numPyramidLevels = 1; % TODO: Compute from resolution to get down to a given size?
    parameters{1}.maxSamplesAtBaseLevel = 500; % a.k.a. numFiducialEdgeSamples
    parameters{1}.numSamplingRegions = 5;
    parameters{1}.numFiducialSquareSamples = 500; % a.k.a. numInteriorSamples
    parameters{1}.maxIterations = 50;
    parameters{1}.convergenceTolerance_angle = .05 * pi / 180;
    parameters{1}.convergenceTolerance_distance = .05;
    parameters{1}.verify_maxPixelDifference = 30;
    parameters{1}.normalizeImage = false;
    
    if parameters{1}.numFiducialSquareSamples > 0
        parameters{1}.scaleTemplateRegionPercent = 1.0 - parameters{1}.fiducialSquareWidthFraction;
    else
        parameters{1}.scaleTemplateRegionPercent = 1.1;
    end
    
    templateQuadCenter = mean(templateQuad,1);
    
    files = dir([trackingDatabaseDirectory, '*.jpg']);
    
    firstIndex = -1;
    lastIndex = -1;
    for iFile = 1:length(files)
        if strcmpi(files(iFile).name, firstFilename)
            firstIndex = iFile;
        end
        
        if strcmpi(files(iFile).name, lastFilename)
            lastIndex = iFile;
        end
    end % for iFile = 1:length(files)
    
    assert(firstIndex ~= -1);
    assert(lastIndex ~= -1);
    
    fileIndexes = firstIndex:filenameIncrement:lastIndex;
    
    images = cell(length(fileIndexes), 1);
    for iFile = 1:length(fileIndexes)
        images{iFile} = imread([trackingDatabaseDirectory,files(fileIndexes(iFile)).name]);
    end % for iFile = firstFilename:filenameIncrement:lastFilename
    
    %     templateMask = roipoly(images{1}, templateQuad(:,1), templateQuad(:,2));
    
%     imshows(images{1});
%     [x,y,c] = ginput(4); quad = [x,y];
    
    [samples,...
        angleX,...
        angleY,...
        angleZ,...
        tX,...
        tY,...
        tZ] = mexPlanar6dofTrack(...
        images{1},...
        templateQuad([1,4,2,3],:) - 1,...
        focalLength_x,...
        focalLength_y,...
        camCenter_x,...
        camCenter_y,...
        templateWidth_mm,...
        parameters{1}.scaleTemplateRegionPercent,...
        parameters{1}.numPyramidLevels,...
        parameters{1}.maxSamplesAtBaseLevel,...
        parameters{1}.numSamplingRegions,...
        parameters{1}.numFiducialSquareSamples,...
        parameters{1}.fiducialSquareWidthFraction,...
        parameters{1}.normalizeImage);
    
    figure(1);
    hold off;
    imshow(images{1});
    hold on;
    plot(templateQuad([1:end,1],1), templateQuad([1:end,1],2))
    scatter(samples(:,1) + templateQuadCenter(1) + 1, samples(:,2) + templateQuadCenter(2) + 1, 'g.');
    
    for iImage = 2:length(images)
        
        [verify_converged,...
            verify_meanAbsoluteDifference,...
            verify_numInBounds,...
            verify_numSimilarPixels,...
            currentQuad,...
            angleX,...
            angleY,...
            angleZ,...
            tX,...
            tY,...
            tZ,...
            tform] = mexPlanar6dofTrack(...
            images{iImage},...
            parameters{1}.maxIterations,...
            parameters{1}.convergenceTolerance_angle,...
            parameters{1}.convergenceTolerance_distance,...
            parameters{1}.verify_maxPixelDifference,...
            parameters{1}.normalizeImage);
        
        figure(2);
        hold off;
        imshow(images{iImage});
        hold on;
        plot(currentQuad([1,3,4,2,1],1) + 1, currentQuad([1,3,4,2,1],2) + 1);
        
        pause(0.5);
    end % for iImage = 2:length(images)
    
    
%     keyboard