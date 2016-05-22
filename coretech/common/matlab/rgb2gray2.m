function I = rgb2gray2(X)
    
    if size(X, 3) == 1
        I = X;
    else
        I = rgb2gray(X);
    end