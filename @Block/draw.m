function draw(this, varargin)

AxesHandle = [];

otherArgs = parseVarargin(varargin{:});

if isempty(AxesHandle)
    AxesHandle = gca;
end

initHandle = isempty(this.handle) || ~ishandle(this.handle) || ...
    get(this.handle, 'Parent') ~= AxesHandle;

if initHandle
    this.handle = patch(this.X, this.Y, this.Z, this.color, ...
        'Parent', AxesHandle, otherArgs{:});
else
    set(this.handle, 'XData', this.X, 'YData', this.Y, 'ZData', this.Z);
end

faces = this.faces;
for i = 1:length(faces)
    draw(this.markers(faces{i}), 'where', AxesHandle);
end

end