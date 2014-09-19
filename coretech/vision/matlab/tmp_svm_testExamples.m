
function tmp_svm_testExamples()
    
    rerunTraining = true;
    
    if rerunTraining
        suffixes = {'100', '100B', '300', '300B', '600', '600B', '1000', '1000B'};
        
        for iSuffix = 1:length(suffixes)
            trainedSvmClassifiers = runTest('~/tmp/labels', suffixes{iSuffix});
            
            save(['~/tmp/trainedSvm', suffixes{iSuffix}], 'trainedSvmClassifiers');
        end
    end
    
    
end % tmp_testExamples()

function trainedSvmClassifiers = runTest(saveFilenamePrefix, suffix)
    load([saveFilenamePrefix, suffix],...
        ['labelNames', suffix], ['labels', suffix], ['featureValues', suffix],...
        'probeLocationsXGrid', 'probeLocationsYGrid');
    
    eval(['trainedSvmClassifiers = svm_train(labels', suffix, ', featureValues', suffix, ');']);
end % runTest()

