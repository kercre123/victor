% Perform all pairwise comparisons to check how distinct elements in the
% library are
function d = CheckIntraLibraryDistances(nnLibrary)

N = size(nnLibrary.probeValues,2);
X = single(nnLibrary.probeValues);
if isfield(nnLibrary, 'gradMagWeights')
  W = single(nnLibrary.gradMagWeights);
else
  W = ones(size(X), 'single');
end
sumW = sum(W,1);

d = nan(N);
for i = 1:N

  whichCols = (i+1):N;
  d(i,(i+1):end) = sum(W(:,whichCols).*single(imabsdiff(X(:,i*ones(1,N-i)), X(:,whichCols))),1) ./ sumW(whichCols);
  
end

if nargout==0
  imagesc(d), axis image
  clear d
end

end


function helper
%%
[iSame,jSame] = find(d < 15);
gridSize = sqrt(size(nnLibrary.probeValues,1));
for i = 1:length(iSame)
  subplot 121, imagesc(reshape(nnLibrary.probeValues(:,iSame(i)), gridSize, gridSize)), axis image 
  title(sprintf('%d of %d: %s', i, length(iSame), nnLibrary.labelNames{nnLibrary.labels(iSame(i))}), 'Interp', 'none')
  subplot 122, imagesc(reshape(nnLibrary.probeValues(:,jSame(i)), gridSize, gridSize)), axis image
  title(sprintf('d=%f: %s', d(iSame(i),jSame(i)), nnLibrary.labelNames{nnLibrary.labels(jSame(i))}), 'Interp', 'none')
  pause
end

end