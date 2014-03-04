% function varargout = editGroundTruth(varargin)

% editGroundTruth();

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

function editGroundTruth_OpeningFcn(hObject, eventdata, handles, varargin)
    global maxErrorSignalCorners;
    global maxTemplateCorners;
    global maxFiducialMarkerCorners;
    global curDisplayType;
    global maxDisplayType;
    global savedDisplayParameters;
    global resolutionHorizontal;
    global resolutionVertical;
    global curTestNumber;
    global maxTestNumber;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;
    global curSet;
    global maxSet;

    curTestNumber = 1;
    maxTestNumber = 1;
    curSequenceNumber = 1;
    maxSequenceNumber = 1;
    curFrameNumber = 1;
    maxFrameNumber = 1;
    curSet = 1;
    maxSet = 2;
    maxErrorSignalCorners = 2;
    maxTemplateCorners = 4;
    maxFiducialMarkerCorners = 4;
    curDisplayType = 1;
    maxDisplayType = 5;
    savedDisplayParameters = cell(5,1);
    savedDisplayParameters{1} = zeros(4,1);
    savedDisplayParameters{2} = [5, 1.5, 0, 0];
    savedDisplayParameters{3} = [5, 1.5, 0, 0];
    savedDisplayParameters{4} = [0, 0, 0, 0];
    savedDisplayParameters{5} = [5, 1.5, 0, 0];
    resolutionHorizontal = 640;
    resolutionVertical = 480;

    setFromSavedDisplayParameters();

    % Choose default command line output for editGroundTruth
    handles.output = hObject;

    % Update handles structure
    guidata(hObject, handles);

    % UIWAIT makes editGroundTruth wait for user response (see UIRESUME)
    % uiwait(handles.figure1);

    global allHandles;
    allHandles = handles;

    loadAllTestsFile()

function varargout = editGroundTruth_OutputFcn(hObject, eventdata, handles)
    % Get default command line output from handles structure
    varargout{1} = handles.output;

%
% Create Functions
%

function curTest_CreateFcn(hObject, eventdata, handles)
    global curTestNumber;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(curTestNumber));

function curSequence_CreateFcn(hObject, eventdata, handles) %#ok<*INUSD>
    global curSequenceNumber;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(curSequenceNumber));

function curImage_CreateFcn(hObject, eventdata, handles)
    global curFrameNumber;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(curFrameNumber));

function curSet_CreateFcn(hObject, eventdata, handles)
    global curSet;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(curSet));

function curDisplayType_CreateFcn(hObject, eventdata, handles)
    global curDisplayType;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(curDisplayType));

function maxTest_CreateFcn(hObject, eventdata, handles)
    global maxTestNumber;
    set(hObject,'String',num2str(maxTestNumber));

function maxSequence_CreateFcn(hObject, eventdata, handles)
    global maxSequenceNumber;
    set(hObject,'String',num2str(maxSequenceNumber));

function maxImage_CreateFcn(hObject, eventdata, handles)
    global maxFrameNumber;
    set(hObject,'String',num2str(maxFrameNumber));

function maxSet_CreateFcn(hObject, eventdata, handles)
    global maxSet;
    set(hObject,'String',num2str(maxSet));

function maxDisplayType_CreateFcn(hObject, eventdata, handles)
    global maxDisplayType;
    set(hObject,'String',num2str(maxDisplayType));

function configFilename_CreateFcn(hObject, eventdata, handles)
    setDefaultGuiObjectColor(hObject);

function figure1_CreateFcn(hObject, eventdata, handles)

function previousSequence_CreateFcn(hObject, eventdata, handles)

function nextSequence_CreateFcn(hObject, eventdata, handles)

function previousImage_CreateFcn(hObject, eventdata, handles)

function nextImage_CreateFcn(hObject, eventdata, handles)

function configFilenameNoteText_CreateFcn(hObject, eventdata, handles)

function previousTest_CreateFcn(hObject, eventdata, handles)

function nextTest_CreateFcn(hObject, eventdata, handles)

function displayParameter1_CreateFcn(hObject, eventdata, handles)
    global displayParameter1;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(displayParameter1));

function displayParameter2_CreateFcn(hObject, eventdata, handles)
    global displayParameter2;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(displayParameter2));

function displayParameter3_CreateFcn(hObject, eventdata, handles)
    global displayParameter3;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(displayParameter3));

function displayParameter4_CreateFcn(hObject, eventdata, handles)
    global displayParameter4;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(displayParameter4));

function resolutionHorizontal_CreateFcn(hObject, eventdata, handles)
    global resolutionHorizontal;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(resolutionHorizontal));

function resolutionVertical_CreateFcn(hObject, eventdata, handles)
    global resolutionVertical;
    setDefaultGuiObjectColor(hObject);
    set(hObject,'String',num2str(resolutionVertical));

%
% End Create Functions
%

%
% Callback Functions
%

function configFilename_Callback(hObject, eventdata, handles)
    loadAllTestsFile()

function setDefaultGuiObjectColor(hObject)
    if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
        set(hObject,'BackgroundColor','white');
    end

function previousTest_Callback(hObject, eventdata, handles)
    global curTestNumber;
    curTestNumber = curTestNumber - 1;
    fixBounds();
    sequenceChanged(handles, true);

