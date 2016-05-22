
% function faceDetection_feret_createTrainingSet()

% faceDetection_feret_createTrainingSet('/Volumes/FastExternal_Mac/colorferet/', '/Volumes/FastExternal_Mac/', '~/Documents/Anki/products-cozmo-large-files/faceDetection/feret%s.dat');

function faceDetection_feret_createTrainingSet(feretBaseDirectory, removePrefix, outputFilenamePattern)
    
    imageDirectories1 = dir([feretBaseDirectory, 'dvd1/data/images/']);
    imageDirectories2 = dir([feretBaseDirectory, 'dvd2/data/images/']);
    
    imageDirectories = {};
    
    for iFile = 3:length(imageDirectories1)
        if imageDirectories1(iFile).isdir
            imageDirectories{end+1} = [feretBaseDirectory, '/dvd1/data/images/', imageDirectories1(iFile).name]; %#ok<AGROW>
        end
    end
    
    for iFile = 3:length(imageDirectories2)
        if imageDirectories2(iFile).isdir
            imageDirectories{end+1} = [feretBaseDirectory, '/dvd2/data/images/', imageDirectories2(iFile).name]; %#ok<AGROW>
        end
    end
    
    showFaces = false;
%     rectangleExpansionPercent = 2.0;
    rectangleExpansionPercent = 1.75;
    
    numFaces1 = saveGroundTruth(imageDirectories, {'*_f*.png'}, showFaces, rectangleExpansionPercent, removePrefix, sprintf(outputFilenamePattern, 'FrontFaces'));
    disp(sprintf('Saved %d front faces', numFaces1));
    
    numFaces2 = saveGroundTruth(imageDirectories, {'*_f*.png','*_rb*.png','*_rc*.png','*_ql*.png','*_qr*.png'}, showFaces, rectangleExpansionPercent, removePrefix, sprintf(outputFilenamePattern, 'MostlyFrontFaces'));
    disp(sprintf('Saved %d mostly front faces', numFaces2));
    
    keyboard
    
end % function faceDetection_feret_createTrainingSet()

function numFaces = saveGroundTruth(imageDirectories, faceFilenamePatterns, showFaces, rectangleExpansionPercent, removePrefix, outputFilename)
    
    faceFilenames = {};
    for iFile = 1:length(imageDirectories)
        for iPattern = 1:length(faceFilenamePatterns)
            files = rdir([imageDirectories{iFile}, '/', faceFilenamePatterns{iPattern}]);

            for jFile = 1:length(files)
                imageFilename = files(jFile).name;
                groundTruthFilename = strrep(strrep(imageFilename, '/data/images/', '/data/ground_truths/name_value/'), '.png', '.mat');
                faceFilenames{end+1} = {imageFilename, groundTruthFilename}; %#ok<AGROW>
            end
        end
    end % for iFile = 1:length(imageDirectories)
    
    fileId = fopen(outputFilename, 'w');
    
    numFaces = 0;
    for iFace = 1:length(faceFilenames)
        load(faceFilenames{iFace}{2});
        
        % TODO: warp the face, or change the grayvalues
        if isfield(data, 'right_eye_coordinates')
            exactTop = min(data.right_eye_coordinates(2), data.left_eye_coordinates(2));
            exactRect = [data.right_eye_coordinates(1), exactTop, data.left_eye_coordinates(1)-data.right_eye_coordinates(1), data.mouth_coordinates(2)-exactTop];
            
            expandedRect = round([...
                exactRect(1) - 0.25*rectangleExpansionPercent*exactRect(3),...
                exactRect(2) - 0.25*rectangleExpansionPercent*exactRect(4),...
                rectangleExpansionPercent*exactRect(3:4)]);
            
            if showFaces
                im = imread(faceFilenames{iFace}{1});
                
                hold off;
                imshows(im);
                hold on;
                
                scatter(data.left_eye_coordinates(1), data.left_eye_coordinates(2), 'r+');
                scatter(data.right_eye_coordinates(1), data.right_eye_coordinates(2), 'r+');
                scatter(data.mouth_coordinates(1), data.mouth_coordinates(2), 'b+');
                
                rectangle('Position', exactRect)
                rectangle('Position', expandedRect)
                
                pause(.5);
            end % if showFaces
            
            lineOut = sprintf('%s   %d', strrep(strrep(faceFilenames{iFace}{1}, removePrefix, ''), '//', '/'), 1);
            lineOut = [lineOut, sprintf('   %d %d %d %d', expandedRect(1), expandedRect(2), expandedRect(3), expandedRect(4))]; %#ok<AGROW>
            lineOut = [lineOut, sprintf('\n')]; %#ok<AGROW>
            
            fwrite(fileId, lineOut);
            numFaces = numFaces + 1;
        else
            % disp(faceFilenames{iFace}{2});
        end        
    end % for iFace = 1:length(faceFilenames)
    
    fclose(fileId);
end


