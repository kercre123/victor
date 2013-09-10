function makeMex(varargin)

mexFiles = getfnames('.', '*.cpp');
numFiles = length(mexFiles);
for i = 1:numFiles
    try
        fprintf('Building %d of %d "%s"...\n', i, numFiles, mexFiles{i});
        compileAnkiMex(mexFiles{i}, varargin{:});
    catch E
       warning('Failed to build "%s": %s', mexFiles{i}, E.message); 
    end
end

end
