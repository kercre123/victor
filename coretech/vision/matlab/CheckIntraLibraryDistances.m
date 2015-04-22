% Perform all pairwise comparisons to check how distinct elements in the
% library are
function d = CheckIntraLibraryDistances(nnLibrary)

N = size(nnLibrary.probeValues,2);
X = single(nnLibrary.probeValues);
W = single(nnLibrary.gradMagWeights);
sumW = sum(W,1);

d = nan(N);
for i = 1:N

  whichCols = (i+1):N;
  d(i,(i+1):end) = sum(W(:,whichCols).*single(imabsdiff(X(:,i*ones(1,N-i)), X(:,whichCols))),1) ./ sumW(whichCols);
  
end

imagesc(d), axis image

end


function helper
%%
[iSame,jSame] = find(d < 10);
for i = 1:length(iSame)
  subplot 121, imagesc(reshape(nnLibrary.probeValues(:,iSame(i)), 32,32)), axis image 
  title(sprintf('%d of %d', i, length(iSame)))
  subplot 122, imagesc(reshape(nnLibrary.probeValues(:,jSame(i)), 32,32)), axis image
  title(d(iSame(i),jSame(i)))
  pause
end

end