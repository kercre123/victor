% function faceDetection_fddb_detectPoses()

% load('~/Documents/datasets/FDDB-folds/FDDB-ellipses.mat', 'ellipses')
% [allDetections, posemap] = faceDetection_fddb_detectPoses(ellipses, 1:length(ellipses), '~/Documents/datasets/FDDB-folds/FDDB-detectedPoses%d.mat');
% save('~/Documents/datasets/FDDB-folds/FDDB-detectedPoses.mat', 'allDetections', 'posemap', '-v7.3');

function [allDetections, posemap] = faceDetection_fddb_detectPoses(ellipses, whichEllipses, filenameOutPattern)
    showDetections = false;
    rerunDetection = true;
    
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
    
    if rerunDetection
        %     allDetections = cell(0,1);
        numDetections = 0;
        
        for iEllipse = whichEllipses
            tic
            curFilename = ellipses{iEllipse}{1};
            curImage = imread(curFilename);
            
            %                 figure(1);
            %                 imshow(curImage);
            
            if size(curImage,3) == 1
                curImage = repmat(curImage, [1,1,3]);
            end
            
            bs = detect(curImage, model, model.thresh);
            bs2 = clipboxes(curImage, bs);
            bs3 = nms_face(bs2,0.3);
            
            %             allDetections{end+1} = {curFilename, iEllipse, bs, bs2, bs3}; %#ok<AGROW>
            %             allDetections{end+1} = {curFilename, iEllipse, bs3}; %#ok<AGROW>
            save(sprintf(filenameOutPattern, iEllipse), 'curFilename', 'iEllipse', 'bs3');
            
            numDetections = numDetections + length(bs3);
            
            if showDetections
                %                 close 1
                %                 figure(1);
                showboxes(curImage, bs3, posemap);
                title('All detections above the threshold');
            end
            
            disp(sprintf('Finished %d in %f', iEllipse, toc()));
        end % for iEllipse = 1:length(ellipses)
        
        disp(sprintf('Detected %d faces', numDetections));
    end % if rerunDetection
    
    if whichEllipses(1) == 1
        allDetections = cell(length(ellipses),1);
        cDetection = 1;
        for iEllipse = 1:length(ellipses)
            load(sprintf(filenameOutPattern, iEllipse), 'curFilename', 'iEllipse', 'bs3');
            
            allDetections{iEllipse} = {curFilename, iEllipse, bs3};
            
            cDetection = cDetection + 1;
        end
        
        save(sprintf(filenameOutPattern, 0), 'allDetections');
    else
        allDetections = [];
    end
end % function faceDetection_fddb_detectPoses()

