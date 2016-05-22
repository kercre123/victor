% function corrected = correctVignetting(image, model)

% Compute model with fit2dCurve

function corrected = correctVignetting(image, model)

polynomialDegree = (length(model) - 1) / 2;

[x,y] = meshgrid(0.5:(size(image,2)-0.5), 0.5:(size(image,1)-0.5));

values = model(1) * ones(size(image));

for degree = 1:polynomialDegree
    values = values + model(2*(degree-1) + 2) * x.^degree;
    values = values + model(2*(degree-1) + 3) * y.^degree;
end

corrected = double(image) .* (1 ./ values);

keyboard
% corrected = correctVignetting(image, model)
