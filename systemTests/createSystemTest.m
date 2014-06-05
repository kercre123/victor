% function createSystemTest(outputFilename, inputFilenameRanges)

% createSystemTest('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_frontoParallel_1000mm_lightOff.json', 'C:/Anki/systemTestImages', 'Z:/Documents/Box Documents/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_52_34_frame0.png','cozmo_date2014_06_04_time16_52_43_frame0.png'}, 1000, 0, 0)
% createSystemTest('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_frontoParallel_1000mm_lightOn.json', 'C:/Anki/systemTestImages', 'Z:/Documents/Box Documents/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_52_57_frame0.png','cozmo_date2014_06_04_time16_53_06_frame0.png'}, 1000, 0, 1)
% createSystemTest('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_frontoParallel_1500mm_lightOff.json', 'C:/Anki/systemTestImages', 'Z:/Documents/Box Documents/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_53_19_frame0.png','cozmo_date2014_06_04_time16_53_28_frame0.png'}, 1500, 0, 0)
% createSystemTest('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_frontoParallel_2000mm_lightOff.json', 'C:/Anki/systemTestImages', 'Z:/Documents/Box Documents/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_53_53_frame0.png','cozmo_date2014_06_04_time16_54_02_frame0.png'}, 2000, 0, 0)
% createSystemTest('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_frontoParallel_2500mm_lightOff.json', 'C:/Anki/systemTestImages', 'Z:/Documents/Box Documents/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_54_26_frame0.png','cozmo_date2014_06_04_time16_54_36_frame0.png'}, 2500, 0, 0)
% createSystemTest('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_frontoParallel_2500mm_lightOn.json', 'C:/Anki/systemTestImages', 'Z:/Documents/Box Documents/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_54_49_frame0.png','cozmo_date2014_06_04_time16_54_58_frame0.png'}, 2500, 0, 1)
% createSystemTest('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_frontoParallel_3000mm_lightOff.json', 'C:/Anki/systemTestImages', 'Z:/Documents/Box Documents/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_55_11_frame0.png','cozmo_date2014_06_04_time16_55_20_frame0.png'}, 3000, 0, 0)
% createSystemTest('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_frontoParallel_4000mm_lightOff.json', 'C:/Anki/systemTestImages', 'Z:/Documents/Box Documents/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_56_30_frame0.png','cozmo_date2014_06_04_time16_56_39_frame0.png'}, 4000, 0, 0)
% createSystemTest('C:/Anki/products-cozmo/systemTests/tests/fiducialDetection_frontoParallel_4000mm_lightOn.json', 'C:/Anki/systemTestImages', 'Z:/Documents/Box Documents/Cozmo SE/systemTestImages_all/', {'cozmo_date2014_06_04_time16_57_04_frame0.png','cozmo_date2014_06_04_time16_57_13_frame0.png'}, 4000, 0, 1)

function createSystemTest(outputFilename, imageCopyPath, inputDirectory, inputFilenameRange, distance, angle, light)

numImagesRequired = 10; % for the .1:.1:1.0 exposures

fullFilename = strrep(mfilename('fullpath'), '\', '/');
slashIndexes = strfind(fullFilename, '/');
fullFilenamePath = fullFilename(1:(slashIndexes(end)));
templateFilename = [fullFilenamePath, 'testTemplate.json'];

jsonTestData = loadjson(templateFilename);
 
possibleInputFiles = dir([inputDirectory, '*.png']);

jsonTestData.Poses = [];

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

numImageFilesFound = (endIndex - startIndex + 1);
if numImageFilesFound ~= numImagesRequired
    disp(sprintf('%s has %d image files', outputFilename, numImageFilesFound));
    return;
end

exposure = 0.1;
for iIn = startIndex:endIndex
    curPose.ImageFile = ['../../../systemTestImages/', possibleInputFiles(iIn).name];
    curPose.Scene.CameraExposure = exposure;
    curPose.Scene.Distance = distance;
    curPose.Scene.angle = angle;    
    curPose.Scene.light = light;
    jsonTestData.Poses{end+1} = curPose;
    exposure = exposure + 0.1;
    
    copyfile([inputDirectory,possibleInputFiles(iIn).name], imageCopyPath);
end

disp(sprintf('Saving %s...', outputFilename));
savejson('', jsonTestData, outputFilename);

% keyboard
