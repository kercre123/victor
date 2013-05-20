function simulateFiducials(varargin)

saveDir = '/Users/andrew/Code/blockIdentification/positivePatches';
blockSize = 12;

% All angles in degrees 
numViewAngles = 4;
minViewAngle = 50;
maxViewAngle = 100;

minElevation = 30;
numElevations = 5;

maxBlurSigma = 1;
numBlurSigma = 2;

minIllOrient = 15;
numIllorient = 5;

minDarkIll = 0.1;
maxDarkIll = 0.5;
numDarkIll = 4;

minContrast = 0.25;
contrastOvershoot = 0.1;
numContrast = 4;

parseVarargin(varargin{:});

[xgrid, ygrid] = meshgrid( (1:blockSize)-blockSize/2 );

h_fig = namedFigure('Fiducial Simulation', 'Color', 'w');

pos = get(h_fig, 'Position');
set(h_fig, 'Position', [pos(1:2) 256 256]);

clf
rectangle('Pos', [.25 .25 .5 .5], 'Curv', [1 1], ...
    'FaceColor', 'k', 'EdgeColor', 'k');
axis image off
set(gca, 'XLim', [0 1], 'YLim', [0 1]);

dot = getframe(gca);
dot = dot.cdata;

warp(dot)
axis image off

xcen = size(dot,2)/2;
ycen = size(dot,1)/2;

camtarget([xcen, ycen, 0])

dist = max(xcen,ycen);

view(2)
camup([0 1 0]);

C = onCleanup(@()multiWaitbar('CloseAll'));

multiWaitbar('View Angles', 0)
for viewAngle = linspace(minViewAngle, maxViewAngle, numViewAngles)
    
    camva(viewAngle)
    
    multiWaitbar('Elevations', 0);
    for elevation = linspace(minElevation, 90, numElevations)
        
        azimuths = linspace(0, 360, 2+round((90-elevation)/2));
        azimuths = azimuths(1:(end-1));
        
        numAzimuths = length(azimuths);
        multiWaitbar('Azimuths', 0);
        multiWaitbar('Azimuths', 'CanCancel', 'on');
        
        for azimuth = azimuths
            
            [x,y,z] = sph2cart(azimuth*pi/180, ...
                elevation*pi/180, dist);
            
            campos([x + xcen, y + ycen, z])
            
            imgBase = getframe(h_fig);
            imgBase = mean(im2double(imgBase.cdata), 3);
            
            for blurSigma = linspace(0, maxBlurSigma, numBlurSigma)
                
                imgBase = imresize(imgBase, [blockSize blockSize], 'bilinear');
                
                if blurSigma > 0
                    g = fspecial('gaussian', max(3, ceil(3*blurSigma)), blurSigma);
                    imgBase = imfilter(imgBase, g, 'replicate');
                end
                
                for orient = linspace(-minIllOrient, -(180-minIllOrient), numIllorient)
                    illDot = xgrid*cos(orient*pi/180) + ygrid*sin(orient*pi/180);
                    minDot = min(illDot(:));
                    maxDot = max(illDot(:));
                    illDot = (illDot - minDot) / (maxDot - minDot);
                    
                    for dark = linspace(minDarkIll, maxDarkIll, numDarkIll)
                        
                        for contrast = linspace(minContrast, 1+contrastOvershoot-dark, numContrast)
                            
                            ill = contrast*illDot + dark;
                            
                            img = im2uint8(imgBase .* ill);
                            
                            filename = sprintf('va%d_el%d_az%d_blur%d_ia%d_dark%d_contrast%d.png', ...
                                round(viewAngle), round(elevation), round(azimuth), ...
                                round(blurSigma), ...
                                round(orient), round(10*dark), round(10*contrast));
                            
                            %                         namedFigure('Simulated Image')
                            %                         imshow(img)
                            %                         truesize(gcf, 5*[blockSize blockSize])
                            
                            imwrite(img, fullfile(saveDir, filename));
                        end
                        
                    end
                end % FOR each ill orient
                
            end % FOR each blurSigma
            
            cancelled = multiWaitbar('Azimuths', 'Increment', 1/numAzimuths);
            
            if cancelled
                break;
            end
        end % FOR each azimuth
        
        multiWaitbar('Elevations', 'Increment', 1/numElevations);
        
        if cancelled
            break;
        end
    end % FOR each elevation
    
    multiWaitbar('View Angles', 'Increment', 1/numViewAngles);
    
    if cancelled
        break;
    end
end % FOR each view angle

end
