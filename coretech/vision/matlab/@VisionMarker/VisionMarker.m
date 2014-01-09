classdef VisionMarker
        
    properties(Constant = true)
            
        ProbeParameters = struct( ...
            'Number', 9, ...          % There will be a Number x Number 2D array of probes
            'WidthFraction', 1/3, ... % As a fraction of square width
            'NumAngles', 8); % How many samples around ring to sample
                
        MinContrastRatio = 1.25;  % bright/dark has to be at least this
        
        XProbes = VisionMarker.CreateProbes('X', VisionMarker.ProbeParameters);
        YProbes = VisionMarker.CreateProbes('Y', VisionMarker.ProbeParameters);
        
    end % Constant Properties
    
    methods(Static = true, Access = 'protected')
        function probes = CreateProbes(coord, params)
            
            assert(mod(params.Number,2)==1, 'Expecting NumProbes to be odd.');
            
            N = params.Number;
            
            squareWidth = 1 / (N + 3);
            probeWidth  = squareWidth * params.WidthFraction;
            
            points = linspace(-squareWidth*(N-1)/2, squareWidth*(N-1)/2, N);
                        
            probeRadius = probeWidth/3;
            probes = cell(1,params.NumAngles);
            angles = linspace(0, 2*pi, params.NumAngles+1);
            
            switch(coord)
                case 'X'
                    points = column(points(ones(N,1),:));
                    for i = 1:params.NumAngles
                        probes{i} = points + probeRadius*cos(angles(i));
                    end
                    
                case 'Y'
                    points = column(points(ones(N,1),:)');
                    for i = 1:params.NumAngles
                        probes{i} = points + probeRadius*sin(angles(i));
                    end
                    
                otherwise
                    error('Expecting "X" or "Y" for "coord" argument.');
            end
               
            probes = [points probes{:}] + 0.5;
            
        end % function CreateProbes()
        
         function [code, corners] = ProbeImage(img, corners)
            
            [nrows,ncols,nbands] = size(img);
            img = im2double(img);
            if nbands > 1
                img = mean(img,3);
            end
            
            % No corners provided? Assume the borders of the image
            if nargin < 2 || isempty(corners)
                corners = [0 0 ncols ncols; 0 nrows 0 nrows]' + 0.5;
            end
            
            assert(isequal(size(corners), [4 2]), ...
                'Corners should be a 4x2 matrix.');
            
            % Figure out the homography to warp the canonical probe points
            % into the image coordinates, according to the corner positions
            H = cp2tform(corners, [0 0 1 1; 0 1 0 1]', 'projective');
            [xi,yi] = tforminv(H, VisionMarker.XProbes, VisionMarker.YProbes);
            
            % Get the interpolated values for each probe point
            means = interp2(img, xi, yi, 'bilinear');
            W = double(~isnan(means));
            means(isnan(means))=0;
            
            % Compute the average values of the points making each probe
            means = reshape(sum(W.*means,2)./sum(W,2), ...
                VisionMarker.ProbeParameters.Number*[1 1]);
            
            % Use the four corner bits to orient and threshold
            orientBits = [means(1,1) means(end,1) means(1,end) means(end,end)];
            
            % LowerLeft is the one that's the darkest
            [~,lowerLeftIndex] = min(orientBits);
            dark   = orientBits(lowerLeftIndex);
            bright = mean(means([1:(lowerLeftIndex-1) (lowerLeftIndex+1):end]));
            
            if bright < VisionMarker.MinContrastRatio*dark
                % not enough contrast to work with
                warning('Not enough contrast ratio to decode VisionMarker.');
                code = [];
                return
            end
            
            threshold = (bright + dark)/2;
            
            code = means < threshold;
            
            % Reorient the code and the corners to match
            reorder = [1 3; 2 4]; % canonical corner ordering
            switch(lowerLeftIndex)
                case 1
                    code = rot90(code);
                    reorder = rot90(reorder);
                case 2
                    % nothing to do
                case 3
                    code = rot90(rot90(code));
                    reorder = rot90(rot90(reorder));
                case 4
                    code = rot90(rot90(rot90(code)));
                    reorder = rot90(rot90(rot90(reorder)));
                otherwise
                    error('lowerLeft should be 1,2,3, or 4.');
            end
            corners = corners(reorder(:),:);
                    
            if nargout == 0
                subplot 121, hold off, imagesc(img), axis image, hold on
                plot(xi, yi, 'r.', 'MarkerSize', 10);
                
                subplot 122, imagesc(~code), axis image
            end
            
        end % function ProbeImage()
        
    end % Static Methods
    
    properties(SetAccess = 'protected')
     
        code;
        corners;
        
    end % Properties
    
    methods 

        function this = VisionMarker(img, varargin)
            
            [this.code, this.corners] = VisionMarker.ProbeImage(img);
            
        end % Constructor VisionMarker()
       
    end % Public Methods
    
    
end % CLASSDEF Marker2D
