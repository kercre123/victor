function h = draw(this, varargin)

where = [];
drawTextLabels = 'long'; % 'long', 'short', or 'none'
FontSize = 14;
FontWeight = 'b';
FontColor = 'b';
FontBackgroundColor = 'w';
EdgeColor = 'g';
FaceAlpha = .3;
FaceColor = 'r';
TopColor = 'b';
Tag = 'BlockMarker2D';

parseVarargin(varargin{:});

if isempty(where)
    h_axes = gca;
    
elseif ishandle(where)
    switch(get(where, 'Type'))
        case 'image'
            h_axes = get(where, 'Parent');
            assert(get(h_axes, 'Type', 'axes'));
            
        case 'axes'
            h_axes = where;
            
        otherwise
            error('Handle must be an image or set of axes.');
    end
else
    h_axes = gca;
    imshow(where);
    hold on
end

h_patch = patch(this.corners([1 2 4 3 1],1), this.corners([1 2 4 3 1],2), ...
    EdgeColor, 'EdgeColor', EdgeColor, 'LineWidth', 2, ...
    'FaceColor', FaceColor, 'FaceAlpha', FaceAlpha, ...
    'Parent', h_axes, 'Tag', Tag);
    
%     h_top = plot(this.corners(this.topSideLUT.(this.topOrient),1), ...
%         this.corners(this.topSideLUT.(this.topOrient),2), ...
%         'Color', TopColor, 'LineWidth', 3, 'Parent', h_axes, 'Tag', Tag);
h_top = plot(this.corners([1 3],1), ...
    this.corners([1 3],2), ...
    'Color', TopColor, 'LineWidth', 3, 'Parent', h_axes, 'Tag', Tag);

h_origin = plot(this.origin(1), this.origin(2), ...
    'w.', 'MarkerSize', 16, 'Tag', Tag);

if ~strcmp(drawTextLabels, 'none')
    
    if strcmp(drawTextLabels, 'long')
        blockLabel = 'Block ';
        faceLabel  = 'Face ';
    else
        blockLabel = 'B';
        faceLabel  = 'F';
    end
    
    h_text = text(mean(this.corners([1 4],1)), mean(this.corners([1 4],2)), ...
        {sprintf('%s%d', blockLabel, this.blockType), ...
        sprintf('%s%d', faceLabel, this.faceType)}, ...
        'Color', FontColor, 'FontSize', FontSize, 'FontWeight', FontWeight, ...
        'BackgroundColor', FontBackgroundColor, ...
        'Hor', 'center', 'Parent', h_axes, 'Tag', Tag);
else
    h_text = []; 
end

if nargout > 0
    h = [h_patch(:); h_top; h_text(:); h_origin];
end


end % METHOD draw()
