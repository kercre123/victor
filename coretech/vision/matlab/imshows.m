% function imshows(varargin)

% imshows(im1, im2, im3); %for separate figures
% imshows({im1, im2, im3}, {im4, im5}); %for subplots
% imshows(im1, [], [], im4); % don't modify figures 2 and 3
% imshows(im1, im2, 'maximize'); % imshow all windows at full size
% imshows(im1, im2, 4); % start figure numbering at 4. In this example, imshow on figures 4 and 5.

function imshows(varargin)
    figNums = 1:length(varargin);
    
    maximizeWindows = false;
    collage = true;
    collageTitles = false;
    for i = 1:length(varargin)
        if ischar(varargin{i})
            if strcmpi(varargin{i}, 'maximize') % Maximize all figure windows
                maximizeWindows = true;
            elseif strcmpi(varargin{i}, 'noCollage') % Don't display images within a cell array as one image
                collage = false;
            elseif strcmpi(varargin{i}, 'collageTitles') % Display numbers for collage
                collage = true;
                collageTitles = true;
            end
         else
             isImage = ~iscell(varargin{i}) && (ndims(varargin{i}) == 2 && min(size(varargin{i}) == [1,1]) == 1) || (ndims(varargin{i}) == 3 && min(size(varargin{i}) == [1,1,1]) == 1);
             if isImage
                 figNums = figNums - 1 + varargin{i};
             end
        end
    end
    
    for i = 1:length(varargin)
        isImage = ~iscell(varargin{i}) && (ndims(varargin{i}) == 2 && min(size(varargin{i}) == [1,1]) == 1) || (ndims(varargin{i}) == 3 && min(size(varargin{i}) == [1,1,1]) == 1);
        if ischar(varargin{i}) || isImage || isempty(varargin{i})
            continue;
        end
        
        figureHandle = figure(figNums(i));
        curIms = varargin{i};
        if iscell(curIms)
            numImages = length(curIms);
            
            if collage
                [bigImage, bigImageArray, numImagesY, numImagesX, imageHeights, ~] = drawCollage(curIms);
                
                hold off;
                imshow(bigImage);
                hold on;
                
                if collageTitles
                    cy = 1;
                    ci = 1;
                    for y = 1:numImagesY
                        cx = 1;
                        for x = 1:numImagesX
                            smallImage = bigImageArray{y,x};

                            if isempty(smallImage)
                                break;
                            end

                            smallImageWidth = size(smallImage, 2);

                            if scale ~= 1
                                smallImageWidth = floor(size(smallImage,2) * scale);
                            end

                            text(cx + smallImageWidth/2 - 10, cy + 10, sprintf('%d',ci), 'Color', [0.5,0.4,0.3], 'FontSize', 14);

                            cx = cx + smallImageWidth;
                            ci = ci + 1;
                        end % for x = 1:numImagesX

                        cy = cy + ceil(max(imageHeights(y,:))*scale);
                    end % for y = 1:numImagesY
                end % collageTitles
                
                if maximizeWindows
                    set(figureHandle, 'units','normalized','outerposition',[0 0 1 1]);
                end
            else % if collage
                for j = 1:numImages
                    subplot(1,length(curIms),j); imshow(curIms{j}, 'Border', 'tight');
                    if maximizeWindows
                        set(figureHandle, 'units','normalized','outerposition',[0 0 1 1]);
                    end
                end
            end % if collage ... else
        else % if iscell(curIms)
            imshow(curIms, 'Border', 'tight');
            if maximizeWindows
                set(figureHandle, 'units','normalized','outerposition',[0 0 1 1]);
            end
        end % if iscell(curIms) ... else
        impixelinfo;
    end % for i = 1:length(varargin)
end % function imshows(varargin)
