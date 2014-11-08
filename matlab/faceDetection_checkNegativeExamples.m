
% function faceDetection_checkNegativeExamples()

% faceDetection_checkNegativeExamples('~/Documents/Anki/products-cozmo-large-files/faceDetection/negativeExamples.dat');

function faceDetection_checkNegativeExamples(negativeExamplesFilename)
    %#ok<*NASGU>
    
    printAllFilenames = false;
    checkWithFaceDetector = false;
    showAllImages = true;
        
    if printAllFilenames
        baseDatasetDirectory = '/Volumes/PeterMac/external';
        files = [];
        files = [files; rdir([baseDatasetDirectory, '/256_ObjectCategories/257.clutter/*.jpg'])];
        files = [files; rdir([baseDatasetDirectory, '/icdar2011/testset/*.jpg'])];
        files = [files; rdir([baseDatasetDirectory, '/256_ObjectCategories/00*/*.jpg'])];
        files = [files; rdir([baseDatasetDirectory, '/256_ObjectCategories/01*/*.jpg'])];
        files = [files; rdir([baseDatasetDirectory, '/256_ObjectCategories/04*/*.jpg'])];
        files = [files; rdir([baseDatasetDirectory, '/256_ObjectCategories/06*/*.jpg'])];
        files = [files; rdir([baseDatasetDirectory, '/256_ObjectCategories/07*/*.jpg'])];
        files = [files; rdir([baseDatasetDirectory, '/256_ObjectCategories/09*/*.jpg'])];
        files = [files; rdir([baseDatasetDirectory, '/256_ObjectCategories/10*/*.jpg'])];
        files = [files; rdir([baseDatasetDirectory, '/256_ObjectCategories/22*/*.jpg'])];
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
%             [faces, eyes] = mexFaceDetect(im, '/Users/pbarnum/Documents/Anki/coretech-external/opencv-2.4.8/data/haarcascades/haarcascade_frontalface_default.xml');
            
            if ~isempty(faces)
                disp(filename);
                
                hold off;
                imshow(im);
                hold on;
                
                for iFace = 1:size(faces,1)
                    rectangle('Position', faces(iFace,1:4), 'LineWidth', 3, 'EdgeColor', [1.0,0,0]);
                end
                
                pause();
            end
            
            filename = fgetl(fileId);
        end % while filename ~= -1
        
        fclose(fileId);
    end % if checkWithFaceDetector
    
    if showAllImages
        numImagesToShow = 15;
        
        fileId = fopen(negativeExamplesFilename, 'r');
        
        filenames = {};
        
        filename = fgetl(fileId);
        % Check if any faces are detected
        while filename ~= -1
            filenames{end+1} = filename; %#ok<AGROW>
            filename = fgetl(fileId);
        end
        
        for iFile = 1:numImagesToShow:length(filenames)
            imagesToShow = cell(numImagesToShow,1);
            for dFile = 1:numImagesToShow
                if (iFile + dFile) > length(filenames)
                    break;
                else
                    curFilename = filenames{iFile + dFile};
                    imagesToShow{dFile} = imread(curFilename);
                    disp(sprintf('(%d) %s (%d)', dFile, curFilename, dFile));
                end
            end
            
            imshows(imagesToShow, 'collageTitles')
            
            pause()
        end % for iFile = 1:length(filenames)
    end % showAllImages
    
    keyboard
end % function faceDetection_checkNegativeExamples()


