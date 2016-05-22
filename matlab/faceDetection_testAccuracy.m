% function faceDetection_testAccuracy()

% [numValidFaces_lbpcascade_frontalface, numFaceImages_lbpcascade_frontalface, numErroneousDetections_lbpcascade_frontalface, numNegativeFiles_lbpcascade_frontalface] = faceDetection_testAccuracy('~/Documents/datasets/lfw/', '/Users/pbarnum/Documents/datasets/tutorial-haartraining-read-only/data/negatives/', '/Users/pbarnum/Documents/Anki/coretech-external/opencv-2.4.8/data/lbpcascades/lbpcascade_frontalface.xml');
% [numValidFaces_peter_face6, numFaceImages_peter_face6, numErroneousDetections_peter_face6, numNegativeFiles_peter_face6] = faceDetection_testAccuracy('~/Documents/datasets/lfw/', '/Users/pbarnum/Documents/datasets/tutorial-haartraining-read-only/data/negatives/', '/Users/pbarnum/Documents/Anki/coretech-external/opencv-2.4.8/data/lbpcascades/peter_face6.xml');

function [numValidFaces, numFaceImages, numErroneousDetections, numNegativeFiles] = faceDetection_testAccuracy(lfwDirectory, negativesDirectory, faceCascadeFilename)
    validFaceSizePercentRange = [0.25, 0.75]; % How big can the face be, relative to the size of the image?
    validFaceLocationPercentMax = 0.25; % How far offcenter can the face be?
    
    scaleFactor = 1.1;
    minNeighbors = 3;
    
    faceImageFilenames = rdir([lfwDirectory,'/*/*.jpg']);
    negativeFilenames = [rdir([negativesDirectory, '*.jpg']), rdir([negativesDirectory, '*.jpeg']), rdir([negativesDirectory, '*.png'])];
    
    numFaceImages = length(faceImageFilenames);
    numNegativeFiles = length(negativeFilenames);
    
    pBar = ProgressBar('Test accuracy');
    pBar.showTimingInfo = true;
    pBar.set_increment(1/(numFaceImages/100));
    pBarCleanup = onCleanup(@()delete(pBar));
        
    numValidFaces = 0;
    for iImage = 1:numFaceImages
        im = rgb2gray2(imread(faceImageFilenames(iImage).name));
        [faces,~] = mexFaceDetect(im, faceCascadeFilename, 'none', scaleFactor, minNeighbors);
        
        if isDetectedFaceValid(validFaceSizePercentRange, validFaceLocationPercentMax, faces, size(im))
            numValidFaces = numValidFaces + 1;
        end
        
        if mod(iImage, 100) == 0
            pBar.increment();
            pBar.set_message(sprintf('%s\n\nFaces detected correctly %d/%d = %0.2f', faceCascadeFilename, numValidFaces, iImage, numValidFaces/iImage));
        end        
    end % for iImage = 1:numFaceImages    
    disp(sprintf('Faces detected correctly %d/%d = %0.2f', numValidFaces, iImage, numValidFaces/iImage));
        
    pBar = ProgressBar('Test accuracy');
    pBar.showTimingInfo = true;
    pBar.set_increment(1/(numNegativeFiles/100));
    pBarCleanup = onCleanup(@()delete(pBar));    
    
    numErroneousDetections = 0;    
    for iImage = 1:numNegativeFiles
        im = rgb2gray2(imread(negativeFilenames(iImage).name));
        
        [faces,~] = mexFaceDetect(im, faceCascadeFilename, 'none', scaleFactor, minNeighbors);
        
        numErroneousDetections = numErroneousDetections + length(faces);
        
        if mod(iImage, 100) == 0
            pBar.increment();
            pBar.set_message(sprintf('%s\n\nSpurrious detections %d/%d = %0.2f per image', faceCascadeFilename, numErroneousDetections, iImage, numErroneousDetections/iImage));
        end        
    end
    disp(sprintf('Spurrious detections %d/%d = %0.2f per image', numErroneousDetections, iImage, numErroneousDetections/iImage));
    
end % function faceDetection_testAccuracy()

function validFaceFound = isDetectedFaceValid(validFaceSizePercentRange, validFaceLocationPercentMax, faces, imSize)
    
    imageCenter = [imSize(2)/2, imSize(1)/2];

    validFaceFound = false;        
    for iFace = 1:length(faces)
        faceCenter = [faces(1)+faces(3)/2, faces(2)+faces(4)/2];

        if sqrt((faceCenter(1) - imageCenter(1)).^2 + (faceCenter(2) - imageCenter(2)).^2) <= (max(imSize)*validFaceLocationPercentMax)
            minFaceDimension = min(faces(3:4));
            maxFaceDimension = max(faces(3:4));

            if minFaceDimension >= min(imSize)*validFaceSizePercentRange(1) &&...
               maxFaceDimension <= max(imSize)*validFaceSizePercentRange(2)

               validFaceFound = true;
               break;
            end
        end
    end % for iFace = 1:length(faces)
end % function isDetectedFaceValid()