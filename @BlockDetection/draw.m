function h = draw(this, where, varargin)

drawTextLabels = true;
FontSize = 14;
FontWeight = 'b';
FontColor = 'b';
FontBackgroundColor = 'w';
EdgeColor = 'g';
FaceAlpha = .3;
FaceColor = 'r';
TopColor = 'b';
Tag = 'BlockDetection';

parseVarargin(varargin{:});

if nargin < 2 || isempty(where)
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

if ~strcmp(this.topOrient, 'none')
    
    h_top = plot(this.corners(this.topSideLUT.(this.topOrient),1), ...
        this.corners(this.topSideLUT.(this.topOrient),2), ...
        'Color', TopColor, 'LineWidth', 3, 'Parent', h_axes, 'Tag', Tag);
% h_top = plot(this.corners([1 3],1), ...
%         this.corners([1 3],2), ...
%         'Color', TopColor, 'LineWidth', 3, 'Parent', h_axes, 'Tag', Tag);
else 
    h_top = [];
end

if drawTextLabels
    h_text = text(mean(this.corners([1 4],1)), mean(this.corners([1 4],2)), ...
        {sprintf('Block %d', this.blockType), sprintf('Face %d', this.faceType)}, ...
        'Color', FontColor, 'FontSize', FontSize, 'FontWeight', FontWeight, ...
        'BackgroundColor', FontBackgroundColor, ...
        'Hor', 'center', 'Parent', h_axes, 'Tag', Tag);
else
    h_text = [];
end

if nargout > 0
    h = [h_patch(:); h_top; h_text(:)];
end

end % METHOD draw()
