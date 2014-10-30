% function faceDetection_detectPoses()

% [allDetections, posemap] = faceDetection_detectPoses('~/Documents/datasets/FDDB-folds/FDDB-ellipses.mat', '~/Documents/datasets/FDDB-folds/FDDB-detectedPoses.mat');

function [allDetections, posemap] = faceDetection_detectPoses(ellipseMatFilename, detectedPosesMatFilename)
    load(ellipseMatFilename, 'ellipses');
    
    load face_p146_small.mat
    model.interval = 5;
    %         model.thresh = min(-0.65, model.thresh);
    model.thresh = min(-0.7, model.thresh);
    
    if length(model.components)==13
        posemap = 90:-15:-90;
    elseif length(model.components)==18
        posemap = [90:-15:15 0 0 0 0 0 0 -15:-15:-90];
    else
        error('Can not recognize this model');
    end
    
    allDetections = cell(0,1);
    
    curFilename = '';
    for iEllipse = 1:length(ellipses)
        if ~strcmp(curFilename, ellipses{iEllipse}{1})
            tic
            curFilename = ellipses{iEllipse}{1};
            curImage = imread(curFilename);
            
            %                 figure(1);
            %                 imshow(curImage);
            
            bs = detect(curImage, model, model.thresh);
            bs2 = clipboxes(curImage, bs);
            bs3 = nms_face(bs2,0.3);
            
            allDetections{end+1} = {curFilename, iEllipse, bs, bs2, bs3}; %#ok<AGROW>
            
            %                 close 1
            %                 figure(1);
            showboxes(curImage, bs3, posemap);
            title('All detections above the threshold');
            
            save(detectedPosesMatFilename, 'allDetections', 'posemap');
            
            disp(sprintf('Finished %d in %f', iEllipse, toc()));
        end
    end % for iEllipse = 1:length(ellipses)

end % function faceDetection_detectPoses()

