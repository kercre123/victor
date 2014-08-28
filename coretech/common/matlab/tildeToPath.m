% function realPath = tildeToPath()

% Returns the string equivalent of '~'

function realPath = tildeToPath()
    if ispc()
        realPath = '';
    else
        oldPath = pwd();
        cd('~');
        realPath = pwd();
        cd(oldPath);
    end
end