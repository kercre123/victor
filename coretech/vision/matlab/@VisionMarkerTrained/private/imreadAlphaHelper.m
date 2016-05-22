function img = imreadAlphaHelper(fname)

[img, ~, alpha] = imread(fname);
img = mean(im2double(img),3);
img(alpha < .5) = 1;

threshold = (max(img(:)) + min(img(:)))/2;
img = double(img > threshold);

end % imreadAlphaHelper()