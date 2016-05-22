function interestPointDisplay(frame, h_axesProc, h_imgProc, varargin)

featureType = 'EdgeFoci';

switch(featureType)
  case 'MSER'
    img = rgb2gray(im2uint8(frame));
    [~,mserFrames] = vl_mser(img, varargin{:});
    
    cla(h_axesProc)
    imagesc(frame, 'Parent', h_axesProc);
    axis(h_axesProc, 'image', 'off')
    subplot(h_axesProc)
    vl_plotframe(vl_ertr(mserFrames));
    
  case 'EdgeFoci'
    scales = 8*2.^[-1/3 0 1/3 2/3 1];
    [h,m] = edgeFoci(frame, scales);
    [my,mx,mscale] = ind2sub(size(h), find(m & h > 0.001));
    
%     set(h_imgProc, 'CData', frame);
%     hold(h_axesProc, 'on')
%     delete(findobj(h_axesProc, 'Tag', 'EdgeFoci'))
%     plot(mx, my, 'r.', 'MarkerSize', 10, 'Parent', h_axesProc, 'Tag', 'EdgeFoci');
    
    cla(h_axesProc)
    imagesc(frame, 'Parent', h_axesProc);
    axis(h_axesProc, 'image', 'off')
    subplot(h_axesProc)
    vl_plotframe([mx my column(scales(mscale))]');
    
  case 'SIFT'
    img = rgb2gray(im2single(frame));
    f = vl_sift(img, varargin{:});
    cla(h_axesProc)
    imagesc(frame, 'Parent', h_axesProc);
    axis(h_axesProc, 'image', 'off');
    subplot(h_axesProc)
    vl_plotframe(f);
    
end


end
