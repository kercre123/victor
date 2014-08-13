
%#ok<*CCAT>

function fiducialClassesWithProbes = TrainProbeTree2_loadImages(varargin)
    blurSigmas = [0 .005 .01]; % as a fraction of the image diagonal
    maxPerturbPercent = 0.05;
    numPerturbations = 100;
    probeLocationsX = linspace(0, 1, 30);
    probeLocationsY = linspace(0, 1, 30);
    
    % TODO: add back
    % leafNodeFraction = 0.9; % fraction of remaining examples that must have same label to consider node a leaf
    
    parseVarargin(varargin{:});
    
    corners = [0 0; 0 1; 1 0; 1 1];
    
    numBlurs = length(blurSigmas);
    
    % Find all the names of fiducial images
    files = rdir('Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/rotated/*.png', [], true);
    
%     newFiles = rdir('Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/rotated/*.png', [], true);
%     files = {files{:}, newFiles{:}};
%     
%     newFiles = rdir('Z:/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/rotated/*.png', [], true);
%     files = {files{:}, newFiles{:}};
%     
%     newFiles = rdir('Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/rotated/*.png', [], true);
%     files = {files{:}, newFiles{:}};
    
    numImages = 0;
    
    % Convert each cell to format {className, filename}
    fiducialClasses = cell(length(files), 1);
    for i = 1:length(files)
        slashInds = find(files{i} == '/');
        dotInds = find(files{i} == '.');
        fiducialClasses{i} = {files{i}((slashInds(end)+1):(dotInds(end)-1)), {files{i}}};
        numImages = numImages + length(files{i});
    end
    
    [xgrid,ygrid] = meshgrid(probeLocationsX, probeLocationsY);
    probeValues   = zeros(numBlurs*numImages*numPerturbations, length(probeLocationsX)*length(probeLocationsY), 'uint8');
    labels        = zeros(numBlurs*numImages*numPerturbations, 1, 'uint32');
    
    % Precompute all the perturbed probe locations once
    xPerturb = cell(1, numPerturbations);
    yPerturb = cell(1, numPerturbations);
    for iPerturb = 1:numPerturbations
        perturbation = (2*rand(4,2) - 1) * maxPerturbPercent;
        corners_i = corners + perturbation;
        T = cp2tform(corners_i, corners, 'projective');
        [xPerturb{iPerturb}, yPerturb{iPerturb}] = tforminv(T, xgrid, ygrid);
        xPerturb{iPerturb} = xPerturb{iPerturb}(:);
        yPerturb{iPerturb} = yPerturb{iPerturb}(:);
    end
    
    % Compute the perturbed probe values
    
    fiducialClassesWithProbes = cell(length(fiducialClasses), 1);
    
    cLabel = 1;
    for iClass = 1:length(fiducialClasses)
        for iFile = 1:length(fiducialClasses{iClass}{2})
            %             fiducialClassesWithImages{iClass}{2}{iFile} = {fiducialClasses{iClass}{2}{iFile}, imreadAlphaHelper(fiducialClasses{iClass}{2}{iFile})};
            img = imreadAlphaHelper(fiducialClasses{iClass}{2}{iFile});
            
            [nrows,ncols,~] = size(img);
    
            imageCoordsX = linspace(0, 1, ncols);
            imageCoordsY = linspace(0, 1, nrows);
    
            for iBlur = 1:numBlurs
                imgBlur = img;
                
                if blurSigmas(iBlur) > 0
                    blurSigma = blurSigmas(iBlur)*sqrt(nrows^2 + ncols^2);
                    imgBlur = separable_filter(img, gaussian_kernel(blurSigma));
                end

                imgBlur = double(imgBlur);
                
                for iPerturb = 1:numPerturbations
                    probeValues(cLabel, :) = 255*uint8(interp2(imageCoordsX, imageCoordsY, imgBlur, xPerturb{iPerturb}, yPerturb{iPerturb}, 'linear', 1));
                    
                    labels(cLabel) = iClass;
                    cLabel = cLabel + 1;
                end
            end % for iBlur = 1:numBlurs
            
        end % for iFile = 1:length(fiducialClasses{iClass}{2})
    end % for iClass = 1:length(fiducialClasses)
    
    keyboard
    
    %     for iImg = 1:numImages
    %
    %         %         if strcmp(fnames{iImg}, 'ALLWHITE')
    %         %             img{iImg} = ones(workingResolution);
    %         %         elseif strcmp(fnames{iImg}, 'ALLBLACK')
    %         %             img{iImg} = zeros(workingResolution);
    %         %         else
    %         img = imreadAlphaHelper(fnames{iImg});
    %         %         end
    %
    %         [nrows,ncols,~] = size(img);
    %         imageCoordsX = linspace(0, 1, ncols);
    %         imageCoordsY = linspace(0, 1, nrows);
    %
    %         [~,labelNames{iImg}] = fileparts(fnames{iImg});
    %
    %         for iBlur = 1:numBlurs
    %             imgBlur = img;
    %             if blurSigmas(iBlur) > 0
    %                 blurSigma = blurSigmas(iBlur)*sqrt(nrows^2 + ncols^2);
    %                 imgBlur = separable_filter(imgBlur, gaussian_kernel(blurSigma));
    %             end
    %
    %             imgGradMag = single(smoothgradient(imgBlur));
    %             imgBlur = single(imgBlur);
    %
    %             probeValues{iBlur,iImg} = zeros(workingResolution^2, numPerturbations, 'single');
    %             gradMagValues{iBlur,iImg} = zeros(workingResolution^2, numPerturbations, 'single');
    %             for iPerturb = 1:numPerturbations
    %                 probeValues{iBlur,iImg}(:,iPerturb) = mean(interp2(imageCoordsX, imageCoordsY, imgBlur, ...
    %                     xPerturb{iPerturb}, yPerturb{iPerturb}, 'linear', 1), 2);
    %
    %                 gradMagValues{iBlur,iImg}(:,iPerturb) = mean(interp2(imageCoordsX, imageCoordsY, imgGradMag, ...
    %                     xPerturb{iPerturb}, yPerturb{iPerturb}, 'linear', 0), 2);
    %             end
    %
    %             labels{iBlur,iImg} = iImg*ones(1,numPerturbations, 'uint32');
    %
    %         end % FOR each blurSigma
    %
    %         pBar.increment();
    %     end
    
end % TrainProbeTree2_loadImages()

function img = imreadAlphaHelper(fname)
    
    [img, ~, alpha] = imread(fname);
    img = mean(im2double(img),3);
    img(alpha < .5) = 1;
    
    threshold = (max(img(:)) + min(img(:)))/2;
    %     img = bwpack(img > threshold);
    img = uint8(img > threshold);
    
end % imreadAlphaHelper()