function previousTest2_Callback(hObject, eventdata, handles)
    global curTestNumber;
    curTestNumber = curTestNumber - 5;
    fixBounds();
    sequenceChanged(handles, true);

function previousTest3_Callback(hObject, eventdata, handles)
    global curTestNumber;
    curTestNumber = curTestNumber - 100;
    fixBounds();
    sequenceChanged(handles, true);

function nextTest_Callback(hObject, eventdata, handles)
    global curTestNumber;
    curTestNumber = curTestNumber + 1;
    fixBounds();
    sequenceChanged(handles, true);

function nextTest2_Callback(hObject, eventdata, handles)
    global curTestNumber;
    curTestNumber = curTestNumber + 5;
    fixBounds();
    sequenceChanged(handles, true);

function nextTest3_Callback(hObject, eventdata, handles)
    global curTestNumber;
    curTestNumber = curTestNumber + 100;
    fixBounds();
    sequenceChanged(handles, true);

function curTest_Callback(hObject, eventdata, handles)
    global curTestNumber;
    curTestNumber = str2double(get(hObject,'String'));
    fixBounds();
    sequenceChanged(handles, true);

function previousSequence_Callback(hObject, eventdata, handles) %#ok<*INUSL>
    global curSequenceNumber;
    curSequenceNumber = curSequenceNumber - 1;
    fixBounds();
    sequenceChanged(handles, true);

function previousSequence2_Callback(hObject, eventdata, handles)
    global curSequenceNumber;
    curSequenceNumber = curSequenceNumber - 5;
    fixBounds();
    sequenceChanged(handles, true);

function previousSequence3_Callback(hObject, eventdata, handles)
    global curSequenceNumber;
    curSequenceNumber = curSequenceNumber - 100;
    fixBounds();
    sequenceChanged(handles, true);

function nextSequence_Callback(hObject, eventdata, handles)
    global curSequenceNumber;
    curSequenceNumber = curSequenceNumber + 1;
    fixBounds();
    sequenceChanged(handles, true);

function nextSequence2_Callback(hObject, eventdata, handles)
    global curSequenceNumber;
    curSequenceNumber = curSequenceNumber + 5;
    fixBounds();
    sequenceChanged(handles, true);

function nextSequence3_Callback(hObject, eventdata, handles)
    global curSequenceNumber;
    curSequenceNumber = curSequenceNumber + 100;
    fixBounds();
    sequenceChanged(handles, true);

function curSequence_Callback(hObject, eventdata, handles)
    global curSequenceNumber;
    curSequenceNumber = str2double(get(hObject,'String'));
    fixBounds();
    sequenceChanged(handles);

function previousImage_Callback(hObject, eventdata, handles) %#ok<*DEFNU>
    global curFrameNumber;
    curFrameNumber = curFrameNumber - 1;
    fixBounds();
    sequenceChanged(handles);

function previousImage2_Callback(hObject, eventdata, handles)
    global curFrameNumber;
    curFrameNumber = curFrameNumber - 5;
    fixBounds();
    sequenceChanged(handles);

function previousImage3_Callback(hObject, eventdata, handles)
    global curFrameNumber;
    curFrameNumber = curFrameNumber - 100;
    fixBounds();
    sequenceChanged(handles);

function nextImage_Callback(hObject, eventdata, handles)
    global curFrameNumber;
    curFrameNumber = curFrameNumber + 1;
    fixBounds();
    sequenceChanged(handles);

function nextImage2_Callback(hObject, eventdata, handles)
    global curFrameNumber;
    curFrameNumber = curFrameNumber + 5;
    fixBounds();
    sequenceChanged(handles);

function nextImage3_Callback(hObject, eventdata, handles)
    global curFrameNumber;
    curFrameNumber = curFrameNumber + 100;
    fixBounds();
    sequenceChanged(handles);

function curImage_Callback(hObject, eventdata, handles)
    global curFrameNumber;
    curFrameNumber = str2double(get(hObject,'String'));
    fixBounds();
    sequenceChanged(handles);

function nextSet_Callback(hObject, eventdata, handles)
    global curSet;
    curSet = curSet + 1;
    fixBounds();
    sequenceChanged(handles);

function nextSet2_Callback(hObject, eventdata, handles)
    global curSet;
    curSet = curSet + 5;
    fixBounds();
    sequenceChanged(handles);

function nextSet3_Callback(hObject, eventdata, handles)
    global curSet;
    curSet = curSet + 100;
    fixBounds();
    sequenceChanged(handles);

function previousSet_Callback(hObject, eventdata, handles)
    global curSet;
    curSet = curSet - 1;
    fixBounds();
    sequenceChanged(handles);

function previousSet2_Callback(hObject, eventdata, handles)
    global curSet;
    curSet = curSet - 5;
    fixBounds();
    sequenceChanged(handles);

function previousSet3_Callback(hObject, eventdata, handles)
    global curSet;
    curSet = curSet - 100;
    fixBounds();
    sequenceChanged(handles);

function curSet_Callback(hObject, eventdata, handles)
    global curSet;
    curSet = str2double(get(hObject,'String'));
    fixBounds();
    sequenceChanged(handles);

