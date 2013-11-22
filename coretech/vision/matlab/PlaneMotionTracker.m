classdef PlaneMotionTracker < handle
    
    properties(Dependent = true, SetAccess = 'protected')
        tx, ty;
        theta;
    end
    
    properties(SetAccess = 'protected')
        It;
    end
    
    properties(GetAccess = 'protected', SetAccess = 'protected')
        
        xgrid, ygrid;
        nrows, ncols;
        xcen, ycen;
        numScales;
        
        tform;
        
        maxIterations;
        convergenceTolerance;
        
    end % PROPERTIES
    
    methods
        function this = PlaneMotionTracker(varargin)
            NumScales = 4;
            Resolution = [40 30]; % [width height]
            CropFactor = 1;
            ConvergenceTolerance = 5e-2;
            MaxIterations = 25;
            parseVarargin(varargin{:});
            
            this.numScales = NumScales;
            
            this.xgrid = cell(1, this.numScales);
            this.ygrid = cell(1, this.numScales);
            
            this.maxIterations = MaxIterations;
            this.convergenceTolerance = ConvergenceTolerance;
            
            this.nrows = Resolution(2);
            this.ncols = Resolution(1);
            
            this.xcen = this.ncols/2;
            this.ycen = this.nrows/2;
            
            width = round(this.ncols * CropFactor);
            height = round(this.nrows * CropFactor);
            
            for i_scale = 1:this.numScales
                spacing = 2^(i_scale-1);
                
                if min(width, height)/spacing < 4 
                    this.numScales = i_scale-1;
                    break;
                end
                
                [this.xgrid{i_scale}, this.ygrid{i_scale}] = meshgrid( ...
                    linspace(-width/2, width/2, width/spacing), ...
                    linspace(-height/2, height/2, height/spacing));
               
            end % FOR each scale
                    
        end % Constructor: GroundPlaneMotionTracker()
        
        function converged = track(this, img1, img2, varargin)
            % Computes relative motion from img1 to img2
            assert(size(img1,3)==1 && size(img2,3)==1, ...
                'Expecting grayscale images.');
            
            assert(isequal(size(img1), [this.nrows this.ncols]) && ...
                isequal(size(img2), [this.nrows this.ncols]), ...
                sprintf('Both images must be [%d x %d].', this.nrows, this.ncols));
            
            this.tform = eye(3); 
            
            for i_scale = this.numScales:-1:1
                
                img1_scaled = mexInterp2(img1, ...
                    this.xgrid{i_scale} + this.xcen, ...
                    this.ygrid{i_scale} + this.ycen);
                
                % Compute derivatives as centered differences:
                spacing = 2^(i_scale-1);
                Ix = spacing*(image_right(img1_scaled)-image_left(img1_scaled))/2;
                Iy = spacing*(image_down(img1_scaled)-image_up(img1_scaled))/2;
 
                %A = [ column(this.xgrid{i_scale}.*Ix) ...
                %    column(this.ygrid{i_scale}.*Iy) ...
                %    column(-this.ygrid{i_scale}.*Ix + this.xgrid{i_scale}.*Iy)];
                
                A = [Ix(:) Iy(:) column(-this.ygrid{i_scale}.*Ix + this.xgrid{i_scale}.*Iy)];
                
                %invAtA = inv(A'*A);
                %invAtA = (A'*A) \ eye(3);
                
                converged = false;
                iteration = 0;
                while iteration < this.maxIterations

                    xi = this.xgrid{i_scale}*this.tform(1,1) + this.ygrid{i_scale}*this.tform(1,2) + this.tform(1,3);
                    yi = this.xgrid{i_scale}*this.tform(2,1) + this.ygrid{i_scale}*this.tform(2,2) + this.tform(2,3);
                    
                    if iteration > 0
                        rmsChange = sqrt(mean(column((xi-xi_prev).^2 + (yi-yi_prev).^2)));
                        if rmsChange < this.convergenceTolerance*spacing
                            converged = true;
                            break;
                        end
                    end
                    
                    %[img2_scaled, outOfBounds] = mexInterp2(img2, xi, yi);
                    img2_scaled = interp2(img2, xi + this.xcen, yi + this.ycen, 'linear');
                    outOfBounds = isnan(img2_scaled);
                    
                    if all(outOfBounds)
                        break;
                    end
                     
                    this.It = img2_scaled - img1_scaled;
                    this.It(outOfBounds) = 0; 
                    
                    Atb = A(~outOfBounds,:)'* this.It(~outOfBounds);
                    
                    %update = invAtA*Atb;
                    AtA = A(~outOfBounds,:)'*A(~outOfBounds,:);
                    update = AtA \ Atb;
                    
                    % Note that we compute the inverse update for
                    % composition with the current transformation. (Thus
                    % the transpose on R_update and multiplying that by the
                    % negative translation update)
                    cosTheta = cos(update(3));
                    sinTheta = sin(update(3));
                    R_update = [cosTheta -sinTheta; sinTheta cosTheta]';
                    t_update = -R_update*[update(1); update(2)];
                    
                    %R_update = [cosTheta -sinTheta; sinTheta cosTheta];
                    %t_update = update(1:2);
                    
                    % Compose with (inverse of) update:
                    this.tform = this.tform * [R_update t_update; 0 0 1];
                    %this.tform = this.tform / [R_update t_update; 0 0 1];
                    
                    iteration = iteration + 1;
                    xi_prev = xi;
                    yi_prev = yi;
                    
                end % WHILE not converged
                
            end % FOR each scale
            
        end % FUNCTION track()
        
        
        function x = get.tx(this)
            x = this.tform(1,3);% - this.ncols/2;
        end
        
        function y = get.ty(this)
            y = this.tform(2,3);% - this.nrows/2;
        end
        
        function t = get.theta(this)
           t = asin(this.tform(2,1)); 
        end
        
        
    end % METHODS
    
end % CLASSDEF PlaneMotionTracker