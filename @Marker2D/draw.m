function h = draw(this, varargin)
% Draw a Marker2D.

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
Tag = class(this);

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
    'w.', 'MarkerSize', 16, 'Parent', h_axes, 'Tag', Tag);

if ~strcmp(drawTextLabels, 'none')
    
    space = '';
    labels = this.IdChars;
    if strcmp(drawTextLabels, 'long')
        space = ' ';
        labels = this.IdNames;
    end
            
    str = cell(1, this.numIDs);
    for i = 1:this.numIDs
        % Note I'm assuming ID values are printable as integers...
        str{i} = sprintf('%s%s%d', labels{i}, space, this.ids(i));
    end
    h_text = text(mean(this.corners([1 4],1)), mean(this.corners([1 4],2)), ...
        str, ...
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
