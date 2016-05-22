
% function [model, inBounds_meanAbsError, all_meanAbsError, minCenterPoint, maxCenterPoint] = fit2dCurve(data, polynomialDegree, fixedParameters, displayResult)

% Output model is in the form [constant, x, y, x^2, y^2, x^3, y^3, ...]

% If a fixedParameter is nan, it is estimated. If it is a number, that
% number is used

% example:
% [x,y] = meshgrid(0.5:(10.5), 0.5:(10.5)); data = 5 + 6*x + 7*y + 40*rand(size(x));
% fixedParameters = [nan, nan, nan]; % [constant, x, y]
% [model, inBounds_meanAbsError, all_meanAbsError, minCenterPoint, maxCenterPoint] = fit2dCurve(data, 1, fixedParameters, [], [], true);

% estimate vignetting (non-robust):
% imr = double(im); [model, inBounds_meanAbsError, all_meanAbsError, minCenterPoint, maxCenterPoint] = fit2dCurve(1./ (imr / max(imr(:))), 2, [nan, nan, nan, nan, nan], [], [], false)

% estimate vignetting (robust):
% imr = double(im); [model, inBounds_meanAbsError, all_meanAbsError, minCenterPoint, maxCenterPoint] = fit2dCurve(1./ (imr / max(imr(:))), 2, [nan, nan, nan, nan, nan], .05, 100, false)

function [model, inBounds_meanAbsError, all_meanAbsError, minCenterPoint, maxCenterPoint] = fit2dCurve(data, polynomialDegree, fixedParameters, robustThreshold, robustNumIterations, displayResult)

if ~exist('fixedParameters', 'var') || isempty(fixedParameters)
    fixedParameters = nan * ones(1, 1 + 2*polynomialDegree);
end

if ~exist('displayResult', 'var')
    displayResult = false;
end

if ~exist('robustThreshold', 'var') || isempty(robustThreshold)
    robustThreshold = Inf;
end

if ~exist('robustNumIterations', 'var') || isempty(robustNumIterations)
    robustNumIterations = 1;
end

numParametersTotal = 1 + 2*polynomialDegree;

assert(length(fixedParameters) == numParametersTotal);

numParametersToEstimate = length(find(isnan(fixedParameters)));

data = double(data);

[x,y] = meshgrid(0.5:(size(data,2)-0.5), 0.5:(size(data,1)-0.5));

x = x(:);
y = y(:);

mask = ones(numel(data), 1);

scaledData = computeScaledData(x, y, data, fixedParameters, polynomialDegree);

if displayResult
    figure(1); close(1);
    figure(2); close(2);
end

for iteration = 1:robustNumIterations
    model = computeModel(x, y, scaledData, mask, fixedParameters, polynomialDegree, numParametersToEstimate, numParametersTotal);

    [~, diff, inBounds_meanAbsError, ~, ~] = computeError(x, y, data, mask, model, polynomialDegree);

    mask = abs(diff) < robustThreshold;

    [values, ~, all_meanAbsError, minCenterPoint, maxCenterPoint] = computeError(x, y, data, ones(numel(data), 1), model, polynomialDegree);

    outputString = [sprintf('After %d iterations, inBounds_meanAbsError:%f all_meanAbsError:%f numInBounds:%d/%d model:[', iteration, inBounds_meanAbsError, all_meanAbsError, length(abs(diff) < robustThreshold), numel(data)), sprintf('%f, ', model), ']'];

    disp(outputString);

    if displayResult
        figure(1);
        hold off;
        surf(reshape(values, size(data)));
        shading interp
        hold on;
        title('original and fit model');
        % surf(reshape(scaledData, size(data)), 'EdgeColor', 'black');
        surf(reshape(scaledData, size(data)), zeros([size(data), 3]), 'EdgeColor', 'white');
        hold off;

%         figure(2);
%         hold off;
%         surf(reshape(scaledData - values, size(data)));
%         shading interp
%         title('difference from model');

%         pause(.5);
    end
end % for iterator = 1:robustNumIterations

if displayResult
    figure(2);
    hold off;
    surf(reshape(scaledData - values, size(data)));
    shading interp
    title('difference from model');
end


% keyboard

function [values, diff, meanAbsError, minCenterPoint, maxCenterPoint] = computeError(x, y, data, mask, model, polynomialDegree)
    numValidSamples = length(find(mask > 0));

    values = model(1) * ones(numValidSamples, 1);

    for degree = 1:polynomialDegree
        values = values + model(2*(degree-1) + 2) * x(mask > 0).^degree;
        values = values + model(2*(degree-1) + 3) * y(mask > 0).^degree;
    end

    diff = values - data(mask > 0);

    meanAbsError = mean(abs(diff));

    if isempty(find(mask == 0, 1))
        valuesMin = min(values(:));        
        valuesMax = max(values(:));        
        
        [indsY, indsX] = find(reshape(values,size(data)) == valuesMin);
        minCenterPoint = [mean(indsY), mean(indsX)];
        
        [indsY, indsX] = find(reshape(values,size(data)) == valuesMax);
        maxCenterPoint = [mean(indsY), mean(indsX)];
    else
        minCenterPoint = [Inf, Inf];
        maxCenterPoint = [Inf, Inf];
    end

function scaledData = computeScaledData(x, y, data, fixedParameters, polynomialDegree)
    scaledData = data(:);

    if ~isnan(fixedParameters(1))
        scaledData = scaledData - fixedParameters(1);
    end

    for degree = 1:polynomialDegree
        curXParameter = 2 + 2*(degree-1);
        curYParameter = 3 + 2*(degree-1);

        if ~isnan(fixedParameters(curXParameter))
            scaledData = scaledData - fixedParameters(2 + 2*(degree-1)) * x.^degree;
        end

        if ~isnan(fixedParameters(curYParameter))
            scaledData = scaledData - fixedParameters(3 + 2*(degree-1)) * y.^degree;
        end
    end

function model = computeModel(x, y, scaledData, mask, fixedParameters, polynomialDegree, numParametersToEstimate, numParametersTotal)
    numValidSamples = length(find(mask > 0));

    A = zeros(numValidSamples, numParametersToEstimate);

    curParameter = 1;

    if isnan(fixedParameters(1))
        A(:,curParameter) = ones(numValidSamples, 1);
        curParameter = curParameter + 1;
    end

    for degree = 1:polynomialDegree
        curXParameter = 2 + 2*(degree-1);
        curYParameter = 3 + 2*(degree-1);

        if isnan(fixedParameters(curXParameter))
            A(:,curParameter) = x(mask > 0).^degree;
            curParameter = curParameter + 1;
        end

        if isnan(fixedParameters(curYParameter))
            A(:,curParameter) = y(mask > 0).^degree;
            curParameter = curParameter + 1;
        end
    end

    modelRaw = scaledData(mask > 0)' / A';

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

