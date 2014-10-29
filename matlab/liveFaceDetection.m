
% function liveFaceDetection()

function liveFaceDetection()
    
    % start camera
    mexCameraCapture(0, 0);
    
    while true
        im = mexCameraCapture(1, 0);
        im = rgb2gray(im(:,:,[3,2,1]));
        
        [faces,eyes] = mexFaceDetect(im);
        
        numFaces = size(faces,1);

        figure(1);
        imshow(im);

        for iFace = 1:numFaces
            rectangle('Position', faces(iFace,1:4), 'LineWidth', 3, 'EdgeColor', [1.0,0,0]);
        end
        
        for iEye = 1:size(eyes,1)
            rectangle('Position', eyes(iEye,2:5), 'LineWidth', 3, 'EdgeColor', [0,1.0,0]);
        end
    end % while true