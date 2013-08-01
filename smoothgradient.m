function [Ix,Iy,Ixx,Iyy,Ixy,Ixxx,Iyyy] = smoothgradient(img, sigma)
%
% [Ix,Iy, <Ixx, Iyy>, <Ixy>, <Ixxx, Iyyy>] = smoothgradient(img, sigma)
%
% [grad_mag] = smoothgradient(img, sigma)
%
%  Computes image gradient information using derivative-of-gaussian
%  filters. If only one output is requested, the gradient magnitude is
%  returned.  Second/third derivatives are only computed if requested.
%
% -----------
% Andrew Stein
%


if nargin == 1
    sigma = 1;
end

if ~isa(img, 'double')
    img = double(img);
end

% create gaussian and derivative of gaussian filters:
% x = [-ceil(3*sigma):ceil(3*sigma)];
% normalization = (1/(sqrt(2*pi)*sigma));
% g = normalization*exp(-x.^2 / (2*sigma^2));
% dg = -x/(sigma^2) .* exp(-(x.^2)/(2*sigma^2)) * normalization;

if nargout <= 2
    [g,dg] = gaussian_kernel(sigma);
elseif nargout <= 5
    [g, dg, d2g] = gaussian_kernel(sigma);
else
    [g, dg, d2g, d3g] = gaussian_kernel(sigma);
end

Ix = imfilter(imfilter(img, dg, 'same', 'symmetric'), g', 'same', 'symmetric');
Iy = imfilter(imfilter(img, dg', 'same', 'symmetric'), g, 'same', 'symmetric');

if nargout > 2
    Ixx = imfilter(imfilter(img, d2g, 'same', 'symmetric'), g', 'same', 'symmetric');
    Iyy = imfilter(imfilter(img, d2g', 'same', 'symmetric'), g, 'same', 'symmetric');
    
    if nargout > 4
       Ixy = (Iy(:,[2:end end],:) - Iy(:,[1 1:end-1],:))/2;
       
       if nargout > 5
           Ixxx = imfilter(imfilter(img, d3g, 'same', 'symmetric'), g', 'same', 'symmetric');
           Iyyy = imfilter(imfilter(img, d3g', 'same', 'symmetric'), g, 'same', 'symmetric');
       end
    end
    
elseif nargout==1
    % if only one ouput argument, return gradient magnitude
    Ix = sqrt( Ix.^2 + Iy.^2);
end

