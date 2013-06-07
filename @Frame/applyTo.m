function varargout = applyTo(this, varargin)

assert(nargout == length(varargin), ...
    'Number of inputs must match number of outputs.');

R = this.Rmat;

varargout = cell(1, nargout);

switch(length(varargin))
    case 1
        P = varargin{1};
        if size(P,1) == 3
            
            varargout{1} = R*P + this.T(:,ones(1,size(P,2)));
            
        elseif size(P,2) == 3
            t = row(this.T);
            varargout{1} = P*R' + t(ones(size(P,1),1),:);
            
        else
            error('P should be Nx3 or 3xN.');
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
end