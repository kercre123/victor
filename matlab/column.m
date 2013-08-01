function vec = column(data)
%
% vec = column(data)
%
%  Same as calling data(:), except that this allows the passing of
%  arbitrarily-indexed data.  For example, you can't do 
% 
%   column_vec = img(:,:,1)(:)
%
%  but this function allows you to get this effect via:
%
%   column_vec = column(img(:,:,1));
%

vec = data(:);

