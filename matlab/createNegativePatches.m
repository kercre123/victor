function createNegativePatches(fname, varargin)

numScales = 3; % starting at half scale
percentiles = [.25 .5 .75];
outputDir = '~/temp/negPatches';
resizeInput = [1024 NaN];
fractionOnThreshold = 0.1;

parseVarargin(varargin{:});

if isdir(fname)
    fnames = getfnames(fname, 'images', 'useFullPath', true);
    for iFile = 1:length(fnames)
        fprintf('File %d of %d: %s\n', iFile, length(fnames), fnames{iFile});
        createNegativePatches(fnames{iFile}, varargin{:});
    end
    return;
end

img = imread(fname);
if size(img,3) > 1
    img = rgb2gray(img);
end

img = imresize(img, resizeInput, 'bilinear');

[~,savePrefix] = fileparts(fname);
savePrefix = fullfile(outputDir, savePrefix);
if ~isdir(outputDir)
    mkdir(outputDir)
end

scales = 2.^(-(1:numScales)) * min(size(img));
for scale = scales
    blockproc(img, scale*[1 1], @(blk)saveHelper(blk,percentiles,savePrefix, fractionOnThreshold));
end


end

function saveHelper(blk, percentiles, savePrefix, binaryFraction)

% saveName = sprintf('%s_%d_%d.png', savePrefix, blk.location(1), blk.location(2));
% imwrite(blk.data, saveName);

sortedData = sort(blk.data(:), 'ascend');
N = length(sortedData);

for i = 1:length(percentiles)
    threshold = sortedData(max(1,min(N,round(percentiles(i)*N))));
    saveName = sprintf('%s_T%d_%d_%d.png', savePrefix, threshold, blk.location(1), blk.location(2)); 
    patch = blk.data > threshold;
    
    fractionOn = sum(patch(:))/numel(patch);
    if fractionOn >= binaryFraction && fractionOn <= 1-binaryFraction
        imwrite(blk.data > threshold, saveName);
    end
end

end
