
% Don't call this function, call decisionTree2_extractFeatures.m

function [labels, featureValues] = decisionTree2_extractFeatures_worker(workQueue, parameters)
    
    xPerturb = parameters.xPerturb;
    yPerturb = parameters.yPerturb;
    classesList = parameters.classesList;
    numBlurs = parameters.numBlurs;
    numResolutions = parameters.numResolutions;
    numPerturbations = parameters.numPerturbations;
    numFeatures = parameters.numFeatures;  
    numPadPixels = parameters.numPadPixels;
    blurSigmas = parameters.blurSigmas;
    probeResolutions = parameters.probeResolutions;

    startTime = tic();
    
    numLabels = numBlurs*length(workQueue)*numPerturbations*numResolutions;
    
    labels = zeros(numLabels, 1, 'int32');
    featureValues = zeros(numFeatures, numLabels, 'uint8');
    
    cLabel = 1;
    
    fprintf('Running %d work items', length(workQueue));
    
    for iWork = 1:length(workQueue)
        iClass = workQueue{iWork}.iClass;
        iFile = workQueue{iWork}.iFile;
        
        img = imreadAlphaHelper(classesList(iClass).filenames{iFile}); 

        imgPadded = padarray(img, [numPadPixels,numPadPixels], 255);

        [nrows,ncols,~] = size(img);

        for iBlur = 1:numBlurs
            imgPaddedAndBlurred = imgPadded;

            if blurSigmas(iBlur) > 0
                blurSigma = blurSigmas(iBlur)*sqrt(nrows^2 + ncols^2);
                imgPaddedAndBlurred = separable_filter(imgPadded, gaussian_kernel(blurSigma));
            end

            for iResolution = 1:numResolutions
                imgPaddedAndBlurredResized = imresize(imresize(imgPaddedAndBlurred, [probeResolutions(iResolution),probeResolutions(iResolution)]), size(imgPaddedAndBlurred), 'nearest');

                for iPerturb = 1:numPerturbations
                    % Probe location assume the left edge of the image is 0 and the right edge is 1
                    imageCoordsX = round(xPerturb{iPerturb} * ncols + 0.5 + numPadPixels);
                    imageCoordsY = round(yPerturb{iPerturb} * nrows + 0.5 + numPadPixels);

                    inds = find(imageCoordsX >= 1 & imageCoordsX <= (ncols+2*numPadPixels) & imageCoordsY >= 1 & imageCoordsY <= (nrows+2*numPadPixels));

                    % If the perturbations are too large relative to
                    % the padding, some indexes will be out of bounds.
                    % The solution is to increase the padding.
                    assert(length(inds) == length(imageCoordsX));

                    % Stripe the data per-probe location
                    for iFeature = 1:length(imageCoordsY)
                        featureValues(iFeature, cLabel) = imgPaddedAndBlurredResized(imageCoordsY(iFeature), imageCoordsX(iFeature));
                    end

%                     if showProbePermutations
%                         imshows({imgPaddedAndBlurredResized((1+numPadPixels):(end-numPadPixels), (1+numPadPixels):(end-numPadPixels)), imresize(reshape(featureValuesArray(cLabel, :), [length(probeLocationsY), length(probeLocationsX)]), size(img), 'nearest')});
%                         pause(0.05);
%                     end

                    labels(cLabel) = iClass;
                    cLabel = cLabel + 1;
                end % for iPerturb = 1:numPerturbations
            end % for iResolution = 1:length(numResolutions)
        end % for iBlur = 1:numBlurs
        
        fprintf('x');
        if mod(iWork, 25) == 0
            fprintf('%d(%0.0fs)', iWork, toc(startTime));
        end
    end % for iWork = 1:length(workQueue)
    
    disp(' ');
end % decisionTree2_extractFeatures_worker()

function img = imreadAlphaHelper(fname)
    
    [img, ~, alpha] = imread(fname);
    img = mean(im2double(img),3);
    img(alpha < .5) = 1;
    
    threshold = (max(img(:)) + min(img(:)))/2;
    %     img = bwpack(img > threshold);
    img = 255*uint8(img > threshold);
    % img = single(img > threshold);
    
end % imreadAlphaHelper()
