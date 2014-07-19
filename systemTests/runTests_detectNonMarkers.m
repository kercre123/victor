
% runTests_detectNonMarkers('~/tmp', '~/tmp',...
%   {'/Users/pbarnum/Documents/datasets/external/KAIST/**/*.jpg',...
%    '/Users/pbarnum/Documents/datasets/external/chars74k/**/*.jpg',...
%    '/Users/pbarnum/Documents/datasets/external/icdar2003/**/*.jpg',...
%    '/Users/pbarnum/Documents/datasets/external/icdar2011/**/*.jpg',...
%    '/Users/pbarnum/Documents/datasets/external/256_ObjectCategories/**/*.jpg',...
%    '/Users/pbarnum/Documents/datasets/external/svt1/**/*.jpg',...
%    '/Users/pbarnum/Documents/datasets/external/IndoorCVPR_09/**/*.jpg',...
%    '/Users/pbarnum/Documents/datasets/external/W31_Images/**/*.jpg',...
%    '/Users/pbarnum/Documents/datasets/external/VOCtrainval_11-May-2009/JPEGImages/**/*.jpg',...
%    '/Users/pbarnum/Documents/datasets/external/egocentric_objects_intel_06_2009/**/*.jpg'});

function runTests_detectNonMarkers(resultsDirectory, temporaryDirectory, filenamePatterns)
    
    numComputeThreads = 3;
    
    ignoreModificationTime = true;
        
    [workQueue_todo, workQueue_all] = computeWorkQueues(filenamePatterns, ignoreModificationTime);
    
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
    
    algorithmParametersN = algorithmParameters;
    algorithmParametersN.extractionFunctionName = 'matlab-with-refinement';
    algorithmParametersN.extractionFunctionId = 20000;
    algorithmParametersN.useMatlabForAll = true;
    algorithmParametersN.maxDetectionSize = [768, 1024];
    
    results_detectQuadsAndMarkers = run_detectQuadsAndMarkers(numComputeThreads, workQueue_todo, workQueue_all, temporaryDirectory, algorithmParametersN); %#ok<NASGU>
    
    save([resultsDirectory,'/latestNonMarkersResults.mat'], '*');
    
    keyboard
    
end % runTests_detectNonMarkers()

function [workQueue_todo, workQueue_all] = computeWorkQueues(filenamePatterns, ignoreModificationTime)
    workQueue_todo = {};
    workQueue_all = {};
    
    for iPattern = 1:length(filenamePatterns)
        curFilenames = rdir(filenamePatterns{iPattern});
        for iFilename = 1:length(curFilenames)
            newWorkItem.iPattern = iPattern;
            newWorkItem.iFilename = iFilename;
            newWorkItem.inputFilename = curFilenames(iFilename).name;
            newWorkItem.outputFilename = [newWorkItem.inputFilename, '.mat'];
            
            workQueue_all{end+1} = newWorkItem; %#ok<AGROW>

            if ignoreModificationTime
                workQueue_todo{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
            
            % If the results don't exist
            if ~exist(newWorkItem.outputFilename, 'file')
                workQueue_todo{end+1} = newWorkItem; %#ok<AGROW>
                continue;
            end
        end
    end
end % computeWorkQueues()

function results_detectQuadsAndMarkers = run_detectQuadsAndMarkers(numComputeThreads, workQueue_todo, workQueue_all, temporaryDirectory, algorithmParameters) %#ok<INUSD>
    allInputFilename = [temporaryDirectory, '/detectNonMarkersInput.mat'];
    
    save(allInputFilename, 'algorithmParameters');
    
    matlabCommandString = ['load(''', allInputFilename, '''); ' , 'runTests_detectNonMarkers_perImage(localWorkQueue, algorithmParameters);'];
    
    runParallelProcesses(numComputeThreads, workQueue_todo, temporaryDirectory, matlabCommandString);
    
    delete(allInputFilename);
    
    results_detectQuadsAndMarkers = cell(0,1);
    
    for iWork = 1:length(workQueue_all)
        load(workQueue_all{iWork}.outputFilename);
        
        if length(results_detectQuadsAndMarkers) < workQueue_all{iWork}.iPattern || isempty(results_detectQuadsAndMarkers{workQueue_all{iWork}.iPattern})
            results_detectQuadsAndMarkers{workQueue_all{iWork}.iPattern} = cell(0,1);
        end
        
        newDetection.detectedQuads = detectedQuads;
        newDetection.detectedQuadValidity = detectedQuadValidity;
        newDetection.detectedMarkers = detectedMarkers;
        newDetection.imageSize = imageSize;
        newDetection.scale = scale;
        
        results_detectQuadsAndMarkers{workQueue_all{iWork}.iPattern}{workQueue_all{iWork}.iFilename} = newDetection;        
    end
end



