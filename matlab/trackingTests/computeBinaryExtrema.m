% function [horizontalExtrema, verticalExtrema] = computeBinaryExtrema(im, getMinima, getMaxima)

function [extrema, horizontalExtrema, verticalExtrema] = computeBinaryExtrema(im, filterWidth, filterSigma, derivativeThreshold, getMinima, getMaxima, blurResult, blurWidth, blurSigma)

if ~exist('blurResult', 'var')
    blurResult = false;
end

im = double(im);

f = [-3  0 3 ;
     -10 0 10;
     -3  0 3 ];

g = fspecial('gaussian',[filterWidth, 1], filterSigma);
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

%     derivativeThreshold = 1*255;

for y = 4:(size(im,1)-3)
    for x = 4:(size(im,2)-3)
        if getMaxima
            if ix(y,x) > derivativeThreshold                
                if ix(y, x-1) < ix(y, x) && ix(y, x+1) < ix(y, x)
                    horizontalExtrema(y,x) = 1;
                end
            end
        end

        if getMinima
            if ix(y,x) < -derivativeThreshold
                if ix(y, x-1) > ix(y, x) && ix(y, x+1) > ix(y, x)
                    horizontalExtrema(y,x) = 1;
                end
            end
        end
    end
end

for x = 4:(size(im,2)-3)
    for y = 4:(size(im,1)-3)
        if getMaxima
            if iy(y,x) > derivativeThreshold
                if iy(y-1, x) < iy(y, x) && iy(y+1, x) < iy(y, x)
                    verticalExtrema(y,x) = 1;
                end
            end
        end

        if getMinima
            if iy(y,x) < -derivativeThreshold
                if iy(y-1, x) > iy(y, x) && iy(y+1, x) > iy(y, x)
                    verticalExtrema(y,x) = 1;
                end
            end
        end
    end
end

if blurResult
    blurWidth2 = floor(blurWidth/2);
    blurWidth = 2*blurWidth2 + 1;
    
    filter = fspecial('gaussian', [blurWidth,1], blurSigma);
    filter = filter / max(filter(:));
    
    extrema = zeros(size(horizontalExtrema));
    
    horizontalExtrema([1:(blurWidth2+1), (end-blurWidth2):end], :) = 0;
    horizontalExtrema(:, [1:(blurWidth2+1), (end-blurWidth2):end]) = 0;
    
    verticalExtrema([1:(blurWidth2+1), (end-blurWidth2):end], :) = 0;
    verticalExtrema(:, [1:(blurWidth2+1), (end-blurWidth2):end]) = 0;
    
%     tic
    [indsy, indsx] = find(horizontalExtrema ~= 0);
    for i = 1:length(indsy)
        x = indsx(i);
        y = indsy(i);
        extrema(y, (x-blurWidth2):(x+blurWidth2)) = max(filter', extrema(y, (x-blurWidth2):(x+blurWidth2)));
    end
    
	[indsy, indsx] = find(verticalExtrema ~= 0);
    for i = 1:length(indsy)
        x = indsx(i);
        y = indsy(i);
        extrema((y-blurWidth2):(y+blurWidth2), x) = max(filter, extrema((y-blurWidth2):(y+blurWidth2), x));
    end
%     toc
    
%     tic
%     for y = (1+blurWidth2):(size(horizontalExtrema,1)-blurWidth2)
%         for x = (1+blurWidth2):(size(horizontalExtrema,2)-blurWidth2)
%             if horizontalExtrema(y,x) ~= 0
%                 extrema(y, (x-blurWidth2):(x+blurWidth2)) = filter;
%             end
%             
%             if verticalExtrema(y,x) ~= 0
%                 extrema((y-blurWidth2):(y+blurWidth2), x) = filter;
%             end
%         end
%     end
%     toc
else
    extrema = max(abs(horizontalExtrema), abs(verticalExtrema));    
end





