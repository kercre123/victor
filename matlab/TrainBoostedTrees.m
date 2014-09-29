function trees = TrainBoostedTrees(data, varargin)

resumeFile = sprintf('%s_resumeState.mat', mfilename('fullpath'));

if nargin == 0 || (nargin == 1 && ischar(data))
  if nargin == 1
    resumeFile = data;
  end
  
  fprintf('Will attempt to resume training from %s.\n', resumeFile);
  
  assert(exist(resumeFile, 'file') > 0, 'Resume file does not exist.');
  
  resumeData = load(resumeFile);
  
  resumeData = rmfield(resumeData, {'resumeData', 'fields', 'requiredFields', 'resumeFile'});
  
  requiredFields = {'trees', 'data', 'maxDepth', 'weights', 'alphas', ...
    'numBoostingIterations', 'useProbability', 'sampleFraction', ...
    'maxDepth', 'resumeSaveTime', 'startIteration'};
  
  if any(~isfield(resumeData, requiredFields))
    error('Missing required data in resume file.');
  end
  
  fields = fieldnames(resumeData);
  for iField = 1:length(fields)
    eval(sprintf('%s = resumeData.%s;', fields{iField}, fields{iField}));
  end
 
  clear resumeData fields requiredFields % don't want to save these into the resumeFile!
  
else
  
  numBoostingIterations = 5;
  useProbability = true;
  sampleFraction = 1;
  maxDepth = 4;
  resumeSaveTime = 5; % in MINUTES!
    
  parseVarargin(varargin{:});

  if sampleFraction < 1
    sampleIndex = rand(1,size(data.probeValues,2)) < sampleFraction;
    data.probeValues = data.probeValues(:,sampleIndex);
    data.gradMagValues = data.gradMagValues(:,sampleIndex);
    data.labels = data.labels(sampleIndex);
    data.numImages = sum(sampleIndex);
  end
  
  startIteration = 1;
  weights = ones(1,size(data.probeValues,2))/size(data.probeValues,2);
  alphas = cell(1, numBoostingIterations);
  
  trees = cell(1, numBoostingIterations);
end

pBar = ProgressBar('Boosting', 'CancelButton', true);
pBar.showTimingInfo = true;
pBarCleanup = onCleanup(@()delete(pBar));
pBar.set_message(sprintf('Training %d boosted trees.', numBoostingIterations));
pBar.set_increment(1/numBoostingIterations);
pBar.set((startIteration-1)/numBoostingIterations);

t_resumeSave = tic;

for iBoost = startIteration:numBoostingIterations
    trees{iBoost} = VisionMarkerTrained.TrainProbeTree('trainingState', data, 'maxDepth', maxDepth, ...
        'baggingSampleFraction', 1, 'redBlackVerifyDepth', 0, 'saveTree', false, 'testTree', false, 'weights', weights);
    
    [weights, alphas{iBoost}] = UpdateBoostingWeights(trees{iBoost}, data.probeValues, data.labels, weights, useProbability);
    
    pBar.increment();
        
    if pBar.cancelled || toc(t_resumeSave) >= resumeSaveTime*60
      fprintf('Saving resume state...');
      startIteration = iBoost + 1;
      save(resumeFile);
      t_resumeSave = tic;
      fprintf('Done.\n');
    end
    
    if pBar.cancelled
      fprintf('Stopping after %d iterations.\n', iBoost);
      break;
    end
end

trees = [trees{:}];

% Put alphas inside tree
for iBoost = 1:length(trees)
    trees(iBoost).alpha = alphas{iBoost};
end

end