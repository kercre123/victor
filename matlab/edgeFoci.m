function [h_multiScale,m] = edgeFoci(img, sigma)

DebugDisplay = nargout == 0;

% just work with grayscale imagery for now
if size(img,3) > 1
  img = rgb2gray(img);
end

img = im2double(img);

alpha = 0.25;
beta = 0.5;
lambda = 1.5;
tau = 0.001; % detection threshold
numTheta = 8;

numSigmas = length(sigma);
h_multiScale = cell(1,numSigmas);
for iSigma = 1:numSigmas
  
 
  % sigma = 16;  % scale
  sigma_0 = 0.5; % inherent amount of blur in the original image
  sigma_adj = max(0, sqrt( (alpha*sigma(iSigma))^2 - sigma_0^2 )); % for blurring
  sigma_u = sqrt((beta*sigma(iSigma))^2 - (alpha*sigma(iSigma))^2);
  nu = asin(beta)/2; % for soft orientation assignment
  
  
  
  % Oriented edges
  [Ix,Iy] = smoothgradient(img, sigma_adj);
  f = sqrt(Ix.^2 + Iy.^2);
  Ix(abs(Ix)<eps) = eps;
  theta = atan(Iy./Ix);
  theta(theta<0) = theta(theta<0) + pi;
  
  % Normalized edges
  fbar = separable_filter(f, gaussian_kernel(alpha*sigma(iSigma)*sqrt(lambda^2-1)));
  fnorm = f ./ max(fbar, 10/sigma(iSigma));
  
  % Separable filters
  [g,dg,dg2] = gaussian_kernel(sigma_u, 2);
  
  if DebugDisplay
    figure(1)
  end
  
  h = cell(1, numTheta);
  [nrows,ncols] = size(img);
  [xgrid,ygrid] = meshgrid(1:ncols,1:nrows);
  for i = 1:numTheta
    % softly assign edge magnitudes similar to this orientation
    theta_i = (i-1)*pi/numTheta;
    fnorm_i = fnorm.*exp(-0.5*(theta - theta_i).^2/nu^2);
    
    G2a = sigma_u^2 * separable_filter(fnorm_i, row(dg2), column(g));
    G2b = sigma_u^2 * separable_filter(fnorm_i, row(dg),  column(dg));
    G2c = sigma_u^2 * separable_filter(fnorm_i, row(g),   column(dg2));
    
    ka = -cos(-theta_i + pi/2)^2;
    kb = 2*sin(-theta_i + pi/2)*cos(-theta_i + pi/2);
    kc = -sin(-theta_i + pi/2)^2;
    
    h_tilde = ka*G2a + kb*G2b + kc*G2c;
    
    xoffset = sigma(iSigma)*cos(theta_i);
    yoffset = sigma(iSigma)*sin(theta_i);
    h{i} = mexInterp2(h_tilde, xgrid - xoffset, ygrid - yoffset) + ...
      mexInterp2(h_tilde, xgrid + xoffset, ygrid + yoffset);
    
    if false && DebugDisplay
      G2i = column(g)*row(dg2)*ka + column(dg)*row(dg)*kb + column(dg2)*row(g)*kc;
      subplot(3,numTheta,i), imagesc(G2i), axis image
      subplot(3,numTheta,i+numTheta), imagesc(h_tilde), axis image
      title(sprintf('\\theta_%d = %.1f', i, theta_i*180/pi), 'FontSize', 16)
      subplot(3,numTheta,i+2*numTheta), imagesc(h{i}), axis image
    end
  end
  
  h_multiScale{iSigma} = mean(cat(3, h{:}), 3);
  
end

h_multiScale = cat(3, h_multiScale{:});
m = imregionalmax(h_multiScale);
index = find(m & h_multiScale>tau);
[my,mx,mscale] = ind2sub(size(m), index);

if DebugDisplay
  figure(2)
  subplot 131, hold off, imagesc(img), axis image, hold on
  plot(mx, my, 'r.', 'MarkerSize', 10);
  subplot 132, hold off, imagesc(f), axis image, hold on
  %mask = f > .05;
  %quiver(xgrid(mask), ygrid(mask), cos(theta(mask)), sin(theta(mask)));
  subplot 133, imagesc(fnorm), axis image
  %subplot 224, imagesc(h), axis image
end

end