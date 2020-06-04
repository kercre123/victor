function varargout = parseVarargin(varargin)
% Parse the list of "extra" arguments pairs, resetting any valid variables.
%
% [<unknownPairs>] = parseVarargin(<Name1, Val1>, <Name2, Val2>, ..., <NameN, ValN>);
%
%   If the "unknownPairs" output is requested, any unrecognized names and
%   their values will be returned as a cell array.
%
% Usage:
% 
%   % declare list of variables and their defaults values
%   var1 = 1;
%   var2 = 'hello';
%
%   % parse varargin
%   parseVarargin(varargin{:});
%
%   % var1 and var2 now updated to optional passed-in values 
%
% ----------
% Jean-Francois Lalonde
% Andrew Stein
%

if nargin==1 && iscell(varargin{1})
	varargin = varargin{1};
end

if ~isempty(varargin) && isstruct(varargin{1})
    assert(nargout > 0, ['Using parseVarargin with a struct requires ' ...
        'at least one output argument. Otherwise there would be no effect.']);
    
    returnUnknowns = nargout > 1;
    
    if returnUnknowns
        toReturn = false(size(varargin));
    end
    
    S = varargin{1};
    varargin = varargin(2:end);
    
    assert(mod(length(varargin),2)==0, ...
        'Expecting a list of name/value pairs.');

    for i = 1:2:length(varargin)
        if isfield(S, varargin{i})
            % Valid property, assign it:
            S.(varargin{i}) = varargin{i+1};
        else
            % Unrecognized property, either mark for return or throw error
            if returnUnknowns
                toReturn(i:(i+1)) = true;
            else
                error('"%s" is not a field of the given struct.', varargin{i});
            end
        end
    end
    
    varargout{1} = S;
    if returnUnknowns
        varargout{2} = varargin{toReturn};
    end
    
else
    assert(mod(length(varargin),2)==0, ...
        'Expecting a list of name/value pairs.');

    returnUnknowns = nargout > 0;
    
    if returnUnknowns
        toReturn = false(size(varargin));
    end

    for i = 1:2:length(varargin)
        if evalin('caller', sprintf('exist(''%s'', ''var'')', varargin{i}))
            % Valid variable, assign it:
            assignin('caller', varargin{i}, varargin{i+1});
        else
            % Unrecognized variable, either mark for return or throw error
            if returnUnknowns
                toReturn(i:(i+1)) = true;
            else
                error('Parameter "%s" not recognized.', varargin{i});
            end
        end
    end

    if returnUnknowns
        varargout{1} = varargin(toReturn);
    end

end % IF object



