% function [extrema, horizontalExtrema, verticalExtrema] = computeBinaryExtrema(im, extremaFilterWidth, extremaFilterSigma, extremaDerivativeThreshold, getMinima, getMaxima, blurWithMatchingFilter, matchingFilterWidth, matchingFilterSigma)

function [extrema, horizontalExtrema, verticalExtrema] = computeBinaryExtrema(...
    im,...
    extremaFilterWidth, extremaFilterSigma, extremaDerivativeThreshold, ...
    getMinima, getMaxima, ...
    blurWithMatchingFilter, matchingFilterWidth, matchingFilterSigma)

if ~exist('blurWithMatchingFilter', 'var')
    blurWithMatchingFilter = false;
end

assert(extremaFilterWidth >= 5);
assert(mod(extremaFilterWidth,2) == 1);

im = double(im);

f = [-3  0 3 ;
     -10 0 10;
     -3  0 3 ];

g = fspecial('gaussian',[extremaFilterWidth, 1], extremaFilterSigma);
g = g / sum(g(:));

imf = imfilter(imfilter(im, g), g');

ix = imfilter(imf, f);
iy = imfilter(imf, f');

ix([1,end], :) = 0;
ix(:, [1,end]) = 0;

iy([1,end], :) = 0;
iy(:, [1,end]) = 0;

horizontalExtrema = zeros(size(ix));
verticalExtrema = zeros(size(iy));

%     extremaDerivativeThreshold = 1*255;

for y = 4:(size(im,1)-3)
    for x = 4:(size(im,2)-3)
        if getMaxima
            if ix(y,x) > extremaDerivativeThreshold
                if ix(y, x-1) < ix(y, x) && ix(y, x+1) <= ix(y, x)
                    horizontalExtrema(y,x) = 1;
                end
            end
        end

        if getMinima
            if ix(y,x) < -extremaDerivativeThreshold
                if ix(y, x-1) > ix(y, x) && ix(y, x+1) >= ix(y, x)
                    horizontalExtrema(y,x) = 1;
                end
            end
        end
    end
end

for x = 4:(size(im,2)-3)
    for y = 4:(size(im,1)-3)
        if getMaxima
            if iy(y,x) > extremaDerivativeThreshold
                if iy(y-1, x) < iy(y, x) && iy(y+1, x) <= iy(y, x)
                    verticalExtrema(y,x) = 1;
                end
            end
        end

        if getMinima
            if iy(y,x) < -extremaDerivativeThreshold
                if iy(y-1, x) > iy(y, x) && iy(y+1, x) >= iy(y, x)
                    verticalExtrema(y,x) = 1;
                end
            end
        end
    end
end

if blurWithMatchingFilter
    matchingFilterWidth2 = floor(matchingFilterWidth/2);
    matchingFilterWidth = 2*matchingFilterWidth2 + 1;

    filter = fspecial('gaussian', [matchingFilterWidth,1], matchingFilterSigma);
    filter = filter / max(filter(:));

    extrema = zeros(size(horizontalExtrema));

    horizontalExtremaBlurred = zeros(size(horizontalExtrema));
    horizontalExtrema([1:(matchingFilterWidth2+1), (end-matchingFilterWidth2):end], :) = 0;
    horizontalExtrema(:, [1:(matchingFilterWidth2+1), (end-matchingFilterWidth2):end]) = 0;

    verticalExtremaBlurred = zeros(size(horizontalExtrema));
    verticalExtrema([1:(matchingFilterWidth2+1), (end-matchingFilterWidth2):end], :) = 0;
    verticalExtrema(:, [1:(matchingFilterWidth2+1), (end-matchingFilterWidth2):end]) = 0;

%     tic
    [indsy, indsx] = find(horizontalExtrema ~= 0);
    for i = 1:length(indsy)
        x = indsx(i);
        y = indsy(i);
%         extrema(y, (x-matchingFilterWidth2):(x+matchingFilterWidth2)) = max(filter', extrema(y, (x-matchingFilterWidth2):(x+matchingFilterWidth2)));
%         horizontalExtremaBlurred(y, (x-matchingFilterWidth2):(x+matchingFilterWidth2)) = max(filter', horizontalExtremaBlurred(y, (x-matchingFilterWidth2):(x+matchingFilterWidth2)));
        extrema(y, (x-matchingFilterWidth2):(x+matchingFilterWidth2)) = filter';
        horizontalExtremaBlurred(y, (x-matchingFilterWidth2):(x+matchingFilterWidth2)) = filter';
    end

	[indsy, indsx] = find(verticalExtrema ~= 0);
    for i = 1:length(indsy)
        x = indsx(i);
        y = indsy(i);
%         extrema((y-matchingFilterWidth2):(y+matchingFilterWidth2), x) = max(filter, extrema((y-matchingFilterWidth2):(y+matchingFilterWidth2), x));
%         verticalExtremaBlurred((y-matchingFilterWidth2):(y+matchingFilterWidth2), x) = max(filter, verticalExtremaBlurred((y-matchingFilterWidth2):(y+matchingFilterWidth2), x));
        extrema((y-matchingFilterWidth2):(y+matchingFilterWidth2), x) = filter;
        verticalExtremaBlurred((y-matchingFilterWidth2):(y+matchingFilterWidth2), x) = filter;
    end
%     toc

    horizontalExtrema = horizontalExtremaBlurred;
    verticalExtrema = verticalExtremaBlurred;

%     tic
%     for y = (1+matchingFilterWidth2):(size(horizontalExtrema,1)-matchingFilterWidth2)
%         for x = (1+matchingFilterWidth2):(size(horizontalExtrema,2)-matchingFilterWidth2)
%             if horizontalExtrema(y,x) ~= 0
%                 extrema(y, (x-matchingFilterWidth2):(x+matchingFilterWidth2)) = filter;
%             end
%
%             if verticalExtrema(y,x) ~= 0
%                 extrema((y-matchingFilterWidth2):(y+matchingFilterWidth2), x) = filter;
%             end
%         end
%     end
%     toc
else
    extrema = max(abs(horizontalExtrema), abs(verticalExtrema));
end





