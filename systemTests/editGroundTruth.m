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

    curPoseIndex = 1;
    curMarkerIndex = 1;
    curDisplayType = 1;
    maxDisplayType = 1;
    displayParameters = cell(maxDisplayType,1);
    savedDisplayParameters = cell(maxDisplayType,1);
    savedDisplayParameters{1} = zeros(4,1);
    resolutionHorizontal = 320;
    resolutionVertical = 240;
    allHandles = handles;

    setFromSavedDisplayParameters();

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

function marker_current_CreateFcn(hObject, ~, ~)
    global curMarkerIndex;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(curMarkerIndex));

function displayType_current_CreateFcn(hObject, ~, ~)
    global curDisplayType;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(curDisplayType));

function pose_max_CreateFcn(hObject, ~, ~)
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

function setDefaultGuiObjectColor(hObject)
    if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
        set(hObject,'BackgroundColor','white');
    end

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

    jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex}.markerType = get(hObject,'String');

    Save()
    
    poseChanged(false);

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

function labelingTypePanel_SelectionChangeFcn(~, ~, ~)
    global pointsType;

    if eventdata.NewValue == handles.templatePoints
        pointsType = 'template';
        poseChanged(false);
    elseif eventdata.NewValue == handles.fiducialMarkerPoints
        pointsType = 'fiducialMarker';
        poseChanged(false);
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
    end
    
    Save();