function previousDisplayType_Callback(hObject, eventdata, handles)
    global curDisplayType;
    setToSavedDisplayParameters();
    curDisplayType = curDisplayType - 1;
    fixBounds();
    setFromSavedDisplayParameters();
    sequenceChanged(handles);

function nextDisplayType_Callback(hObject, eventdata, handles)
    global curDisplayType;
    setToSavedDisplayParameters();
    curDisplayType = curDisplayType + 1;
    fixBounds();
    setFromSavedDisplayParameters();
    sequenceChanged(handles);

function curDisplayType_Callback(hObject, eventdata, handles)
    global curDisplayType;
    setToSavedDisplayParameters();
    curDisplayType = str2double(get(hObject,'String'));
    fixBounds();
    setFromSavedDisplayParameters();
    sequenceChanged(handles);

function displayParameter1_Callback(hObject, eventdata, handles)
    global displayParameter1;
    displayParameter1 = str2double(get(hObject,'String'));
    sequenceChanged(handles);

function displayParameter2_Callback(hObject, eventdata, handles)
    global displayParameter2;
    displayParameter2 = str2double(get(hObject,'String'));
    sequenceChanged(handles);

function displayParameter3_Callback(hObject, eventdata, handles)
    global displayParameter3;
    displayParameter3 = str2double(get(hObject,'String'));
    sequenceChanged(handles);

function displayParameter4_Callback(hObject, eventdata, handles)
    global displayParameter4;
    displayParameter4 = str2double(get(hObject,'String'));
    sequenceChanged(handles);

function resolutionHorizontal_Callback(hObject, eventdata, handles)
    global resolutionHorizontal;
    resolutionHorizontal = str2double(get(hObject,'String'));
    sequenceChanged(handles, false, true);

function resolutionVertical_Callback(hObject, eventdata, handles)
    global resolutionVertical;
    resolutionVertical = str2double(get(hObject,'String'));
    sequenceChanged(handles, false, true);

function labelingTypePanel_SelectionChangeFcn(hObject, eventdata, handles)
    global pointsType;

    if eventdata.NewValue == handles.errorSignalPoints
        pointsType = 'errorSignal';
        sequenceChanged(handles);
    elseif eventdata.NewValue == handles.templatePoints
        pointsType = 'template';
        sequenceChanged(handles);
    elseif eventdata.NewValue == handles.fiducialMarkerPoints
        pointsType = 'fiducialMarker';
        sequenceChanged(handles);
    else
        assert(false);
    end

%
% End Callback Functions
%

function setToSavedDisplayParameters()
    global curDisplayType;
    global displayParameter1;
    global displayParameter2;
    global displayParameter3;
    global displayParameter4;
    global savedDisplayParameters;

    savedDisplayParameters{curDisplayType}(1) = displayParameter1;
    savedDisplayParameters{curDisplayType}(2) = displayParameter2;
    savedDisplayParameters{curDisplayType}(3) = displayParameter3;
    savedDisplayParameters{curDisplayType}(4) = displayParameter4;

function setFromSavedDisplayParameters()
    global curDisplayType;
    global displayParameter1;
    global displayParameter2;
    global displayParameter3;
    global displayParameter4;
    global savedDisplayParameters;

    displayParameter1 = savedDisplayParameters{curDisplayType}(1);
    displayParameter2 = savedDisplayParameters{curDisplayType}(2);
    displayParameter3 = savedDisplayParameters{curDisplayType}(3);
    displayParameter4 = savedDisplayParameters{curDisplayType}(4);

