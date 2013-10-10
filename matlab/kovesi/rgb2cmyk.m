% RGB2CMYK - RGB to cmyk colour space
%
% Usage: cmyk = rgb2cmyk(im)
%
% This function wraps up calls to MAKECFORM and APPLYCFORM in a conveenient
% form.  
%
% See also:  RGB2LAB, RGB2NRGB, MAP2GEOSOFTTBL

% Peter Kovesi
% Centre for Exploration Targeting 
% The University of Western Australia
% peter.kovesi at uwa edu au

% October 2012

function cmyk = rgb2cmyk(im)

    cform = makecform('srgb2cmyk');
    cmyk = applycform(im, cform);