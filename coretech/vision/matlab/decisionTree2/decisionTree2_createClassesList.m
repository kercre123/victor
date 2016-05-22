% function classesList = decisionTree2_createClassesList(varargin)

% Generate a classesList, with one class for each fiducial code
% type

% Simple Example:
% classesList = decisionTree2_createClassesList('markerFilenamePatterns', {'Z:/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/rotated/*.png'});

% Example:
% classesList = decisionTree2_createClassesList();

function classesList = decisionTree2_createClassesList(varargin)
    %#ok<*CCAT1>
    %#ok<*CCAT>
    
    % TODO: add nonMarkers
    %     nonMarkerFilenamePatterns = {};
    
    markerFilenamePatterns = {...
        'Z:/Box Sync/Cozmo SE/VisionMarkers/letters/withFiducials/rotated/*.png',...
        'Z:/Box Sync/Cozmo SE/VisionMarkers/symbols/withFiducials/rotated/*.png',...
        'Z:/Box Sync/Cozmo SE/VisionMarkers/dice/withFiducials/rotated/*.png',...
        'Z:/Box Sync/Cozmo SE/VisionMarkers/ankiLogoMat/unpadded/rotated/*.png'};
    
    parseVarargin(varargin{:});
    
    % Find all the names of fiducial images
    files = {};
    for iPattern = 1:length(markerFilenamePatterns)
        markerFilenamePatterns{iPattern} = strrep(markerFilenamePatterns{iPattern}, '\', '/');
        markerFilenamePatterns{iPattern} = strrep(markerFilenamePatterns{iPattern}, '//', '/');
        
        if ispc()
            markerFilenamePatterns{iPattern} = strrep(markerFilenamePatterns{iPattern}, '~/', 'Z:/');
        else
            markerFilenamePatterns{iPattern} = strrep(markerFilenamePatterns{iPattern}, 'Z:/', '~/');
            markerFilenamePatterns{iPattern} = strrep(markerFilenamePatterns{iPattern}, 'z:/', '~/');
        end
        
        markerFilenamePatterns{iPattern} = strrep(markerFilenamePatterns{iPattern}, '~/', [tildeToPath(),'/']);
        
        newFiles = rdir(markerFilenamePatterns{iPattern}, [], true);
        files = {files{:}, newFiles{:}};
    end
    
    % Convert each cell to format {className, filename}
    for i = 1:length(files)
        slashInds = find(files{i} == '/');
        dotInds = find(files{i} == '.');
        
        classesList(i).labelName = files{i}((slashInds(end)+1):(dotInds(end)-1)); %#ok<AGROW>
        classesList(i).filenames = {files{i}};  %#ok<AGROW>
    end
    
    