function rowvec = row(data)
%
% rowvec = row(data)
%
%  Same as calling data(:)', except that this allows the passing of
%  arbitrarily-indexed data.  For example, you can't do 
% 
%   row_vec = (img(:,:,1)(:))'
%
%  but this function allows you to get this effect via:
%
%   row_vec = row(img(:,:,1));
%

rowvec = data(:)';