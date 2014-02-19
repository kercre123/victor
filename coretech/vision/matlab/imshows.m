%imshows(images)
%imshows(im1,im2,im3); %for seperate figures
%imshows({im1,im2,im3},{im4,im5}); %for subplots

function imshows(varargin)

figNums=1:length(varargin);

maximizeWindows = false;
for i=1:length(varargin)
    if ischar(varargin{i})
        if strcmpi(varargin{i}, 'maximize')
            maximizeWindows = true;
        end
    end
end

for i=1:length(varargin)
    if ischar(varargin{i})
        continue;
    end
    
    figureHandle = figure(figNums(i));
    curIms=varargin{i};
    if iscell(curIms)
        for j=1:length(curIms)
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