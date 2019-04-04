function response = separable_filter(img_data, fx, fy, padMethod)
% Simple wrapper to call imfilter with a pair of separable kernels.
%
% response = separable_filter(img_data, fx, fy)
%
%    Filters img_data with the kernel 'fx' and filters the result with
%    'fy'.  If 'fy' is not provided it defaults to fx'.  
%
%    imfilter is used with the options 'same' and 'symmetric' for both
%    filtering operations.
%
%    Example:
%      Perform separable filtering with a gaussian kernel in each
%      direction:
%
%      g = gaussian_kernel(2); % produces a horizontal kernel (row vector)
%      response = separable_filter(data, g, g');
%
% ------------
% Andrew Stein
%

if nargin < 3 || isempty(fy)
	fy = fx';
end

if nargin < 4
    padMethod = 'symmetric';
end

response = imfilter(imfilter(img_data, fx, ...
   'same', padMethod), fy, 'same', padMethod);

end
