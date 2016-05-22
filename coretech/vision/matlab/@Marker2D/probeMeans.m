function means = probeMeans(this, img, tform)

% Transform the probe locations to match the image coordinates
[xImg, yImg] = tforminv(tform, this.Xprobes, this.Yprobes);

% Get means
assert(ismatrix(img), 'Image should be grayscale.');
[nrows,ncols] = size(img);
xImg = max(1, min(ncols, xImg));
yImg = max(1, min(nrows, yImg));
index = round(yImg) + (round(xImg)-1)*size(img,1);
img = img(index);
means = reshape(sum(this.ProbeWeights.*img,2), size(this.Layout));

assert(isequal(size(means), size(this.Layout)), ...
    'Means size does not match layout size.');

end % FUNCTION probeMeans()