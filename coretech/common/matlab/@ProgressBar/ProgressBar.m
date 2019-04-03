% Create a progress bar "object".
%
% progressbar = ProgressBar(msg, <'CancelButton', true>)
%
%  Creates a progress bar object.  This may be easier to use or more
%  powerful than Matlab's "waitbar" in some situations.  Specifying the
%  optional "CancelButton, true" property/value pair will enable a Cancel
%  button.  When clicked, that button will set the ProgressBar's
%  'cancelled' property to true.  
%
%  The returned progress bar has the following member functions:
%
%    progressbar.set(value) - Set bar to a specific position.  Value is a
%       fraction al value between 0 and 1.
%
%    progressbar.get_progress() - Returns the position of the progress bar,
%       as a value between 0 and 1.
%
%    progressbar.set_increment(value) - Sets the value used by the increment() 
%       function (see below). Default is 0.1
%
%    progressbar.add(value) - Increments bar by 'value'.  The value
%       can be negative (resulting in a decrement operation), but will not
%       move the bar beyond 0 or 1.  
% 
%    progressbar.increment() - Increments bar by a set amount, specified by
%       set_increment().  Will not increment beyond 0 or 1.
%
%    progressbar.show() or .hide() - Show/Hide the progress bar.
%
%    progressbar.set_message(msg) - Sets the message on the progress bar to
%       the input string 'msg'
%
%    progressbar.set_name(name) - Sets the name of the progress bar's
%       figure to 'name'.  (Initially, this defaults to 'Progress Bar'.)
%
%    progressbar.set_position(pos) - Sets the progress bar's screen
%       position.  Use 4-element 'pos' to specify [x y width height], or
%       2-element 'pos' to specify the center of the progress bar 
%       [x_cen  y_cen] only and leave width/height unchanged.
%
%    progressbar.get_handle() - returns the graphics handle to the progress
%       bar
%
%    progressbar.ishandle() - true if the progress bar is still a valid,
%       usable object.  false if not, e.g. if the figure was closed or
%       deleted somehow.
%
%
%
% See also: waitbar
%
% ------------
% Andrew Stein
%

