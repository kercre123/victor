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