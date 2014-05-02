
% function [model, meanError] = fit2dCurve(data, polynomialDegree, fixedParameters, displayResult)

% Output model is in the form [constant, x, y, x^2, y^2, x^3, y^3, ...]

% If a fixedParameter is nan, it is estimated. If it is a number, that
% number is used

% example:
% [x,y] = meshgrid(0.5:(10.5), 0.5:(10.5)); data = 5 + 6*x + 7*y + 40*rand(size(x));
% fixedParameters = [nan, nan, nan]; % [constant, x, y]
% [model, meanAbsError, centerPoint] = fit2dCurve(data, 1, fixedParameters, true);

% estimate vignetting:
% imr = double(im); [model, meanError, centerPoint] = fit2dCurve(1./ (imr / max(imr(:))), 2, [nan, nan, nan, nan, nan], false)

function [model, meanAbsError, centerPoint] = fit2dCurve(data, polynomialDegree, fixedParameters, displayResult)

if ~exist('fixedParameters', 'var') || isempty(fixedParameters)
    fixedParameters = nan * ones(1, 1 + 2*polynomialDegree);
end

if ~exist('displayResult', 'var')
    displayResult = false;
end

numParametersTotal = 1 + 2*polynomialDegree;

assert(length(fixedParameters) == numParametersTotal);

numParametersToEstimate = length(find(isnan(fixedParameters)));

[x,y] = meshgrid(0.5:(size(data,2)-0.5), 0.5:(size(data,1)-0.5));

x = x(:);
y = y(:);

data = double(data);

scaledData = data(:);

A = zeros(length(scaledData), numParametersToEstimate); 

curParameter = 1;

if isnan(fixedParameters(1))
    A(:,curParameter) = ones(length(scaledData), 1);
    curParameter = curParameter + 1;
else
    scaledData = scaledData - fixedParameters(1);
end

for degree = 1:polynomialDegree
    curXParameter = 2 + 2*(degree-1);
    curYParameter = 3 + 2*(degree-1);
    
    if isnan(fixedParameters(curXParameter))
        A(:,curParameter) = x.^degree;
        curParameter = curParameter + 1;
    else
        scaledData = scaledData - fixedParameters(2 + 2*(degree-1)) * x.^degree;
    end
    
    if isnan(fixedParameters(curYParameter))
        A(:,curParameter) = y.^degree;
        curParameter = curParameter + 1;
    else
        scaledData = scaledData - fixedParameters(3 + 2*(degree-1)) * y.^degree;
    end
end

modelRaw = scaledData' / A';

curRawParameter = 1;

model = zeros(1, numParametersTotal);

if isnan(fixedParameters(1))
    model(1) = modelRaw(curRawParameter);
    curRawParameter = curRawParameter + 1;
else
    model(1) = fixedParameters(1);    
end

for degree = 1:polynomialDegree
    curXParameter = 2 + 2*(degree-1);
    curYParameter = 3 + 2*(degree-1);
    
    if isnan(fixedParameters(curXParameter))
        model(curXParameter) = modelRaw(curRawParameter);
        curRawParameter = curRawParameter + 1;
    else
        model(curXParameter) = fixedParameters(curXParameter);
    end
    
    if isnan(fixedParameters(curYParameter))
        model(curYParameter) = modelRaw(curRawParameter);
        curRawParameter = curRawParameter + 1;
    else
        model(curYParameter) = fixedParameters(curYParameter);
    end
end

values = model(1) * ones(length(scaledData), 1);

for degree = 1:polynomialDegree
    values = values + model(2*(degree-1) + 2) * x(:).^degree;
    values = values + model(2*(degree-1) + 3) * y(:).^degree;
end

diff = values - data(:);

meanAbsError = mean(abs(diff));

valuesMax = max(values(:));

[indsY, indsX] = find(reshape(values,size(data)) == valuesMax);

centerPoint = [mean(indsY), mean(indsX)];

if displayResult
    close 1
    close 2
    
    figure(1);
    hold off;
    surf(reshape(values, size(data))); 
    shading interp
    hold on; 
    title('original and fit model');
    % surf(reshape(scaledData, size(data)), 'EdgeColor', 'black');
    surf(reshape(scaledData, size(data)), zeros([size(data), 3]), 'EdgeColor', 'white');
    hold off;

    figure(2);
    hold off;
    surf(reshape(scaledData - values, size(data))); 
    shading interp
    title('difference from model');
end

% keyboard