function detectAndAddMarkers()
    global jsonTestData;
    global curPoseIndex;
    global image;

    % TODO
    markers = simpleDetector(image, 'quadRefinementIterations', 0);

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
    global pointsType;
    global curPoseIndex;
    global curMarkerIndex;

    jsonTestFilename = [get(allHandles.testJsonFilename1, 'String'), '/', get(allHandles.testJsonFilename2, 'String')];
    jsonTestFilename = strrep(jsonTestFilename, '\', '/');
    slashIndexes = strfind(jsonTestFilename, '/');
    dataPath = jsonTestFilename(1:(slashIndexes(end)));

    % Try to load the file, then add a .json extension and try again
    
    loadSucceeded = false;    
    try
        jsonTestData = loadjson(jsonTestFilename);
        loadSucceeded = true;
    catch        
    end
    
    if ~loadSucceeded
        jsonTestFilename = [jsonTestFilename, '.json'];
        
        try
            jsonTestData = loadjson(jsonTestFilename);
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

    pointsType = 'fiducialMarker';

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

    curPoseIndex = 1;
    curMarkerIndex = 1;
    
    poseChanged(true)

    return;

function sanitizeJson()
    % Don't call this function directly, call fixBounds() instead

    global jsonTestData;

    if ~isfield(jsonTestData, 'Poses')
        assert(false);
    end

    if isstruct(jsonTestData.Poses)
        jsonTestData.Poses = {jsonTestData.Poses};
    end

    for iPose = 1:length(jsonTestData.Poses)
        if ~isfield(jsonTestData.Poses{iPose}, 'VisionMarkers')
            jsonTestData.Poses{iPose}.VisionMarkers = [];
        end

        if ~isfield(jsonTestData.Poses{iPose}, 'ImageFile')
            assert(false);
        end

        if ~isfield(jsonTestData.Poses{iPose}, 'NumMarkers')
            jsonTestData.Poses{iPose}.NumMarkers = 0;
        end

        if ~isfield(jsonTestData.Poses{iPose}, 'RobotPose')
            jsonTestData.Poses{iPose}.RobotPose.Angle = 0;
            jsonTestData.Poses{iPose}.RobotPose.Axis = [1,0,0];
            jsonTestData.Poses{iPose}.RobotPose.HeadAngle = 0;
            jsonTestData.Poses{iPose}.RobotPose.Translation = [0,0,0];
        end

        if isstruct(jsonTestData.Poses{iPose}.VisionMarkers)
            jsonTestData.Poses{iPose}.VisionMarkers = {jsonTestData.Poses{iPose}.VisionMarkers};
        end

        maxMarkerIndex = length(jsonTestData.Poses{iPose}.VisionMarkers);

        if length(jsonTestData.Poses{iPose}.VisionMarkers) < maxMarkerIndex
            jsonTestData.Poses{iPose}.VisionMarkers{end+1} = [];
        end

        for iMarker = 1:maxMarkerIndex
            if ~isfield(jsonTestData.Poses{iPose}.VisionMarkers{iMarker}, 'Name')
                jsonTestData.Poses{iPose}.VisionMarkers{iMarker}.Name = 'MessageVisionMarker';
            end

            if ~isfield(jsonTestData.Poses{iPose}.VisionMarkers{iMarker}, 'markerType')
                jsonTestData.Poses{iPose}.VisionMarkers{iMarker}.markerType = 'MARKER_UNKNOWN';
            end

            if ~isfield(jsonTestData.Poses{iPose}.VisionMarkers{iMarker}, 'timestamp')
                jsonTestData.Poses{iPose}.VisionMarkers{iMarker}.timestamp = 0;
            end
        end

        if isstruct(jsonTestData.Poses{iPose}.RobotPose)
            jsonTestData.Poses{iPose}.RobotPose = {jsonTestData.Poses{iPose}.RobotPose};
        end
    end

function boundsFixed = fixBounds()
    global jsonTestData;
    global curPoseIndex;
    global curDisplayType;
    global maxDisplayType;
    global curMarkerIndex;

    boundsFixed = false;

    sanitizeJson();

    curPoseIndexOriginal = curPoseIndex;

    curPoseIndex = max(1, min(length(jsonTestData.Poses), curPoseIndex));
    curDisplayType = max(1, min(maxDisplayType, curDisplayType));
    curMarkerIndex = max(1, min(curMarkerIndex, getMaxMarkerIndex(curPoseIndex)));

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

    fixBounds();

    curImageFilename = [dataPath, jsonTestData.Poses{curPoseIndex}.ImageFile];
    try
        image = imread(curImageFilename);
    catch
        disp(sprintf('Could not load %s', curImageFilename));
        return;
    end

    slashIndexes = strfind(curImageFilename, '/');
    curImageFilenameWithoutPath = curImageFilename((slashIndexes(end)+1):end);
    set(allHandles.pose_current, 'String', num2str(curPoseIndex))
    set(allHandles.pose_max, 'String', num2str(length(jsonTestData.Poses)))
    set(allHandles.marker_current, 'String', num2str(curMarkerIndex))
    set(allHandles.displayType_current, 'String', num2str(curDisplayType))
    set(allHandles.displayType_max, 'String', num2str(maxDisplayType))
    set(allHandles.displayType_parameter1, 'String', num2str(displayParameters{1}))
    set(allHandles.displayType_parameter2, 'String', num2str(displayParameters{2}))
    set(allHandles.displayType_parameter3, 'String', num2str(displayParameters{3}))
    set(allHandles.displayType_parameter4, 'String', num2str(displayParameters{4}))
    set(allHandles.resolutionHorizontal, 'String', num2str(resolutionHorizontal))
    set(allHandles.resolutionVertical, 'String', num2str(resolutionVertical))
    set(allHandles.marker_max, 'String', num2str(length(jsonTestData.Poses{curPoseIndex}.VisionMarkers)));

    if isfield(jsonTestData.Poses{curPoseIndex}, 'VisionMarkers') && length(jsonTestData.Poses{curPoseIndex}.VisionMarkers) >= curMarkerIndex
        set(allHandles.markerType, 'String', jsonTestData.Poses{curPoseIndex}.VisionMarkers{curMarkerIndex}.markerType);
    else
        set(allHandles.markerType, 'String', 'NO MARKER');
    end

    slashIndexes = strfind(jsonTestData.Poses{curPoseIndex}.ImageFile, '/');
    imageFilename = jsonTestData.Poses{curPoseIndex}.ImageFile((slashIndexes(end)+1):end);
    set(allHandles.pose_filename, 'String', imageFilename);

    if strcmp(pointsType, 'template')
        set(allHandles.templatePoints, 'Value', 1);
        set(allHandles.fiducialMarkerPoints, 'Value', 0);
    elseif strcmp(pointsType, 'fiducialMarker')
        set(allHandles.templatePoints, 'Value', 0);
        set(allHandles.fiducialMarkerPoints, 'Value', 1);
    else
        assert(false);
    end

    fixBounds();

    imageFigureHandle = figure(100);

    hold off;

    originalXlim = xlim();
    originalYlim = ylim();

    if resolutionHorizontal ~= size(image,2) || resolutionVertical ~= size(image,1)
        imageResized = imresize(image, [resolutionVertical,resolutionHorizontal], 'nearest');
    else
        imageResized = image;
    end

    if curDisplayType == 1
        % original image
        set(allHandles.panelDisplayType, 'Title', 'Display Type: Original')
        set(allHandles.textDisplayParameter1, 'String', 'NULL')
        set(allHandles.textDisplayParameter2, 'String', 'NULL')
        set(allHandles.textDisplayParameter3, 'String', 'NULL')
        set(allHandles.textDisplayParameter4, 'String', 'NULL')
    end

    imageHandle = imshow(imageResized);

    if ~resetZoom
        xlim(originalXlim);
        ylim(originalYlim);
    end

    set(imageHandle,'ButtonDownFcn',@ButtonClicked);
    hold on;

    xScaleInv = resolutionHorizontal / size(image,2);
    yScaleInv = resolutionVertical / size(image,1);

    if strcmp(pointsType, 'template')
%             allCorners = jsonTestData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners;
%
%             if length(allCorners) == 4
%                 % first, sort the corners
%                 cornersX = [allCorners{1}.x, allCorners{2}.x, allCorners{3}.x, allCorners{4}.x];
%                 cornersY = [allCorners{1}.y, allCorners{2}.y, allCorners{3}.y, allCorners{4}.y];
%
%                 centerX = mean(cornersX);
%                 centerY = mean(cornersY);
%
%                 [thetas,~] = cart2pol(cornersX-centerX, cornersY-centerY);
%                 [~,sortedIndexes] = sort(thetas);
%
%                 for i = 1:4
%                     jsonTestData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners{i}.x = cornersX(sortedIndexes(i));
%                     jsonTestData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners{i}.y = cornersY(sortedIndexes(i));
%                 end
%
%                 allCorners = jsonTestData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners;
%
%                 % second, plot the sorted corners
%                 plotHandle = plot(...
%                     xScaleInv*([allCorners{1}.x,allCorners{2}.x,allCorners{3}.x,allCorners{4}.x,allCorners{1}.x]+0.5),...
%                     yScaleInv*([allCorners{1}.y,allCorners{2}.y,allCorners{3}.y,allCorners{4}.y,allCorners{1}.y]+0.5),...
%                     'b');
%                 set(plotHandle, 'HitTest', 'off')
%             end
%
%             for i = 1:length(allCorners)
%                 scatterHandle = scatter(xScaleInv*(allCorners{i}.x+0.5), yScaleInv*(allCorners{i}.y+0.5), 'b+');
%                 set(scatterHandle, 'HitTest', 'off')
%             end
    elseif strcmp(pointsType, 'fiducialMarker')
        for iMarker = 1:length(jsonTestData.Poses{curPoseIndex}.VisionMarkers)
            if iMarker == curMarkerIndex
                linePlotType = 'g';
            else
                linePlotType = 'y';
            end

           [cornersX, cornersY] = getFiducialCorners(curPoseIndex, iMarker);

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
%     global allHandles;
    global imageFigureHandle;
    global imageHandle;
    global pointsType;
    global resolutionHorizontal;
    global resolutionVertical;

    axesHandle  = get(imageHandle,'Parent');
    imPosition = get(axesHandle, 'CurrentPoint') - 0.5;

    imPosition = imPosition(1,1:2);

    if imPosition(1) < 0 || imPosition(2) < 0 || imPosition(1) > size(image,2) || imPosition(2) > size(image,1)
        return;
    end

    fixBounds();

    [cornersX, cornersY, whichCorners] = getFiducialCorners(curPoseIndex, curMarkerIndex);

    buttonType = get(imageFigureHandle,'selectionType');
    if strcmp(buttonType, 'normal') % left click
        xScale = size(image,2) / resolutionHorizontal;
        yScale = size(image,1) / resolutionVertical;

        newPoint.x = imPosition(1) * xScale;
        newPoint.y = imPosition(2) * yScale;

        if strcmp(pointsType, 'template')
            % TODO: implement
            assert(false);
        elseif strcmp(pointsType, 'fiducialMarker')
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
            else
                disp('Cannot add point, because only 4 fiduciual marker corners are allowed');
            end
        end

        Save();
    elseif strcmp(buttonType, 'alt') % right click
        xScaleInv = resolutionHorizontal / size(image,2);
        yScaleInv = resolutionVertical / size(image,1);

        minDist = Inf;
        minInd = -1;

        if strcmp(pointsType, 'template')
            % TODO: implement
            assert(false);
        elseif strcmp(pointsType, 'fiducialMarker')
            ci = 0;
            for i = 1:length(whichCorners)
                if ~whichCorners(i)
                    continue;
                end

                ci = ci + 1;

                dist = sqrt((cornersX(ci)*xScaleInv - imPosition(1))^2 + (cornersY(ci)*yScaleInv - imPosition(2))^2);
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

                Save();
            end
        end
    end

    poseChanged(false);