classdef ProgressBar < handle
    
    properties (Constant = true)
        CHAR_WIDTH = 60;
        CHAR_HEIGHT = 4;
    end

	methods (Access = 'public')
		
		% Constructor:
		function pb = ProgressBar(msg, varargin)

            if nargin==1 && isa(msg, 'ProgressBar')
                pb = msg;
                return;
            elseif nargin==0 || isempty(msg)
                msg = '';
            end

            cancelArgs = {};
            cancelIndex = find(strcmpi(varargin, 'CancelButton'));
			if ~isempty(cancelIndex)
				if varargin{cancelIndex+1}==true
					cancelArgs = {'CreateCancelBtn', ...
						@(~,~)cancelButtonCallback(pb)};
				end
				varargin(cancelIndex + [0 1]) = [];
			end
			
			pb.handle = waitbar(0, ...
                repmat('A', [pb.CHAR_HEIGHT pb.CHAR_WIDTH]), ...
                'Name', 'Progress Bar', cancelArgs{:}, varargin{:});
            waitbar(0, pb.handle, msg);
			pb.inc = 0.1;
			pb.current_value = 0;
            
			pb.axes_handle = findobj(pb.handle, 'Type', 'axes');
            pb.msg_handle = get(pb.axes_handle, 'Title');
			pb.timing_msg_handle = get(pb.axes_handle, 'XLabel');
						
			% Make the bar itself erase properly so the value can be
			% decremented, which is not the default behavior of waitbars:
			h = findall(pb.handle, 'Type', 'patch');
			set(h, 'EraseMode', 'normal');

            % Don't use tex interpreter for messages:
            set(pb.msg_handle, 'Interpreter', 'none');
            
            set(pb.handle, 'CloseRequestFcn', @(~,~)delete(pb));
		end
			
		% Public set/get methods:
		function set(pb, new_value)
			pb.current_value = new_value;
			waitbar(new_value, pb.handle);
			
			if pb.showTimingInfo 
				if new_value == 0
					pb.startTime = tic;
                    pb.cancelled = false;
				end
			else
				set(pb.timing_msg_handle, 'String', '');
			end
				
		end
		
		function value = get_progress(pb) 
			value = pb.current_value;
		end
		
		function set_increment( pb, inc_value )
			pb.inc = inc_value;
		end
		
		function increment(pb)
			pb.increment_by_value(pb.inc);
			
			if pb.showTimingInfo 
				timeElapsed = toc(pb.startTime);
				if timeElapsed > 3600
					timeElapsedUnits = 'hr';
					timeElapsedDivisor = 3600;
				elseif timeElapsed > 60
					timeElapsedUnits = 'min';
					timeElapsedDivisor = 60;
				else
					timeElapsedUnits = 'sec';
					timeElapsedDivisor = 1;
				end
				
				new_msg = sprintf('Time elapsed = %.1f %s', ...
					timeElapsed/timeElapsedDivisor, timeElapsedUnits);
				
				if pb.current_value < 1-eps
					
					timeRemaining = (1-pb.current_value) * ...
						timeElapsed / pb.current_value;
					if timeRemaining > 3600
						timeRemainingUnits = 'hr';
						timeRemaining = timeRemaining / 3600;
					elseif timeRemaining > 60
						timeRemainingUnits = 'min';
						timeRemaining = timeRemaining / 60;
					else
						timeRemainingUnits = 'sec';
					end
					
					new_msg = sprintf('%s, Estimated time remaining = %.1f %s', ...
						new_msg, timeRemaining, timeRemainingUnits);
					
				end
				
				set(pb.timing_msg_handle, 'String', new_msg);
				
			end
		end
		
		function h = get_handle(pb)
			h = pb.handle;
		end
		
		function add(pb, value)
			pb.increment_by_value(value);
		end
		
		function show(pb)
			set(pb.handle, 'Visible', 'on');
            pb.ishidden = false;
		end
		
		function hide(pb)
			set(pb.handle, 'Visible', 'off');
            pb.ishidden = true;
		end
		
		function set_message(pb, msg)
			waitbar(pb.current_value, pb.handle, ...
                textwrap({msg}, pb.CHAR_WIDTH));
            pb.message = msg;
        end
        
        function append_message(pb, msg)
            pb.set_message([pb.message ' ' msg]);
        end            
        
		function set_name(pb, name)
			set(pb.handle, 'Name', name);
		end
		
		function set_position(pb, pos)
			switch(length(pos))
				case 4
					set(pb.handle, 'Position', pos);
				case 2
					crnt_pos = get(pb.handle, 'Position');
					set(pb.handle, 'Position', [pos(1:2)-crnt_pos(3:4)/2 crnt_pos(3:4)]);
				otherwise
					error('You must specify [x y] or [x y width height].');
			end
        end
		
        function delete(pb)
            if ishandle(pb.handle)
                delete(pb.handle);
            end
        end
        
        function valid = ishandle(pb)
            valid = isvalid(pb) && ishandle(pb.handle);
        end

	end

	methods (Access = 'protected')

		% 		function init(pb, msg, varargin)
		% 			%if pb.is_initialized
		% 			%	return
		% 			%end
		%
		% 			pb.handle = waitbar(0, msg, 'Name', 'Progress Bar', varargin{:});
		% 			pb.inc = 0.1;
		% 			pb.current_value = 0;
		% 			%pb.is_initialized = true;
		%
		% 		end
		
		function increment_by_value(pb, value)

			pb.current_value = max(0, min(1, pb.current_value + value));
			pb.set(pb.current_value);

        end

        function cancelButtonCallback(pb)
            pb.cancelled = true;
            pb.append_message('- Cancelled!');
        end
	end % protected methods

	properties (SetAccess = 'protected', GetAccess = 'protected')

		%is_initialized = false;
		current_value = 0;
		inc = 0.1;
		handle = [];
        msg_handle = [];
		axes_handle = [];
		timing_msg_handle = [];
		
		startTime = tic;
		
    end
    
    properties (SetAccess = 'protected', GetAccess = 'public')
        message = '';
        ishidden = false;
        cancelled = false;
	end
	
	properties (SetAccess = 'public', GetAccess = 'public')
		showTimingInfo = false;
	end
	
end


