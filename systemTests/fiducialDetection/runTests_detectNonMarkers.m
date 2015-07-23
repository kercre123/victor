
% runTests_detectNonMarkers('~/tmp', '~/tmp', {'datasets/external', 'datasets/external/00results'}, {'/Users/pbarnum/Documents/datasets/external/KAIST/**/*.jpg', '/Users/pbarnum/Documents/datasets/external/chars74k/**/*.jpg', '/Users/pbarnum/Documents/datasets/external/icdar2003/**/*.jpg', '/Users/pbarnum/Documents/datasets/external/icdar2011/**/*.jpg', '/Users/pbarnum/Documents/datasets/external/256_ObjectCategories/**/*.jpg', '/Users/pbarnum/Documents/datasets/external/svt1/**/*.jpg', '/Users/pbarnum/Documents/datasets/external/IndoorCVPR_09/**/*.jpg', '/Users/pbarnum/Documents/datasets/external/W31_Images/**/*.jpg', '/Users/pbarnum/Documents/datasets/external/VOCtrainval_11-May-2009/JPEGImages/**/*.jpg', '/Users/pbarnum/Documents/datasets/external/egocentric_objects_intel_06_2009/**/*.jpg'});
% runTests_detectNonMarkers('~/tmp', '~/tmp', {'datasets/external', 'datasets/external/00results'}, {'/Users/pbarnum/Documents/datasets/external/256_ObjectCategories/177.saturn/*.jpg'});

function runTests_detectNonMarkers(resultsDirectory, temporaryDirectory, replaceStringForOutput, filenamePatterns)
    
    numComputeThreads = 1;
    
    ignoreModificationTime = false;
    
    [workQueue_todo, workQueue_all] = computeWorkQueues(filenamePatterns, replaceStringForOutput, ignoreModificationTime);
    
    disp(sprintf('workQueue_todo has %d elements', length(workQueue_todo)));
    
    algorithmParameters.scaleImage_thresholdMultiplier = 1.0;
    algorithmParameters.scaleImage_numPyramidLevels = 3;
    algorithmParameters.component1d_minComponentWidth = 0;
    algorithmParameters.component1d_maxSkipDistance = 0;
    algorithmParameters.component_minimumNumPixels = 10;
    algorithmParameters.component_maximumNumPixels = 10000;
    algorithmParameters.component_sparseMultiplyThreshold = 1000.0;
    algorithmParameters.component_solidMultiplyThreshold = 2.0;
    algorithmParameters.component_minHollowRatio = 1.0;
    algorithmParameters.quads_minQuadArea = 100 / 4;
    algorithmParameters.quads_quadSymmetryThreshold = 2.0;
    algorithmParameters.quads_minDistanceFromImageEdge = 2;
    algorithmParameters.decode_minContrastRatio = 1.25;
    algorithmParameters.refine_quadRefinementMaxCornerChange = 2;
    algorithmParameters.refine_numRefinementSamples = 100;
    algorithmParameters.refine_quadRefinementIterations = 25;
    algorithmParameters.useMatlabForAll = false;
    algorithmParameters.useMatlabForQuadExtraction = false;
    algorithmParameters.matlab_embeddedConversions = EmbeddedConversionsManager();
    algorithmParameters.drawOutputImage = false;
    algorithmParameters.showImageDetectionWidth = 640;
    
    algorithmParametersN = algorithmParameters;
    algorithmParametersN.extractionFunctionName = 'matlab-with-refinement';
    algorithmParametersN.extractionFunctionId = 20000;
    algorithmParametersN.useMatlabForAll = true;
    algorithmParametersN.maxDetectionSize = [768, 1024];
    
    results_detectQuadsAndMarkers = run_detectQuadsAndMarkers(numComputeThreads, workQueue_todo, workQueue_all, temporaryDirectory, algorithmParametersN);
    
    save([resultsDirectory,'/latestNonMarkersResults.mat'], '*');
    
    [results_quads, results_markers] = run_compileNonMarkers(results_detectQuadsAndMarkers); %#ok<ASGLU,NASGU>
    
    nonMarkerQuads = {}; 
    for i = 1:length(results_quads)
        for j = 1:length(results_quads{i}.allQuads_pixelValues)
            nonMarkerQuads{end+1} = results_quads{i}.allQuads_pixelValues{j}; %#ok<AGROW>
        end
    end
    
    markerQuads = {};
    for i = 1:length(results_markers)
        for j = 1:length(results_markers{i}.markers_pixelValues)
            markerQuads{end+1} = results_markers{i}.markers_pixelValues{j}; %#ok<AGROW>
        end
    end
    
    save([resultsDirectory,'/latestNonMarkersResults.mat'], '*');
    save([resultsDirectory,'/latestNonMarkersResults_samplesOnly.mat'], 'nonMarkerQuads', 'markerQuads');
    
