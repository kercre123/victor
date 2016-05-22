% function varargout = editGroundTruth(varargin)

% editGroundTruth();

%#ok<*DEFNU>

function varargout = editGroundTruth(varargin)
    
    % Begin initialization code - DO NOT EDIT
    gui_Singleton = 1;
    gui_State = struct('gui_Name',       mfilename, ...
        'gui_Singleton',  gui_Singleton, ...
        'gui_OpeningFcn', @editGroundTruth_OpeningFcn, ...
        'gui_OutputFcn',  @editGroundTruth_OutputFcn, ...
        'gui_LayoutFcn',  [] , ...
        'gui_Callback',   []);
    if nargin && ischar(varargin{1})
        gui_State.gui_Callback = str2func(varargin{1});
    end
    
    if nargout
        [varargout{1:nargout}] = gui_mainfcn(gui_State, varargin{:});
    else
        gui_mainfcn(gui_State, varargin{:});
    end
    % End initialization code - DO NOT EDIT
    
function editGroundTruth_OpeningFcn(hObject, ~, handles, varargin)
    global curDisplayType;
    global maxDisplayType;
    global curMarkerIndex;
    global savedDisplayParameters;
    global displayParameters;
    global resolutionHorizontal;
    global resolutionVertical;
    global curPoseIndex;
    global allHandles;
    global markerClipboard;
    global pointsType;
    global setFigurePosition;
    global lockResolution;
    
    curPoseIndex = 1;
    curMarkerIndex = 1;
    curDisplayType = 1;
    maxDisplayType = 2;
    displayParameters = cell(maxDisplayType,1);
    savedDisplayParameters = cell(maxDisplayType,1);
    savedDisplayParameters{1} = zeros(4,1);
    savedDisplayParameters{2} = [0.1, 0.1, 0, 0];
    resolutionHorizontal = 320;
    resolutionVertical = 240;
    allHandles = handles;
    markerClipboard = [];
    pointsType = 'fiducialMarker';
    setFigurePosition = false;
    lockResolution = false;
    
    setFromSavedDisplayParameters();
    
    figure(100);
    
    % Choose default command line output for editGroundTruth
    handles.output = hObject;
    
    % Update handles structure
    guidata(hObject, handles);
    
    % UIWAIT makes editGroundTruth wait for user response (see UIRESUME)
    % uiwait(handles.figure1);
    
    loadTestFile()
    
function varargout = editGroundTruth_OutputFcn(~, ~, handles)
    % Get default command line output from handles structure
    varargout{1} = handles.output;
    
    %
    % Create Functions
    %
    
function pose_current_CreateFcn(hObject, ~, ~)
    global curPoseIndex;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(curPoseIndex));
    
function test_current_CreateFcn(hObject, ~, ~)
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(1));
    
function marker_current_CreateFcn(hObject, ~, ~)
    global curMarkerIndex;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(curMarkerIndex));
    
function test_numMarkersWithContext_CreateFcn(hObject, ~, ~)
    setDefaultGuiObjectColor(hObject);
    
function test_numPartialMarkersWithContext_CreateFcn(hObject, ~, ~)
    setDefaultGuiObjectColor(hObject);
    
function displayType_current_CreateFcn(hObject, ~, ~)
    global curDisplayType;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(curDisplayType));
    
function pose_max_CreateFcn(hObject, ~, ~)
    set(hObject,'String',num2str(1));
    
function test_max_CreateFcn(hObject, ~, ~)
    set(hObject,'String',num2str(1));
    
function marker_max_CreateFcn(~, ~, ~)
    
function displayType_max_CreateFcn(hObject, ~, ~)
    global maxDisplayType;
    set(hObject,'String',num2str(maxDisplayType));
    
function testJsonFilename1_CreateFcn(hObject, ~, ~)
    setDefaultGuiObjectColor(hObject);
    
function testJsonFilename2_CreateFcn(hObject, ~, ~)
    setDefaultGuiObjectColor(hObject);
    
function figure1_CreateFcn(~, ~, ~)
    
function pose_previous1_CreateFcn(~, ~, ~)
    
function pose_next1_CreateFcn(~, ~, ~)
    
function configFilenameNoteText_CreateFcn(~, ~, ~)
    
function pose_filename_CreateFcn(~, ~, ~)
    
function displayType_parameter1_CreateFcn(hObject, ~, ~)
    global displayParameters;
    setDefaultGuiObjectColor(hObject);
    
    if ~isempty(displayParameters)
        set(hObject,'String',num2str(displayParameters{1}));
    end
    
function displayType_parameter2_CreateFcn(hObject, ~, ~)
    global displayParameters;
    setDefaultGuiObjectColor(hObject);
    
    if ~isempty(displayParameters)
        set(hObject,'String',num2str(displayParameters{2}));
    end
    
function displayType_parameter3_CreateFcn(hObject, ~, ~)
    global displayParameters;
    setDefaultGuiObjectColor(hObject);
    if ~isempty(displayParameters)
        set(hObject,'String',num2str(displayParameters{3}));
    end
    
function displayType_parameter4_CreateFcn(hObject, ~, ~)
    global displayParameters;
    setDefaultGuiObjectColor(hObject);
    if ~isempty(displayParameters)
        set(hObject,'String',num2str(displayParameters{4}));
    end
    
function resolutionHorizontal_CreateFcn(hObject, ~, ~)
    global resolutionHorizontal;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(resolutionHorizontal));
    
function resolutionVertical_CreateFcn(hObject, ~, ~)
    global resolutionVertical;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(resolutionVertical));
    
    %
    % End Create Functions
    %
    
    %
    % Callback Functions
    %
    
function testJsonFilename1_Callback(~, ~, ~)
    loadTestFile()
    
function testJsonFilename2_Callback(~, ~, ~)
    loadTestFile()
    
