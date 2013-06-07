function h = draw(this, varargin)

where = [];
Mode = '2D';
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

if isempty(where)
    h_axes = gca;
    
elseif ishandle(where)
    switch(get(where, 'Type'))
        case 'image'
            Mode = 'image';
            h_axes = get(where, 'Parent');
            assert(get(h_axes, 'Type', 'axes'));
            
        case 'axes'
            h_axes = where;
            
        otherwise
            error('Handle must be an image or set of axes.');
    end
else
    Mode = 'image';
    h_axes = gca;
    imshow(where);
    hold on
end

switch(Mode)
    case {'image', '2D'}
        h_patch = patch(this.imgCorners([1 2 4 3 1],1), this.imgCorners([1 2 4 3 1],2), ...
            EdgeColor, 'EdgeColor', EdgeColor, 'LineWidth', 2, ...
            'FaceColor', FaceColor, 'FaceAlpha', FaceAlpha, ...
            'Parent', h_axes, 'Tag', Tag);
        
        if ~strcmp(this.topOrient, 'none')
            
            %     h_top = plot(this.corners(this.topSideLUT.(this.topOrient),1), ...
            %         this.corners(this.topSideLUT.(this.topOrient),2), ...
            %         'Color', TopColor, 'LineWidth', 3, 'Parent', h_axes, 'Tag', Tag);
            h_top = plot(this.imgCorners([1 3],1), ...
                this.imgCorners([1 3],2), ...
                'Color', TopColor, 'LineWidth', 3, 'Parent', h_axes, 'Tag', Tag);
        else
            h_top = [];
        end
        
        h_origin = plot(this.imgCorners(1,1), this.imgCorners(1,2), 'w.', 'MarkerSize', 16);
        
        if drawTextLabels
            h_text = text(mean(this.imgCorners([1 4],1)), mean(this.imgCorners([1 4],2)), ...
                {sprintf('Block %d', this.blockType), sprintf('Face %d', this.faceType)}, ...
                'Color', FontColor, 'FontSize', FontSize, 'FontWeight', FontWeight, ...
                'BackgroundColor', FontBackgroundColor, ...
                'Hor', 'center', 'Parent', h_axes, 'Tag', Tag);
        else
            h_text = [];
        end
        
        if nargout > 0
            h = [h_patch(:); h_top; h_text(:); h_origin];
        end
        
    case {'3D', 'world'}
        
        X = this.P([1 3 4 2], 1);
        Y = this.P([1 3 4 2], 2);
        Z = this.P([1 3 4 2], 3);
        
        initHandles = isempty(this.handles) || ...
            ~ishandle(this.handles(1)) || ...
            get(this.handles(1), 'Parent') ~= h_axes;

        if initHandles
            this.handles(1) = patch(X,Y,Z, 'w', 'EdgeColor', 'w', 'Parent', h_axes);
            wasHeld = ishold(h_axes);
            hold(h_axes, 'on');
            this.handles(2) = plot3(X(1), Y(1), Z(1), 'k.', 'MarkerSize', 24, 'Parent', h_axes);
            if ~wasHeld
                hold(h_axes, 'off');
            end
        else
            set(this.handles(1), 'XData', X, 'YData', Y, 'ZData', Z);
            set(this.handles(2), 'XData', X(1), 'YData', Y(1), 'ZData', Z(1));
        end
        
    otherwise
        error('Unrecognized drawing mode "%s"', Mode);
        
end % SWITCH(Mode)

end % METHOD draw()
