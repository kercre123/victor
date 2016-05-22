% function trackLines(im)

% im1 = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_38.png');
% im2 = imread('C:/Anki/systemTestImages/cozmo_2014-01-29_11-41-05_0.png');

% trackLines(imresize(im1,[120,160]));

function trackLines(im)

useDerivative = true;
% useDerivative = false;

im = double(im);

if useDerivative

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

%     imshows(im/255, imf/255, ix/255, iy/255, ixMaxima, iyMaxima);
else % if useDerivative

    f = [40,40,40, 0, 180,180,180];

    scale = 1000;

    left = matchTemplate(im, f(end:-1:1));
    right = matchTemplate(im, f);
    top = matchTemplate(im, f(end:-1:1)');
    bottom = matchTemplate(im, f');

    left = suppressHorizontalNonMinima(left);
    right = suppressHorizontalNonMinima(right);
    top = suppressVerticalNonMinima(top);
    bottom = suppressVerticalNonMinima(bottom);    

    leftB = zeros(size(left)); leftB(left < scale) = 1;
    rightB = zeros(size(right)); rightB(right < scale) = 1;
    topB = zeros(size(top)); topB(top < scale) = 1;
    bottomB = zeros(size(bottom)); bottomB(bottom < scale) = 1;

    totalB = max(topB, max(bottomB,(max(leftB, rightB))));

%     imshows(im/255, left/scale, right/scale, top/scale, bottom/scale, leftB, rightB, topB, bottomB, totalB, 'maximize');
    imshows(im/255, leftB, rightB, topB, bottomB, totalB, 'maximize');

end % if useDerivative ... else

keyboard

function suppressed = suppressVerticalNonMinima(im)
    suppressed = inf * ones(size(im));

    for y = 2:(size(im,1)-1)
        xs = 2:(size(im,2)-1);

        validInds = (im(y, xs) < im(y-1, xs)) & (im(y, xs) < im(y+1, xs));

        suppressed(y,validInds) = im(y,validInds);
    end

function suppressed = suppressHorizontalNonMinima(im)
    suppressed = inf * ones(size(im));

    for x = 2:(size(im,2)-1)
        ys = 2:(size(im,1)-1);

        validInds = (im(ys, x) < im(ys, x-1)) & (im(ys, x) < im(ys, x+1));

        suppressed(validInds,x) = im(validInds,x);
    end

function score = matchTemplate(im, template)
    score = inf * ones(size(im));

    templateSize = size(template);
    templateSize2 = floor(templateSize / 2);
    templateSize = 2*templateSize2 + 1;

    limitsY = [1+templateSize2(1), size(im,1)-templateSize2(1)];
    limitsX = [1+templateSize2(2), size(im,2)-templateSize2(2)];

    for y = limitsY(1):limitsY(2)
        for x = limitsX(1):limitsX(2)
            diff = sum(abs(im((y-templateSize2(1)):(y+templateSize2(1)), (x-templateSize2(2)):(x+templateSize2(2))) - template));
            score(y,x) = diff;
        end
    end




