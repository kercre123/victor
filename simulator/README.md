# Webots Simulator Setup

## Dependencies

### Matlab

Most of the vision processing, at least in the beginning, will be done with Matlab. To run simulations that process camera images you will need Matlab. 

You will also need to set `DYLD_LIBRARY_PATH` to include the path to your Matlab libs. Add the following to your `.bash_profile` or `.bashrc` (you'll need to correctly specify the version of Matlab you're using, e.g. '2012b'):

    export matlabroot="/Applications/MATLAB_R2012b.app"
    export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:${matlabroot}/bin/maci64:${matlabroot}/sys/os/maci6

## License

A license is required to run the Webots simulator. Submit a ticket to the Helpdesk to request a license. 

## Setup

Once Webots is installed, test it out by trying opening `simulator/worlds/bradsWorld.wbt`.

1. If you see errors regarding now being able to start Matlab you may need to add a simlink to your path:

        sudo ln -s /Applications/MATLAB_R2013a.app/bin/matlab /usr/bin/matlab

1. Go to the menu item Webots->Preferences. Select OpenGL and check the 'Disable shadows' box. Shadows can particularly interfere with the downward facing camera. We can assume this area will be actively lit on the robot.

1. Add `WEBOTS_HOME` environment variable by adding the following line to your `.bashrc` or `.bash_profile`:

        export WEBOTS_HOME=/Applications/Webots

This is necessary to build controller or supervisor projects from the command line, which seems necessary when you have more than one controller or supervisor in your world.

## Troubleshooting

### Insufficient shared memory

At times, I have seen error messages to the effect of 'cannot allocate sufficient shared memory'.  There appears to be a memory leak of some kind in Webots and you may need to clear them out manually.

Use the command line tool ipcs to view what shared memory segments are currently allocated in the system. Run

    ipcs -p

to view the shared memory segments. Verify that these segments are not associated with a running process by making sure that the associated process IDs do not refer to an existing  process. You can do this with the command

    ps -ax | grep <CPID or LPID>

Once you have verified these are orphaned shared memory segments, you can remove them with the command 

    ipcrm  -m <shared memory ID>   (The shared memory ID is under the ID column of the ipcs output)


You should also remove any semaphores with

    ipcrm -s <semaphore ID>

If the shared memory segments are still lingering around and their KEY reads 0, the parent process needs to be killed. Run the command

    kill -9 <LPID>

where LPID is the LPID of the shared memory segment. It's probably identical for all the segments.
