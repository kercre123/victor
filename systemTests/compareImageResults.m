% function compareImageResults(filenamePatterns)

% compareImageResults({'/Users/pbarnum/Documents/Anki/products-cozmo-large-files/systemTestsData/results/dateTime_2014-07-24_16-29-32/images/1/1/*.png', '/Users/pbarnum/Documents/Anki/products-cozmo-large-files/systemTestsData/results/dateTime_2014-07-25_11-29-33/images/1/1/*.png'});

function compareImageResults(filenamePatterns)
    assert(iscell(filenamePatterns))
    
    filenames = cell(length(filenamePatterns), 1);
    
    for iPattern = 1:length(filenamePatterns)
        curFilenames = rdir(filenamePatterns{iPattern});
        
        filenames{iPattern} = cell(length(curFilenames), 1);
        for iFile = 1:length(curFilenames)
            filenames{iPattern}{iFile} = curFilenames(iFile).name;
        end
    end
    
    numCols = ceil(sqrt(length(filenames)));
    numRows = ceil(length(filenames) / numCols);
    
    iFile = 1;
    
    while true
        slashIndex1 = strfind(filenames{1}{iFile}, '/');
    
        for iPattern = 2:length(filenames)
            slashIndexN = strfind(filenames{iPattern}{iFile}, '/');
            if ~strcmp(filenames{1}{iFile}((slashIndex1(end)+1):end), filenames{iPattern}{iFile}((slashIndexN(end)+1):end))
                disp('File names don''t match');
                keyboard
            end
        end
        
        images = cell(length(filenames), 1);
        for iPattern = 1:length(filenames)
            images{iPattern} = imread(filenames{iPattern}{iFile});
        end
        
        for iPattern = 1:length(filenames)
            figure(200);
            subplot(numRows, numCols, iPattern);
            imshow(images{iPattern});
            slashIndex = find(filenames{iPattern}{iFile} == '/');
            title(filenames{iPattern}{iFile}((slashIndex(end)+1):end))
        end
        [~,~,c] = ginput(1);
        
        if c == int32('a')
            iFile = iFile - 1;
        elseif c == int32('s')
            iFile = iFile + 1;
        elseif c == int32('d')
            iFile = iFile - 10;
        elseif c == int32('f')
            iFile = iFile + 10;
        elseif c == int32('g')
            iFile = iFile - 100;
        elseif c == int32('h')
            iFile = iFile + 100;
        elseif c == int32('q')
            break;
        end
        
        if iFile < 1
            iFile = length(filenames{1});
        elseif iFile > length(filenames{1})
            iFile = 1;
        end
    end % while true
    
    keyboard