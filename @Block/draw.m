function draw(this, varargin)

if isempty(this.handle)
    this.handle = patch(this.X, this.Y, this.Z, this.color, varargin{:});
else
    set(this.handle, 'XData', this.X, 'YData', this.Y, 'ZData', this.Z);
end

end