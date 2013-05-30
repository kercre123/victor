function sampleNegativePatches(varargin)

negativeImageDir = '/Users/andrew/Code/blockIdentificationData/negativeImages';
negativePatchDir = '/Users/andrew/Code/blockIdentificationData/negativePatches';
numScales = 5;
scaleFactor = 1.5;
winSize = 13;
sampleFraction = 0.2;

parseVarargin(varargin{:});

files = getfnames(negativeImageDir, 'images');

C = onCleanup(@()multiWaitbar('CloseAll'));

numFiles = length(files);
multiWaitbar('File', 0)
for i_file = 1:numFiles
    
    img = imread(fullfile(negativeImageDir, files{i_file}));
    
    [~,prefix] = fileparts(fname);
    
    for i_scale = 1:numScales
        if i_scale > 1
            img = imresize(img, 1/scaleFactor, 'lanczos3');
        end
        
        blockproc(img, [winSize winSize], ...
            @(blockStruct)saveBlock(prefix, negativePatchDir, ...
            blockStruct, i_scale, sampleFraction));
    end
        
    if multiWaitbar('File', 'Increment', 1/numFiles)
        disp('User cancelled')
        break;
    end
end

end


function saveBlock(prefix, outDir, blockStruct, scale, sampleFraction)

if rand <= sampleFraction
    
    outFile = fullfile(outDir, sprintf('%s_scale%d_%d-%d.png', ...
        prefix, scale, blockStruct.location(1), blockStruct.location(2)));
    
    imwrite(blockStruct.data, outFile);
    
end
        
    
end
