% function faceDetection_fddb_convertEllipseFiles()

% ellipses = faceDetection_fddb_convertEllipseFiles('~/Documents/datasets/FDDB-originalPics', '~/Documents/datasets/FDDB-folds');
% save('~/Documents/datasets/FDDB-folds/FDDB-ellipses.mat', 'ellipses');

function ellipses = faceDetection_fddb_convertEllipseFiles(fddbImageDirectory, fddbFoldsDirectory)

    ellipseFilenames = {};
    for iFold = 1:10
        ellipseFilenames{end+1} = [fddbFoldsDirectory, sprintf('/FDDB-fold-%02d-ellipseList.txt',iFold)]; %#ok<AGROW>
    end

    ellipses = cell(0,1);
    for iFold = 1:10
        fileID = fopen(ellipseFilenames{iFold}, 'r');
        tline = fgets(fileID);

        state = 0; % 0=imageFilename 1=numEllipses 2=ellipses

        while tline ~= -1
            if state == 0
                curFilename = [fddbImageDirectory, '/', tline(1:(end-1)), '.jpg'];
                curEllipses = {};
                state = 1;
            elseif state == 1
                numEllipsesLeft = str2num(tline);
                state = 2;
            elseif state == 2
                curEllipse = str2num(tline);
%                 ellipses{end+1} = {curFilename, curEllipse(1:(end-1))}; %#ok<AGROW>
                curEllipses{end+1} = curEllipse(1:(end-1)); %#ok<AGROW>

                numEllipsesLeft = numEllipsesLeft - 1;

                if numEllipsesLeft == 0
                    ellipses{end+1} = {curFilename, curEllipses}; %#ok<AGROW>
                    state = 0;
                end
            else
                assert(false);
            end

            tline = fgets(fileID);
        end % while ~empty(tline)

        fclose(fileID);
    end % for iFold = 1:10
end % function faceDetection_fddb_convertEllipseFiles()
