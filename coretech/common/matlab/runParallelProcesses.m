% function runParallelProcesses(numComputeThreads, workQueue, matlabCommandString)

% numComputeThreads is the number of Matlab processes to create

% workQueue is a 1D cell array of work, that is automatically split between
% the different Matlab processes. Each process will have a cell array
% "localWorkQueue" in its Matlab workspace, to be used to determine what 
% pieces of work to compute.

% matlabCommandString is the command that is executed in each of the different Matlab processes

function runParallelProcesses(numComputeThreads, workQueue, temporaryDirectory, matlabCommandString)
    if numComputeThreads == 1 || isempty(workQueue)
        localWorkQueue = workQueue; %#ok<NASGU>
        eval(matlabCommandString);
    else
        if ispc()
            matlabLaunchCommand = 'matlab';            
            commandString = 'start /b ';
        else
            matlabLaunchCommand = [matlabroot, '/bin/matlab'];
            commandString = 'screen -A -m -d ';
        end 
        
        matlabLaunchParameters = ' -noFigureWindows -nosplash ';
        hideWindowsCommand = 'frames = java.awt.Frame.getFrames; for frameIdx = 3:length(frames) try awtinvoke(frames(frameIdx),''setVisible'',0); catch end; end;';
        
        threadCompletionMutexFilenames = cell(numComputeThreads, 1);
        workerInputFilenames = cell(numComputeThreads, 1);
        
        % launch threads
        for iThread = 0:(numComputeThreads-1)
            threadCompletionMutexFilenames{iThread+1} = [temporaryDirectory, sprintf('runParallelProcesses_mutex%d.mat', iThread)];
            workerInputFilenames{iThread+1} = [temporaryDirectory, sprintf('runParallelProcesses_input%d.mat', iThread)];
            
            localWorkQueue = workQueue((1+iThread):numComputeThreads:end); %#ok<NASGU>
            save(workerInputFilenames{iThread+1}, 'localWorkQueue');
            
            try
                oldWarnings = warning();
                warning('off', 'MATLAB:DELETE:FileNotFound');
                delete(threadCompletionMutexFilenames{iThread+1});
                warning(oldWarnings);
            catch
            end
            
            commandString = [commandString, matlabLaunchCommand, matlabLaunchParameters, ' -r "', hideWindowsCommand, '; load(''', workerInputFilenames{iThread+1},'''); ', matlabCommandString, '; mut=1; save(''', threadCompletionMutexFilenames{iThread+1}, ''',''mut''); exit;"'];
            
            system(commandString);
        end
        
        % Wait for the thread to complete
        for iThread = 1:length(threadCompletionMutexFilenames)
            while ~exist(threadCompletionMutexFilenames{iThread}, 'file')
                pause(.5);
            end
            delete(threadCompletionMutexFilenames{iThread});
            delete(workerInputFilenames{iThread});
        end
    end
end
