% function preprocessFace(im)

function preprocessFace(im)
    gamma = 0.2;
    
    % Gamma brighted and normalize from 0 to 1
    imGamma = (1/(255^gamma)) * double(im).^gamma;
    
    % TODO: figure out a way to use more bits
    imGamma(imGamma<0.5) = 0.5;
    
    % DoG filtering
    
    sigma0 = 1.0;
    sigma1 = 2.0;
    
    filter0Width = 2*ceil(sigma0*3*2/2) + 1;
    filter1Width = 2*ceil(sigma1*3*2/2) + 1;
    
    filter0 = fspecial('gaussian',[filter0Width,filter0Width],sigma0);
    filter1 = fspecial('gaussian',[filter1Width,filter1Width],sigma1);
    
    imF = imfilter(imGamma, filter1) - imfilter(imGamma, filter0);
    
%     imFScaled = imF - min(min(imF(10:(end-10),10:(end-10))));
%     imFScaled = imFScaled / max(max(imFScaled(10:(end-10),10:(end-10))));
    
    % first pass of robust filtering
    
    alpha = 0.1;
    tau = 10;
    
    divisor1 = mean(abs(imF(:)).^alpha) ^ (1/alpha);
    imF1 = imF .* (1./divisor1);
    
    divisor2 = mean(min(tau, abs(imF1(:))) .^ alpha).^(1/alpha);
    imF2 = imF1 .* (1./divisor2);
    
    keyboard