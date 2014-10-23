function varargout = compareImageResults(varargin)
    % COMPAREIMAGERESULTS MATLAB code for compareImageResults.fig
    %      COMPAREIMAGERESULTS, by itself, creates a new COMPAREIMAGERESULTS or raises the existing
    %      singleton*.
    %
    %      H = COMPAREIMAGERESULTS returns the handle to a new COMPAREIMAGERESULTS or the handle to
    %      the existing singleton*.
    %
    %      COMPAREIMAGERESULTS('CALLBACK',hObject,eventData,handles,...) calls the local
    %      function named CALLBACK in COMPAREIMAGERESULTS.M with the given input arguments.
    %
    %      COMPAREIMAGERESULTS('Property','Value',...) creates a new COMPAREIMAGERESULTS or raises the
    %      existing singleton*.  Starting from the left, property value pairs are
    %      applied to the GUI before compareImageResults_OpeningFcn gets called.  An
    %      unrecognized property name or invalid value makes property application
    %      stop.  All inputs are passed to compareImageResults_OpeningFcn via varargin.
    %
    %      *See GUI Options on GUIDE's Tools menu.  Choose "GUI allows only one
    %      instance to run (singleton)".
    %
    % See also: GUIDE, GUIDATA, GUIHANDLES
    
    % Edit the above text to modify the response to help compareImageResults
    
    % Last Modified by GUIDE v2.5 23-Oct-2014 15:49:32
       
    % Begin initialization code - DO NOT EDIT
    gui_Singleton = 1;
    gui_State = struct('gui_Name',       mfilename, ...
        'gui_Singleton',  gui_Singleton, ...
        'gui_OpeningFcn', @compareImageResults_OpeningFcn, ...
        'gui_OutputFcn',  @compareImageResults_OutputFcn, ...
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
    
    
    % --- Executes just before compareImageResults is made visible.
function compareImageResults_OpeningFcn(hObject, eventdata, handles, varargin)
    % This function has no output args, see OutputFcn.
    % hObject    handle to figure
    % eventdata  reserved - to be defined in a future version of MATLAB
    % handles    structure with handles and user data (see GUIDATA)
    % varargin   command line arguments to compareImageResults (see VARARGIN)
    
    % Choose default command line output for compareImageResults
    handles.output = hObject;
    
    % Update handles structure
    guidata(hObject, handles);
    
    % UIWAIT makes compareImageResults wait for user response (see UIRESUME)
    % uiwait(handles.figure1);
    
    global filenamePatterns;
    global imageFilenames;
    global curImageIndex;
    global maxImageIndex;
    global stopLoading;
    global playWaitTime;
    
    filenamePatterns = {...
        '~/Documents/Anki/products-cozmo-large-files/systemTestsData/results/dateTime_2014-10-23_13-41-34/images/c-with-refinement/*.png',...
        '~/Documents/Anki/products-cozmo-large-files/systemTestsData/results/dateTime_2014-10-23_13-41-34/images/c-with-refinement-binomial/*.png',...
        '',...
        ''};
        
    curImageIndex = 1;
    maxImageIndex = 1;
    stopLoading = zeros(length(filenamePatterns), 1);
    imageFilenames = cell(length(filenamePatterns), 1);
    playWaitTime = 0.1;
    
    for iPattern = 1:length(filenamePatterns)
        imageFilenames{iPattern} = rdir(filenamePatterns{iPattern});
    end
        
    LoadImages(1);
    LoadImages(2);
    LoadImages(3);
    LoadImages(4);
    
    ShowImages()
            
    % --- Outputs from this function are returned to the command line.
function varargout = compareImageResults_OutputFcn(hObject, eventdata, handles)
    % varargout  cell array for returning output args (see VARARGOUT);
    % hObject    handle to figure
    % eventdata  reserved - to be defined in a future version of MATLAB
    % handles    structure with handles and user data (see GUIDATA)
    
    % Get default command line output from handles structure
    varargout{1} = handles.output;
    
function setDefaultGuiObjectColor(hObject)
    if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
        set(hObject,'BackgroundColor','white');
    end    
    
    %
    % Create Functions
    %
    
function filenamePattern1_CreateFcn(hObject, ~, ~)
    global filenamePatterns;
    setDefaultGuiObjectColor(hObject);
    set(hObject, 'String', filenamePatterns{1});  
    
function filenamePattern2_CreateFcn(hObject, ~, ~)
    global filenamePatterns;
    setDefaultGuiObjectColor(hObject);
    set(hObject, 'String', filenamePatterns{2});
    
function filenamePattern3_CreateFcn(hObject, ~, ~)
    global filenamePatterns;
    setDefaultGuiObjectColor(hObject);
    set(hObject, 'String', filenamePatterns{3});
    
function filenamePattern4_CreateFcn(hObject, ~, ~)
    global filenamePatterns;
    setDefaultGuiObjectColor(hObject);
    set(hObject, 'String', filenamePatterns{4});
        
function playWait_CreateFcn(hObject, eventdata, handles)
    global playWaitTime;
    setDefaultGuiObjectColor(hObject);
    set(hObject, 'String', num2str(playWaitTime));
    
    %
    % End Create Functions
    %
    
    %
    % Callback Functions
    %
    
function filenamePattern1_Callback(hObject, ~, ~)
    global filenamePatterns;
    filenamePatterns{1} = get(hObject,'String');
    LoadImages(1);
    
function filenamePattern2_Callback(hObject, ~, ~)
    global filenamePatterns;
    filenamePatterns{2} = get(hObject,'String');
    LoadImages(2);
    
function filenamePattern3_Callback(hObject, ~, ~)
    global filenamePatterns;
    filenamePatterns{3} = get(hObject,'String');
    LoadImages(3);
        
function filenamePattern4_Callback(hObject, ~, ~)
    global filenamePatterns;
    filenamePatterns{4} = get(hObject,'String');
    LoadImages(4);
        
function previousImage1_Callback(~, ~, ~)
    global curImageIndex;
    curImageIndex = max(1, curImageIndex - 1);
    ShowImages();
    
function previousImage2_Callback(~, ~, ~)
    global curImageIndex;
    curImageIndex = max(1, curImageIndex - 10);
    ShowImages();

function previousImage3_Callback(~, ~, ~)
    global curImageIndex;
    curImageIndex = max(1, curImageIndex - 100);
    ShowImages();

function nextImage1_Callback(~, ~, ~)
    global curImageIndex;
    global maxImageIndex;    
    curImageIndex = min(maxImageIndex, curImageIndex + 1);
    ShowImages();

function nextImage2_Callback(~, ~, ~)
    global curImageIndex;
    global maxImageIndex;    
    curImageIndex = min(maxImageIndex, curImageIndex + 10);
    ShowImages();

function nextImage3_Callback(~, ~, ~)
    global curImageIndex;
    global maxImageIndex;    
    curImageIndex = min(maxImageIndex, curImageIndex + 100);
    ShowImages();

function playButton_Callback(~, ~, ~)
    global playWaitTime;
    global playing;
    global curImageIndex;
    global maxImageIndex;
    
    playing = true;
    while playing && curImageIndex <= maxImageIndex
        t0 = tic();
        curImageIndex = curImageIndex + 1;
        ShowImages();
        t1 = toc(t0);
        pause(playWaitTime - t1);
    end
    
    curImageIndex = min(curImageIndex, maxImageIndex);

function stopButton_Callback(~, ~, ~)
    global playing;
    playing = false;
    
function playWait_Callback(~, ~, ~)
    global playWaitTime;
    playWaitTime = str2num(get(hObject,'String'));
    
    %
    % End Callback Functions
    %

function LoadImages(whichPattern)
    global filenamePatterns;
    global imageFilenames;
    global images;
    global maxImageIndex;
    global stopLoading;
    
    if isempty(filenamePatterns{whichPattern})
        return;
    end
    
    imageFilenames{whichPattern} = rdir(filenamePatterns{whichPattern});
    
    % Should be a mutex to work correctly always, but this will work most of the time
    stopLoading(whichPattern) = true;
    
    for iPattern = 1:length(filenamePatterns)
        if ~isempty(filenamePatterns{whichPattern}) && ~isempty(imageFilenames{iPattern})
            if length(imageFilenames{whichPattern}) ~= length(imageFilenames{iPattern})
                disp('Different number of images in these directories')
                keyboard
            end
        end
    end
    
    maxImageIndex = length(imageFilenames{whichPattern});
    
    images{whichPattern} = cell(length(imageFilenames{whichPattern}),1);
    
    pause(.1);
    stopLoading(whichPattern) = false;
    
    disp(sprintf('Loading images for %d', whichPattern))
    % This loading will go on in the background
    for iFile = 1:length(imageFilenames{whichPattern})
        if stopLoading(whichPattern)
            break;
        end
        
%         tic
%         images{iPattern}{iFile} = imread(imageFilenames{whichPattern}(iFile).name);
%         toc
    end
    disp(sprintf('Done loading images for %d', whichPattern))
    
function ShowImages()
    global filenamePatterns;
    global imageFilenames;
    global images;
    global curImageIndex;
    
    validPatternIndexes = [];
    for i = 1:length(filenamePatterns)
        if ~isempty(filenamePatterns{i})
            validPatternIndexes(end+1) = i; %#ok<AGROW>
        end
    end
    
    numRows = ceil(sqrt(length(validPatternIndexes)));
    numCols = ceil(length(validPatternIndexes) / numRows);
    
    for iPattern = 1:length(validPatternIndexes)
        figure(200);
        subplot(numRows, numCols, iPattern);
        
        curFilename = imageFilenames{validPatternIndexes(iPattern)}(curImageIndex).name;
        
        if isempty(images{validPatternIndexes(iPattern)})
            curImage = [];
        else
            curImage = images{validPatternIndexes(iPattern)}{curImageIndex};
        end
        
        % If the image is not cached yet, load and cache it
        if isempty(curImage)
            curImage = imread(curFilename);
            images{validPatternIndexes(iPattern)}{curImageIndex} = curImage;
        end
        
        imshow(curImage);
                
        slashIndex = find(curFilename == '/');
        title(curFilename((slashIndex(end)+1):end))
    end
    
    
    
    




