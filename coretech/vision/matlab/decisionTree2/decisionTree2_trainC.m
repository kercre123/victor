% function tree = decisionTree2_trainC()
%
% Based off of VisionMarkerTrained.TrainProbeTree, but split into a
% separate function, to prevent too much messing around with the
% VisionMarkerTrained class hierarchy

% As input, this function needs a list of labels and featureValues. Start with the following two lines for any example:
% classesList = decisionTree2_createClassesList();
% [labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid] = decisionTree2_extractFeatures(fiducialClassesList);

% Example:
% [tree, minimalTree, cTree, trainingFailures, testOnTrain_numCorrect, testOnTrain_numTotal] = decisionTree2_trainC(labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid);

% Example using only 128 as the grayvalue threshold
% [tree, minimalTree, cTree, trainingFailures, testOnTrain_numCorrect, testOnTrain_numTotal] = decisionTree2_trainC(labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid, 'u8MinDistanceForSplits', 255, 'u8ThresholdsToUse', 128);

function [tree, minimalTree, cTree, trainingFailures, testOnTrain_numCorrect, testOnTrain_numTotal] = decisionTree2_trainC(labelNames, labels, featureValues, probeLocationsXGrid, probeLocationsYGrid, varargin)
    
    t_start = tic();
    
    assert(size(labels,2) == 1);
    assert(size(labels,1) == size(featureValues, 2));
    
    leafNodeFraction = 1.0; % What percent of the labels must be the same for it to be considered a leaf node?
    leafNodeNumItems = 1; % If a node has less-than-or-equal-to this number of items, it is a leaf no matter what
    u8MinDistanceForSplits = 20; % How close can the value for splitting on two values of one feature? Set to 255 to disallow splitting more than once per feature (e.g. multiple splits for one probe in the same location).
    u8MinDistanceFromThreshold = 0; % If a feature value is closer than this to the threshold, pass it to both subpaths
    u8ThresholdsToUse = []; % If set, only use these grayvalues to threshold (For example, set to [128] to only split on 128);
    featuresUsed = zeros(size(featureValues,1), 256, 'uint8'); % If you don't want to train on some of the features or u8 thresholds, set some of the featuresUsed to true
    cInFilenamePrefix = 'c:/tmp/treeTraining/treeTraining_'; % Temporary location for when useCVersion = true
    cOutFilenamePrefix = 'c:/tmp/treeTraining/treeTraining_out_';
    %cTrainingExecutable = 'C:/Anki/products-cozmo/build/Visual Studio 11/bin/RelWithDebInfo/run_trainDecisionTree.exe';
    cTrainingExecutable = 'C:/Anki/products-cozmo/build/Visual Studio 11/bin/Debug/run_trainDecisionTree.exe';
    maxThreads = 4;
    maxSavingThreads = 8;
    
    parseVarargin(varargin{:});
    
    u8ThresholdsToUse = uint8(u8ThresholdsToUse);
    
    if ~ispc()
        cInFilenamePrefix = strrep(cInFilenamePrefix, 'c:', tildeToPath());
        cOutFilenamePrefix = strrep(cOutFilenamePrefix, 'c:', tildeToPath());
        
        if ~isempty(strfind(cTrainingExecutable, 'C:/Anki/products-cozmo'))
            cTrainingExecutable = [tildeToPath(), '/Documents/Anki/products-cozmo/build/Xcode/bin/Debug/run_trainDecisionTree'];
%             cTrainingExecutable = [tildeToPath(), '/Documents/Anki/products-cozmo/build/Xcode/bin/Release/run_trainDecisionTree'];
        end
    end
    
    [~,~,~] = mkdir(cInFilenamePrefix);
    [~,~,~] = mkdir(cOutFilenamePrefix);
    
    decisionTree2_saveInputs(cInFilenamePrefix, labelNames, labels, featureValues, featuresUsed, u8ThresholdsToUse, maxSavingThreads);
    
    pause(.01);
   
    [tree, cTree] = decisionTree2_runCVersion(cTrainingExecutable, cInFilenamePrefix, cOutFilenamePrefix, size(featureValues,1), leafNodeFraction, leafNodeNumItems, u8MinDistanceForSplits, u8MinDistanceFromThreshold, labelNames, probeLocationsXGrid, probeLocationsYGrid, maxThreads);
    
    if isempty(tree)
        error('Training failed!');
    end
    
    if ~isempty(u8ThresholdsToUse)
        vals = unique(cTree.u8Thresholds);
        u8ThresholdsToUseS = sort(u8ThresholdsToUse);
        
        % There also may be a 0 threshold, which is the defaul value (used for leaves)
        
        if length(u8ThresholdsToUseS) == length(vals)
            for i = 1:length(u8ThresholdsToUseS)
                assert(u8ThresholdsToUseS(i) == vals(i));
            end
        elseif length(u8ThresholdsToUseS) == (length(vals)-1) && vals(1) == 0
            for i = 1:length(u8ThresholdsToUseS)
                assert(u8ThresholdsToUseS(i) == vals(i+1));
            end
        elseif length(u8ThresholdsToUseS) == (length(vals)-2) && vals(1) == 0 && vals(end) == 255
            for i = 1:length(u8ThresholdsToUseS)
                assert(u8ThresholdsToUseS(i) == vals(i+1));
            end
        else
            assert(false)
        end
    end
    
    t_train = toc(t_start);
    
    [numInternalNodes, numLeaves] = decisionTree2_countNumNodes(tree);
    
    disp(sprintf('Training completed in %f seconds. Tree has %d nodes.', t_train, numInternalNodes + numLeaves));
    
    minimalTree = decisionTree2_stripTree(tree);
    
    t_test = tic();
    
    [testOnTrain_numCorrect, testOnTrain_numTotal] = decisionTree2_testOnTrainingData(cTree, featureValues, labels, labelNames, probeLocationsXGrid, probeLocationsYGrid);
    
    t_test = toc(t_test);
    
    disp(sprintf('Tested tree on training set in %f seconds.', t_test));

    trainingFailures = [];

