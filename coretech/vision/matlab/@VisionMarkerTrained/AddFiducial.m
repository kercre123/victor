function [imgNew, AlphaChannel] = AddFiducial(img, varargin)

CropImage = true;
InvertImage = false;
OutputSize = 512;
PadOutside = false;
FiducialColor = [0 0 0];
TransparentColor = [1 1 1];
TransparencyTolerance = 0.1;
OutputFile = '';

parseVarargin(varargin{:});

AlphaChannel = [];
if ischar(img)
    [img, ~, AlphaChannel] = imread(img);
    img = im2double(img);
    
    AlphaChannel = im2double(AlphaChannel);
end

if isempty(AlphaChannel)
    AlphaChannel = ones(size(img,1),size(img,2));
end

img = im2double(img);
if size(img,3)==1
    img = repmat(img,[1 1 3]);
end
%img = mean(im2double(img),3);

if CropImage
    black = any(img<0.5,3) & AlphaChannel > 0;   
    
    row = any(black,1); 
    xmin = find(row, 1, 'first');
    xmax = find(row, 1, 'last');
    
    col = any(black,2);
    ymin = find(col, 1, 'first');
    ymax = find(col, 1, 'last');
    
    %imgDim = max(ymax-ymin+1, xmax-xmin+1);
    
    img = img(ymin:ymax, xmin:xmax,:);
    AlphaChannel = AlphaChannel(ymin:ymax, xmin:xmax);
end

[nrows,ncols,~] = size(img);
if nrows > ncols
    padding = nrows - ncols;
    if mod(padding,2)==0
        padding = padding*[0.5 0.5];
    else
        padding = [floor(padding/2) ceil(padding/2)];
    end
    img = padarray(padarray(img, [0 padding(1)], 1, 'pre'), [0 padding(2)], 1, 'post');
    AlphaChannel = padarray(padarray(AlphaChannel, [0 padding(1)], 1, 'pre'), [0 padding(2)], 1, 'post');
    
elseif ncols > nrows
    padding = ncols - nrows;
    if mod(padding,2)==0
        padding = padding*[0.5 0.5];
    else
        padding = [floor(padding/2) ceil(padding/2)];
    end
    img = padarray(padarray(img, [padding(1) 0], 1, 'pre'), [padding(2) 0], 1, 'post');
    AlphaChannel = padarray(padarray(AlphaChannel, [padding(1) 0], 1, 'pre'), [padding(2) 0], 1, 'post');
end

[nrows,ncols,~] = size(img);
assert(nrows==ncols, 'By now, image should be square.');

[squareWidth_pix, padding_pix] = VisionMarkerTrained.GetFiducialPixelSize(...
    nrows, 'CodeImageOnly');

if PadOutside
    imgNew = padarray(img, round(2*padding_pix+squareWidth_pix)*[1 1], 1, 'both');
    AlphaChannel = padarray(AlphaChannel, round(2*padding_pix+squareWidth_pix)*[1 1], 1, 'both');
    
    padding_pix = round(padding_pix);
    squareWidth_pix = round(squareWidth_pix);
    
    [nrows,ncols,~] = size(imgNew);
    for i=1:3
        imgNew(padding_pix+(1:squareWidth_pix), (padding_pix+1):(ncols-padding_pix),i) = FiducialColor(i);
        imgNew((nrows-padding_pix-squareWidth_pix+1):(nrows-padding_pix), (padding_pix+1):(ncols-padding_pix),i) = FiducialColor(i);
        
        imgNew((padding_pix+1):(nrows-padding_pix), padding_pix+(1:squareWidth_pix),i) = FiducialColor(i);
        imgNew((padding_pix+1):(nrows-padding_pix), (ncols-padding_pix-squareWidth_pix+1):(ncols-padding_pix),i) = FiducialColor(i);
    end
