% function files = listFilesFromSubdirectories(baseDirectory)

% files = listFilesFromSubdirectories('C:\datasets\faces\lfw');

function files = listFilesFromSubdirectories(baseDirectory, outFilename)

baseDirectory = strrep(baseDirectory, '\', '/');

topList = dir(baseDirectory);
topList = topList(3:end);

files = cell(0,0);

for iFile = 1:length(topList)
    if topList(iFile).isdir
        subFiles = listFilesFromSubdirectories([baseDirectory, '/', topList(iFile).name]);
        
        for i = 1:length(subFiles)
            files{(end+1)} = subFiles{i};
        end        
    else
        files{end+1} = [baseDirectory, '/', topList(iFile).name]; %#ok<*AGROW>
    end
end

if exist('outFilename', 'var') && ~isempty(outFilename)
    fileId = fopen(outFilename, 'w');
    
    for iFile = 1:length(files)
        fprintf(fileId, '%s\n', files{iFile});
    end

    fclose(fileId);
end