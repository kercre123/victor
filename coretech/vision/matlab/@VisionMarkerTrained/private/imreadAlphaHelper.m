function img = imreadAlphaHelper(fname)

[img, ~, alpha] = imread(fname);
img = mean(im2double(img),3);

if any(alpha(:) < 255)
  % Figure out whether this is a white-on-black or black-on-white marker by
  % looking at the mean value of the non-transparent stuff
  alpha = im2double(alpha);
  meanVal = mean(img(alpha > .5));
  if meanVal < .5
    % Black printing on white background
    img(alpha < .5) = 1;
  else
    % White printing on black background
    img(alpha < .5) = 0;
  end
end

threshold = (max(img(:)) + min(img(:)))/2;
img = double(img > threshold);

end % imreadAlphaHelper()