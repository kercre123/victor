function varargout = applyTo(this, varargin)
% Apply a Pose's rotation and translation to a set of points.

%assert(nargout == length(varargin), ...
%    'Number of inputs must match number of outputs.');

R = this.Rmat;

varargout = cell(1, nargout);

switch(length(varargin))
    case 1
        P = varargin{1};
        if size(P,1) == 3
            % Pin = [X; Y; Z] where X,Y,Z are row vectors
            % Pout = R*P + T  
            N = size(P,2);
            varargout{1} = R*P + this.T(:,ones(1,N));
        
        elseif size(P,2) == 3
            % Pin = [X(:) Y(:) Z(:)] 
            % Pout = P*R' + T
            N = size(P,1);
            t = row(this.T);
            varargout{1} = P*R' + t(ones(N,1),:);
            
        else
            error('P should be Nx3 or 3xN.');
        end
        
        if nargout==3
            % Also outputing Jacobian matrices
            dP_dRmat = zeros(3*N,9);
            dPdT = zeros(3*N,3);
            
            dP_dRmat(1:3:end,1:3:end) =  P';
            dP_dRmat(2:3:end,2:3:end) =  P';
            dP_dRmat(3:3:end,3:3:end) =  P';
            
            dPdT(1:3:end,1) = ones(N,1);
            dPdT(2:3:end,2) = ones(N,1);
            dPdT(3:3:end,3) = ones(N,1);
            
            % Yay chain rule
            dPdR = dP_dRmat * this.dRmat_dRvec;
            
            varargout{2} = dPdR;
            varargout{3} = dPdT;
        end
            
    case 3
        Xin = varargin{1};
        Yin = varargin{2};
        Zin = varargin{3};
        
        varargout{1} = R(1,1)*Xin + R(1,2)*Yin + R(1,3)*Zin + this.T(1);
        varargout{2} = R(2,1)*Xin + R(2,2)*Yin + R(2,3)*Zin + this.T(2);
        varargout{3} = R(3,1)*Xin + R(3,2)*Yin + R(3,3)*Zin + this.T(3);
        
    otherwise
        error('Unrecognized number of inputs.');
end

end % FUNCTION applyTo()