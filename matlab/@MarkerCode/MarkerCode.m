classdef MarkerCode
    
    properties(Constant = true)
        MinVariance = 1e-2;
    end
    
    properties
        name;
        numProbes;
        numBits;
        
        modelType;
        
        model;
        modelMean;
        modelVar;
        modelCov;
    end
    
    methods
        function this = MarkerCode(img, varargin)
            
            Name = '';
            NumProbes = 6; % probe pattern is NumProbes x NumProbes
            NumBits = 1; % 1 for binary, 2 for four grayscale levels, ...
            CornerBits = 11; % Bits to represent each x and y for each corner
            ModelType = 'NearestNeighbor'; % 'NearestNeighbor' or 'Gaussian'
            NumPerturbations = 100;
            UseFullCovariance = true;
            Display = false;
            
            parseVarargin(varargin{:});
            
            this.name      = Name;
            this.numProbes = NumProbes;
            this.numBits   = NumBits;
            this.modelType = ModelType;
            
            if ischar(img)
                filename = img;
                if isempty(this.name)
                    [~,this.name] = fileparts(filename);
                end
                
                img = imread(filename);
            end
            
            img = im2double(img);
            if size(img,3)>1
                img = mean(img,3);
            end
            
            [nrows,ncols] = size(img);
            
            % Make square
            if nrows > ncols
                temp = ones(nrows,nrows);
                pad = round((nrows-ncols)/2);
                temp(:,pad+(1:ncols)) = img;
                img = temp;
                ncols = nrows;
            elseif ncols > nrows
                temp = ones(ncols,ncols);
                pad = round((ncols-nrows)/2);
                temp(pad+(1:nrows),:) = img;
                img = temp;
                nrows = ncols;
            end
            
            this.model = cell(1, NumPerturbations);
            for i_perturb = 1:NumPerturbations
                % Randomly perturb the image corners
                xcorners = ncols*([0 1 0 1] + 0.025*randn(1,4));
                ycorners = nrows*([0 0 1 1] + 0.025*randn(1,4));
                
                [this.model{i_perturb}, xgrid, ygrid] = MarkerCode.probeImage(img, xcorners, ycorners, NumProbes, NumBits);
                
            end
            
            if strcmpi(this.modelType, 'Gaussian')
                % Compute the Gaussian representation for this code
                codes = [codes{:}];
                
                this.modelMean = reshape(mean(codes,2), [NumProbes NumProbes]);
                
                this.modelVar  = reshape(var(codes,0,2), [NumProbes NumProbes]);
                
                if UseFullCovariance
                    this.modelCov = cov(codes');
                else
                    this.modelCov = [];
                end
                
            end
            
            codeLength = 4*2*CornerBits + NumProbes*NumProbes*NumBits;
            
            if nargout == 0 || Display == true
                clf
                
                subplot 131
                imagesc(img), axis image
                hold on
                plot(xgrid*ncols, ygrid*nrows, 'r.', 'MarkerSize', 20);
                title({sprintf('Full Covariance Representation = %d values', NumProbes^2 + NumProbes^4), ...
                    sprintf('Diagonal Covariance Representation = %d values', 2*NumProbes^2)})
                
                subplot 132
                imagesc(this.modelMean), axis image
                title({'Code Mean', sprintf('Code Length = %d bits (%d bytes)', codeLength, ceil(codeLength/8))})
                
                subplot 133
                imagesc(sqrt(this.modelVar)), axis image
                title('Code Std. Dev.')
                
                colormap(gray)
            end
            
        end % CONSTRUCTOR MarkerCode()
        
        function newCode = rot90(this)
            newCode = this;
            newCode.modelMean = rot90(this.modelMean);
            newCode.modelVar  = rot90(this.modelVar);
            
            if ~isempty(newCode.modelCov)
                NumProbes = size(this.modelMean,1);
                index = column(rot90(reshape(1:NumProbes^2, [NumProbes NumProbes])));
                newCode.modelCov = newCode.modelCov(index,index);
            end
        end % FUNCTION rot90()
            
        function similarity = compare(this, code)
            
            if numel(this.modelMean) ~= numel(code)
                error('Size of model and given code must match.');
            end
            
            meanDiff = this.modelMean(:) - code(:);
            
            if isempty(this.modelCov)
                sigma = diag(this.modelVar(:));
            else
                sigma = this.modelCov;
            end
            sigma = sigma + diag(MarkerCode.MinVariance*ones(size(sigma,1),1));
            
            similarity = exp(-.5*(meanDiff'*(sigma\meanDiff)));
                        
        end % FUNCTION compare()
        
    end % Methods
    
    methods(Static = true)
        
        function [code, xgrid, ygrid] = probeImage(img, corners, NumProbes, NumBits)
            
            img = im2double(img);
            [nrows,ncols,nbands] = size(img);
            if nbands > 1
                img = mean(img,3);
            end
            
            [xgrid,ygrid] = meshgrid( (0.5/NumProbes) : 1/NumProbes : (1-0.5/NumProbes));
            xgrid = xgrid(:);
            ygrid = ygrid(:);
            
            xgrid = ncols*[(xgrid-0.25/NumProbes) xgrid (xgrid+0.25/NumProbes) (xgrid-.25/NumProbes) xgrid (xgrid+0.25/NumProbes) (xgrid-.25/NumProbes) xgrid (xgrid+0.25/NumProbes)];
            ygrid = nrows*[[ygrid ygrid ygrid]-0.25/NumProbes ygrid ygrid ygrid [ygrid ygrid ygrid]+.25/NumProbes];
            
            % Adjust the probe locations accordingly, using a homography
            assert(isequal(size(corners), [4 2]), 'Corners should be a 4x2 matrix.');
            H = cp2tform(corners, [0 ncols 0 ncols; 0 0 nrows nrows]', 'projective');
            [x,y] = tforminv(H, xgrid, ygrid);
              
            % Get the interpolated values for this perturbation
            code = interp2(img, x, y, 'bilinear');
            W = double(~isnan(code));
            code(isnan(code))=0;
            
            code = reshape(sum(W.*code,2)./sum(W,2), [NumProbes NumProbes]);
            
            code = round(NumBits * code);
            
        end % FUNCTION probeImage()
        
    end % STATIC METHODS
end % CLASSDEF MarkerCode