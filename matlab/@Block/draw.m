function draw(this, varargin)

AxesHandle = [];
DrawMarkers = true;

otherArgs = parseVarargin(varargin{:});

if isempty(AxesHandle)
    AxesHandle = gca;
end

initHandle = isempty(this.handle) || ~ishandle(this.handle) || ...
    get(this.handle, 'Parent') ~= AxesHandle;

% Get the position of the block w.r.t. the world coordinate system:
pos = getPosition(this, 'World');
X = reshape(pos(:,1), 4, []);
Y = reshape(pos(:,2), 4, []);
Z = reshape(pos(:,3), 4, []);
    
if initHandle
    this.handle = patch(X, Y, Z, this.color, ...
        'Parent', AxesHandle, otherArgs{:});
else
    set(this.handle, 'XData', X, 'YData', Y, 'ZData', Z);
end

if DrawMarkers
    for i = 1:length(this.markers)
        draw(this.markers{i}, 'where', AxesHandle);
    end
end

end