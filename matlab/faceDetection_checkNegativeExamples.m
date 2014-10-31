
% function faceDetection_checkNegativeExamples()

% faceDetection_checkNegativeExamples('~/Documents/Anki/products-cozmo-large-files/face/detection/negativeExamples.dat');

function faceDetection_checkNegativeExamples(negativeExamplesFilename)
    %#ok<*NASGU>
    
    printAllFilenames = false;
    checkWithFaceDetector = true;
    
    if printAllFilenames
        baseDatasetDirectory = '/Volumes/PeterMac/external';
        files = [];
        files = [files; rdir([baseDatasetDirectory, '/256_ObjectCategories/257.clutter/*.jpg'])];
        files = [files; rdir([baseDatasetDirectory, '/icdar2011/testset/*.jpg'])];
        %     files = [files; rdir([baseDatasetDirectory, ''])];
        
        for iFile = 1:length(files)
            disp(files(iFile).name);
        end
    end % if printAllFilenames
    
    if checkWithFaceDetector
        fileId = fopen(negativeExamplesFilename, 'r');
        
        filename = fgetl(fileId);
        % Check if any faces are detected
        while filename ~= -1
            %         fprintf('%d/%d ', iFile, length(files));
            im = rgb2gray2(imread(filename));
            [faces, eyes] = mexFaceDetect(im);
            if ~isempty(faces)
                disp(filename);
                
                %             figure(1);
                
                hold off;
                imshow(im);
                hold on;
                
                for iFace = 1:size(faces,1)
                    rectangle('Position', faces(iFace,1:4), 'LineWidth', 3, 'EdgeColor', [1.0,0,0]);
                end
                
                pause();
            else
                %             disp(sprintf(' '));
            end
            
            filename = fgetl(fileId);
        end % while filename ~= -1
        
        fclose(fileId);
    end
    
    keyboard
end % function faceDetection_checkNegativeExamples()

function filenames = getFilenames()
end % function filenames = getFilenames()