% function faceDetection_webfaces_detectPoses()

% allFaces = faceDetection_webfaces_readLandmarks('/Users/pbarnum/Documents/datasets/webfaces');
% [allDetections, posemap] = faceDetection_webfaces_detectPoses(allFaces, 1:length(allFaces), '~/Documents/datasets/webfaces/webfaces-detectedPoses%d.mat');
% save('~/Documents/datasets/webfaces/webfaces-detectedPoses.mat', 'allDetections', 'posemap', '-v7.3');

function [allDetections, posemap] = faceDetection_webfaces_detectPoses(allFaces, whichFaces, filenameOutPattern)
    showDetections = false;
    rerunDetection = false;
    
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
        numDetections = 0;
        
        for iFile = whichFaces
            tic
            curFilename = allFaces{iFile}{1};
            curImage = imread(curFilename);
                        
            if size(curImage,3) == 1
                curImage = repmat(curImage, [1,1,3]);
            end
            
            bs = detect(curImage, model, model.thresh);
            bs2 = clipboxes(curImage, bs);
            bs3 = nms_face(bs2,0.3);
            
            %             allDetections{end+1} = {curFilename, iFile, bs, bs2, bs3}; %#ok<AGROW>
            %             allDetections{end+1} = {curFilename, iFile, bs3}; %#ok<AGROW>
            save(sprintf(filenameOutPattern, iFile), 'curFilename', 'iFile', 'bs3');
            
            numDetections = numDetections + length(bs3);
            
            if showDetections
                %                 close 1
                %                 figure(1);
                showboxes(curImage, bs3, posemap);
                title('All detections above the threshold');
            end
            
            disp(sprintf('Finished %d in %f', iFile, toc()));
        end % for iFile = 1:length(ellipses)
        
        disp(sprintf('Detected %d faces', numDetections));
    end % if rerunDetection
    
    if whichFaces(1) == 1
        allDetections = cell(length(allFaces),1);
        cDetection = 1;
        for iFile = 1:length(allFaces)
            load(sprintf(filenameOutPattern, iFile), 'curFilename', 'iFile', 'bs3');
            
            allDetections{iFile} = {curFilename, iFile, bs3};
            
            cDetection = cDetection + 1;
        end
        
        save(sprintf(filenameOutPattern, 0), 'allDetections');
    else
        allDetections = [];
    end
end % function faceDetection_webfaces_detectPoses()

