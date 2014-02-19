function w = createProbeWeights(n, probeGap, probeRadius, probeSigma, codePadding)

% patternWidth = (1 - 2*codePadding);
% squareWidth = patternWidth/n;
% 
% [xgrid, ygrid] = meshgrid((linspace(probeGap*squareWidth, (1-probeGap)*squareWidth, 2*probeRadius+1) - squareWidth/2));
% xgrid = row(xgrid);
% ygrid = row(ygrid);
% 
% w = exp(-(xgrid.^2 + ygrid.^2) / (2*probeSigma^2));
% w = w/sum(w);
% w = w(ones(n^2,1),:);

w = ones(n*n,9)/9;

end