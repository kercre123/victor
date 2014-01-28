% function labelGroundTruth(testFilename)

% labelGroundTruth('C:/Anki/systemTestImages/test_1.json');

function labelGroundTruth(testFilename)

jsonData = loadjson(testFilename);

disp(sprintf('test file has %d sequences', length(jsonData.sequences)));

for iSequence = 1:length(jsonData.sequences)
    curSequence = jsonData.sequences{iSequence};
    
    curSequence.frameNumbers
    disp(sprintf('filenamePattern is %s', curSequence.filenamePattern));
end