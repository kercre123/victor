function [newWeights, alpha] = UpdateBoostingWeights(tree, probeValues, labels, oldWeights, useProbability)

% Following "Multi-class AdaBoost," by Zhu et al.
% http://web.stanford.edu/~hastie/Papers/samme.pdf

secondBestMultiplier = 1;

numLabels = double(max(labels(:))); % K

% Get misclassifications on training data
probePattern = VisionMarkerTrained.ProbePattern;
numExamples = size(probeValues,2);
c = zeros(1,numExamples);


pBar = ProgressBar('Update Boosting Weights');
pBar.showTimingInfo = true;
pBarCleanup = onCleanup(@()delete(pBar));
pBar.set_message('Running tree on training examples.');
pBar.set_increment(100/numExamples);
pBar.set(0);

if useProbability
    p = cell(numExamples,1);
    Y = -ones(numExamples,numLabels)/(numLabels-1);
    for iExample = 1:numExamples
        [~,labelID] = TestTree(tree, probeValues(:,iExample), [], 0.5, probePattern, false);
        
        p{iExample} = accumarray(labelID(:), ones(size(labelID)), [numLabels 1])';
        
        Y(iExample,labels(iExample)) = 1;
        
        if mod(iExample,100)==0
            pBar.increment();
        end
    end
    p = vertcat(p{:});
    p = p ./ (sum(p,2)*ones(1,numLabels));
    
%     % weighted class probabilities:
%     W = oldWeights(ones(numLabels,1),:)';
%     pClass = sum(W.*p,1);
%     pClass = max(eps, pClass/sum(pClass));
%     
%     log_pClass = log(pClass);
%     sumLogPk = sum(log_pClass);
%     alpha = (numLabels-1)*(log_pClass - (1/numLabels)*sumLogPk);
    alpha = 0;
        
    newWeights = oldWeights .* exp(-(numLabels-1)/numLabels * sum(Y.*log(max(eps,p)),2))';
    
else
    
    for iExample = 1:numExamples
        [~,labelID] = TestTree(tree, probeValues(:,iExample), [], 0.5, probePattern, false);
        
        if isscalar(labelID) || all(labelID == labelID(1))
            c(iExample) = labelID(1);
        else
            p = accumarray(labelID(:), ones(size(labelID)));
            
            if secondBestMultiplier > 1 
                % Return the class with the max p value, only if it is
                % sufficiently larger than the second largest value
                [pSorted, sortIndex] = sort(p, 'descend');
                if pSorted(1) > secondBestMultiplier*pSorted(2)
                    c(iExample) = sortIndex(1);
                else
                    c(iExample) = -1;
                end
            else
                assert(secondBestMultiplier == 1)
                [~,c(iExample)] = max(p);
            end
            
        end
        
        if mod(iExample,100)==0
            pBar.increment();
        end
    end % FOR each example


    misclassifications = c ~= labels;
    fprintf('%d misclassifications.\n', sum(misclassifications));
    
    err = sum(oldWeights.*double(misclassifications)) / sum(oldWeights);
    assert(err>=0 && err<=1, 'Expecting err value between 0 and 1.');
    err = max(eps,min(1-eps,err)); % avoid log(0) and divide-by-zero below
    alpha = log((1 - err)/err) + log(numLabels-1);
    newWeights = oldWeights .* exp(alpha*double(misclassifications));
        
end

newWeights = newWeights / sum(newWeights);

fprintf('Mean abs diff in weights = %f\n', mean(abs(oldWeights-newWeights)));

% figure(99)
% plot(newWeights), hold on
end