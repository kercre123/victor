function draw(this, varargin)

where = [];
labelFaceType = false;

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

P = getPosition(this);
X = P([1 3 4 2], 1);
Y = P([1 3 4 2], 2);
Z = P([1 3 4 2], 3);

initHandles = isempty(this.handles) || ...
    ~ishandle(this.handles(1)) || ...
    get(this.handles(1), 'Parent') ~= h_axes;

if initHandles
    this.handles(1) = patch(X,Y,Z, 'w', 'EdgeColor', 'w', 'Parent', h_axes);
    wasHeld = ishold(h_axes);
    hold(h_axes, 'on');
    this.handles(2) = plot3(X(1), Y(1), Z(1), 'k.', 'MarkerSize', 24, 'Parent', h_axes);
    if labelFaceType
        this.handles(3) = text(mean(X), mean(Y), mean(Z), ...
            sprintf('Face %d', this.faceType), 'Horizontal', 'center'); %#ok<UNRCH>
    end
    if ~wasHeld
        hold(h_axes, 'off');
    end
else
    set(this.handles(1), 'XData', X, 'YData', Y, 'ZData', Z);
    set(this.handles(2), 'XData', X(1), 'YData', Y(1), 'ZData', Z(1));
    if labelFaceType
        set(this.handles(3), 'XData', mean(X), 'YData', mean(Y), 'ZData', mean(Z)); %#ok<UNRCH>
    end
end
   
end % METHOD draw()
