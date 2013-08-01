function h_fig = namedFigure(name, varargin)
% Creates a figure with the specified name, or activates the existing one.
%
% h_fig = namedFigure(name, varargin)
%
%
% ------------
% Andrew Stein
%

h_fig = findobj(0, 'Type', 'Figure', 'Name', name);
if isempty(h_fig)
    h_fig = figure('Name', name, varargin{:});
    
else
    if length(h_fig) > 1
        warning('Multiple figures named "%s" found. Activating first.', name);
        h_fig = h_fig(1);
    end
    
    if ~isempty(varargin)
        set(h_fig, varargin{:});
    end
    
    if nargout==0
        figure(h_fig)
        clear h_fig
    end
end

end