function test_numMarkersWithContext_Callback(hObject, ~, ~)
    global jsonTestData;
    
    NumMarkersWithContext_previous = jsonTestData.NumMarkersWithContext;
    
    jsonTestData.NumMarkersWithContext = str2double(get(hObject,'String'));
    
    if NumMarkersWithContext_previous ~= jsonTestData.NumMarkersWithContext
        Save();
    end
    
function test_numPartialMarkersWithContext_Callback(hObject, ~, ~)
    global jsonTestData;
    
    NumPartialMarkersWithContext_previous = jsonTestData.NumPartialMarkersWithContext;
    
    jsonTestData.NumPartialMarkersWithContext = str2double(get(hObject,'String'));
    
    if NumPartialMarkersWithContext_previous ~= jsonTestData.NumPartialMarkersWithContext
        Save();
    end
    
function test_previous1_Callback(~, ~, ~)
    global allHandles;
    [allTestNames, newIndex] = getTestNamesFromDirectory();
    newIndex = max(1, min(length(allTestNames), newIndex-1));
    set(allHandles.testJsonFilename2, 'String', allTestNames{newIndex});
    loadTestFile()
    
function test_next1_Callback(~, ~, ~)
    global allHandles;
    [allTestNames, newIndex] = getTestNamesFromDirectory();
    newIndex = max(1, min(length(allTestNames), newIndex+1));
    set(allHandles.testJsonFilename2, 'String', allTestNames{newIndex});
    loadTestFile()
    
function test_previous2_Callback(~, ~, ~)
    global allHandles;
    [allTestNames, newIndex] = getTestNamesFromDirectory();
    newIndex = max(1, min(length(allTestNames), newIndex-10));
    set(allHandles.testJsonFilename2, 'String', allTestNames{newIndex});
    loadTestFile()
    
function test_next2_Callback(~, ~, ~)
    global allHandles;
    [allTestNames, newIndex] = getTestNamesFromDirectory();
    newIndex = max(1, min(length(allTestNames), newIndex+10));
    set(allHandles.testJsonFilename2, 'String', allTestNames{newIndex});
    loadTestFile()
    
function test_previous3_Callback(~, ~, ~)
    global allHandles;
    [allTestNames, newIndex] = getTestNamesFromDirectory();
    newIndex = max(1, min(length(allTestNames), newIndex-100));
    set(allHandles.testJsonFilename2, 'String', allTestNames{newIndex});
    loadTestFile()
    
function test_next3_Callback(~, ~, ~)
    global allHandles;
    [allTestNames, newIndex] = getTestNamesFromDirectory();
    newIndex = max(1, min(length(allTestNames), newIndex+100));
    set(allHandles.testJsonFilename2, 'String', allTestNames{newIndex});
    loadTestFile()
    
function pose_previous1_Callback(~, ~, ~)
    global curPoseIndex;
    curPoseIndex = curPoseIndex - 1;
    fixBounds();
    poseChanged(true);
    
function pose_previous2_Callback(~, ~, ~)
    global curPoseIndex;
    curPoseIndex = curPoseIndex - 5;
    fixBounds();
    poseChanged(true);
    
function pose_previous3_Callback(~, ~, ~)
    global curPoseIndex;
    curPoseIndex = curPoseIndex - 100;
    fixBounds();
    poseChanged(true);
    
function pose_next1_Callback(~, ~, ~)
    global curPoseIndex;
    curPoseIndex = curPoseIndex + 1;
    fixBounds();
    poseChanged(true);
    
function pose_next2_Callback(~, ~, ~)
    global curPoseIndex;
    curPoseIndex = curPoseIndex + 5;
    fixBounds();
    poseChanged(true);
    
function pose_next3_Callback(~, ~, ~)
    global curPoseIndex;
    curPoseIndex = curPoseIndex + 100;
    fixBounds();
    poseChanged(true);
    
function pose_current_Callback(hObject, ~, ~)
    global curPoseIndex;
    curPoseIndex = str2double(get(hObject,'String'));
    fixBounds();
    poseChanged(true);
    
function test_current_Callback(hObject, ~, ~)
    global allHandles;
    [allTestNames, ~] = getTestNamesFromDirectory();
    newIndex = str2double(get(hObject,'String'));
    newIndex = max(1, min(length(allTestNames), newIndex));
    set(allHandles.testJsonFilename2, 'String', allTestNames{newIndex});
    loadTestFile()
    
function marker_next1_Callback(~, ~, ~)
    global curMarkerIndex;
    curMarkerIndex = curMarkerIndex + 1;
    fixBounds();
    poseChanged(false);
    
function marker_next2_Callback(~, ~, ~)
    global curMarkerIndex;
    curMarkerIndex = curMarkerIndex + 5;
    fixBounds();
    poseChanged(false);
    
function marker_next3_Callback(~, ~, ~)
    global curMarkerIndex;
    curMarkerIndex = curMarkerIndex + 100;
    fixBounds();
    poseChanged(false);
    
function marker_previous1_Callback(~, ~, ~)
    global curMarkerIndex;
    curMarkerIndex = curMarkerIndex - 1;
    fixBounds();
    poseChanged(false);
    
function marker_previous2_Callback(~, ~, ~)
    global curMarkerIndex;
    curMarkerIndex = curMarkerIndex - 5;
    fixBounds();
    poseChanged(false);
    
function marker_previous3_Callback(~, ~, ~)
    global curMarkerIndex;
    curMarkerIndex = curMarkerIndex - 100;
    fixBounds();
    poseChanged(false);
    
function marker_current_Callback(hObject, ~, ~)
    global curMarkerIndex;
    curMarkerIndex = str2double(get(hObject,'String'));
    fixBounds();
    poseChanged(false);
    
function marker_rotate_Callback(~, ~, ~)
    global curPoseIndex;
    global curMarkerIndex;
    rotateMarker(curPoseIndex, curMarkerIndex);
    fixBounds();
    poseChanged(false);
    