else
    imgNew = padarray(img, round(padding_pix+squareWidth_pix)*[1 1], 1, 'both'); %#ok<UNRCH>
    AlphaChannel = padarray(AlphaChannel, round(padding_pix+squareWidth_pix)*[1 1], 1, 'both'); %#ok<UNRCH>
    
    padding_pix = round(padding_pix);
    squareWidth_pix = round(squareWidth_pix);
    
    [nrows,ncols,~] = size(imgNew);
    for i=1:3
        imgNew([1:squareWidth_pix (end-squareWidth_pix+1):end],:,i) = FiducialColor(i);
        imgNew(:,[1:squareWidth_pix (end-squareWidth_pix+1):end],i) = FiducialColor(i);
    end
end

imgNew = imresize(imgNew, OutputSize*[1 1], 'nearest');
AlphaChannel = imresize(AlphaChannel, OutputSize*[1 1], 'nearest');

if ~isempty(TransparentColor)
    transImg = repmat(reshape(TransparentColor, [1 1 3]), OutputSize*[1 1]);
    AlphaChannel = double(AlphaChannel) .* double(any(abs(imgNew - transImg)>TransparencyTolerance,3));
end   

imgNew(AlphaChannel(:,:,ones(1,3)) < TransparencyTolerance) = 1;

imgNew = max(0, min(1, imgNew));
AlphaChannel = max(0, min(1, AlphaChannel));

if InvertImage
    [squareWidth_pix, padding_pix] = VisionMarkerTrained.GetFiducialPixelSize(OutputSize, 'WithUnpaddedFiducial');
    interiorRegion = round(padding_pix*.75 + squareWidth_pix):round(OutputSize - (padding_pix*.75 + squareWidth_pix));
    imgNew(interiorRegion,interiorRegion,:) = 1 - imgNew(interiorRegion,interiorRegion,:);
    AlphaChannel(interiorRegion,interiorRegion) = 1 - AlphaChannel(interiorRegion,interiorRegion);
end

if ~isempty(OutputFile)
    imwrite(imgNew, OutputFile, 'Alpha', AlphaChannel);
end


if nargout == 0
    subplot 131, imagesc(max(0,min(1,imresize(img, OutputSize*[1 1]))), 'AlphaData', AlphaChannel), axis image
    
    squareFrac = VisionMarkerTrained.SquareWidthFraction;
    paddingFrac = VisionMarkerTrained.FiducialPaddingFraction;
    
    if PadOutside
        subplot 132, hold off, imagesc([-paddingFrac 1+paddingFrac], [-paddingFrac 1+paddingFrac], imgNew), axis image, hold on
        rectangle('Pos', [(squareFrac+paddingFrac)*[1 1] (1-2*squareFrac-2*paddingFrac)*[1 1]], 'EdgeColor', 'r', 'LineStyle', '--');
        plot([0 0 1 1], [0 1 0 1], 'y+');
        
        subplot 133, hold off, imagesc(imgNew), axis image, hold on
        Corners = [padding_pix+1 padding_pix+1;
            padding_pix+1 nrows-padding_pix-1;
            ncols-padding_pix-1 padding_pix+1;
            ncols-padding_pix-1 nrows-padding_pix-1];
        plot(Corners(:,1), Corners(:,2), 'y+');
        rectangle('Pos', [(squareWidth_pix+2*padding_pix)*[1 1] (nrows-2*squareWidth_pix-4*padding_pix)*[1 1]], 'EdgeColor', 'r', 'LineStyle', '--');
    else
        subplot 132, hold off, imagesc([0 1], [0 1], imgNew), axis image, hold on
        rectangle('Pos', [(squareFrac+paddingFrac)*[1 1] (1-2*squareFrac-2*paddingFrac)*[1 1]], 'EdgeColor', 'r', 'LineStyle', '--');
        plot([0 0 1 1], [0 1 0 1], 'y+');
        
        subplot 133, hold off, imagesc(imgNew), axis image, hold on
        Corners = [1 1;
            1 nrows;
            ncols 1;
            ncols nrows];
        plot(Corners(:,1), Corners(:,2), 'y+');
        rectangle('Pos', [(squareWidth_pix+padding_pix)*[1 1] (nrows-2*squareWidth_pix-2*padding_pix)*[1 1]], 'EdgeColor', 'r', 'LineStyle', '--');
    end
    
    colormap(gray)
end

end