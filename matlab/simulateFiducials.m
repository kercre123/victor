function simulateFiducials(varargin)

saveImages = false;
saveDir = '/Users/andrew/Code/blockIdentificationData/positivePatches';

baseImageDim = 64;

blockHalfWidth = 5;
dotRadius = blockHalfWidth - 1;

minElevation = 30;
numElevations = 5;

maxBlurSigma = 1; % starts at 0
numBlurSigma = 2;

minIllOrient = 15;
numIllorient = 5;

minDarkIll = 0.2;
maxDarkIll = 0.8;
numDarkIll = 4;

minContrast = 0.25;
contrastOvershoot = 0.1;
numContrast = 4;

parseVarargin(varargin{:});

if saveImages && ~isdir(saveDir)
    mkdir(saveDir)
end

blockSize = 2*blockHalfWidth + 1;

% for illumination simulation
[xgrid, ygrid] = meshgrid( (1:blockSize)-blockSize/2 );

% theta for plotting a circular patch
t = linspace(0, 2*pi, 200);

if ~saveImages
    namedFigure('Simulated Image');
    clf
    h_sim = imagesc(zeros(blockSize)); axis image off
    colormap(gray)
    %truesize(h_fig, 5*[blockSize blockSize])
end

namedFigure('Base Fiducial');
clf
pos = get(gcf, 'Pos');
set(gcf, 'Pos', [pos(1:2) baseImageDim baseImageDim]);

h_axes = axes('Pos', [0 0 1 1]);

grid off
axis image xy
set(gca, 'XTick', [], 'YTick', [], ...
    'XColor', 'w', 'YColor', 'w', ...
    'XLim', (blockHalfWidth+1)*[-1 1], ...
    'YLim', (blockHalfWidth+1)*[-1 1]);

C = onCleanup(@()multiWaitbar('CloseAll'));

multiWaitbar('Elevation Angles', 0)
for elevationAngle = linspace(minElevation,90,numElevations)
    
    azimuths = linspace(0, 180, 2+round((90-elevationAngle)/4));
    azimuths = azimuths(1:(end-1));
    
    numAzimuths = length(azimuths);
    multiWaitbar('Azimuth Angles', 0);
        
    for azimuthAngle = azimuths
        
        x = dotRadius*sin(elevationAngle*pi/180)*cos(t);
        y = dotRadius*sin(t);
        temp = [cos(azimuthAngle*pi/180) sin(azimuthAngle*pi/180); ...
            -sin(azimuthAngle*pi/180) cos(azimuthAngle*pi/180)] * [x; y];
        %h_dot = plot(temp(1,:), temp(2,:), 'k', 'LineWidth', 2);
        h_dot = patch(temp(1,:), temp(2,:), 'k', ...
            'Parent', h_axes);
        
        % Make sure dot gets drawn before we call getframe, otherwise we
        % might get a blank image
        drawnow;
        pause(0.05);
        
        imgBase = getframe(h_axes);
        imgBase = mean(im2double(imgBase.cdata), 3);
        
        imgBase = max(0, min(1, imresize(imgBase, [blockSize blockSize], 'bicubic')));
        
        for blurSigma = linspace(0, maxBlurSigma, numBlurSigma)
            
            if blurSigma > 0
                g = fspecial('gaussian', max(3, ceil(3*blurSigma)), blurSigma);
                imgBlur = imfilter(imgBase, g, 'replicate');
            else
                imgBlur = imgBase;
            end
            
            multiWaitbar('Illumination Orientation', 0);
            multiWaitbar('Illumination Orientation', 'CanCancel', 'on');
            for orient = linspace(-minIllOrient, -(180-minIllOrient), numIllorient)
                illDot = xgrid*cos(orient*pi/180) + ygrid*sin(orient*pi/180);
                minDot = min(illDot(:));
                maxDot = max(illDot(:));
                illDot = (illDot - minDot) / (maxDot - minDot);
                
                for dark = linspace(minDarkIll, maxDarkIll, numDarkIll)
                    
                    for contrast = linspace(minContrast, 1+contrastOvershoot-dark, numContrast)
                        
                        ill = contrast*illDot + dark;
                        
                        img = im2uint8(imgBlur .* ill);
                        
                        if saveImages
                            filename = sprintf('el%d_az%d_blur%d_ia%d_dark%d_contrast%d.png', ...
                                round(elevationAngle), round(azimuthAngle), ...
                                round(blurSigma), ...
                                round(orient), round(10*dark), round(10*contrast));
                            
                            imwrite(img, fullfile(saveDir, filename));
                        else
                            set(h_sim, 'CData', img);
                            drawnow
                            pause(.2)
                        end
                        
                    end % FOR each constrast
                    
                end % FOR each dark point
                
                cancelled = multiWaitbar('Illumination Orientation', 'Increment', 1/numIllorient);
                if cancelled, break, end
            end % FOR each ill orient
            
            if cancelled, break, end

        end % FOR each blurSigma
        
        delete(h_dot)
        
        if cancelled, break, end
        multiWaitbar('Azimuth Angles', 'Increment', 1/numAzimuths);
        
    end % FOR each azimuthAngle
    
    if cancelled, break, end
    multiWaitbar('Elevation Angles', 'Increment', 1/numElevations);
    
end % FOR each elevationAngle