function marker_copy_Callback(~, ~, ~)
    global jsonTestData;
    global curPoseIndex;
    global markerClipboard;
    
    fixBounds();
    
    markerClipboard.NumMarkers = jsonTestData.Poses{curPoseIndex}.NumMarkers;
    markerClipboard.VisionMarkers = jsonTestData.Poses{curPoseIndex}.VisionMarkers;
    
function marker_paste_Callback(~, ~, ~)
    global jsonTestData;
    global curPoseIndex;
    global markerClipboard;
    
    fixBounds();
    
    if ~isempty(markerClipboard)
        jsonTestData.Poses{curPoseIndex}.NumMarkers = markerClipboard.NumMarkers;
        jsonTestData.Poses{curPoseIndex}.VisionMarkers = markerClipboard.VisionMarkers;
        
        Save();
        
        poseChanged(true);
    end
    
function displayType_previous_Callback(~, ~, ~)
    global curDisplayType;
    setToSavedDisplayParameters();
    curDisplayType = curDisplayType - 1;
    fixBounds();
    setFromSavedDisplayParameters();
    poseChanged(false);
    
function displayType_next_Callback(~, ~, ~)
    global curDisplayType;
    setToSavedDisplayParameters();
    curDisplayType = curDisplayType + 1;
    fixBounds();
    setFromSavedDisplayParameters();
    poseChanged(false);
    
function displayType_current_Callback(hObject, ~, ~)
    global curDisplayType;
    setToSavedDisplayParameters();
    curDisplayType = str2double(get(hObject,'String'));
    fixBounds();
    setFromSavedDisplayParameters();
    poseChanged(false);
    
function displayType_parameter1_Callback(hObject, ~, ~)
    global displayParameters;
    displayParameters{1} = str2double(get(hObject,'String'));
    poseChanged(false);
    
function displayType_parameter2_Callback(hObject, ~, ~)
    global displayParameters;
    displayParameters{2} = str2double(get(hObject,'String'));
    poseChanged(false);
    
function displayType_parameter3_Callback(hObject, ~, ~)
    global displayParameters;
    displayParameters{3} = str2double(get(hObject,'String'));
    poseChanged(false);
    
function displayType_parameter4_Callback(hObject, ~, ~)
    global displayParameters;
    displayParameters{4} = str2double(get(hObject,'String'));
    poseChanged(false);
    
function resolutionHorizontal_Callback(hObject, ~, ~)
    global resolutionHorizontal;
    resolutionHorizontal = str2double(get(hObject,'String'));
    poseChanged(true);
    
function resolutionVertical_Callback(hObject, ~, ~)
    global resolutionVertical;
    resolutionVertical = str2double(get(hObject,'String'));
    poseChanged(true);
    
    
    
    
    
    % --- Executes on button press in lockResolution.
function lockResolution_Callback(hObject, ~, ~)
    global lockResolution;
    lockResolution = get(hObject,'Value');
    
function marker_clearAll_Callback(~, ~, ~)
    global jsonTestData;
    global curPoseIndex;
    
    jsonTestData.Poses{curPoseIndex} = rmfield(jsonTestData.Poses{curPoseIndex}, 'VisionMarkers');
    
    poseChanged(false);
    
function marker_autoDetect_Callback(~, ~, ~)
    detectAndAddMarkers();
    
function markerType_Callback(hObject, ~, ~)
    global jsonTestData;
    global curPoseIndex;
    global curMarkerIndex;
    
    curName = get(hObject,'String');
    
    if length(curName) < 8 ||...
            ~strcmp(curName(1:8), 'MARKER_')
        
        curName = ['MARKER_', curName];
    end
    
    markerType_previous = jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex}.markerType;
    
    jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex}.markerType = curName;
    
    if ~strcmp(markerType_previous, jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex}.markerType)
        Save();
        poseChanged(false);
    end
    
function menu_label_Callback(~, ~, ~)
    
function menu_labelAllImages_Callback(~, ~, ~)
    
function label_eraseAndLabel_Callback(~, ~, ~)
    global jsonTestData;
    global curPoseIndex;
    
    disp('Erasing all labels and auto-labeling')
    
    for iPose = 1:length(jsonTestData.Poses)
        jsonTestData.Poses{iPose} = rmfield(jsonTestData.Poses{iPose}, 'VisionMarkers');
    end
    
    fixBounds();
    
    for iPose = 1:length(jsonTestData.Poses)
        curPoseIndex = iPose;
        
        poseChanged(true);
        
        detectAndAddMarkers();
        
        curImageFilename = jsonTestData.Poses{iPose}.ImageFile;
        slashIndexes = strfind(curImageFilename, '/');
        curImageFilenameWithoutPath = curImageFilename((slashIndexes(end)+1):end);
        
        disp(sprintf('%d/%d "%s" detected %d markers', iPose, length(jsonTestData.Poses), curImageFilenameWithoutPath, length(jsonTestData.Poses{iPose}.VisionMarkers)));
    end
    
    poseChanged(true);
    
    Save();
    
function menu_replicateLabels_Callback(~, ~, ~)
    
function menu_eraseAndReplicate_Callback(~, ~, ~)
    global jsonTestData;
    global curPoseIndex;
    
    disp('Copying the markers and labels on the current pose to all poses')
    
    fixBounds();
    
    curPose = jsonTestData.Poses{curPoseIndex};
    
    for iPose = 1:length(jsonTestData.Poses)
        jsonTestData.Poses{iPose}.NumMarkers = curPose.NumMarkers;
        jsonTestData.Poses{iPose}.VisionMarkers = curPose.VisionMarkers;
    end
    
    poseChanged(true);
    
    Save();
    
function menu_eraseAll_Callback(~, ~, ~)
    
function menu_eraseAllLabels_Callback(~, ~, ~)
    global jsonTestData;
    %     global curPoseIndex;
    
    disp('Erasing all labels')
    
    for iPose = 1:length(jsonTestData.Poses)
        jsonTestData.Poses{iPose} = rmfield(jsonTestData.Poses{iPose}, 'VisionMarkers');
    end
    
    fixBounds();
    
    poseChanged(true);
    
    Save();
    
