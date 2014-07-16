% function runParallelProcesses(numComputeThreads, workQueue, matlabCommandString)

% numComputeThreads is the number of Matlab processes to create

% workQueue is a 1D cell array of work, that is automatically split between
% the different Matlab processes. Each process will have a cell array
% "localWorkQueue" in its Matlab workspace, to be used to determine what 
% pieces of work to compute.

% matlabCommandString is the command that is executed in each of the different Matlab processes

function runParallelProcesses(numComputeThreads, workQueue, matlabCommandString)
    if numComputeThreads == 1 || isempty(workQueue)
        localWorkQueue = workQueue; %#ok<NASGU>
        eval(matlabCommandString);
    else
        %             matlabLaunchCommand = 'matlab -nojvm -noFigureWindows -nosplash ';
        
        if ispc()
            matlabLaunchCommand = 'matlab';
            matlabLaunchParameters = ' -noFigureWindows -nosplash ';
        elseif ismac()
            asssert(false); % TODO: support this
            matlabLaunchCommand = matlabroot;
            matlabLaunchParameters = ' ';
        else
            asssert(false); % TODO: support this
        end
        
        hideWindowsCommand = 'frames = java.awt.Frame.getFrames; for frameIdx = 3:length(frames) try awtinvoke(frames(frameIdx),''setVisible'',0); catch end; end;';
        
        threadCompletionMutexFilenames = cell(numComputeThreads, 1);
        workerInputFilenames = cell(numComputeThreads, 1);
        
        % launch threads
        for iThread = 0:(numComputeThreads-1)
            threadCompletionMutexFilenames{iThread+1} = sprintf('runParallelProcesses_mutex%d.mat', iThread);
            workerInputFilenames{iThread+1} = sprintf('runParallelProcesses_input%d.mat', iThread);
            
            localWorkQueue = workQueue((1+iThread):numComputeThreads:end); %#ok<NASGU>
            save(workerInputFilenames{iThread+1}, 'localWorkQueue');
            
            try
                oldWarnings = warning();
                warning('off', 'MATLAB:DELETE:FileNotFound');
                delete(threadCompletionMutexFilenames{iThread+1});
                warning(oldWarnings);
            catch
            end
            
            if ispc()
                commandString = ['start /b '];
            elseif ismac()
                commandString = ['open -na '];
            else
                % TODO: support this
                assert(false);
            end
            
            commandString = [commandString, matlabLaunchCommand];
            
            if ispc()
                % Do nothing
            elseif ismac()
                commandString = [commandString, ' --args'];
            else
                % TODO: support this
                assert(false);
            end
            
            commandString = [commandString, matlabLaunchParameters, ' -r "', hideWindowsCommand, '; load(''', workerInputFilenames{iThread+1},'''); ', matlabCommandString, '; mut=1; save(''', threadCompletionMutexFilenames{iThread+1}, ''',''mut''); exit;"'];
            
            system(commandString);
        end
        
        % Wait for the thread to complete
        for iThread = 1:length(threadCompletionMutexFilenames)
            while ~exist(threadCompletionMutexFilenames{iThread}, 'file')
                pause(.1);
            end
            delete(threadCompletionMutexFilenames{iThread});
            delete(workerInputFilenames{iThread});
        end
    end
end
