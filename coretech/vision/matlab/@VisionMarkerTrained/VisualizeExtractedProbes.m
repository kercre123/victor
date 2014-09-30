function VisualizeExtractedProbes(data, varargin)
% Visualize a specified or sampled set of labels from ExtractProbeValues.

labels = [];
sample = [];
numProbes = 32;

parseVarargin(varargin{:});

if ~isempty(sample)
  if ~isempty(labels) 
    warning('Ignoring given labels.');
  end
  
  if ischar(sample)
    labels = find(cellfun(@(x)~isempty(x), regexp(data.labelNames, sample)));
  else
    assert(isscalar(sample), 'Sample should be scalar.');
    numLabels = length(data.labelNames);
    
    if sample < 1   %#ok<BDSCI>
      labels = find(rand(1,numLabels) < sample);
    else
      randIndex = randperm(numLabels);
      labels = randIndex(1:min(sample, numLabels));
    end
  end
end
  
assert(~isempty(labels), 'Labels should not be empty.');
N = length(labels);
numCols = ceil(sqrt(N));
numRows = ceil(N/numCols);

for i = 1:N
  subplot(numRows,numCols,i)
  imagesc(reshape(mean(data.probeValues(:,data.labels==labels(i)),2), ...
    numProbes, numProbes))
  axis image
  title(data.labelNames{labels(i)}, 'Interpreter', 'none')
end

colormap(gray)



end
