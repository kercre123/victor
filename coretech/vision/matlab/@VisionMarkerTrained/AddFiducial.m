function imgNew = AddFiducial(img, varargin)

CropImage = true;

parseVarargin(varargin{:});

img = mean(im2double(img),3);

if CropImage
    [nrows,ncols] = size(img);
    
    row = any(img<1,1); %#ok<UNRCH>
    xmin = find(row, 1, 'first');
    xmax = find(row, 1, 'last');
    
    col = any(img<1,2);
    ymin = find(col, 1, 'first');
    ymax = find(col, 1, 'last');
    
    imgDim = max(ymax-ymin+1, xmax-xmin+1);
    
    img = img(ymin:min(nrows,(ymin+imgDim-1)), xmin:min(ncols,(xmin+imgDim-1)));
end

[nrows,ncols] = size(img);
if nrows > ncols
    padding = nrows - ncols;
    if mod(padding,2)==0
        padding = padding*[0.5 0.5];
    else
        padding = [floor(padding/2) ceil(padding/2)];
    end
    img = padarray(padarray(img, [0 padding(1)], 1, 'pre'), [0 padding(2)], 1, 'post');
    
elseif ncols > nrows
    padding = ncols - nrows;
    if mod(padding,2)==0
        padding = padding*[0.5 0.5];
    else
        padding = [floor(padding/2) ceil(padding/2)];
    end
    img = padarray(padarray(img, [padding(1) 0], 1, 'pre'), [padding(2) 0], 1, 'post');
end

[nrows,ncols] = size(img);
assert(nrows==ncols, 'By now, image should be square.');

[squareWidth_pix, padding_pix] = VisionMarkerTrained.GetFiducialPixelSize(nrows);
imgNew = padarray(img, 2*padding_pix+squareWidth_pix*[1 1], 1, 'both');

[nrows,ncols,~] = size(imgNew);
imgNew(padding_pix+(1:squareWidth_pix), (padding_pix+1):(ncols-padding_pix)) = 0;
imgNew((nrows-padding_pix-squareWidth_pix+1):(nrows-padding_pix), (padding_pix+1):(ncols-padding_pix)) = 0;

imgNew((padding_pix+1):(nrows-padding_pix), padding_pix+(1:squareWidth_pix)) = 0;
imgNew((padding_pix+1):(nrows-padding_pix), (ncols-padding_pix-squareWidth_pix+1):(ncols-padding_pix)) = 0;

if nargout == 0
    subplot 121, imagesc(img), axis image
    
    squareFrac = VisionMarkerTrained.SquareWidthFraction;
    paddingFrac = VisionMarkerTrained.FiducialPaddingFraction*VisionMarkerTrained.SquareWidthFraction;
    subplot 122, imagesc([-paddingFrac 1+paddingFrac], [-paddingFrac 1+paddingFrac], imgNew), axis image
    rectangle('Pos', [(squareFrac+paddingFrac)*[1 1] (1-2*squareFrac-2*paddingFrac)*[1 1]], 'EdgeColor', 'r', 'LineStyle', '--');
    
    colormap(gray)
end

end