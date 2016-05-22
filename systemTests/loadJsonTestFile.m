
function data = loadJsonTestFile(testFilename)
    testFilename = strrep(strrep(testFilename, '\', '/'), '//', '/');
    matFilename = strrep(testFilename, '.json', '.mat');
    
    doConversion = false;
    
    % If the results don't exist
    if ~exist(matFilename, 'file')
        doConversion = true;
    else
        modificationTime_mat = dir(matFilename);
        modificationTime_mat = modificationTime_mat(1).datenum;
        
        modificationTime_json = dir(testFilename);
        modificationTime_json = modificationTime_json(1).datenum;
        
        if modificationTime_mat < modificationTime_json
            doConversion = true;
        end
    end
    
    if doConversion
        jsonData = loadjson(testFilename);
        save(matFilename, 'jsonData');
    else
        load(matFilename);
    end
    
    if ~exist('modificationTime_json', 'var')
        modificationTime_json = dir(testFilename);
        modificationTime_json = modificationTime_json(1).datenum;
    end
    
    slashIndexes = strfind(testFilename, '/');
    lastSlashIndex = slashIndexes(end);
    testPath = testFilename(1:lastSlashIndex);
    testFilename = testFilename((lastSlashIndex+1):end);
    
    jsonData = sanitizeJsonTest(jsonData);
    
    % Assign to the structure fields
    data.jsonData = jsonData;
    data.testFilename = testFilename;
    data.testPath = testPath;
    data.testFileModificationTime = modificationTime_json;
    