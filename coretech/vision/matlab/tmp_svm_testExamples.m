
function tmp_svm_testExamples()
    
    rerunTraining = false;
    
    suffixes = {'100', '100B', '300', '300B', '600', '600B', '1000', '1000B'};
    %         suffixes = {'100B', '300', '300B', '600', '600B', '1000', '1000B'};
    
    if rerunTraining
        for iSuffix = 1:length(suffixes)
            trainedSvmClassifiers = runTest('~/tmp/labels', suffixes{iSuffix});
            
            save(['~/tmp/trainedSvm', suffixes{iSuffix}], 'trainedSvmClassifiers');
        end
    end
    
    for iSuffix = 1:length(suffixes)
        if suffixes{iSuffix}(end) == 'B'
            finalSuffix = [suffixes{iSuffix}(1:(end-1)), ''];
            load(['~/tmp/labels', suffixes{iSuffix}(1:(end-1))], 'probeLocationsXGrid', 'probeLocationsYGrid');
        else
            finalSuffix = [suffixes{iSuffix}(1:end), 'B'];
            load(['~/tmp/labels', suffixes{iSuffix}], 'probeLocationsXGrid', 'probeLocationsYGrid');
        end
        
        load(['~/tmp/trainedSvm', suffixes{iSuffix}]);
        
        load(['~/tmp/labels', finalSuffix],...
            ['labelNames', finalSuffix], ['labels', finalSuffix], ['featureValues', finalSuffix]);
        
        [numCorrect, numTotal] = computeAccuracy(trainedSvmClassifiers, labels100B, labelNames100B, featureValues100B);
        
        keyboard
    end
    
end % tmp_testExamples()

function trainedSvmClassifiers = runTest(saveFilenamePrefix, suffix)
    load([saveFilenamePrefix, suffix],...
        ['labelNames', suffix], ['labels', suffix], ['featureValues', suffix],...
        'probeLocationsXGrid', 'probeLocationsYGrid');
    
    eval(['trainedSvmClassifiers = svm_train(labels', suffix, ', featureValues', suffix, ');']);
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
