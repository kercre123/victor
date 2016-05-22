
% function liveFaceDetection()

function liveFaceDetection()
    
    % start camera
    mexCameraCapture(0, 0);
    
    while true
        im = mexCameraCapture(1, 0);
        im = rgb2gray(im(:,:,[3,2,1]));
        
        if min(size(im) == [720,1280]) == 1
            im = imresize(im, [360,640]);
        end
        
%         tic
%         [faces,eyes] = mexFaceDetect(im, '/Users/pbarnum/Documents/Anki/coretech-external/opencv-2.4.8/data/lbpcascades/lbpcascade_frontalface.xml');
%             [faces,eyes] = mexFaceDetect(im, '/Users/pbarnum/Documents/Anki/coretech-external/opencv-2.4.8/data/lbpcascades/lbpcascade_frontalface_part.xml');
        [faces,eyes] = mexFaceDetect(im, '/Users/pbarnum/Documents/Anki/coretech-external/opencv-2.4.8/data/lbpcascades/peter_face6.xml', '', 1.1, 4);
%         toc
        
        numFaces = size(faces,1);

        figure(1);
        hold off;
        imshow(im);
        hold on;

        for iFace = 1:numFaces
            rectangle('Position', faces(iFace,1:4), 'LineWidth', 3, 'EdgeColor', [1.0,0,0]);
        end
        
        for iEye = 1:size(eyes,1)
            rectangle('Position', eyes(iEye,2:5), 'LineWidth', 3, 'EdgeColor', [0,1.0,0]);
        end
    end % while true