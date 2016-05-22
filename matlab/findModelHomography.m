function [model, H, score, matches] = findModelHomography(img, models)

useRANSAC = true;
numRansacIterations = 100;
inlierThreshold = 32;
matchThreshold = 1.25;

if ~iscell(models)
  models = {models};
end

img = im2single(img);

if size(img,3) > 1
  img = rgb2gray(img);
end

img_model = createModel(img, 'Observation');

numModels = length(models);
H_model = cell(1, numModels);
score_model = zeros(1, numModels);
matches_model = cell(1,numModels);
for iModel = 1:numModels
  
  [matches_model{iModel},~] = vl_ubcmatch(img_model.descriptors, ...
    models{iModel}.descriptors, matchThreshold);
  
  numMatches = size(matches_model{iModel},2);
  
  if useRANSAC
    if numMatches < 4
      continue;
    end
    
    X_img   = [img_model.frames(1:2,matches_model{iModel}(1,:)); ones(1,numMatches)];
    X_model = [models{iModel}.frames(1:2,matches_model{iModel}(2,:)); ones(1,numMatches)];
    
    H_current = cell(1, numRansacIterations);
    inliers = cell(1, numRansacIterations);
    numInliers = zeros(1, numRansacIterations);
    for iRansac = 1:numRansacIterations
      samples = vl_colsubset(1:numMatches,4);
      
      A = [];
      for sample = 1:samples
        A = [A; kron(X_img(:,sample)', vl_hat(X_model(:,sample)))];
      end
      [~,~,V] = svd(A);
      H_current{iRansac} = reshape(V(:,9),3,3);
      
      X_imgWarp = H_current{iRansac} * X_img;
      
      du = X_imgWarp(1,:)./X_imgWarp(3,:) - X_model(1,:)./X_model(3,:);
      dv = X_imgWarp(2,:)./X_imgWarp(3,:) - X_model(2,:)./X_model(3,:);
      inliers{iRansac} = (du.*du + dv.*dv) < inlierThreshold*inlierThreshold;
      numInliers(iRansac) = sum(inliers{iRansac});
      
    end % FOR each RANSAC iteration
    
    [maxNumInliers, bestRansacIndex] = max(numInliers);
    
    H_model{iModel} = H_current{bestRansacIndex};
    score_model(iModel) = maxNumInliers;
    matches_model{iModel} = matches_model{iModel}(:,inliers{bestRansacIndex});
    
  else
    score_model(iModel) = numMatches / size(img_model.frames,2);
  end
  
end % FOR each model

% Return best model
[maxScore, bestModelIndex] = max(score_model);
H = H_model{bestModelIndex};
score = maxScore;
model = models{bestModelIndex};
matches = matches_model{bestModelIndex}(2,:);

end