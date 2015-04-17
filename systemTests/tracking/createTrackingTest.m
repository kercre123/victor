% function createTrackingTest(outputFilename, inputFilenameRanges)

% createTrackingTest('/Users/pbarnum/Documents/Anki/products-cozmo-large-files/systemTestsData/trackingScripts/tracking_00000.json', '/Users/pbarnum/Documents/Anki/products-cozmo-large-files/systemTestsData/trackingImages', '/Users/pbarnum/Documents/datasets/tracking/2015_04_16pickAndPlace/', {'cozmo1_img_544810.jpg','cozmo1_img_549305.jpg'}, [155.83712, 117.87848, 374.98139, 371.84817, 320, 240, 25.0])

function createTrackingTest(outputFilename, imageCopyPath, inputDirectory, inputFilenameRange, physicalParameters)
    
    fullFilename = strrep(mfilename('fullpath'), '\', '/');
    slashIndexes = strfind(fullFilename, '/');
    fullFilenamePath = fullFilename(1:(slashIndexes(end)));
    templateFilename = [fullFilenamePath, '../testTemplate.json'];
    
    slashIndexes2 = strfind(outputFilename, '/');
    jsonShortFilename = outputFilename((slashIndexes2(end)+1):(end-5));
    
    if imageCopyPath(end) ~= '/'
        imageCopyPath = [imageCopyPath, '/'];
    end
    
    if inputDirectory(end) ~= '/'
        inputDirectory = [inputDirectory, '/'];
    end
    
    jsonTestData = loadjson(templateFilename);
    
    possibleInputFiles = dir([inputDirectory, '*.', inputFilenameRange{1}((end-2):end)]);
    
    jsonTestData.Poses = [];
    
    jsonTestData = sanitizeJsonTest(jsonTestData);
    
    firstFile = inputFilenameRange{1};
    lastFile = inputFilenameRange{2};
    
    startIndex = -1;
    for iIn = 1:length(possibleInputFiles)
        if strcmp(firstFile, possibleInputFiles(iIn).name)
            startIndex = iIn;
            break;
        end
    end
    
    endIndex = -1;
    for iIn = startIndex:length(possibleInputFiles)
        if strcmp(lastFile, possibleInputFiles(iIn).name)
            endIndex = iIn;
            break;
        end
    end
    
    assert(startIndex ~= -1);
    assert(endIndex ~= -1);
    
    if startIndex < endIndex
        numImageFilesFound = endIndex - startIndex + 1;
        increment = 1;
    else
        numImageFilesFound = startIndex - endIndex+ 1;
        increment = -1;
    end
    
    disp(sprintf('%s has %d image files', outputFilename, numImageFilesFound));
    
    for iIn = startIndex:increment:endIndex
        curPose.ImageFile = ['../trackingImages/', jsonShortFilename, '_', possibleInputFiles(iIn).name];
        jsonTestData.Poses{end+1} = curPose;
        
        inFilename = [inputDirectory,possibleInputFiles(iIn).name];
        outFilename = [imageCopyPath, jsonShortFilename, '_', possibleInputFiles(iIn).name];
        
        system(['cp ', inFilename, ' ', outFilename]);
    end
    
    % For speed, also save a copy with just the filenames
    outputFilename_all = [outputFilename(1:(end-5)), '_all.json'];   
    
    jsonTestData.CameraCalibration.center_x = physicalParameters(1);
    jsonTestData.CameraCalibration.center_y = physicalParameters(2);
    jsonTestData.CameraCalibration.focalLength_x = physicalParameters(3);
    jsonTestData.CameraCalibration.focalLength_y = physicalParameters(4);
    jsonTestData.CameraCalibration.ncols = physicalParameters(5);
    jsonTestData.CameraCalibration.nrows = physicalParameters(6);

    jsonTestData.Blocks(1).templateWidth_mm = physicalParameters(7);
    
    disp(sprintf('Saving %s...', outputFilename_all));
    savejson('', jsonTestData, outputFilename_all);
    
    % keyboard
end % createTrackingTest()
