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

function loadConfigFile()
    global jsonAllTestsData;
    global curTestNumber;
    global maxTestNumber;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;
    global jsonConfigFilename;
    global jsonData;
    global dataPath;
    
    jsonConfigFilename = [dataPath, jsonAllTestsData.tests{curTestNumber}];
    jsonConfigFilename = strrep(jsonConfigFilename, '\', '/');
    jsonData = loadjson(jsonConfigFilename);

    slashIndexes = strfind(jsonConfigFilename, '/');
    dataPath = jsonConfigFilename(1:(slashIndexes(end)));
    
    if ~iscell(jsonData.sequences)
        jsonData.sequences = {jsonData.sequences};
    end
        
function loadAllTestsFile()
    global jsonAllTestsFilename;
    global jsonAllTestsData;
    global curTestNumber;
    global maxTestNumber;
    global curSequenceNumber;
    global maxSequenceNumber;
    global image;    
    global imageFigureHandle;
    global imageHandle;
    global allHandles;

    imageFigureHandle = figure(100);
    
    jsonAllTestsFilename = get(allHandles.configFilename, 'String');
    jsonAllTestsFilename = strrep(jsonAllTestsFilename, '\', '/');
    jsonAllTestsData = loadjson(jsonAllTestsFilename);
    
    curTestNumber = 1;
    maxTestNumber = length(jsonAllTestsData.tests);
    
    curSequenceNumber = 1;
    
    curImageNumber = 1;
    
    loadConfigFile();
    
    image = rand([480,640]);  

    pointer = [
        nan, nan, nan, nan, 2,   2,   2,   2,   2,   2,   2,   2,   nan, nan, nan, nan;
        nan, nan, nan, 2,   nan, nan, nan, nan, nan, nan, nan, nan, 2,   nan, nan, nan;
        nan, nan, 2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2,   nan, nan;
        nan, 2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2,   nan;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2;
        nan, 2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2,   nan;
        nan, nan, 2,   nan, nan, nan, nan, nan, nan, nan, nan, nan, nan, 2,   nan, nan;
        nan, nan, nan, 2,   nan, nan, nan, nan, nan, nan, nan, nan, 2,   nan, nan, nan;
        nan, nan, nan, nan, 2,   2,   2,   2,   2,   2,   2,   2,   nan, nan, nan, nan;];

    
    set(imageFigureHandle,'Pointer','custom','PointerShapeCData',pointer,'PointerShapeHotSpot',(size(pointer))/2)

    sequenceChanged(allHandles, true)

function editGroundTruth_OpeningFcn(hObject, eventdata, handles, varargin)
    % Choose default command line output for editGroundTruth
    handles.output = hObject;

    % Update handles structure
    guidata(hObject, handles);

    % UIWAIT makes editGroundTruth wait for user response (see UIRESUME)
    % uiwait(handles.figure1);

    global allHandles;
    allHandles = handles;
    
    loadAllTestsFile()

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

function varargout = editGroundTruth_OutputFcn(hObject, eventdata, handles)
    % Get default command line output from handles structure
    varargout{1} = handles.output;

function previousSequence_Callback(hObject, eventdata, handles) %#ok<*INUSL>
    global curTestNumber;
    global maxTestNumber;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    if curSequenceNumber > 1
        curSequenceNumber = curSequenceNumber - 1;
    end

    sequenceChanged(handles, true);

function nextSequence_Callback(hObject, eventdata, handles)
    global curTestNumber;
    global maxTestNumber;    
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    if curSequenceNumber < maxSequenceNumber
        curSequenceNumber = curSequenceNumber + 1;
    end

    sequenceChanged(handles, true);

function curSequence_Callback(hObject, eventdata, handles)
    global curTestNumber;
    global maxTestNumber;    
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    curSequenceNumberNew = str2double(get(hObject,'String'));

    if curSequenceNumberNew <= maxSequenceNumber && curSequenceNumberNew >= 1
        curSequenceNumber = curSequenceNumberNew;
    end

    sequenceChanged(handles);

function previousImage_Callback(hObject, eventdata, handles) %#ok<*DEFNU>
    global curTestNumber;
    global maxTestNumber;    
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    if curFrameNumber > 1
        curFrameNumber = curFrameNumber - 1;
    end

    sequenceChanged(handles);

function nextImage_Callback(hObject, eventdata, handles)
    global curTestNumber;
    global maxTestNumber;
    global curSequenceNumber; %#ok<*NUSED>
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    if curFrameNumber < maxFrameNumber
        curFrameNumber = curFrameNumber + 1;
    end

    sequenceChanged(handles);

function curImage_Callback(hObject, eventdata, handles)
    global curTestNumber;
    global maxTestNumber;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    curFrameNumberNew = str2double(get(hObject,'String'));

    if curFrameNumberNew <= maxSequenceNumber && curFrameNumberNew >= 1
        curFrameNumber = curFrameNumberNew;
    end

    sequenceChanged(handles);

function curSequence_CreateFcn(hObject, eventdata, handles) %#ok<*INUSD>
    global curTestNumber;
    global maxTestNumber;    
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
        set(hObject,'BackgroundColor','white');
    end

    set(hObject,'String',num2str(curSequenceNumber));

function curImage_CreateFcn(hObject, eventdata, handles)
    global curTestNumber;
    global maxTestNumber;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
        set(hObject,'BackgroundColor','white');
    end

    set(hObject,'String',num2str(curFrameNumber));

function maxSequence_CreateFcn(hObject, eventdata, handles)
    global curTestNumber;
    global maxTestNumber;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    set(hObject,'String',num2str(maxSequenceNumber));

function maxImage_CreateFcn(hObject, eventdata, handles)
    global curTestNumber;
    global maxTestNumber;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    set(hObject,'String',num2str(maxFrameNumber));

function sequenceChanged(handles, resetAll)
    global jsonData;
    global dataPath;
    global curTestNumber;
    global maxTestNumber;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;
    global image;
    global allHandles;
    global imageFigureHandle;
    global imageHandle;
            
    if ~exist('resetAll', 'var')
        resetAll = false;
    end
    
    if curSequenceNumber < 1
        curSequenceNumber = 1;
    end

    if curSequenceNumber > maxSequenceNumber
        curSequenceNumber = maxSequenceNumber;
    end
    
    if resetAll
        loadConfigFile();
        
        maxSequenceNumber = length(jsonData.sequences);

        curFrameNumber = 1;
        maxFrameNumber = length(jsonData.sequences{curSequenceNumber}.frameNumbers);
       
        for i = 1:maxSequenceNumber
            if isfield(jsonData.sequences{i}, 'groundTruth') && ~iscell(jsonData.sequences{i}.groundTruth)
                jsonData.sequences{i}.groundTruth = {jsonData.sequences{i}.groundTruth};
            end
        end
    else
        if curFrameNumber < 1
            curFrameNumber = 1;
        end

        if curFrameNumber > maxFrameNumber
            curFrameNumber = maxFrameNumber;
        end
    end
    
    curFilename = [dataPath, sprintf(jsonData.sequences{curSequenceNumber}.filenamePattern, jsonData.sequences{curSequenceNumber}.frameNumbers(curFrameNumber))];
    image = imread(curFilename);

    set(handles.curTest, 'String', num2str(curTestNumber))
    set(handles.maxTest, 'String', num2str(maxTestNumber))
    set(handles.curSequence, 'String', num2str(curSequenceNumber))
    set(handles.maxSequence, 'String', num2str(maxSequenceNumber))
    set(handles.curImage, 'String', num2str(curFrameNumber))
    set(handles.maxImage, 'String', num2str(maxFrameNumber))

    index = findFrameNumberIndex(jsonData, curSequenceNumber, curFrameNumber);

    imageFigureHandle = figure(100);
    
    hold off;
    
    originalXlim = xlim();
    originalYlim = ylim();

    imageHandle = imshow(image);

    if ~resetAll
        xlim(originalXlim);
        ylim(originalYlim);
    end

    set(imageHandle,'ButtonDownFcn',@axes1_ButtonDownFcn);
    hold on;

    if index ~= -1
        if isempty(jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners)
            jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners = {};
        end
        
        if ~iscell(jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners)
            jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners = {jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners};
        end
        
        allCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners;
        
        for i = 1:length(allCorners)
            scatterHandle = scatter(allCorners{i}.x+0.5, allCorners{i}.y+0.5, 'r+');
            set(scatterHandle, 'HitTest', 'off')
        end
    end

function configFilename_Callback(hObject, eventdata, handles)
    loadAllTestsFile()

function configFilename_CreateFcn(hObject, eventdata, handles)
    if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
        set(hObject,'BackgroundColor','white');
    end

function axes1_ButtonDownFcn(hObject, eventdata, handles)
    global jsonConfigFilename;
    global jsonData;
    global dataPath;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;
    global image;
    global allHandles;
    global imageFigureHandle;
    global imageHandle;

    axesHandle  = get(imageHandle,'Parent');
    imPosition = get(axesHandle, 'CurrentPoint') - 0.5;

    imPosition = imPosition(1,1:2);

    if imPosition(1) < 0 || imPosition(2) < 0 || imPosition(1) > size(image,2) || imPosition(2) > size(image,1)
        return;
    end

    index = findFrameNumberIndex(jsonData, curSequenceNumber, curFrameNumber);

    if index == -1
        if ~isfield(jsonData.sequences{curSequenceNumber}, 'groundTruth')
            jsonData.sequences{curSequenceNumber}.groundTruth = [];
        end
        
        allFrameNumbers = jsonData.sequences{curSequenceNumber}.frameNumbers;
        jsonData.sequences{curSequenceNumber}.groundTruth{end+1}.frameNumber = allFrameNumbers(curFrameNumber);
        jsonData.sequences{curSequenceNumber}.groundTruth{end}.corners = [];
        index = length(jsonData.sequences{curSequenceNumber}.groundTruth);
    end

    % disp(index)
    buttonType = get(imageFigureHandle,'selectionType');
    if strcmp(buttonType, 'normal') % left click
        newPoint.x = imPosition(1);
        newPoint.y = imPosition(2);

        if isempty(jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners)
            jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners = {};
        end
        
        if ~iscell(jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners)
            jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners = {jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners};
        end
        
        jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners{end+1} = newPoint;
        savejson('',jsonData,jsonConfigFilename);
    elseif strcmp(buttonType, 'alt') % right click
        minDist = Inf;
        minInd = -1;

        for i = 1:length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners)
            curCorner = jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners{i};
            dist = sqrt((curCorner.x - imPosition(1))^2 + (curCorner.y - imPosition(2))^2);
            if dist < minDist
                minDist = dist;
                minInd = i;
            end
        end

        
        if minInd ~= -1 && minDist < (min(size(image,1),size(image,2))/50)
            newCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners([1:(minInd-1),(minInd+1):end]);
            jsonData.sequences{curSequenceNumber}.groundTruth{index}.corners = newCorners;
            savejson('',jsonData,jsonConfigFilename);
        end
    end

    sequenceChanged(allHandles);

function figure1_ButtonDownFcn(hObject, eventdata, handles)

function figure1_CreateFcn(hObject, eventdata, handles)

function previousSequence_CreateFcn(hObject, eventdata, handles)

function nextSequence_CreateFcn(hObject, eventdata, handles)

function previousImage_CreateFcn(hObject, eventdata, handles)

function nextImage_CreateFcn(hObject, eventdata, handles)

function configFilenameNoteText_CreateFcn(hObject, eventdata, handles)

function previousTest_CreateFcn(hObject, eventdata, handles)

function maxTest_CreateFcn(hObject, eventdata, handles)
    global maxTestNumber;
    set(hObject,'String',num2str(maxTestNumber));

function nextTest_CreateFcn(hObject, eventdata, handles)

function curTest_CreateFcn(hObject, eventdata, handles)
    if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
        set(hObject,'BackgroundColor','white');
    end

    global curTestNumber;
    set(hObject,'String',num2str(curTestNumber));

function previousTest_Callback(hObject, eventdata, handles)
    global curTestNumber;
    global maxTestNumber;
    global curSequenceNumber;
    global maxSequenceNumber;
    global curFrameNumber;
    global maxFrameNumber;

    if curTestNumber > 1
        curTestNumber = curTestNumber - 1;
        curSequenceNumber = 1;
    end
    
    sequenceChanged(handles, true);
    
function nextTest_Callback(hObject, eventdata, handles)
    global curTestNumber;
    global maxTestNumber;
    
    if curTestNumber < maxTestNumber
        curTestNumber = curTestNumber + 1;
        curSequenceNumber = 1;
    end

    sequenceChanged(handles, true);

function curTest_Callback(hObject, eventdata, handles)
    global curTestNumber;
    global maxTestNumber;
    
    curTestNumberNew = str2double(get(hObject,'String'));

    if curTestNumberNew <= maxTestNumber && curTestNumberNew >= 1
        curTestNumber = curTestNumberNew;
    end
    
    curSequenceNumber = 1;

    sequenceChanged(handles, true);

function errorSignalPoints_Callback(hObject, eventdata, handles)
    global pointsType;
    if get(hObject,'Value')
        pointsType = 'errorSignal';
    end
    
    sequenceChanged(handles);

function templatePoints_Callback(hObject, eventdata, handles)
    global pointsType;
    if get(hObject,'Value')
        pointsType = 'template';
    end
    
    sequenceChanged(handles);
    
    
