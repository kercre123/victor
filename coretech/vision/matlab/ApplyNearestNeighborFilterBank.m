function responses = ApplyNearestNeighborFilterBank(probeValues)

persistent filters

if isempty(filters)
  % Create filters
  filters = cell(1,4);
  filters{1} = [1 0 -1];
  filters{2} = [1 0 -1]';
  filters{3} = [1 0 0 0 -1];
  filters{4} = [1 0 0 0 -1]';
end

numFilters = length(filters);

if isstruct(probeValues)
  probeValues = probeValues.probeValues;
  N = size(probeValues,2);
  fprintf('Applying filter bank to %d examples\n', N);
  responses = cell(1, N);
  gridSize = sqrt(size(probeValues,1));
  for i = 1:N
    responses{i} = ApplyNearestNeighborFilterBank(reshape(probeValues(:,i), gridSize, gridSize));
  end
  responses = [responses{:}];
  return
end

responses = cell(1, numFilters);
for iFilter = 1:numFilters
  if isempty(filters{iFilter})
    responses{iFilter} = probeValues(:);
  else
    responses{iFilter} = uint8((abs(imfilter(double(probeValues), filters{iFilter}, 255))));
  end
  
  if nargout == 0
    subplot(1, numFilters, iFilter)
    imagesc(responses{iFilter}, [0 255]), axis image
  else
    responses{iFilter} = column(responses{iFilter});
  end
end

responses = vertcat(responses{:});

end