end % decisionTree2_trainC()

% Save the inputs to file, for running the complete C version of the training
% cInFilenamePrefix should be the prefix for all the files to save, such as '~/tmp/trainingFiles_'
function decisionTree2_saveInputs(cInFilenamePrefix, labelNames, labels, featureValues, featuresUsed, u8ThresholdsToUse, maxSavingThreads)    
    totalTic = tic();
   
    pBar = ProgressBar('decisionTree2_train', 'CancelButton', true);
    pBar.showTimingInfo = true;
    pBarCleanup = onCleanup(@()delete(pBar));
    
    pBar.set_message('Saving inputs for C training');
    pBar.set_increment(1/(size(featureValues,1)+4));
    pBar.set(0);
    
    if size(labelNames, 1) ~= 1
        labelNames = labelNames';
    end
    
    if size(labels, 1) ~= 1
        labels = labels';
    end
    
    if size(u8ThresholdsToUse,1) ~= 1
        u8ThresholdsToUse = u8ThresholdsToUse';
    end
    
    mexSaveEmbeddedArray(...
        {uint8(featuresUsed), labelNames, int32(labels) - 1, uint8(u8ThresholdsToUse)},...
        {[cInFilenamePrefix, 'featuresUsed.array'], [cInFilenamePrefix, 'labelNames.array'], [cInFilenamePrefix, 'labels.array'], [cInFilenamePrefix, 'u8ThresholdsToUse.array']});
    
    pBar.increment();
    pBar.increment();
    pBar.increment();
    pBar.increment();
    
    for iFeature = 1:maxSavingThreads:size(featureValues,1)
        allArrays = {};
        allFilenames = {};
        
        pBar.set_message(sprintf('Saving inputs for C training %d/%d', iFeature, size(featureValues,1)));
        
        for iThread = 1:maxSavingThreads
            index = iFeature + iThread - 1;
            if index > size(featureValues,1)
                break;
            end
            
            curFeatureValues = featureValues(index,:);
            
            if size(curFeatureValues, 1) ~= 1
                curFeatureValues = curFeatureValues';
            end
            
            allArrays{end+1} = uint8(curFeatureValues); %#ok<AGROW>
            allFilenames{end+1} = [cInFilenamePrefix, sprintf('featureValues%d.array', index-1)]; %#ok<AGROW>
        end
        
        mexSaveEmbeddedArray(allArrays, allFilenames, 6, maxSavingThreads);
        
        for i = 1:maxSavingThreads
            pBar.increment();
        end
    end
    
    disp(sprintf('Total time to save: %f seconds', toc(totalTic)));
    
    clear pBar;       
end % decisionTree2_saveInputs()

function [tree, cTree] = decisionTree2_runCVersion(cTrainingExecutable, cInFilenamePrefix, cOutFilenamePrefix, numFeatures, leafNodeFraction, leafNodeNumItems, u8MinDistanceForSplits, u8MinDistanceFromThreshold, labelNames, probeLocationsXGrid, probeLocationsYGrid, maxThreads)
    % First, run the training
    trainingTic = tic();
    command = sprintf('"%s" "%s" %d %f %d %d %d %d "%s"', cTrainingExecutable, cInFilenamePrefix, numFeatures, leafNodeFraction, leafNodeNumItems, u8MinDistanceForSplits, u8MinDistanceFromThreshold, maxThreads, cOutFilenamePrefix);
    disp(['Starting C training: ', command]);
    result = system(command);
    disp(sprintf('C training finished in %f seconds', toc(trainingTic)));
    
    if result ~= 0
        tree = [];
        cTree = [];
        keyboard
        return;
    end
    
    % Next, load and convert the tree into the matlab format
    convertingTic = tic();
    
    disp('Loading and converting c tree to matlab format')
    
    cTree.depths = mexLoadEmbeddedArray([cOutFilenamePrefix, 'depths.array'])';
    infoGains = mexLoadEmbeddedArray([cOutFilenamePrefix, 'bestEntropys.array'])';
    cTree.whichFeatures = mexLoadEmbeddedArray([cOutFilenamePrefix, 'whichFeatures.array'])';
    cTree.u8Thresholds = mexLoadEmbeddedArray([cOutFilenamePrefix, 'u8Thresholds.array'])';
    cTree.leftChildIndexs = mexLoadEmbeddedArray([cOutFilenamePrefix, 'leftChildIndexs.array'])';
    cpuUsageSamples = mexLoadEmbeddedArray([cOutFilenamePrefix, 'cpuUsageSamples.array'])';
    
    tree = decisionTree2_convertCTree(cTree.depths, infoGains, cTree.whichFeatures, cTree.u8Thresholds, cTree.leftChildIndexs, labelNames, probeLocationsXGrid, probeLocationsYGrid);
    
    figure(50);
    plot(cpuUsageSamples, 'b+');
    title('CPU usage over time');
    limits = axis();
    limits(3:4) = [0,100];
    axis(limits)
    
    disp(sprintf('Conversion done in %f seconds (average CPU usage %0.2f%%)', toc(convertingTic), mean(cpuUsageSamples)));
end % decisionTree2_runCVersion

    