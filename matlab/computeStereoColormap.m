% function colors = computeStereoColormap(numDisparities)

% colors = computeStereoColormap(128);
% colors = colors(:,3:-1:1); % For matlab


function colors = computeStereoColormap(numDisparities)
    % y = r + g
    % cyan = g + b
    
    rgbPoints = [0,0,255; 0,255,255; 0,255,0; 255,255,0; 255,0,0];
    
    %% Normal RGB interpolation
    colors = [ ...
        uint8([linspace(rgbPoints(1,1),rgbPoints(2,1),25); linspace(rgbPoints(1,2),rgbPoints(2,2),25); linspace(rgbPoints(1,3),rgbPoints(2,3),25)]');
        uint8([linspace(rgbPoints(2,1),rgbPoints(3,1),25); linspace(rgbPoints(2,2),rgbPoints(3,2),25); linspace(rgbPoints(2,3),rgbPoints(3,3),25)]');
        uint8([linspace(rgbPoints(3,1),rgbPoints(4,1),25); linspace(rgbPoints(3,2),rgbPoints(4,2),25); linspace(rgbPoints(3,3),rgbPoints(4,3),25)]');
        uint8([linspace(rgbPoints(4,1),rgbPoints(5,1),25); linspace(rgbPoints(4,2),rgbPoints(5,2),25); linspace(rgbPoints(4,3),rgbPoints(5,3),25)]')];
    
    colors = permute(colors, [3,1,2]);
    colors = imresize(colors, [1,numDisparities], 'bilinear');
    colors = squeeze(colors);
    colors = colors(end:-1:1,:);
    colors(1,:) = 0;
    
    % colorsToShow = permute(colors, [3,1,2]);
    % colorsToShow = repmat(colorsToShow, [100,1,1]);
    % imshow(colorsToShow)
    
    return
    
    %% Quadratic RGB interpolation
    numAlphas = 25;
    
    numRgbPoints = size(rgbPoints,1);
    
    alphaExponents = [0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 3.0, 4.0]; 
    
    alphasRaw = linspace(0, 1, numAlphas);
    
    alphaImages = {};
    
    for iAlpha = 1:length(alphaExponents)
        alphas = alphasRaw .^ alphaExponents(iAlpha);
        colors2 = zeros((numRgbPoints-1)*numAlphas, 3);
        for i = 1:(numRgbPoints-1)
            channels = zeros(1, numAlphas, 3);
            for c = 1:3
                channels(:,:,c) = rgbPoints(i+1,c) * alphas + rgbPoints(i,c) * (1-alphas);
            end

            startIndex = ((i-1)*numAlphas + 1);
            endIndex = (i*numAlphas);
            colors2(startIndex:endIndex, :) = channels;
        end % for i = 1:size(labPoints,1)

        colors2ToShow = uint8(permute(colors2, [3,1,2]));
        colors2ToShow = repmat(colors2ToShow, [100,1,1]);
        
        alphaImages{end+1} = colors2ToShow;
    end
    
    imshows(alphaImages)
    %imshows({colorsToShow, 0, colors2ToShow})
    
    keyboard
    
    %% LAB interpolation
    numAlphas = 25;
    labPoints = rgb2lab(rgbPoints);
    numLabPoints = size(labPoints,1);
    alphas = linspace(0, 1, numAlphas);
    
    labColors = zeros(1, (numLabPoints-1)*numAlphas, 3);
    for i = 1:(numLabPoints-1)
        channels = zeros(1, numAlphas, 3);
        for c = 1:3
            channels(:,:,c) = labPoints(i+1,c) * alphas + labPoints(i,c) * (1-alphas);
        end
        
        startIndex = ((i-1)*numAlphas + 1);
        endIndex = (i*numAlphas);
        labColors(1, startIndex:endIndex, :) = channels;
    end % for i = 1:size(labPoints,1)
    
    labPointsToShow = im2uint8(lab2rgb(labColors));
    labPointsToShow = repmat(labPointsToShow, [100,1,1]);
    
    
    
    
    imshows({colorsToShow,0,labPointsToShow})
    
    keyboard
    
    
%     colorsLab = rgb2lab(colors);
%     
%     f = [1,51]; 
%     f = f / sum(f);
%     colorsBlurred = imfilter(colorsLab, f, 'replicate');
%     colorsBlurred = imfilter(colorsBlurred, f, 'replicate');
%     colorsBlurred = imfilter(colorsBlurred, f, 'replicate');
%     colorsBlurred = imfilter(colorsBlurred, f, 'replicate');
%     colorsBlurred = imfilter(colorsBlurred, f, 'replicate');
%     colorsBlurred = imfilter(colorsBlurred, f, 'replicate');
%     
%     colorsBlurred = lab2rgb(colorsBlurred);
%     
%     imshows({repmat(colors, [100,1,1]), 0, repmat(colorsBlurred, [100,1,1])})
    
    keyboard
    
function rgb = lab2rgb(im)
    cform = makecform('lab2srgb');
    
    if strcmp(class(im),'uint8')
        im = double(im)/255;
    end
    
    rgb = applycform(im, cform);
    
    