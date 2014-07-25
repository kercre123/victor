if ~exist('markerQuads', 'var') || ~exist('nonMarkerQuads', 'var')
    load /Users/andrew/Downloads/latestNonMarkersResults_samplesOnly.mat
end

negativeImageDir = '~/temp/negPatches';
outputSize = 256;
chunkSize = 1000; % number per directory

if ~isdir(negativeImageDir)
    mkdir(negativeImageDir);
end

pBar = ProgressBar('Negative Patches', 'CancelButton', true);
pBar.showTimingInfo = true;
pBarCleanup = onCleanup(@()delete(pBar));
pBar.set_increment(1/(length(nonMarkerQuads) + length(markerQuads)));

chunkCtr = 1;

for i = 1:length(nonMarkerQuads)
    if mod(i, chunkSize) == 1
        chunkDir = sprintf('chunk%.4d', chunkCtr);
        if ~isdir(fullfile(negativeImageDir, chunkDir))
            mkdir(fullfile(negativeImageDir, chunkDir));
        end
        chunkCtr = chunkCtr + 1;
    end
    
    fname = fullfile(negativeImageDir, chunkDir, sprintf('nonMarkerQuads_%.6d.png', i));
    threshold = (min(nonMarkerQuads{i}(:))+max(nonMarkerQuads{i}(:)))/2;
    imwrite(imresize(double(nonMarkerQuads{i} > threshold), outputSize*[1 1]), fname);
    
    pBar.increment();
    
    if pBar.cancelled
        disp('User cancelled.');
        return;
    end
end

for i = 1:length(markerQuads)
    if mod(i, chunkSize) == 1
        chunkDir = sprintf('chunk%.4d', chunkCtr);
        if ~isdir(fullfile(negativeImageDir, chunkDir))
            mkdir(fullfile(negativeImageDir, chunkDir));
        end
        chunkCtr = chunkCtr + 1;
    end
    
    fname = fullfile(negativeImageDir, sprintf('markerQuads_%.6d.png', i));
    threshold = (min(markerQuads{i}(:))+max(markerQuads{i}(:)))/2;
    imwrite(imresize(double(markerQuads{i} > threshold), outputSize*[1 1]), fname);
    pBar.increment();
    
    if pBar.cancelled
        disp('User cancelled.');
        return;
    end
end