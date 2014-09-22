
function tmp_svm_testExamples()
    
    rerunTraining = true;
    
    suffixes = {'100'};
    % suffixes = {'100B', '300', '300B', '600', '600B', '1000', '1000B'};
    
    if rerunTraining
        for iSuffix = 1:length(suffixes)
            trainedSvmClassifiers = runTest('/Volumes/FastExternal/tmp/treeTraining/labels', suffixes{iSuffix}, 'polynomial3');
            
            save(['~/tmp/trainedSvmPolynomial3', suffixes{iSuffix}], 'trainedSvmClassifiers');
        end
    end
    
    for iSuffix = 1:length(suffixes)
        if suffixes{iSuffix}(end) == 'B'
            finalSuffix = [suffixes{iSuffix}(1:(end-1)), ''];
            load(['/Volumes/FastExternal/tmp/treeTraining/labels', suffixes{iSuffix}(1:(end-1))], 'probeLocationsXGrid', 'probeLocationsYGrid');
        else
            finalSuffix = [suffixes{iSuffix}(1:end), 'B'];
            load(['/Volumes/FastExternal/tmp/treeTraining/labels', suffixes{iSuffix}], 'probeLocationsXGrid', 'probeLocationsYGrid');
        end
        
        load(['~/tmp/trainedSvmPolynomial3', suffixes{iSuffix}]);
        
        load(['/Volumes/FastExternal/tmp/treeTraining/labels', finalSuffix],...
            ['labelNames', finalSuffix], ['labels', finalSuffix], ['featureValues', finalSuffix]);
        
        [numCorrect, numTotal] = computeAccuracy(trainedSvmClassifiers, labels100B, labelNames100B, featureValues100B);
        
        keyboard
    end
    
end % tmp_testExamples()

function trainedSvmClassifiers = runTest(saveFilenamePrefix, suffix, kernelType)
    load([saveFilenamePrefix, suffix],...
        ['labelNames', suffix], ['labels', suffix], ['featureValues', suffix],...
        'probeLocationsXGrid', 'probeLocationsYGrid');
    
    if strcmp(kernelType, 'linear')
        eval(['trainedSvmClassifiers = svm_train(labels', suffix, ', featureValues', suffix, ');']);
    elseif strcmp(kernelType, 'polynomial3')
        eval(['trainedSvmClassifiers = svm_train(labels', suffix, ', featureValues', suffix, ', ''libsvmParameters'', ''-m 2000 -t 1 '');']);
    else
        keyboard
    end
end % runTest()


function [numCorrect, numTotal] = computeAccuracy(trainedSvmClassifiers, labels, labelNames, featureValues)
    
    pBar = ProgressBar('svm_testExamples');
    pBar.showTimingInfo = true;
    pBarCleanup = onCleanup(@()delete(pBar));
    
    pBar.set_increment(5 / size(featureValues, 2));
    pBar.set(0);
    
    maxLabelId = max(labels);
    
    numCorrect = zeros(maxLabelId + 1, 1);
    numTotal = zeros(maxLabelId + 1, 1);
    for iImage = 1:size(featureValues, 2)
        [~, labelId] = svm_predict(trainedSvmClassifiers, featureValues(:,iImage), [], [], [], labelNames, 0, 255);

        numTotal(labelId) = numTotal(labelId) + 1;

        if labelId == labels(iImage)
            numCorrect(labelId) = numCorrect(labelId) + 1;
        end
        
        if mod(iImage, 5) == 0
            pBar.set_message(sprintf('Testing %d/%d. Current accuracy %d/%d=%f', iImage, size(featureValues, 2), sum(numCorrect), sum(numTotal), sum(numCorrect)/sum(numTotal)));
            pBar.increment();
        end
    end
end
