function VisualizeExtractedProbes(data, varargin)
% Visualize a specified or sampled set of labels from ExtractProbeValues.

labels = [];
sample = [];
numProbes = VisionMarkerTrained.ProbeParameters.GridSize;
threshold = [];
showGradMag = false;

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
if N > 100
  warning('Refusing to display more than 100 examples.');
  N = 100;
end

numCols = ceil(sqrt(N));
numRows = ceil(N/numCols);

clf
for i = 1:N
  subplot(numRows,numCols,i)
  
  values = data.probeValues;
  if showGradMag
    values = data.gradMagValues;
  end
  
  if isempty(threshold)
    img = reshape(mean(values(:,data.labels==labels(i)),2), ...
      numProbes, numProbes);
  else
    img = reshape(mode(double(values(:,data.labels==labels(i)) > threshold),2), ...
      numProbes, numProbes);
    %img = reshape(mean(data.probeValues(:,data.labels==labels(i)),2) > threshold, ...
    %  numProbes, numProbes);
  end
  
  imagesc(img)
  axis image
  title(sprintf('%d: %s', labels(i), data.labelNames{labels(i)}), 'Interpreter', 'none')
end

colormap(gray)


end
