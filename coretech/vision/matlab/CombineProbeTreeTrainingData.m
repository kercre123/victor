function dataCombined = CombineProbeTreeTrainingData(data1, data2)

assert(isstruct(data1) && isstruct(data2) && isequal(fieldnames(data1), fieldnames(data2)), ...
  'Expecting two structs with the same fields.');

% Keep a single invalid label at the end
assert(strcmp(data1.labelNames{end}, 'INVALID'), 'Expecting last labelName in data1 to be "INVALID".');
assert(strcmp(data2.labelNames{end}, 'INVALID'), 'Expecting last labelName in data2 to be "INVALID".');

dataCombined.probeValues = [data1.probeValues data2.probeValues];
if isfield(data1, 'gradMagValues')
  dataCombined.gradMagValues = [data1.gradMagValues data2.gradMagValues];
end
dataCombined.fnames = [data1.fnames; data2.fnames];
dataCombined.labelNames = [data1.labelNames(1:end-1) data2.labelNames];
data1.labels(data1.labels == length(data1.labelNames)) = length(dataCombined.labelNames);
dataCombined.labels = [data1.labels data2.labels+length(data1.labelNames)-1];

dataCombined.numImages = data1.numImages + data2.numImages;
dataCombined.xgrid = data1.xgrid;
dataCombined.ygrid = data1.ygrid;

assert(isequal(data1.xgrid, data2.xgrid) && isequal(data1.ygrid, data2.ygrid), ...
  'Both structs xgrid and ygrid members must be the same.');

assert(dataCombined.numImages == size(dataCombined.probeValues,2), ...
  'Combined numImages should match number of columns in combined probeValues matrix.');

end
