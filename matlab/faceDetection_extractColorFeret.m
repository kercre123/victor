
% function faceDetection_extractColorFeret(baseDirectory)

% faceDetection_extractColorFeret('/Volumes/FastExternal_Mac/colorferet/')

function faceDetection_extractColorFeret(baseDirectory)
   
    files = dir(baseDirectory);
    
    % First, extract bz2 (and delete bz2)
    for iFile = 3:length(files)
        if length(files(iFile).name) < 3
            continue;
        end
        
        curFilename = [baseDirectory, '/', files(iFile).name];
        
        if strcmp(curFilename((end-3):end), '.bz2')
            disp(['Extracting ', curFilename]);
            result = system(['bunzip2 ', curFilename]);
            if result ~= 0
                disp('Error');
                keyboard
%             else
%                 result = system(['rm ', curFilename]);
%                 if result ~= 0
%                     disp('Error');
%                     keyboard
%                 end
            end
        elseif files(iFile).isdir
            faceDetection_extractColorFeret(curFilename);
        end
    end
    
    files = dir(baseDirectory);
    
    % Second
    % 1. Convert ppm to png (and delete ppm)
    % 2. Convert xml to mat (don't delete xml)
    for iFile = 3:length(files)
        if length(files(iFile).name) < 3
            continue;
        end
        
        curFilename = [baseDirectory, '/', files(iFile).name];
        
        if strcmp(curFilename((end-3):end), '.ppm')
            disp(['Resaving ', curFilename]);
            
            filenamePng = [curFilename(1:(end-3)), 'png'];

            im = imread(curFilename);
            imwrite(im, filenamePng);

            delete(curFilename);
            
%             keyboard

%         elseif strcmp(curFilename((end-3):end), '.xml')
        elseif ~isempty(strfind(curFilename, 'name_value')) && strcmp(curFilename((end-3):end), '.txt')
            % Convert text file to json and mat
            % TODO: Matlab crashes when trying to load the xml files?
            
            jsonFilename = [curFilename(1:(end-4)), '.json'];
            matFilename = [curFilename(1:(end-4)), '.mat'];
            
            if ~exist(jsonFilename, 'file') || ~exist(matFilename, 'file')
                disp(['Parsing ', curFilename])
                
                textLines = textread(curFilename, '%s', 'whitespace', '\n\r');

                data = struct();
                for iLine = 1:length(textLines)
                    textLines{iLine} = strsplit(textLines{iLine}, '=');
                    textLines{iLine}{2} = strsplit(textLines{iLine}{2}, ' ');

                    if ~isempty(strfind(textLines{iLine}{1}, 'coordinates'))
                        for iCoord = 1:length(textLines{iLine}{2})
                            textLines{iLine}{2}{iCoord} = str2num(textLines{iLine}{2}{iCoord});
                        end
                        textLines{iLine}{2} = cell2mat(textLines{iLine}{2});
                    end

                    data = setfield(data, textLines{iLine}{1}, textLines{iLine}{2});
                end

                savejson('', data, jsonFilename);
                save(matFilename, 'data');
            end % if ~exist(jsonFilename, 'file') || &exist(matFilename, 'file')
            
%             keyboard
        end
    end
    
    
    