% function svm_train()

% Train a one-vs-all classifier for each label

% Example:
% classesList = decisionTree2_createClassesList('markerFilenamePatterns', {'Z:/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/rotated/*.png'});
% [labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(classesList, 'blurSigmas', [0, .01], 'numPerturbations', 10, 'probeResolutions', [512,32]);
% trainedSvmClassifiers = svm_train(labels, featureValues);

function trainedSvmClassifiers = svm_train(labels, featureValues, varargin)
    
    libsvmParameters = '-m 2000 -t 0 ';
    
    parseVarargin(varargin{:});
    
    uniqueLabels = unique(labels);
    
    % This could be supported, if there's a use case
    assert(max(labels) == length(uniqueLabels))
    
    %     featureValuesF64 = double(featureValues') / (255/2) - 1;
    featureValuesF64 = double(featureValues');
    
    trainedSvmClassifiers = cell(length(uniqueLabels), 1);
    
    for iLabel = 1:length(uniqueLabels)
        tic
        labelsOneVsAll = zeros(size(labels));
        labelsOneVsAll(labels == uniqueLabels(iLabel)) = 1;
        labelsOneVsAll(labels ~= uniqueLabels(iLabel)) = -1;
        
        weight = length(find(labelsOneVsAll==1)) / length(labelsOneVsAll);
        
        trainedSvmClassifiers{iLabel} = svmtrain(labelsOneVsAll, featureValuesF64, [libsvmParameters, sprintf('-w-1 %f', weight)]);
        disp(sprintf('Trained %d/%d in %f seconds', iLabel, length(uniqueLabels), toc()));
    end
    
    %     training_label_vector = [1,1,1,-1,-1,-1]'; training_instance_matrix = [1,1,1,2,2,2]';
    %     classifier = svmtrain(training_label_vector, training_instance_matrix)
    
    %     keyboard
    