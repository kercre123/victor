
% function faceDetection_fddb_createTrainingSet()

% load('~/Documents/datasets/FDDB-folds/FDDB-ellipses.mat');
% load('~/Documents/datasets/FDDB-folds/FDDB-detectedPoses.mat');
% faceDetection_fddb_createTrainingSet(ellipses, allDetections, posemap, '~/Documents/Anki/products-cozmo-large-files/faceDetection/fddbFrontFaces.dat');

function faceDetection_fddb_createTrainingSet(ellipses, allDetections, posemap, outputFilename)
    
    showDetections = false;
    
    fileId = fopen(outputFilename, 'w');
    
    for iDetection = 1:length(allDetections)
        curImage = rgb2gray2(imread(allDetections{iDetection}{1}));
        
        goodFaces = zeros(0,4);
        
        for iFace = 1:length(allDetections{iDetection}{3})
            % Only use frontal and +-15 degrees faces
            if abs(posemap(allDetections{iDetection}{3}(iFace).c)) > 15
                continue;
            end
            
            x = mean(mean(allDetections{iDetection}{3}(iFace).xy(:,[1,3])));
            y = mean(mean(allDetections{iDetection}{3}(iFace).xy(:,[2,4])));
            
            %             keyboard
            
            curEllipses = ellipses{iDetection}{2};
            
            bestEllipseDistance = Inf;
            bestEllipseIndex = -1;
            for iEllipse = 1:length(curEllipses)
                curDistance = sqrt((x-curEllipses{iEllipse}(4)).^2 + (y-curEllipses{iEllipse}(5)).^2);
                
                if curDistance < bestEllipseDistance
                    bestEllipseDistance = curDistance;
                    bestEllipseIndex = iEllipse;
                end
            end % for iEllipse = minEllipse:maxEllipse
            
            if bestEllipseIndex ~= -1
                bestEllipse = curEllipses{bestEllipseIndex};
                bestEllipseHalfWidth = bestEllipse(2);
                
                if sqrt(2)*bestEllipseHalfWidth > bestEllipseDistance
                    if showDetections
                        hold off;
                        imshows(curImage);
                        hold on;
                        p = calculateEllipse(bestEllipse(4), bestEllipse(5), bestEllipse(2), bestEllipse(1), bestEllipse(3), 500);   plot(p(:,1), p(:,2), '.-');
                    end
                                        
                    curRect = [bestEllipse(4) - bestEllipse(2), bestEllipse(5) - bestEllipse(1), bestEllipse(2)*2, bestEllipse(1)*2];
                    
                    if showDetections
                        rectangle('Position', curRect);
                    end
                    
                    curRect = round(curRect);
                    curRect(1:2) = curRect(1:2) - 1; % matlab to c
                    curRect(1) = max(curRect(1), 0);
                    curRect(2) = max(curRect(2), 0);
                    
                    if curRect(1) + curRect(3) >= size(curImage,2)
                        curRect(3) =  size(curImage,2) - 1;
                    end
                    
                    if curRect(2) + curRect(4) >= size(curImage,1)
                        curRect(4) =  size(curImage,1) - 1;
                    end
                    
                    goodFaces(end+1, :) = curRect; %#ok<AGROW>
                    
                    if showDetections
                        pause(.25);
                    end
                end
            end % if bestEllipseIndex ~= -1
        end % for iFace = 1:length(allDetections{iDetection}{3})
        
        numGoodFaces = size(goodFaces,1);
        if numGoodFaces > 0
            ind = strfind(allDetections{iDetection}{1}, 'FDDB-originalPics');
            
            lineOut = sprintf('%s   %d', allDetections{iDetection}{1}(ind:end), numGoodFaces);
            
            for iFace = 1:numGoodFaces
                
                lineOut = [lineOut, sprintf('   %d %d %d %d', goodFaces(iFace,1), goodFaces(iFace,2), goodFaces(iFace,3), goodFaces(iFace,4))]; %#ok<AGROW>
            end
            
            lineOut = [lineOut, sprintf('\n')]; %#ok<AGROW>
            
            fwrite(fileId, lineOut);
        end % if numGoodFaces > 0
    end % for iDetection = 1:length(allDetections)
    
    fclose(fileId);
    
    %     curDetection = 0;
    %     for iEllipse = 1:length(allDetections)
    %         if curDetection == 0 || ~strcmp(ellipses{iEllipse}{1}, allDetections{curDetection}{1})
    %             curImage = rgb2gray2(imread(ellipses{iEllipse}{1}));
    %         end
    %
    %         keyboard
    %     end % for iEllipse = 1:length(allDetections)
    
end % function faceDetection_fddb_createTrainingSet()


function [X,Y] = calculateEllipse(x, y, a, b, angle, steps)
    % From http://stackoverflow.com/questions/2153768/draw-ellipse-and-ellipsoid-in-matlab
    %
    %# This functions returns points to draw an ellipse
    %#
    %#  @param x     X coordinate
    %#  @param y     Y coordinate
    %#  @param a     Semimajor axis
    %#  @param b     Semiminor axis
    %#  @param angle Angle of the ellipse (in degrees)
    %#
    
    narginchk(5, 6);
    if nargin<6, steps = 36; end
    
    beta = -angle * (pi / 180);
    sinbeta = sin(beta);
    cosbeta = cos(beta);
    
    alpha = linspace(0, 360, steps)' .* (pi / 180);
    sinalpha = sin(alpha);
    cosalpha = cos(alpha);
    
    X = x + (a * cosalpha * cosbeta - b * sinalpha * sinbeta);
    Y = y + (a * cosalpha * sinbeta + b * sinalpha * cosbeta);
    
    if nargout==1, X = [X Y]; end
end % function calculateEllipse()
