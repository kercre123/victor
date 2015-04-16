largeFilesScriptDir = '/Users/andrew/Code/products-cozmo-large-files/systemTestsData/scripts/';

doDisplay = -1;

matFiles = getfnames(largeFilesScriptDir, '*.mat');

numFiles = length(matFiles);

probeValues = cell(1, numFiles);
thresholds = cell(1, numFiles);
% probeValues2 = cell(1, numFiles);
markerTypes = cell(1, numFiles);

pBar = ProgressBar(sprintf('Extracting probe values from %d images.', numFiles));
pBar.showTimingInfo = true;
pBar.set(0);
pBar.set_increment(1/numFiles);

for iFile = 1:numFiles
  load(fullfile(largeFilesScriptDir, matFiles{iFile}), 'jsonData');
  
  numPoses = length(jsonData.Poses);
  
  
  probeValues{iFile} = cell(1, numPoses);
  thresholds{iFile} = cell(1, numPoses);
  %probeValues2{iFile} = cell(1, numPoses);
  markerTypes{iFile} = cell(1, numPoses);
  
  if numPoses == 1 && ~iscell(jsonData.Poses)
    jsonData.Poses = {jsonData.Poses};
  end
  
  for iPose = 1:numPoses
    imgFile = ['/Users/andrew/Code/products-cozmo-large-files/systemTestsData/scripts/' jsonData.Poses{iPose}.ImageFile];
    
    doingDisplay = rand <= doDisplay;
    if doingDisplay
      figure(iPose), colormap(gray)
      set(iPose, 'Name', imgFile)  
      numCols = ceil(sqrt(jsonData.Poses{iPose}.NumMarkers));
      numRows = ceil(jsonData.Poses{iPose}.NumMarkers/numCols);
    end
    
    img = mean(im2double(imread(imgFile)), 3);
    
    probeValues{iFile}{iPose} = cell(1, jsonData.Poses{iPose}.NumMarkers);
    thresholds{iFile}{iPose} = zeros(1, jsonData.Poses{iPose}.NumMarkers);
%     probeValues2{iFile}{iPose} = cell(1, jsonData.Poses{iPose}.NumMarkers);
    markerTypes{iFile}{iPose} = cell(1, jsonData.Poses{iPose}.NumMarkers);
    
    if jsonData.Poses{iPose}.NumMarkers == 1 && ~iscell(jsonData.Poses{iPose}.VisionMarkers)
      jsonData.Poses{iPose}.VisionMarkers = {jsonData.Poses{iPose}.VisionMarkers};
    end
      
    for iMarker = 1:jsonData.Poses{iPose}.NumMarkers
      
      marker = jsonData.Poses{iPose}.VisionMarkers{iMarker};
      corners = [ ...
        marker.x_imgUpperLeft   marker.y_imgUpperLeft; ...
        marker.x_imgLowerLeft   marker.y_imgLowerLeft; ...
        marker.x_imgUpperRight  marker.y_imgUpperRight; ...
        marker.x_imgLowerRight  marker.y_imgLowerRight];
      
      tform = cp2tform([0 0 ; 0 1;  1 0; 1 1], corners, 'projective');
      
      thresholds{iFile}{iPose}(iMarker) = VisionMarkerTrained.ComputeThreshold(img, tform);
      probes = VisionMarkerTrained.GetProbeValues(img, tform);
      probeValues{iFile}{iPose}{iMarker} = probes(:);% > threshold;
%       probeValues2{iFile}{iPose}{iMarker} = column(probes > separable_filter(probes, gaussian_kernel(7)));
      
%       subplot 131, imagesc(probes), axis image
%       subplot 132, imagesc(reshape(probeValues{iFile}{iPose}{iMarker}, 32, 32)), axis image
%       subplot 133, imagesc(reshape(probeValues2{iFile}{iPose}{iMarker}, 32, 32)), axis image
%       pause
      
      %if all(probeValues{iFile}{iPose}{iMarker} == probeValues{iFile}{iPose}{iMarker}(1))
      %  keyboard
      %end
      
      markerTypes{iFile}{iPose}{iMarker} = jsonData.Poses{iPose}.VisionMarkers{iMarker}.markerType;
      
      if doingDisplay
        subplot(numRows, numCols, iMarker)
        imagesc(reshape(probeValues{iFile}{iPose}{iMarker}, [32 32]), [0 1]), axis image
        title(markerTypes{iFile}{iPose}{iMarker}, 'Interpreter', 'none')
      end
      
      %       temp = VisionMarkerTrained.GetProbeValues(img, tform);
      %       subplot 131, imagesc(temp), axis image
      %       subplot 132, imagesc(temp> threshold), axis image
      %       subplot 133, imagesc(temp > separable_filter(temp, gaussian_kernel(7))), axis image
      %       pause
      
    end % FOR each marker
    
    probeValues{iFile}{iPose} = [probeValues{iFile}{iPose}{:}];
    %probeValues2{iFile}{iPose} = [probeValues2{iFile}{iPose}{:}];
    %markerTypes{iFile}{iPose} = [markerTypes{iFile}{iPose}{:}];
    
  end % FOR each pose
  
  probeValues{iFile} = [probeValues{iFile}{:}];
  thresholds{iFile} = [thresholds{iFile}{:}];
  %probeValues2{iFile} = [probeValues2{iFile}{:}];
  markerTypes{iFile} = [markerTypes{iFile}{:}];
  
  pBar.increment();
  