function loadConfigFile()
    global jsonAllTestsData;
    global curTestNumber;
    global jsonConfigFilename;
    global jsonData;
    global dataPath;

    jsonConfigFilename = [dataPath, jsonAllTestsData.tests{curTestNumber}];
    jsonConfigFilename = strrep(jsonConfigFilename, '\', '/');

    jsonData = loadjson(jsonConfigFilename);

    if ~iscell(jsonData.sequences)
        jsonData.sequences = {jsonData.sequences};
    end

    return;

function loadAllTestsFile()
    global jsonAllTestsFilename;
    global jsonAllTestsData;
    global jsonConfigFilename;
    global curTestNumber;
    global maxTestNumber;
    global curSequenceNumber;
    global image;
    global imageFigureHandle;
    global allHandles;
    global pointsType;
    global dataPath;

    imageFigureHandle = figure(100);

    pointsType = 'errorSignal';

    jsonAllTestsFilename = get(allHandles.configFilename, 'String');
    jsonAllTestsFilename = strrep(jsonAllTestsFilename, '\', '/');

    try
        jsonAllTestsData = loadjson(jsonAllTestsFilename);
    catch
        disp(sprintf('Could not load all tests json file %s', jsonAllTestsFilename));
        return;
    end

    slashIndexes = strfind(jsonAllTestsFilename, '/');
    dataPath = jsonAllTestsFilename(1:(slashIndexes(end)));

    curTestNumber = 1;
    maxTestNumber = length(jsonAllTestsData.tests);

    curSequenceNumber = 1;

    image = rand([480,640]);

    N = nan;
    
% White ring 1-width
%     pointer = [
%         N, N, N, N, 2, 2, 2, 2, 2, 2, 2, 2, N, N, N, N;
%         N, N, N, 2, N, N, N, N, N, N, N, N, 2, N, N, N;
%         N, N, 2, N, N, N, N, N, N, N, N, N, N, 2, N, N;
%         N, 2, N, N, N, N, N, N, N, N, N, N, N, N, 2, N;
%         2, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 2;
%         2, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 2;
%         2, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 2;
%         2, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 2;
%         2, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 2;
%         2, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 2;
%         2, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 2;
%         2, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 2;
%         N, 2, N, N, N, N, N, N, N, N, N, N, N, N, 2, N;
%         N, N, 2, N, N, N, N, N, N, N, N, N, N, 2, N, N;
%         N, N, N, 2, N, N, N, N, N, N, N, N, 2, N, N, N;
%         N, N, N, N, 2, 2, 2, 2, 2, 2, 2, 2, N, N, N, N;];

% Alternating white-black ring 1-width
%     pointer = [
%         N, N, N, N, 2, 1, 2, 1, 2, 1, 2, 1, N, N, N, N;
%         N, N, N, 1, N, N, N, N, N, N, N, N, 2, N, N, N;
%         N, N, 2, N, N, N, N, N, N, N, N, N, N, 1, N, N;
%         N, 1, N, N, N, N, N, N, N, N, N, N, N, N, 2, N;
%         2, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 1;
%         1, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 2;
%         2, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 1;
%         1, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 2;
%         2, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 1;
%         1, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 2;
%         2, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 1;
%         1, N, N, N, N, N, N, N, N, N, N, N, N, N, N, 2;
%         N, 2, N, N, N, N, N, N, N, N, N, N, N, N, 1, N;
%         N, N, 1, N, N, N, N, N, N, N, N, N, N, 2, N, N;
%         N, N, N, 2, N, N, N, N, N, N, N, N, 1, N, N, N;
%         N, N, N, N, 1, 2, 1, 2, 1, 2, 1, 2, N, N, N, N;];

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

    try
        loadConfigFile();
    catch
        disp(sprintf('Could not load specific tests json file %s', jsonConfigFilename));
        return;
    end

    sequenceChanged(allHandles, true)

function index = findFrameNumberIndex(jsonData, sequenceNumberIndex, frameNumberIndex)
    index = -1;

    if ~isfield(jsonData.sequences{sequenceNumberIndex}, 'groundTruth')
        return;
    end

    for i = 1:length(jsonData.sequences{sequenceNumberIndex}.groundTruth)
        curFrameNumber = jsonData.sequences{sequenceNumberIndex}.groundTruth{i}.frameNumber;
        queryFrameNumber = jsonData.sequences{sequenceNumberIndex}.frameNumbers(frameNumberIndex);

    %     disp(sprintf('%d) %d %d', i, curFrameNumber, queryFrameNumber));

        if curFrameNumber == queryFrameNumber
            index = i;
            return;
        end
    end

    return;

function sanitizeGroundTruthJson()
% Don't call this function directly, call fixBounds() instead
    global jsonData;
    global curSequenceNumber;
    global curFrameNumber;

    % Add ground truth field, if not there already
    if ~isfield(jsonData.sequences{curSequenceNumber}, 'groundTruth')
        jsonData.sequences{curSequenceNumber}.groundTruth = {};
    end
    
    if ~iscell(jsonData.sequences{curSequenceNumber}.groundTruth)
        jsonData.sequences{curSequenceNumber}.groundTruth = {jsonData.sequences{curSequenceNumber}.groundTruth};
    end
    
    % For all indexes, remove empty sets of fiducialMarkerCorners
    for iSequence = 1:length(jsonData.sequences)
        for iFrame = 1:length(jsonData.sequences{iSequence}.groundTruth)
            if isfield(jsonData.sequences{iSequence}.groundTruth{iFrame}, 'fiducialMarkerCorners')
                if isempty(jsonData.sequences{iSequence}.groundTruth{iFrame}.fiducialMarkerCorners)
                    jsonData.sequences{iSequence}.groundTruth{iFrame}.fiducialMarkerCorners = {{}};
                end
                
                if ~iscell(jsonData.sequences{iSequence}.groundTruth{iFrame}.fiducialMarkerCorners{1})
                    jsonData.sequences{iSequence}.groundTruth{iFrame}.fiducialMarkerCorners = {jsonData.sequences{iSequence}.groundTruth{iFrame}.fiducialMarkerCorners};
                end
                
                numSets = length(jsonData.sequences{iSequence}.groundTruth{iFrame}.fiducialMarkerCorners);
                validSets = zeros(numSets,1);
                for iSet = 1:numSets
                    if ~isempty(jsonData.sequences{iSequence}.groundTruth{iFrame}.fiducialMarkerCorners{iSet})
                        validSets(iSet) = 1;
                    end
                end
                
                validSets = logical(validSets);
                jsonData.sequences{iSequence}.groundTruth{iFrame}.fiducialMarkerCorners = jsonData.sequences{iSequence}.groundTruth{iFrame}.fiducialMarkerCorners(validSets);
            end
        end
    end
    
    index = findFrameNumberIndex(jsonData, curSequenceNumber, curFrameNumber);

    if index ~= -1
        % Add errorSignalCorners, if not there already
        if ~isfield(jsonData.sequences{curSequenceNumber}.groundTruth{index}, 'errorSignalCorners') || isempty(jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners)
            jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners = {};
        end

        if ~iscell(jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners)
            jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners = {jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners};
        end

        % Add templateCorners, if not there already
        if ~isfield(jsonData.sequences{curSequenceNumber}.groundTruth{index}, 'templateCorners') || isempty(jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners)
            jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners = {};
        end

        if ~iscell(jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners)
            jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners = {jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners};
        end

        % Add fiducialMarkerCorners, if not there already
        if ~isfield(jsonData.sequences{curSequenceNumber}.groundTruth{index}, 'fiducialMarkerCorners') || isempty(jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners)
            jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners = {{}};
        end

%         if ~iscell(jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners{1})
%             jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners = {jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners};
%         end

        % Add an empty set at the end of the fiducialMarkerCorners list
        if ~isempty(jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners{end})
            jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners{end+1} = {};
        end
    end

function boundsFixed = fixBounds()
    global curTestNumber;
    global maxTestNumber;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;
    global curSet;
    global maxSet;
    global curDisplayType;
    global maxDisplayType;
    global pointsType;
    global jsonData;

    boundsFixed = false;

    curTestNumberOriginal = curTestNumber;
    curSequenceNumberOriginal = curSequenceNumber;
    curFrameNumberOriginal = curFrameNumber;
    curSetOriginal = curSet;

    curTestNumber = max(1, min(maxTestNumber, curTestNumber));
    curSequenceNumber = max(1, min(maxSequenceNumber, curSequenceNumber));
    curFrameNumber = max(1, min(maxFrameNumber, curFrameNumber));
    curDisplayType = max(1, min(maxDisplayType, curDisplayType));

    sanitizeGroundTruthJson();

    if ~strcmpi(pointsType, 'fiducialMarker')
        maxSet = 1;
    else
        index = findFrameNumberIndex(jsonData, curSequenceNumber, curFrameNumber);
        
        if index == -1
            maxSet = 1;
        else
            maxSet = length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners);
        end
    end

    curSet = max(1, min(maxSet, curSet));

    if curTestNumberOriginal ~= curTestNumber
        curSequenceNumber = 1;
        boundsFixed = true;
    elseif curSequenceNumberOriginal ~= curSequenceNumber || curFrameNumberOriginal ~= curFrameNumber || curSetOriginal ~= curSet || curDisplayType ~= maxDisplayType
        boundsFixed = true;
    end

function sequenceChanged(handles, resetAll, resetZoomOnly)
    global jsonConfigFilename;
    global jsonData;
    global dataPath;
    global curTestNumber;
    global maxTestNumber;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;
    global curSet;
    global maxSet;
    global curDisplayType;
    global maxDisplayType;
    global displayParameter1;
    global displayParameter2;
    global displayParameter3;
    global displayParameter4;
    global image;
    global imageFigureHandle;
    global imageHandle;
    global pointsType;
    global resolutionHorizontal;
    global resolutionVertical;

    if ~exist('resetAll', 'var')
        resetAll = false;
    end

    if ~exist('resetZoomOnly', 'var')
        resetZoomOnly = false;
    end

    fixBounds();

    if resetAll
        try
            loadConfigFile();
        catch
            disp(sprintf('Could not load specific tests json file %s', jsonConfigFilename));
            return;
        end

        maxSequenceNumber = length(jsonData.sequences);

        curFrameNumber = 1;
        maxFrameNumber = length(jsonData.sequences{curSequenceNumber}.frameNumbers);

        for i = 1:maxSequenceNumber
            if isfield(jsonData.sequences{i}, 'groundTruth') && ~iscell(jsonData.sequences{i}.groundTruth)
                jsonData.sequences{i}.groundTruth = {jsonData.sequences{i}.groundTruth};
            end
        end
%     else
%         if curFrameNumber < 1
%             curFrameNumber = 1;
%         end
%
%         if curFrameNumber > maxFrameNumber
%             curFrameNumber = maxFrameNumber;
%         end
    end

    curFilename = [dataPath, sprintf(jsonData.sequences{curSequenceNumber}.filenamePattern, jsonData.sequences{curSequenceNumber}.frameNumbers(curFrameNumber))];
    image = imread(curFilename);

    set(handles.curTest, 'String', num2str(curTestNumber))
    set(handles.maxTest, 'String', num2str(maxTestNumber))
    set(handles.curSequence, 'String', num2str(curSequenceNumber))
    set(handles.maxSequence, 'String', num2str(maxSequenceNumber))
    set(handles.curImage, 'String', num2str(curFrameNumber))
    set(handles.maxImage, 'String', num2str(maxFrameNumber))
    set(handles.curSet, 'String', num2str(curSet))
    set(handles.maxSet, 'String', num2str(maxSet))
    set(handles.curDisplayType, 'String', num2str(curDisplayType))
    set(handles.maxDisplayType, 'String', num2str(maxDisplayType))
    set(handles.displayParameter1, 'String', num2str(displayParameter1))
    set(handles.displayParameter2, 'String', num2str(displayParameter2))
    set(handles.displayParameter3, 'String', num2str(displayParameter3))
    set(handles.displayParameter4, 'String', num2str(displayParameter4))
    set(handles.resolutionHorizontal, 'String', num2str(resolutionHorizontal))
    set(handles.resolutionVertical, 'String', num2str(resolutionVertical))

    if curSequenceNumber==1 && curFrameNumber==1
        set(handles.templatePoints, 'Enable', 'on');
    else
        set(handles.templatePoints, 'Enable', 'off');
        if strcmpi(pointsType, 'template')
            pointsType = 'errorSignal';
        end
    end

    if strcmp(pointsType, 'errorSignal')
        set(handles.errorSignalPoints, 'Value', 1);
        set(handles.templatePoints, 'Value', 0);
        set(handles.fiducialMarkerPoints, 'Value', 0);
    elseif strcmp(pointsType, 'template')
        set(handles.errorSignalPoints, 'Value', 0);
        set(handles.templatePoints, 'Value', 1);
        set(handles.fiducialMarkerPoints, 'Value', 0);
    elseif strcmp(pointsType, 'fiducialMarker')
        set(handles.errorSignalPoints, 'Value', 0);
        set(handles.templatePoints, 'Value', 0);
        set(handles.fiducialMarkerPoints, 'Value', 1);
    else
        assert(false);
    end

    fixBounds();

    index = findFrameNumberIndex(jsonData, curSequenceNumber, curFrameNumber);

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
        set(handles.panelDisplayType, 'Title', 'Display Type: Original')
        set(handles.textDisplayParameter1, 'String', 'NULL')
        set(handles.textDisplayParameter2, 'String', 'NULL')
        set(handles.textDisplayParameter3, 'String', 'NULL')
        set(handles.textDisplayParameter4, 'String', 'NULL')
    elseif curDisplayType == 2
        % harris corner detected
        set(handles.panelDisplayType, 'Title', 'Display Type: Harris')
        set(handles.textDisplayParameter1, 'String', 'kernel width')
        set(handles.textDisplayParameter2, 'String', 'sigma')
        set(handles.textDisplayParameter3, 'String', 'NULL')
        set(handles.textDisplayParameter4, 'String', 'NULL')

%         kernel = fspecial('gaussian',[5 1],1.5); % the default kernel
        kernel = fspecial('gaussian',[displayParameter1 1], displayParameter2);
        c = cornermetric(imageResized, 'Harris', 'FilterCoefficients', kernel);
        cp = c - min(c(:));
        cp = cp / max(cp(:));
        imageResized = cp;
    elseif curDisplayType == 3
        % Shi-Tomasi corner detected
        set(handles.panelDisplayType, 'Title', 'Display Type: Shi-Tomasi')
        set(handles.textDisplayParameter1, 'String', 'kernel width')
        set(handles.textDisplayParameter2, 'String', 'sigma')
        set(handles.textDisplayParameter3, 'String', 'NULL')
        set(handles.textDisplayParameter4, 'String', 'NULL')

%         kernel = fspecial('gaussian',[5 1],1.5); % the default kernel
        kernel = fspecial('gaussian',[displayParameter1 1], displayParameter2);
        c = cornermetric(imageResized, 'MinimumEigenvalue', 'FilterCoefficients', kernel);
        cp = c - min(c(:));
        cp = cp / max(cp(:));
        imageResized = cp;
    elseif curDisplayType == 4
        % Quad-box template detected
        set(handles.panelDisplayType, 'Title', 'Display Type: Quad-Box')
        set(handles.textDisplayParameter1, 'String', 'kernel width')
        set(handles.textDisplayParameter2, 'String', 'sigma')
        set(handles.textDisplayParameter3, 'String', 'NULL')
        set(handles.textDisplayParameter4, 'String', 'NULL')

%         kernel = fspecial('gaussian',[5 1],1.5); % the default kernel
        h = 11;
        h2 = floor(h/2);

        % normal +-1 filter
%         kernel = ones(h,h);
%         kernel(h2:end, h2:end) = -1;
%         kernel = kernel / sum(kernel(:));
%
%         imageResized = imfilter(double(imageResized), kernel);
%         imageResized = imageResized - min(imageResized(:));
%         imageResized = imageResized / max(imageResized(:));
%         imageResized = 500.^(imageResized);
%         imageResized = imageResized / max(imageResized(:));

        % template

        kernel = 220*ones(h,h);
        kernel(h2:end, h2:end) = 40;

        imageResized = double(imageResized);

        filteredImageA = zeros(size(imageResized));
        for y = (1+h2):(size(imageResized,1)-h2)
            for x = (1+h2):(size(imageResized,2)-h2)
                sad = sum(sum(abs(imageResized((y-h2):(y+h2), (x-h2):(x+h2)) - kernel)));
                filteredImageA(y,x) = sad;
            end
        end

        kernel = imrotate(kernel, 270);
        filteredImageB = zeros(size(imageResized));
        for y = (1+h2):(size(imageResized,1)-h2)
            for x = (1+h2):(size(imageResized,2)-h2)
                sad = sum(sum(abs(imageResized((y-h2):(y+h2), (x-h2):(x+h2)) - kernel)));
                filteredImageB(y,x) = sad;
            end
        end

        filteredImage = min(filteredImageA, filteredImageB);

        imageResized = filteredImage;
        imageResized = imageResized / max(imageResized(:));
        imageResized = 10.^(imageResized);
        imageResized = imageResized / max(imageResized(:))*3;
%         imageResized = imageResized - .26;
%         imageResized = imageResized * 100;
    elseif curDisplayType == 5
        kernel = ones(5,5);
        kernel = kernel / sum(kernel(:));

        filteredImage = imfilter(imageResized, kernel);

        ulImage = zeros(size(imageResized));
        offset = 5;
        for y = (1+offset):(size(imageResized,1)-offset)
            for x = (1+offset):(size(imageResized,2)-offset)
                ulImage(y,x) = filteredImage(y-offset, x-offset)/3 +...
                               filteredImage(y-offset, x)/3 +...
                               filteredImage(y, x-offset)/3 -...
                               filteredImage(y, x);
            end
        end

        ulImage = ulImage / 255;

        figure(1); imshow(ulImage);

        figure(100);
        kernel = fspecial('gaussian',[displayParameter1 1], displayParameter2);
        c = cornermetric(imageResized, 'Harris', 'FilterCoefficients', kernel);
        cp = c - min(c(:));
        cp = cp / max(cp(:));
        imageResized = cp;
    end

    imageHandle = imshow(imageResized);

    if ~resetAll && ~resetZoomOnly
        xlim(originalXlim);
        ylim(originalYlim);
    end

    set(imageHandle,'ButtonDownFcn',@ButtonClicked);
    hold on;

    if index ~= -1
        xScaleInv = resolutionHorizontal / size(image,2);
        yScaleInv = resolutionVertical / size(image,1);

        if strcmp(pointsType, 'errorSignal')
            allCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners;

            if length(allCorners) == 2
                plotHandle = plot(xScaleInv*([allCorners{1}.x,allCorners{2}.x]+0.5), yScaleInv*([allCorners{1}.y,allCorners{2}.y]+0.5), 'r');
                set(plotHandle, 'HitTest', 'off')
            end

            for i = 1:length(allCorners)
                scatterHandle = scatter(xScaleInv*(allCorners{i}.x+0.5), yScaleInv*(allCorners{i}.y+0.5), 'r+');
                set(scatterHandle, 'HitTest', 'off')
            end
        elseif strcmp(pointsType, 'template')
            allCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners;

            if length(allCorners) == 4
                % first, sort the corners
                cornersX = [allCorners{1}.x, allCorners{2}.x, allCorners{3}.x, allCorners{4}.x];
                cornersY = [allCorners{1}.y, allCorners{2}.y, allCorners{3}.y, allCorners{4}.y];

                centerX = mean(cornersX);
                centerY = mean(cornersY);

                [thetas,~] = cart2pol(cornersX-centerX, cornersY-centerY);
                [~,sortedIndexes] = sort(thetas);

                for i = 1:4
                    jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners{i}.x = cornersX(sortedIndexes(i));
                    jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners{i}.y = cornersY(sortedIndexes(i));
                end

                allCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners;

                % second, plot the sorted corners
                plotHandle = plot(...
                    xScaleInv*([allCorners{1}.x,allCorners{2}.x,allCorners{3}.x,allCorners{4}.x,allCorners{1}.x]+0.5),...
                    yScaleInv*([allCorners{1}.y,allCorners{2}.y,allCorners{3}.y,allCorners{4}.y,allCorners{1}.y]+0.5),...
                    'b');
                set(plotHandle, 'HitTest', 'off')
            end

            for i = 1:length(allCorners)
                scatterHandle = scatter(xScaleInv*(allCorners{i}.x+0.5), yScaleInv*(allCorners{i}.y+0.5), 'b+');
                set(scatterHandle, 'HitTest', 'off')
            end
        elseif strcmp(pointsType, 'fiducialMarker')
            maxSet = length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners);

            for iSet = 1:length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners)
                if iSet == curSet
                    linePlotType = 'g';
                else
                    linePlotType = 'y';
                end

                allCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners{iSet};
                if length(allCorners) == 4
                    % first, sort the corners
                    cornersX = [allCorners{1}.x, allCorners{2}.x, allCorners{3}.x, allCorners{4}.x];
                    cornersY = [allCorners{1}.y, allCorners{2}.y, allCorners{3}.y, allCorners{4}.y];

                    centerX = mean(cornersX);
                    centerY = mean(cornersY);

                    [thetas,~] = cart2pol(cornersX-centerX, cornersY-centerY);
                    [~,sortedIndexes] = sort(thetas);

                    for i = 1:4
                        jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners{iSet}{i}.x = cornersX(sortedIndexes(i));
                        jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners{iSet}{i}.y = cornersY(sortedIndexes(i));
                    end

                    allCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners{iSet};

                    % second, plot the sorted corners
                    plotHandle = plot(...
                        xScaleInv*([allCorners{1}.x,allCorners{2}.x,allCorners{3}.x,allCorners{4}.x,allCorners{1}.x]+0.5),...
                        yScaleInv*([allCorners{1}.y,allCorners{2}.y,allCorners{3}.y,allCorners{4}.y,allCorners{1}.y]+0.5),...
                        linePlotType);
                    set(plotHandle, 'HitTest', 'off')
                end

                for i = 1:length(allCorners)
                    scatterHandle = scatter(xScaleInv*(allCorners{i}.x+0.5), yScaleInv*(allCorners{i}.y+0.5), [linePlotType,'+']);
                    set(scatterHandle, 'HitTest', 'off')
                end
            end % for iSet = 1:length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners)
        else
            assert(false);
        end

    end

