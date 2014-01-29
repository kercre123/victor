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

global maxErrorSignalCorners;
global maxTemplateCorners;

maxErrorSignalCorners = 2;
maxTemplateCorners = 4;

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
    global maxSequenceNumber;
    global image;
    global imageFigureHandle;
    global imageHandle;
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

    curImageNumber = 1;

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

    try
        loadConfigFile();
    catch
        disp(sprintf('Could not load specific tests json file %s', jsonConfigFilename));
        return;
    end

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
    global jsonConfigFilename;
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
    global pointsType;

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

    if curSequenceNumber==1 && curFrameNumber==1
        set(handles.templatePoints, 'Enable', 'on');
    else
        set(handles.templatePoints, 'Enable', 'off');
        pointsType = 'errorSignal';
    end

    if strcmp(pointsType, 'errorSignal')
        set(handles.errorSignalPoints, 'Value', 1);
        set(handles.templatePoints, 'Value', 0);
    elseif strcmp(pointsType, 'template')
        set(handles.errorSignalPoints, 'Value', 0);
        set(handles.templatePoints, 'Value', 1);
    end

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
        if strcmp(pointsType, 'errorSignal')
            if ~isfield(jsonData.sequences{curSequenceNumber}.groundTruth{index}, 'errorSignalCorners') || isempty(jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners)
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners = {};
            end

            if ~iscell(jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners)
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners = {jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners};
            end

            allCorners = jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners;
            plotType = 'r+';

            if length(allCorners) == 2
                plotHandle = plot([allCorners{1}.x,allCorners{2}.x]+0.5, [allCorners{1}.y,allCorners{2}.y]+0.5, 'r');
                set(plotHandle, 'HitTest', 'off')
            end
        elseif strcmp(pointsType, 'template')
            if ~isfield(jsonData.sequences{curSequenceNumber}.groundTruth{index}, 'templateCorners') || isempty(jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners)
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners = {};
            end

            if ~iscell(jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners)
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners = {jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners};
            end

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
                    [allCorners{1}.x,allCorners{2}.x,allCorners{3}.x,allCorners{4}.x,allCorners{1}.x]+0.5,...
                    [allCorners{1}.y,allCorners{2}.y,allCorners{3}.y,allCorners{4}.y,allCorners{1}.y]+0.5,...
                    'b');
                set(plotHandle, 'HitTest', 'off')
            end

            plotType = 'b+';
        end

        for i = 1:length(allCorners)
            scatterHandle = scatter(allCorners{i}.x+0.5, allCorners{i}.y+0.5, plotType);
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
    global pointsType;
    global maxErrorSignalCorners;
    global maxTemplateCorners;

    axesHandle  = get(imageHandle,'Parent');
    imPosition = get(axesHandle, 'CurrentPoint') - 0.5;

    imPosition = imPosition(1,1:2);

    if imPosition(1) < 0 || imPosition(2) < 0 || imPosition(1) > size(image,2) || imPosition(2) > size(image,1)
        return;
    end

    index = findFrameNumberIndex(jsonData, curSequenceNumber, curFrameNumber);

    if index == -1
        if ~isfield(jsonData.sequences{curSequenceNumber}, 'groundTruth')
            jsonData.sequences{curSequenceNumber}.groundTruth = {};
        end

        allFrameNumbers = jsonData.sequences{curSequenceNumber}.frameNumbers;
        jsonData.sequences{curSequenceNumber}.groundTruth{end+1}.frameNumber = allFrameNumbers(curFrameNumber);
%         jsonData.sequences{curSequenceNumber}.groundTruth{end}.errorSignalCorners = {};
        index = length(jsonData.sequences{curSequenceNumber}.groundTruth);
    end

    % disp(index)
    buttonType = get(imageFigureHandle,'selectionType');
    if strcmp(buttonType, 'normal') % left click
        newPoint.x = imPosition(1);
        newPoint.y = imPosition(2);

        if strcmp(pointsType, 'errorSignal')
            if ~isfield(jsonData.sequences{curSequenceNumber}.groundTruth{index}, 'errorSignalCorners') || isempty(jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners)
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners = {};
            end

            if ~iscell(jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners)
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners = {jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners};
            end

            if(length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners) < maxErrorSignalCorners)
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners{end+1} = newPoint;
            else
                disp(sprintf('Cannot add point, because only %d error signal corners are allowed', maxErrorSignalCorners));
            end
        elseif strcmp(pointsType, 'template')
            if ~isfield(jsonData.sequences{curSequenceNumber}.groundTruth{index}, 'templateCorners') || isempty(jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners)
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners = {};
            end

            if ~iscell(jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners)
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners = {jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners};
            end

            if(length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners) < maxTemplateCorners)
                jsonData.sequences{curSequenceNumber}.groundTruth{index}.templateCorners{end+1} = newPoint;
             else
                disp(sprintf('Cannot add point, because only %d template corners are allowed', maxTemplateCorners));
            end
        end

        savejson('',jsonData,jsonConfigFilename);
    elseif strcmp(buttonType, 'alt') % right click
        minDist = Inf;
        minInd = -1;

        if strcmp(pointsType, 'errorSignal')
            for i = 1:length(jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners)
                curCorner = jsonData.sequences{curSequenceNumber}.groundTruth{index}.errorSignalCorners{i};
                dist = sqrt((curCorner.x - imPosition(1))^2 + (curCorner.y - imPosition(2))^2);
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
                dist = sqrt((curCorner.x - imPosition(1))^2 + (curCorner.y - imPosition(2))^2);
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
    global curSequenceNumber;
    global curTestNumber;
    global maxTestNumber;

    if curTestNumber < maxTestNumber
        curTestNumber = curTestNumber + 1;
        curSequenceNumber = 1;
    end

    sequenceChanged(handles, true);

function curTest_Callback(hObject, eventdata, handles)
    global curSequenceNumber;
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


