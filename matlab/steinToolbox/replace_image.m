function h = replace_image(h, img)
%
% h_img = replace_image(<h>, img)
%
%  Replaces the image data of the provided handle with the given image,
%  leaving any other axes settings (title, labels, limits, etc) unchanged.
%
%  'h' should be the handle for an image or a set of axes which have an
%  image as a child.  If not provided, the current axes are used.  The
%  returned handle 'h_img' is the handle to the image whose contents was
%  replaced (which could possibly be the same as 'h').
%
%  If no image is provided, you will be able to click on a set of axes
%  which has an image.
%

if nargin==1 
	img = [];
	if (~isscalar(h) || ~ishandle(h(1)))
		% User passed in just an image.  Use current axes.
		img = h;
		h = gca;
	end
elseif nargin==0
    h = gca;
	img = [];
end

while isempty(img)
    % No image defined, let the user select one
    h_img = select_axes(1);
    img = getimage(h_img);
end

handle_type = get(h, 'Type');
if strcmp(handle_type, 'axes')
	c = get(h, 'Children');
	image_index = find(strcmp(get(c,'Type'),'image'));
	
	if(isempty(image_index))
		error('Axes have no image as a child!');
	elseif(length(image_index)>1)
		warning('Axes have multiple images, using first found.');
	end

	h = c(image_index(1));
	
elseif ~strcmp(handle_type, 'image')
	error('Handle must be an image or axes with a child image!');
	
end

set(h, 'CData', img);