function ButtonClicked(hObject, eventdata, handles)
    global jsonConfigFilename;
    global jsonData;
    global curSequenceNumber;
    global curFrameNumber;
    global curSet;
    global maxSet;
    global image;
    global allHandles;
    global imageFigureHandle;
    global imageHandle;
    global pointsType;
    global maxErrorSignalCorners;
    global maxTemplateCorners;
    global maxFiducialMarkerCorners;
    global resolutionHorizontal;
    global resolutionVertical;

    axesHandle  = get(imageHandle,'Parent');
    imPosition = get(axesHandle, 'CurrentPoint') - 0.5;

    imPosition = imPosition(1,1:2);

    if imPosition(1) < 0 || imPosition(2) < 0 || imPosition(1) > size(image,2) || imPosition(2) > size(image,1)
        return;
    end

    fixBounds();

    index = findFrameNumberIndex(jsonData, curSequenceNumber, curFrameNumber);

    if index == -1
        allFrameNumbers = jsonData.sequences{curSequenceNumber}.frameNumbers;
        jsonData.sequences{curSequenceNumber}.groundTruth{end+1}.frameNumber = allFrameNumbers(curFrameNumber);
        index = length(jsonData.sequences{curSequenceNumber}.groundTruth);
    end

    % disp(index)
    buttonType = get(imageFigureHandle,'selectionType');
    if strcmp(buttonType, 'normal') % left click
        xScale = size(image,2) / resolutionHorizontal;
        yScale = size(image,1) / resolutionVertical;

        newPoint.x = imPosition(1) * xScale;
        newPoint.y = imPosition(2) * yScale;

        if strcmp(pointsType, 'errorSignal')
            if(length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners) < maxErrorSignalCorners)
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners{end+1} = newPoint;
            else
                disp(sprintf('Cannot add point, because only %d error signal corners are allowed', maxErrorSignalCorners));
            end
        elseif strcmp(pointsType, 'template')
            if(length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners) < maxTemplateCorners)
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners{end+1} = newPoint;
             else
                disp(sprintf('Cannot add point, because only %d template corners are allowed', maxTemplateCorners));
            end
        elseif strcmp(pointsType, 'fiducialMarker')
            if(length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners{curSet}) < maxFiducialMarkerCorners)
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners{curSet}{end+1} = newPoint;
            else
                disp(sprintf('Cannot add point, because only %d fiduciual marker corners are allowed', maxFiducialMarkerCorners));
            end
        end

        savejson('',jsonData,jsonConfigFilename);
    elseif strcmp(buttonType, 'alt') % right click
        xScaleInv = resolutionHorizontal / size(image,2);
        yScaleInv = resolutionVertical / size(image,1);

        minDist = Inf;
        minInd = -1;

        if strcmp(pointsType, 'errorSignal')
            for i = 1:length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners)
                curCorner = jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners{i};
                dist = sqrt((curCorner.x*xScaleInv - imPosition(1))^2 + (curCorner.y*yScaleInv - imPosition(2))^2);
                if dist < minDist
                    minDist = dist;
                    minInd = i;
                end
            end

            if minInd ~= -1 && minDist < (min(size(image,1),size(image,2))/50)
                newCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners([1:(minInd-1),(minInd+1):end]);
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners = newCorners;
                savejson('',jsonData,jsonConfigFilename);
            end
        elseif strcmp(pointsType, 'template')
            for i = 1:length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners)
                curCorner = jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners{i};
                dist = sqrt((curCorner.x*xScaleInv - imPosition(1))^2 + (curCorner.y*yScaleInv - imPosition(2))^2);
                if dist < minDist
                    minDist = dist;
                    minInd = i;
                end
            end

            if minInd ~= -1 && minDist < (min(size(image,1),size(image,2))/50)
                newCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners([1:(minInd-1),(minInd+1):end]);
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners = newCorners;
                savejson('',jsonData,jsonConfigFilename);
            end
        elseif strcmp(pointsType, 'fiducialMarker')
            for i = 1:length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners{curSet})
                curCorner = jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners{curSet}{i};
                dist = sqrt((curCorner.x*xScaleInv - imPosition(1))^2 + (curCorner.y*yScaleInv - imPosition(2))^2);
                if dist < minDist
                    minDist = dist;
                    minInd = i;
                end
            end

            if minInd ~= -1 && minDist < (min(size(image,1),size(image,2))/50)
                newCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners{curSet}([1:(minInd-1),(minInd+1):end]);
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.fiducialMarkerCorners{curSet} = newCorners;
                savejson('',jsonData,jsonConfigFilename);
            end
        end
    end

    sequenceChanged(allHandles);



