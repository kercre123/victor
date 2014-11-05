function C = TestBoostedTrees(trees, img, tform, threshold, pattern)

secondBestMultiplier = 1;
useProbability = trees(1).alpha == 0;

numTrees = length(trees);
numClasses = length(trees(1).labels);


if useProbability
  
  h = cell(numTrees,1);
  for iTree = 1:numTrees
    
    [~,labelID] = TestTree(trees(iTree), img, tform, threshold, pattern, false);
    
    p = accumarray(labelID(:), ones(size(labelID)), [numClasses 1]);
    p = p/sum(p);
    log_p = log(max(eps,p));
    
    h{iTree} = (numClasses-1)*(log_p - sum(log_p)/numClasses);
    
  end % FOR each tree
  
  C = sum([h{:}],2);
  %     [pSorted, sortIndex] = sort(p, 'descend');
  %
  %     if pSorted(1) > secondBestMultiplier*pSorted(2)
  %       C = sortIndex(1);
  %     else
  %       C = 0;
  %     end
  
else
  
  C = zeros(numTrees,numClasses);
  for iTree = 1:numTrees
    
    [~,labelID] = TestTree(trees(iTree), img, tform, threshold, pattern, false);
    
    if isscalar(labelID) || all(labelID == labelID(1))
      c = labelID(1);
    else
      p = accumarray(labelID(:), ones(size(labelID)), [numClasses 1]);
      
      if secondBestMultiplier > 1
        % Return the class with the max p value, only if it is
        % sufficiently larger than the second largest value
        [pSorted, sortIndex] = sort(p, 'descend');
        if pSorted(1) > secondBestMultiplier*pSorted(2)
          c = sortIndex(1);
        else
          c = -1;
        end
      else
        assert(secondBestMultiplier == 1)
        [~,c] = max(p);
      end
    end
    
    C(iTree, c) = trees(iTree).alpha;
  end
  
  %[~,C] = max(sum(C,1));
  C = sum(C,1);
  
end

end