end % FOR each file

realData = struct( ...
  'probeValues', [probeValues{:}]', ...
  'thresholds', [thresholds{:}]');

%   'probeValues2', [probeValues2{:}]'); 

realData.markerTypes = [markerTypes{:}];

clear probeValues*
clear thresholds
clear markerTypes
delete(pBar)

%% 
if exist('data', 'var') && isstruct(data)
  
  % Strip rotation from labelNames and add 'MARKER_'
  labelNamesUnrotated = cellfun(@(name)['MARKER_' upper(strrep(strrep(strrep(strrep(name, '_000', ''), '_090', ''), '_270', ''), '_180', ''))], data.labelNames, 'UniformOutput', false);

  fprintf('Predicting with single tree.\n');
  predictedLabels = predict(ctree, single(realData.probeValues));
  accSingle = sum(strcmp(labelNamesUnrotated(predictedLabels), realData.markerTypes)) / length(realData.markerTypes);
  
  numTreesList = [1 2 4 8 10];
  acc = zeros(1, length(numTreesList));
  acc2 = zeros(1, length(numTreesList));
  accPruned = zeros(1, length(numTreesList));
  accPruned2 = zeros(1, length(numTreesList));
  
  for iTreeList = 1:length(numTreesList)
    iTree = numTreesList(iTreeList);
    fprintf('Predicting with %d of %d trees in the ensemble.\n', iTree, ensembleCTrees.NumTrained);
    predictedLabels = predict(ensembleCTrees, single(realData.probeValues), 'learners', 1:iTree);
    
    predictedLabels2 = predict(ensembleCTrees, single(realData.probeValues2), 'learners', 1:iTree);
    
    predictedLabelsPruned  = predict(prunedEnsemble, single(realData.probeValues), 'learners', 1:iTree);
    predictedLabelsPruned2 = predict(prunedEnsemble, single(realData.probeValues2), 'learners', 1:iTree);
    
    acc(iTreeList) = sum(strcmp(labelNamesUnrotated(predictedLabels), realData.markerTypes)) / length(realData.markerTypes);    
    acc2(iTreeList) = sum(strcmp(labelNamesUnrotated(predictedLabels2), realData.markerTypes)) / length(realData.markerTypes);    
    
    accPruned(iTreeList) = sum(strcmp(labelNamesUnrotated(predictedLabelsPruned), realData.markerTypes)) / length(realData.markerTypes);    
    accPruned2(iTreeList) = sum(strcmp(labelNamesUnrotated(predictedLabelsPruned2), realData.markerTypes)) / length(realData.markerTypes);    
  end
  
  hold off
  plot([0 ensembleCTrees.NumTrained], accSingle*[1 1], 'k--');
  hold on
  plot(numTreesList, acc, 'r')
  plot(numTreesList, acc2, 'g')
  plot(numTreesList, accPruned, 'b')
  plot(numTreesList, accPruned2, 'c')
  
  legend('single', 'bagged', 'bagged+pruned', 'bagged2', 'bagged2+pruned')
  
  %%
  
  accPruned = zeros(1, prunedEnsemble.NumTrained);
  for iTree = 1:prunedEnsemble.NumTrained
    fprintf('Predicting with %d of %d pruned trees in the ensemble.\n', iTree, prunedEnsemble.NumTrained);
    predictedLabels = predict(prunedEnsemble, single(realData.probeValues), 'learners', 1:iTree);
    accPruned(iTree) = sum(strcmp(labelNamesUnrotated(predictedLabels), realData.markerTypes)) / length(realData.markerTypes);    
  end
  
  plot(accPruned, 'r')
  
  legend('single', 'bagged', 'bagged+pruned')
  
end