%     for i = 1:length(nonMarkerQuads) imshow(imresize(nonMarkerQuads{i}/max(nonMarkerQuads{i}(:))*.75, [256,256])); pause(.03); end
%     for i = 1:length(markerQuads) imshow(imresize(markerQuads{i}/max(markerQuads{i}(:))*.75, [256,256])); pause(); end
    
    keyboard
    
end % runTests_detectNonMarkers()

function [workQueue_todo, workQueue_all] = computeWorkQueues(filenamePatterns, replaceStringForOutput, ignoreModificationTime)
    workQueue_todo = {};
    workQueue_all = {};
    
    for iPattern = 1:length(filenamePatterns)
        curFilenames = rdir(filenamePatterns{iPattern});
        for iFilename = 1:length(curFilenames)
            newWorkItem.iPattern = iPattern;
            newWorkItem.iFilename = iFilename;
            newWorkItem.inputFilename = curFilenames(iFilename).name;
            newWorkItem.dataOutputFilename = strrep([newWorkItem.inputFilename, '.mat'], replaceStringForOutput{1}, replaceStringForOutput{2});
            newWorkItem.imageOutputFilename = strrep([newWorkItem.inputFilename, '_out.png'], replaceStringForOutput{1}, replaceStringForOutput{2});
            
            workQueue_all{end+1} = newWorkItem; %#ok<AGROW>
            
            if ignoreModificationTime
                workQueue_todo{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
            
            % If the results don't exist
            if ~exist(newWorkItem.dataOutputFilename, 'file')
                workQueue_todo{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
        end
    end
end % computeWorkQueues()

% Call this manually, if you want to delete all .mat files generated for
% each individual image
function deleteIntermediateOutputFiles(workQueue_all)
    for iWork = 1:length(workQueue_all)
        delete(workQueue_all{iWork}.dataOutputFilename);
        delete(workQueue_all{iWork}.imageOutputFilename);
    end
end % deleteIntermediateOutputFiles()

function results_detectQuadsAndMarkers = run_detectQuadsAndMarkers(numComputeThreads, workQueue_todo, workQueue_all, temporaryDirectory, algorithmParameters) %#ok<INUSD>
    allInputFilename = [temporaryDirectory, '/detectNonMarkersInput.mat'];
    
    save(allInputFilename, 'algorithmParameters');
    
    matlabCommandString = ['dbstop error; load(''', allInputFilename, '''); ' , 'runTests_detectNonMarkers_perImage(localWorkQueue, algorithmParameters);'];
    
    runParallelProcesses(numComputeThreads, workQueue_todo, temporaryDirectory, matlabCommandString);
    
    delete(allInputFilename);
    
    results_detectQuadsAndMarkers = cell(0,1);
    
%     for iWork = 1:length(workQueue_all)
%         try
%             load(workQueue_all{iWork}.dataOutputFilename);
%         catch
%             disp(['Deleting ', workQueue_all{iWork}.dataOutputFilename]);
%             delete(workQueue_all{iWork}.dataOutputFilename)
%         end
%     end
    
    for iWork = 1:length(workQueue_all)
        load(workQueue_all{iWork}.dataOutputFilename);
        
        if length(results_detectQuadsAndMarkers) < workQueue_all{iWork}.iPattern || isempty(results_detectQuadsAndMarkers{workQueue_all{iWork}.iPattern})
            results_detectQuadsAndMarkers{workQueue_all{iWork}.iPattern} = cell(0,1);
        end
        
        newDetection.inputFilename = workQueue_all{iWork}.inputFilename;
        newDetection.dataOutputFilename = workQueue_all{iWork}.dataOutputFilename;
        newDetection.imageOutputFilename = workQueue_all{iWork}.imageOutputFilename;
        newDetection.detectedQuads = detectedQuads;
        newDetection.detectedQuadValidity = detectedQuadValidity;
        newDetection.detectedMarkers = detectedMarkers;
        newDetection.allQuads_pixelValues = allQuads_pixelValues;
        newDetection.markers_pixelValues = markers_pixelValues;
        newDetection.imageSize = imageSize;
        newDetection.scale = scale;
        
        results_detectQuadsAndMarkers{workQueue_all{iWork}.iPattern}{workQueue_all{iWork}.iFilename} = newDetection;
    end
end % run_detectQuadsAndMarkers()

function [results_quads, results_markers] = run_compileNonMarkers(results_detectQuadsAndMarkers)
    results_quads = {};
    results_markers = {};
    
    for iPattern = 1:length(results_detectQuadsAndMarkers)
        for iFilename = 1:length(results_detectQuadsAndMarkers{iPattern})
            curDetection = results_detectQuadsAndMarkers{iPattern}{iFilename};
            
            if ~isempty(curDetection.detectedQuads)
                results_quads{end+1} = curDetection; %#ok<AGROW>
            end
            
            if ~isempty(curDetection.detectedMarkers)
                results_markers{end+1} = curDetection; %#ok<AGROW>
            end
        end
    end
end % run_compileNonMarkers
