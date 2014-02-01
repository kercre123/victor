% function trackLines(im)

% im1 = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_38.png');
% im2 = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_0.png');

% trackLines(imresize(im1,[120,160]));

function trackLines(im)

im = double(im);

f = [-3  0 3 ;
     -10 0 10;
     -3  0 3 ];
% f = [-3  -3  0 3  3 ;
%      -10 -10 0 10 10;
%      -3  -3  0 3  3];

% f = f / sum(f(:));

g = fspecial('gaussian',[3 1], 1.0);
% g = fspecial('gaussian',[7 1], 3.0);
g = g / sum(g(:));

imf = imfilter(imfilter(im, g), g');

ix = imfilter(imf, f);
iy = imfilter(imf, f');

ix([1,end], :) = 0; 
ix(:, [1,end]) = 0;

iy([1,end], :) = 0; 
iy(:, [1,end]) = 0;

ixMaxima = zeros(size(ix));
iyMaxima = zeros(size(iy));

derivativeThreshold = 1*255;

for y = 4:(size(im,1)-3)
    for x = 4:(size(im,2)-3)
        if ix(y,x) > derivativeThreshold
            if ix(y, x-1) < ix(y, x) && ix(y, x+1) < ix(y, x)
                ixMaxima(y,x) = 1;
            end
        end
    end
end

for x = 4:(size(im,2)-3)
    for y = 4:(size(im,1)-3)
        if iy(y,x) > derivativeThreshold
            if iy(y-1, x) < iy(y, x) && iy(y+1, x) < iy(y, x)
                iyMaxima(y,x) = 1;
            end
        end
    end
end

imshows(im/255, imf/255, ix/255, iy/255, ixMaxima, iyMaxima);

keyboard