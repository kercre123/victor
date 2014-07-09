% function imshows(varargin)

% imshows(im1, im2, im3); %for separate figures
% imshows({im1, im2, im3}, {im4, im5}); %for subplots
% imshows(im1, [], [], im4); % don't modify figures 2 and 3
% imshows(im1, im2, 'maximize'); % imshow all windows at full size
% imshows(im1, im2, 4); % start figure numbering at 4. In this example, imshow on figures 4 and 5.

function imshows(varargin)
    
    figNums = 1:length(varargin);
    
    maximizeWindows = false;
    for i = 1:length(varargin)
        if ischar(varargin{i})
            if strcmpi(varargin{i}, 'maximize')
                maximizeWindows = true;
            end
        elseif min(size(varargin{i}) == [1,1]) == 1
            figNums = figNums - 1 + varargin{i};
        end
    end
    
    for i = 1:length(varargin)
        if ischar(varargin{i}) || min(size(varargin{i}) == [1,1]) == 1 || isempty(varargin{i})
            continue;
        end
        
        figureHandle = figure(figNums(i));
        curIms = varargin{i};
        if iscell(curIms)
            for j = 1:length(curIms)
                subplot(1,length(curIms),j); imshow(curIms{j});
                if maximizeWindows
                    set(figureHandle, 'units','normalized','outerposition',[0 0 1 1]);
                end
            end
        else
            imshow(curIms, 'Border', 'tight');
            if maximizeWindows
                set(figureHandle, 'units','normalized','outerposition',[0 0 1 1]);
            end
        end
        impixelinfo;
    end