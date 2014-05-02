
% function [model, meanError] = fit2dCurve(data, polynomialDegree)

% example:
% [x,y] = meshgrid(0:(10), 0:(10)); data = 5 + 6*x + 7*y + 40*rand(size(x));
% [model, meanError] = fit2dCurve(data, 1, true);

function [model, meanError] = fit2dCurve(data, polynomialDegree, displayResult)

[x,y] = meshgrid(0:(size(data,2)-1), 0:(size(data,1)-1));

x = x(:);
y = y(:);

dataPercent = double(data(:)) / double(max(data(:)));

% dataV = double(data(:));

A = ones(length(dataPercent), 1);

for degree = 1:polynomialDegree
    A(:,end+1) = x.^degree;
    A(:,end+1) = y.^degree;
end

% model = A' / dataPercent';
model = dataPercent' / A'

values = model(1) * ones(length(dataPercent), 1);

for degree = 1:polynomialDegree
    model(2*(degree-1) + 2)
    model(2*(degree-1) + 3)
    values = values + model(2*(degree-1) + 2) * x(:).^degree;
    values = values + model(2*(degree-1) + 3) * y(:).^degree;
end

diff = values - dataPercent;

meanError = mean(diff);

if exist('displayResult', 'var') && displayResult > 0
    figure(1);
    hold off;
    surf(reshape(values, size(data))); 
    hold on; 
    title('original and fit model');
    % surf(reshape(dataPercent, size(data)), 'EdgeColor', 'black');
    surf(reshape(dataPercent, size(data)), zeros([size(data), 3]), 'EdgeColor', 'white');
    hold off;

    figure(2);
    hold off;
    title('difference from model');
    surf(reshape(dataPercent - values, size(data))); 
end

% keyboard
