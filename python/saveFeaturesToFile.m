
%function saveFeaturesToFile()

suffixes = {'100', '100B', '300', '300B', '600', '600B'};

for i = 1:length(suffixes)
  inputFilename = ['/Users/pbarnum/tmp/labels', suffixes{i}];
  variableName = ['featureValues', suffixes{i}];
  
  load(inputFilename, variableName);
  
  eval(['curFeatureValues = ', variableName, ';'])

  curFeatureValuesBinary = curFeatureValues;
  curFeatureValuesBinary(curFeatureValues<128) = 0; 
  curFeatureValuesBinary(curFeatureValuesBinary>0) = 1;

  tic
  for j = 1:size(curFeatureValues,1)
    outputFilename = ['/Users/pbarnum/tmp/pythonFeatures/featureValues', suffixes{i}, sprintf('_%d.array', j-1)];
    mexSaveEmbeddedArray(curFeatureValues(j,:), outputFilename);
    
    outputFilename = ['/Users/pbarnum/tmp/pythonFeatures/featureValuesBinary', suffixes{i}, sprintf('_%d.array', j-1)];
    mexSaveEmbeddedArray(curFeatureValuesBinary(j,:), outputFilename);
    
    if mod(j, 50) == 0
        disp(sprintf('Finished %d/%d %d/%d in %f', i, length(suffixes), j, size(curFeatureValues,1), toc()));
    end
  end
  
  clear('featureValues*')
end