function [g, dg, d2g, d3g] = gaussian_kernel(sigma, num_std)
% Construct a Gaussian kernel for filtering, etc.
%
% [g, <dg>, <d2g>, <d3g>] = gaussian_kernel(sigma, <num_std>)
%
%   Produces a Gaussian kernel with +/- num_std*sigma support, where num_std 
%   must be a scalar.  If requested, also returns the (1st, 2nd, and 3rd) 
%   derivatives of a Gaussian with the same sigma.  If num_std is unspecified, 
%   it will default to 3.
%
%
% [g, ...] = gaussian_kernel(sigma, x)
%
%   Produces a Gaussian kernel with specified support, x, where x is a vector.
%   For example, x might be -3:3, which would provide +/- 3*sigma support if 
%   sigma=1.
%
% 
% In either case, the outputs are row vectors. 'g' will sum to one. The 
% 1st and 3rd derivative filters are normalized such that each half encloses 
% an area of 0.5 -- i.e., sum(abs(dg)) = 1.  The second derivative will be 
% normalized such that it sums to zero -- specifically, the area under the 
% positive part of the filter is adjusted to equal the area under the
% negative part.
%
% ------------
% Andrew Stein
%

if nargin<2
	num_std = 3;
end

if isscalar(num_std)
	halfwidth = ceil(num_std*sigma);
	x = -halfwidth:halfwidth;
else
	x = num_std(:)';
	if mod(length(x),2)==0
		error('Support vector x should have odd length.');
	end
end

% Special case:
if isempty(sigma) || sigma==0 
	g = 1;
	dg = [.5 0 -.5];
	d2g = [1 -2 1];
	d3g = [.5 -1 0 1 -.5];
	return
    
elseif length(x)==3
    g = DiscreteGaussian(sigma, .01);
	
	% Make sure to only return middle 3 elements
	N = length(g);
	if N>3
		mid = (N+1)/2;
		g = g(mid + [-1 0 1]);
        g = g/sum(g); % need to renormalize to ensure sum==1
	end
	
    if nargout > 1
        dg = conv(g, [.5 0 -.5], 'same');
        if nargout > 2
            d2g = conv(g, [1 -2 1], 'same');
            if nargout > 3
                d3g = conv(g, [.5 -1 0 1 -.5], 'same');
            end
        end
    end
    
    return
end

g = exp(-x.^2 / (2*sigma^2));

% Normalize original Gaussian to sum to one.
g = g/sum(g);

% 1st Derivative
if nargout>=2
	dg = -x/(sigma^2) .* g;
	%dg = dg / (2*sum(abs(dg(1:(end+1)/2))));
end

% 2nd Derivative
if nargout>=3
    d2g = (x.^2/(sigma^4) - 1/(sigma^2)) .* g;
% 	neg_index = d2g < 0;
% 	d2g(neg_index) = -sum(d2g(~neg_index))*d2g(neg_index)/sum(d2g(neg_index));
	
% 	pos_index = d2g > 0;
% 	d2g(pos_index) = -sum(d2g(~pos_index))*d2g(pos_index)/sum(d2g(pos_index));
	
%    d2g = d2g / sum(abs(d2g));
end

% 3rd Derivative
if nargout>=4
    d3g = (3*x/(sigma^4) - x.^3/(sigma^6)) .* g;
    %d3g = d3g / (2*sum(abs(d3g(1:(end+1)/2))));
end


	
end