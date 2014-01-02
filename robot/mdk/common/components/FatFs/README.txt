/*!
 @defgroup FatFS FatFs Component
 @{
This is a modified version of the "FatFs" library,
based on the ff9a/R0.09a version from here:
http://elm-chan.org/fsw/ff/00index_e.html
(this link also includes documentation for now)

Your app/project will need to somehow include an ffconf.h
You can use the default ffconf.h just by adding FatFs/default_ffconf
sub-component to your ComponentList in your project Makefile,
or you can customize ffconf.h by making a copy of
FatFs/default_ffconf/leon_code/ffconf.h in your own project tree.

Also this component needs to link with the back-end functions
described in diskio.h. (disk_initialize, dist_status, disk_read, disk_write,
disk_ioctl, get_fattime). You can write these functions from scratch
in your app. Or you can include another component that implements
these functions.

FatFs/diskio_wrapper is a component that implements these.

FatFs/diskio_wrapper provides sub-components that make it easy
to interface with other parts of the MDK, without having to
implement these functions yourself.

FatFs/diskio_wrapper/SdMem implements a wrapper to the SdMem
component. Please see the README.txt of the SdMem component
for further details.

FatFs/diskio_wrapper/SpiMem implements a spi flash backend

FatFs/diskio_wrapper/DDRmem implements a ramdisk

FatFs/diskio_wrapper/Coalescer implements an intermediary module which
coalesces writes and reads. This should make SD card access faster.
For an example see testApps/components/SDHostFatFsWithCoalescer.
Coalescer may also be used with backends that require a big sector size,
if you want the filesystem to have a small sector size.
(No example available yet. future example will show how to use this with SpiMem)

Not yet implemented, future components will probably include
FatFs/diskio_wrapper/USB_mass_storage?
FatFs/diskio_wrapper/NAND_flash?

Please note:
+ You must give little-endian pointers to f_write_le or
    f_read_le (ie. buffers filled by shaves)
+ You must give big-endian pointers to f_write_be or
    f_read_be (ie. leon data structures)
+ The f_write fuction is different from official FatFs,
    it has a final argument which may be le_pointer, or be_pointer
+ the disk_read and disk_write functions implemented by a diskio_wrapper
    may be called with unaligned buffers.
+ If you want to see an example of SdMem in use with FatFs, take a look
    at testApps/components/SDHostFatFsTest

SdMem and FatFs and their dependencies probably won't all fit
into the 32 kilobyte LRAM. 
The correct way to put SdMem, and FatFs into CMX is to use a custom
ldscript. For an example of how to do it see
testApps/components/SDHostFatFsTest/scripts/SDHostFatFsTest.ldscript

wich contains something similar to:

@code
SECTIONS {
    .cmx.SomeOutputSectionName : {
        ./output/obj/drvCpr.o(.text.*)
        ./output/obj/drvTimer.o(.text.*)
        ./output/obj/sdMem.o(.text.*)
        ./output/obj/ff.o(.text.*)
    }
}
INSERT AFTER .cmx.text;

INCLUDE Myriad1_default_memory_map.ldscript
@endcode

Please note that the specified object filename must be exactly what
the 'sparc-elf-ld' receives on its command line, otherwise it thinks
it's a different file, and tries to link it twice.
 @}
*/