function menu_cache_Callback(~, ~, ~)
    
function menu_cacheAllTests_Callback(~, ~, ~)
    global allHandles;
    
    testDirectory = strrep(get(allHandles.testJsonFilename1, 'String'), '\', '/');
    
    files = dir([testDirectory,'/*.json']);
    
    for iFile = 1:length(files)
        tic
        loadJsonTestFile([testDirectory, '/', files(iFile).name]);
        disp(sprintf('Cached %d/%d %s in %f seconds', iFile, length(files), files(iFile).name, toc()));
    end
    
function labelingTypePanel_SelectionChangeFcn(~, eventdata, ~)
    global pointsType;
    global allHandles;
    
    if eventdata.NewValue == allHandles.templatePoints
        pointsType = 'template';
        poseChanged(true);
    elseif eventdata.NewValue == allHandles.fiducialMarkerPoints
        pointsType = 'fiducialMarker';
        poseChanged(true);
    elseif eventdata.NewValue == allHandles.noPoints
        pointsType = 'none';
        poseChanged(true);
    else
        assert(false);
    end
    
    % --- If Enable == 'on', executes on mouse press in 5 pixel border.
    % --- Otherwise, executes on mouse press in 5 pixel border or over pose_filename.
function pose_filename_ButtonDownFcn(~, ~, ~)
    global jsonTestData;
    global dataPath;
    global curPoseIndex;
    
    curImageFilename = [dataPath, jsonTestData.Poses{curPoseIndex}.ImageFile];
    curImageFilename = strrep(curImageFilename, '//' , '/');
    clipboard('copy', curImageFilename)
    disp(sprintf('Copied to clipboard: %s', curImageFilename));
    
    %
    % End Callback Functions
    %
    
function [allTestNames, curTestIndex] = getTestNamesFromDirectory()
    global allHandles;
    
    testDirectory = strrep(get(allHandles.testJsonFilename1, 'String'), '\', '/');
    testFilename = strrep(get(allHandles.testJsonFilename2, 'String'), '\', '/');
    
    if length(testFilename) > 5 && ~strcmp(testFilename((end-4):end), '.json')
        testFilename = [testFilename, '.json'];
    end
    
    files = dir([testDirectory,'/*.json']);
    allTestNames = cell(length(files), 1);
    
    curTestIndex = 1;
    for i = 1:length(files)
        %         allTestNames{i} = [testDirectory, '/', files(i).name];
        allTestNames{i} = files(i).name;
        
        if strcmp(testFilename, files(i).name)
            curTestIndex = i;
        end
    end
    
function setDefaultGuiObjectColor(hObject)
    if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
        set(hObject,'BackgroundColor','white');
    end
    
function setToSavedDisplayParameters()
    global curDisplayType;
    global displayParameters;
    global savedDisplayParameters;
    
    for i = 1:4
        savedDisplayParameters{curDisplayType}(i) = displayParameters{i};
    end
    
function setFromSavedDisplayParameters()
    global curDisplayType;
    global displayParameters;
    global savedDisplayParameters;
    
    for i = 1:4
        displayParameters{i} = savedDisplayParameters{curDisplayType}(i);
    end
    
function [cornersX, cornersY, whichCorners] = getFiducialCorners(poseIndex, markerIndex)
    global jsonTestData;
    
    cornersX = [];
    cornersY = [];
    whichCorners = zeros(4,1);
    
    if markerIndex > length(jsonTestData.Poses{poseIndex}.VisionMarkers)
        return;
    end
    
    % If markerIndex is empty, return all corners for all complete quads
    
    if isempty(markerIndex)
        cornersX = zeros(4,0);
        cornersY = zeros(4,0);
        whichCorners = zeros(4,0);
        
        for iMarker = 1:length(jsonTestData.Poses{poseIndex}.VisionMarkers)
            curMarkerData = jsonTestData.Poses{poseIndex}.VisionMarkers{iMarker};
            
            if isfield(curMarkerData, 'x_imgUpperLeft') && isfield(curMarkerData, 'y_imgUpperLeft') &&...
                    isfield(curMarkerData, 'x_imgUpperRight') && isfield(curMarkerData, 'y_imgUpperRight') && ...
                    isfield(curMarkerData, 'x_imgLowerRight') && isfield(curMarkerData, 'y_imgLowerRight') && ...
                    isfield(curMarkerData, 'x_imgLowerLeft') && isfield(curMarkerData, 'y_imgLowerLeft')
                
                newCornersX = zeros(4,1);
                newCornersY = zeros(4,1);
                newWhichCorners = ones(4,1);
                
                newCornersX(1) = curMarkerData.x_imgUpperLeft;
                newCornersY(1) = curMarkerData.y_imgUpperLeft;
                newCornersX(2) = curMarkerData.x_imgUpperRight;
                newCornersY(2) = curMarkerData.y_imgUpperRight;
                newCornersX(3) = curMarkerData.x_imgLowerRight;
                newCornersY(3) = curMarkerData.y_imgLowerRight;
                newCornersX(4) = curMarkerData.x_imgLowerLeft;
                newCornersY(4) = curMarkerData.y_imgLowerLeft;
                
                cornersX(:, end+1) = newCornersX; %#ok<AGROW>
                cornersY(:, end+1) = newCornersY; %#ok<AGROW>
                whichCorners(:, end+1) = newWhichCorners; %#ok<AGROW>
            end
        end % for iMarker = 1:length(jsonTestData.Poses{poseIndex}.VisionMarkers)
    else % if isempty(markerIndex)
        cornersX = [];
        cornersY = [];
        whichCorners = zeros(4,1);
        
        curMarkerData = jsonTestData.Poses{poseIndex}.VisionMarkers{markerIndex};
        
        if isfield(curMarkerData, 'x_imgUpperLeft') && isfield(curMarkerData, 'y_imgUpperLeft')
            cornersX(end+1) = curMarkerData.x_imgUpperLeft;
            cornersY(end+1) = curMarkerData.y_imgUpperLeft;
            whichCorners(1) = 1;
        end
        
        if isfield(curMarkerData, 'x_imgUpperRight') && isfield(curMarkerData, 'y_imgUpperRight')
            cornersX(end+1) = curMarkerData.x_imgUpperRight;
            cornersY(end+1) = curMarkerData.y_imgUpperRight;
            whichCorners(2) = 1;
        end
        
        if isfield(curMarkerData, 'x_imgLowerRight') && isfield(curMarkerData, 'y_imgLowerRight')
            cornersX(end+1) = curMarkerData.x_imgLowerRight;
            cornersY(end+1) = curMarkerData.y_imgLowerRight;
            whichCorners(3) = 1;
        end
        
        if isfield(curMarkerData, 'x_imgLowerLeft') && isfield(curMarkerData, 'y_imgLowerLeft')
            cornersX(end+1) = curMarkerData.x_imgLowerLeft;
            cornersY(end+1) = curMarkerData.y_imgLowerLeft;
            whichCorners(4) = 1;
        end
    end % if isempty(poseIndex) ... else
    
function rotateMarker(poseIndex, markerIndex)
    global jsonTestData;
    
    if markerIndex > length(jsonTestData.Poses{poseIndex}.VisionMarkers)
        return;
    end
    
    curMarkerData = jsonTestData.Poses{poseIndex}.VisionMarkers{markerIndex};
    
    % If the marker has four corners, rotate it 90 degrees
    if isfield(curMarkerData, 'x_imgUpperLeft') && isfield(curMarkerData, 'y_imgUpperLeft') &&...
            isfield(curMarkerData, 'x_imgUpperRight') && isfield(curMarkerData, 'y_imgUpperRight') &&...
            isfield(curMarkerData, 'x_imgLowerRight') && isfield(curMarkerData, 'y_imgLowerRight') &&...
            isfield(curMarkerData, 'x_imgLowerLeft') && isfield(curMarkerData, 'y_imgLowerLeft')
        
        jsonTestData.Poses{poseIndex}.VisionMarkers{markerIndex}.x_imgLowerLeft = curMarkerData.x_imgUpperLeft;
        jsonTestData.Poses{poseIndex}.VisionMarkers{markerIndex}.y_imgLowerLeft = curMarkerData.y_imgUpperLeft;
        
        jsonTestData.Poses{poseIndex}.VisionMarkers{markerIndex}.x_imgUpperLeft = curMarkerData.x_imgUpperRight;
        jsonTestData.Poses{poseIndex}.VisionMarkers{markerIndex}.y_imgUpperLeft = curMarkerData.y_imgUpperRight;
        
        jsonTestData.Poses{poseIndex}.VisionMarkers{markerIndex}.x_imgUpperRight = curMarkerData.x_imgLowerRight;
        jsonTestData.Poses{poseIndex}.VisionMarkers{markerIndex}.y_imgUpperRight = curMarkerData.y_imgLowerRight;
        
        jsonTestData.Poses{poseIndex}.VisionMarkers{markerIndex}.x_imgLowerRight = curMarkerData.x_imgLowerLeft;
        jsonTestData.Poses{poseIndex}.VisionMarkers{markerIndex}.y_imgLowerRight = curMarkerData.y_imgLowerLeft;
        
        Save();
    end
    
function detectAndAddMarkers()
    global jsonTestData;
    global curPoseIndex;
    global image;
    
    % TODO
    markers = simpleDetector(image, 'quadRefinementIterations', 0, 'showComponents', true);
    
    for iMarker = 1:length(markers)
        newMarker.x_imgUpperLeft = markers{iMarker}.corners(1,1);
        newMarker.y_imgUpperLeft = markers{iMarker}.corners(1,2);
        newMarker.x_imgLowerLeft= markers{iMarker}.corners(2,1);
        newMarker.y_imgLowerLeft= markers{iMarker}.corners(2,2);
        newMarker.x_imgUpperRight = markers{iMarker}.corners(3,1);
        newMarker.y_imgUpperRight = markers{iMarker}.corners(3,2);
        newMarker.x_imgLowerRight = markers{iMarker}.corners(4,1);
        newMarker.y_imgLowerRight = markers{iMarker}.corners(4,2);
        newMarker.markerType = markers{iMarker}.name;
        
        jsonTestData.Poses{curPoseIndex}.VisionMarkers{end+1} = newMarker;
    end
    
    poseChanged(false);
    
    Save();
    
function maxIndex = getMaxMarkerIndex(poseIndex)
    global jsonTestData;
    
    numMarkers = length(jsonTestData.Poses{poseIndex}.VisionMarkers);
    
    if numMarkers == 0
        maxIndex = 1;
        return;
    end
    
    [cornersX, ~, ~] = getFiducialCorners(poseIndex, numMarkers);
    
    if length(cornersX) == 4
        maxIndex = numMarkers + 1;
    else
        maxIndex = numMarkers;
    end
    
function loadTestFile()
    global jsonTestFilename;
    global jsonTestData;
    global dataPath;
    global allHandles;
    global image;
    global imageFigureHandle;
    global curPoseIndex;
    global curMarkerIndex;
    
    jsonTestFilename = [get(allHandles.testJsonFilename1, 'String'), '/', get(allHandles.testJsonFilename2, 'String')];
    jsonTestFilename = strrep(jsonTestFilename, '\', '/');
    slashIndexes = strfind(jsonTestFilename, '/');
    dataPath = jsonTestFilename(1:(slashIndexes(end)));
    
    % Try to load the file, then add a .json extension and try again
    
    loadSucceeded = false;
    try
        jsonTestData = loadJsonTestFile(jsonTestFilename);
        jsonTestData = jsonTestData.jsonData;
        loadSucceeded = true;
    catch
    end
    
    if ~loadSucceeded
        jsonTestFilename = [jsonTestFilename, '.json'];
        
        try
            jsonTestData = loadJsonTestFile(jsonTestFilename);
            jsonTestData = jsonTestData.jsonData;
            loadSucceeded = true;
        catch
        end
    end
    
    if ~loadSucceeded
        disp(sprintf('Could not load json file %s', jsonTestFilename));
        return;
    end
    
    image = rand([240,320]);
    
    imageFigureHandle = figure(100);
    
    N = nan;
    
    % Alternating white-black ring 2-width
    pointer = [
        N, N, N, N, 2, 1, 2, 1, 2, 1, 2, 1, N, N, N, N;
        N, N, N, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, N, N, N;
        N, N, 2, 1, N, N, N, N, N, N, N, N, 2, 1, N, N;
        N, 1, 2, N, N, N, N, N, N, N, N, N, N, 1, 2, N;
        2, 2, N, N, N, N, N, N, N, N, N, N, N, N, 1, 1;
        1, 1, N, N, N, N, N, N, N, N, N, N, N, N, 2, 2;
        2, 2, N, N, N, N, N, N, N, N, N, N, N, N, 1, 1;
        1, 1, N, N, N, N, N, N, N, N, N, N, N, N, 2, 2;
        2, 2, N, N, N, N, N, N, N, N, N, N, N, N, 1, 1;
        1, 1, N, N, N, N, N, N, N, N, N, N, N, N, 2, 2;
        2, 2, N, N, N, N, N, N, N, N, N, N, N, N, 1, 1;
        1, 1, N, N, N, N, N, N, N, N, N, N, N, N, 2, 2;
        N, 2, 1, N, N, N, N, N, N, N, N, N, N, 2, 1, N;
        N, N, 1, 2, N, N, N, N, N, N, N, N, 1, 2, N, N;
        N, N, N, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, N, N, N;
        N, N, N, N, 1, 2, 1, 2, 1, 2, 1, 2, N, N, N, N;];
    
    set(imageFigureHandle,'Pointer','custom','PointerShapeCData',pointer,'PointerShapeHotSpot',(size(pointer))/2)
    
    curPoseIndex = 5;
    curMarkerIndex = 1;
    
    poseChanged(true)
    
    %     Save();
    
    return;
    
function boundsFixed = fixBounds()
    global jsonTestData;
    global curPoseIndex;
    global curDisplayType;
    global maxDisplayType;
    global curMarkerIndex;
    
    boundsFixed = false;
    
    jsonTestData = sanitizeJsonTest(jsonTestData);
    
    curPoseIndexOriginal = curPoseIndex;
    
    curPoseIndex = max(1, min(length(jsonTestData.Poses), curPoseIndex));
    curMarkerIndex = max(1, min(curMarkerIndex, getMaxMarkerIndex(curPoseIndex)));
    
    if curDisplayType > maxDisplayType
        curDisplayType = 1;
    elseif curDisplayType < 1
        curDisplayType = maxDisplayType;
    end
    
    if curPoseIndexOriginal ~= curPoseIndex || curDisplayType ~= maxDisplayType
        boundsFixed = true;
    end
    
function poseChanged(resetZoom)
    global jsonTestData;
    global dataPath;
    global curPoseIndex;
    global curMarkerIndex;
    global curDisplayType;
    global maxDisplayType;
    global displayParameters;
    global image;
    global imageFigureHandle;
    global imageHandle;
    global pointsType;
    global resolutionHorizontal;
    global resolutionVertical;
    global allHandles;
    global setFigurePosition;
    global lockResolution;
    
    fixBounds();
    
    curImageFilename = [dataPath, jsonTestData.Poses{curPoseIndex}.ImageFile];
    try
        image = imread(curImageFilename);
    catch
        disp(sprintf('Could not load %s', curImageFilename));
        return;
    end
    
    if ~lockResolution
        resolutionHorizontal = size(image, 2);
        resolutionVertical = size(image, 1);
    end
    
    [allTestNames, curTestIndex] = getTestNamesFromDirectory();
    
    set(allHandles.pose_current, 'String', num2str(curPoseIndex));
    set(allHandles.pose_max, 'String', num2str(length(jsonTestData.Poses)));
    set(allHandles.test_current, 'String', num2str(curTestIndex));
    set(allHandles.test_max, 'String', num2str(length(allTestNames)));
    set(allHandles.test_numMarkersWithContext, 'String', num2str(jsonTestData.NumMarkersWithContext));
    set(allHandles.test_numPartialMarkersWithContext, 'String', num2str(jsonTestData.NumPartialMarkersWithContext));
    set(allHandles.marker_current, 'String', num2str(curMarkerIndex));
    set(allHandles.displayType_current, 'String', num2str(curDisplayType));
    set(allHandles.displayType_max, 'String', num2str(maxDisplayType));
    set(allHandles.displayType_parameter1, 'String', num2str(displayParameters{1}));
    set(allHandles.displayType_parameter2, 'String', num2str(displayParameters{2}));
    set(allHandles.displayType_parameter3, 'String', num2str(displayParameters{3}));
    set(allHandles.displayType_parameter4, 'String', num2str(displayParameters{4}));
    set(allHandles.resolutionHorizontal, 'String', num2str(resolutionHorizontal));
    set(allHandles.resolutionVertical, 'String', num2str(resolutionVertical));
    set(allHandles.marker_max, 'String', num2str(length(jsonTestData.Poses{curPoseIndex}.VisionMarkers)));
    set(allHandles.lockResolution, 'Value', lockResolution);
    
    if isfield(jsonTestData.Poses{curPoseIndex}, 'VisionMarkers') && length(jsonTestData.Poses{curPoseIndex}.VisionMarkers) >= curMarkerIndex
        set(allHandles.markerType, 'String', jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex}.markerType(8:end));
    else
        set(allHandles.markerType, 'String', 'NO MARKER');
    end
    
    slashIndexes = strfind(jsonTestData.Poses{curPoseIndex}.ImageFile, '/');
    imageFilename = jsonTestData.Poses{curPoseIndex}.ImageFile((slashIndexes(end)+1):end);
    set(allHandles.pose_filename, 'String', imageFilename);
    
    if strcmp(pointsType, 'template')
        set(allHandles.templatePoints, 'Value', 1);
        set(allHandles.fiducialMarkerPoints, 'Value', 0);
        set(allHandles.noPoints, 'Value', 0);
    elseif strcmp(pointsType, 'fiducialMarker')
        set(allHandles.templatePoints, 'Value', 0);
        set(allHandles.fiducialMarkerPoints, 'Value', 1);
        set(allHandles.noPoints, 'Value', 0);
    elseif strcmp(pointsType, 'none')
        set(allHandles.templatePoints, 'Value', 0);
        set(allHandles.fiducialMarkerPoints, 'Value', 0);
        set(allHandles.noPoints, 'Value', 1);
    else
        assert(false);
    end
    
    fixBounds();
    
    imageFigureHandle = figure(100);
    
    hold off;
    
    originalXlim = xlim();
    originalYlim = ylim();
    
    if curDisplayType == 1
        % original image
        set(allHandles.panelDisplayType, 'Title', 'Display Type: Original')
        set(allHandles.textDisplayParameter1, 'String', 'NULL')
        set(allHandles.textDisplayParameter2, 'String', 'NULL')
        set(allHandles.textDisplayParameter3, 'String', 'NULL')
        set(allHandles.textDisplayParameter4, 'String', 'NULL')
        processedImage = image;
    elseif curDisplayType == 2
        set(allHandles.panelDisplayType, 'Title', 'Display Type: Canny')
        set(allHandles.textDisplayParameter1, 'String', 'thresh')
        set(allHandles.textDisplayParameter2, 'String', 'sigma')
        set(allHandles.textDisplayParameter3, 'String', 'NULL')
        set(allHandles.textDisplayParameter4, 'String', 'NULL')
        processedImage = edge(rgb2gray2(image), 'canny', displayParameters{1}, displayParameters{2});
    end
    
    if resolutionHorizontal ~= size(image,2) || resolutionVertical ~= size(image,1)
        imageResized = imresize(processedImage, [resolutionVertical,resolutionHorizontal], 'nearest');
    else
        imageResized = processedImage;
    end
    
    if strcmp(pointsType, 'template')
        padAmount = floor(size(imageResized)/2);
        imageResized = padarray(imageResized, padAmount);        
    else
        padAmount = [0,0];
    end
    
    imageHandle = imshow(imageResized, 'Border', 'tight');
    
    % One time, move the image figure
    if ~setFigurePosition
        set(imageFigureHandle, 'OuterPosition', [455, 1, 1366, 1080]);
        setFigurePosition = true;
    end
    
    if ~resetZoom
        xlim(originalXlim);
        ylim(originalYlim);
    end
    
    set(imageHandle,'ButtonDownFcn',@ButtonClicked);
    hold on;
    
    xScaleInv = resolutionHorizontal / size(image,2);
    yScaleInv = resolutionVertical / size(image,1);
    
    if strcmp(pointsType, 'template') || strcmp(pointsType, 'fiducialMarker')
        
        for iMarker = 1:length(jsonTestData.Poses{curPoseIndex}.VisionMarkers)
            if iMarker == curMarkerIndex
                linePlotType = 'g';
            else
                linePlotType = 'y';
            end
            
            [cornersX, cornersY] = getFiducialCorners(curPoseIndex, iMarker);
            
            cornersY = cornersY + padAmount(1);
            cornersX = cornersX + padAmount(2);
            
            % Plot the corners (and if 4 corners exist, the quad as well)
            if length(cornersX) == 4
                %                 plotHandle = plot(...
                %                     xScaleInv*([cornersX(1),cornersX(2),cornersX(3),cornersX(4),cornersX(1)]+0.5),...
                %                     yScaleInv*([cornersY(1),cornersY(2),cornersY(3),cornersY(4),cornersY(1)]+0.5),...
                %                     linePlotType);
                %                 set(plotHandle, 'HitTest', 'off')
                
                plotHandle = plot(...
                    xScaleInv*([cornersX(2),cornersX(3),cornersX(4),cornersX(1)]+0.5),...
                    yScaleInv*([cornersY(2),cornersY(3),cornersY(4),cornersY(1)]+0.5),...
                    linePlotType, 'LineWidth', 1);
                set(plotHandle, 'HitTest', 'off')
                
                plotHandle = plot(...
                    xScaleInv*([cornersX(1),cornersX(2)]+0.5),...
                    yScaleInv*([cornersY(1),cornersY(2)]+0.5),...
                    linePlotType, 'LineWidth', 3);
                set(plotHandle, 'HitTest', 'off')
                
                markerName = jsonTestData.Poses{curPoseIndex}.VisionMarkers{iMarker}.markerType(8:end);
                textHandle = text((cornersX(1)+cornersX(4))/2 + 5, (cornersY(1)+cornersY(4))/2, markerName, 'Color', linePlotType);
                set(textHandle, 'HitTest', 'off')
            end
            
            for i = 1:length(cornersX)
                scatterHandle = scatter(xScaleInv*(cornersX(i)+0.5), yScaleInv*(cornersY(i)+0.5), [linePlotType,'o']);
                set(scatterHandle, 'HitTest', 'off')
            end
        end % for iMarker = 1:length(jsonTestData.Poses{curPoseIndex}.VisionMarkers)
    elseif strcmp(pointsType, 'none')
        % do nothing
    else
        assert(false);
    end
    
function Save()
    global jsonTestData;
    global jsonTestFilename;
    
    fixBounds();
    
    jsonTestDataToSave = jsonTestData;
    
    % Remove any marker with fewer than four corners
    for iPose = 1:length(jsonTestData.Poses)
        originalMarkers = jsonTestData.Poses{iPose}.VisionMarkers;
        newMarkers = {};
        
        for iMarker = 1:length(originalMarkers)
            [cornersX, ~, ~] = getFiducialCorners(iPose, iMarker);
            if length(cornersX) == 4
                newMarkers{end+1} = originalMarkers{iMarker}; %#ok<AGROW>
            end
        end
        
        jsonTestDataToSave.Poses{iPose}.VisionMarkers = newMarkers;
        jsonTestDataToSave.Poses{iPose}.NumMarkers = length(newMarkers);
    end
    
    savejson('',jsonTestDataToSave,jsonTestFilename);
    
function ButtonClicked(~, ~, ~)
    global jsonTestData;
    global curPoseIndex;
    global curMarkerIndex;
    global image;
    global imageFigureHandle;
    global imageHandle;
    global pointsType;
    global resolutionHorizontal;
    global resolutionVertical;
    
    changed = false;
    
    axesHandle  = get(imageHandle, 'Parent');
    imPosition = get(axesHandle, 'CurrentPoint') - 0.5;
    
    imPosition = imPosition(1,1:2);
    
    if imPosition(1) < 0 || imPosition(2) < 0 || imPosition(1) > size(image,2) || imPosition(2) > size(image,1)
        return;
    end
    
    fixBounds();
    
    [cornersX, cornersY, whichCorners] = getFiducialCorners(curPoseIndex, curMarkerIndex);
    
    xScale = size(image,2) / resolutionHorizontal;
    yScale = size(image,1) / resolutionVertical;

    if strcmp(pointsType, 'template')
        padAmount = floor(size(image) / 2);
    elseif strcmp(pointsType, 'fiducialMarker')
        padAmount = [0,0];
    end

    newPoint.x = (imPosition(1) - padAmount(2)) * xScale;
    newPoint.y = (imPosition(2) - padAmount(1)) * yScale;
    
    buttonType = get(imageFigureHandle, 'selectionType');
    if strcmp(buttonType, 'normal') % left click       
%         disp(newPoint)
        if strcmp(pointsType, 'template') || strcmp(pointsType, 'fiducialMarker')
            if sum(whichCorners) < 4
                if ~whichCorners(1)
                    jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex}.x_imgUpperLeft = newPoint.x;
                    jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex}.y_imgUpperLeft = newPoint.y;
                elseif ~whichCorners(2)
                    jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex}.x_imgUpperRight = newPoint.x;
                    jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex}.y_imgUpperRight = newPoint.y;
                elseif ~whichCorners(3)
                    jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex}.x_imgLowerRight = newPoint.x;
                    jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex}.y_imgLowerRight = newPoint.y;
                elseif ~whichCorners(4)
                    jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex}.x_imgLowerLeft = newPoint.x;
                    jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex}.y_imgLowerLeft = newPoint.y;
                else
                    assert(false);
                end
                
                changed = true;
            else
                disp('Cannot add point, because only 4 fiduciual marker corners are allowed');
            end
        end
    elseif strcmp(buttonType, 'alt') % right click       
        minDist = Inf;
        minInd = -1;
        
        if strcmp(pointsType, 'template') || strcmp(pointsType, 'fiducialMarker')
            ci = 0;
            for i = 1:length(whichCorners)
                if ~whichCorners(i)
                    continue;
                end
                
                ci = ci + 1;
                
                dist = sqrt((cornersX(ci) - newPoint.x)^2 + (cornersY(ci) - newPoint.y)^2);
                if dist < minDist
                    minDist = dist;
                    minInd = i;
                end
            end
            
            if minInd ~= -1 && minDist < (min(size(image,1),size(image,2))/50)
                newMarkerData = jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex};
                
                if minInd == 1
                    newMarkerData = rmfield(newMarkerData, 'x_imgUpperLeft');
                    newMarkerData = rmfield(newMarkerData, 'y_imgUpperLeft');
                elseif minInd == 2
                    newMarkerData = rmfield(newMarkerData, 'x_imgUpperRight');
                    newMarkerData = rmfield(newMarkerData, 'y_imgUpperRight');
                elseif minInd == 3
                    newMarkerData = rmfield(newMarkerData, 'x_imgLowerRight');
                    newMarkerData = rmfield(newMarkerData, 'y_imgLowerRight');
                elseif minInd == 4
                    newMarkerData = rmfield(newMarkerData, 'x_imgLowerLeft');
                    newMarkerData = rmfield(newMarkerData, 'y_imgLowerLeft');
                else
                    assert(false);
                end
                
                jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex} = newMarkerData;
                changed = true;
            end
        end % elseif strcmp(pointsType, 'fiducialMarker')
    elseif strcmp(buttonType, 'extend') % shift + left click
        [cornersX, cornersY, ~] = getFiducialCorners(curPoseIndex, []);
        
        minDist = Inf;
        minInd = -1;
        
        if strcmp(pointsType, 'template') || strcmp(pointsType, 'fiducialMarker')
            ci = 0;
            
            for iMarker = 1:size(cornersX,2)
                ci = ci + 1;
                
                meanCornersX = mean(cornersX(:,iMarker));
                meanCornersY = mean(cornersY(:,iMarker));
                
                dist = sqrt((meanCornersX - newPoint.x)^2 + (meanCornersY - newPoint.y)^2);
                if dist < minDist
                    minDist = dist;
                    minInd = iMarker;
                end
            end % for i = 1:size(cornersX,2)
            
            if minInd ~= -1
                curMarkerIndex = minInd;
                fixBounds();
            end
        end % elseif strcmp(pointsType, 'fiducialMarker')
    end
    
    if changed
        Save();
    end
    
    poseChanged(false);
    
