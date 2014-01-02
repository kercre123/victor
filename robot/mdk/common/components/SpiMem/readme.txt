Compile with 
make clean all CCOPT_EXTRA=-DSPI_FLASH_WRITE_ONE_BYTE_PER_COMMAND

if your flash chip only supports one byte to be programmed per 0x02 command.
For example the SST25VF016B behaves this way
