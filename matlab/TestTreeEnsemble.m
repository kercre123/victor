function p = TestTreeEnsemble(ensemble, img, tform, threshold, pattern)

assert(isstruct(ensemble), 'Ensemble should be struct array.');

numTrees = length(ensemble);
numLabels = length(ensemble(1).labels);

p = cell(numTrees,1);
for iTree = 1:numTrees
    assert(length(ensemble(iTree).labels) == numLabels, ...
        'All trees in the ensemble should have same number of labels.');
    
    [~, labelID] = TestTree(ensemble(iTree), img, tform, threshold, pattern, false);
    p{iTree} = accumarray(labelID(:), ones(size(labelID)), [numLabels 1]);
    p{iTree} = p{iTree}/sum(p{iTree}); % move outside loop and vectorize?
end

p = sum([p{:}],2);
p = p / sum(p);

if nargout == 0
    bar(1:numLabels, p);
end

end
