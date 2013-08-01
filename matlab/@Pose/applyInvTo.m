function varargout = applyInvTo(this, varargin)

invFrame = inv(this);
varargout = cell(1,nargout);
[varargout{:}] = invFrame.applyTo(varargin{:});

end