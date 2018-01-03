# Comms protocol between STM and Helper Head

Requirements:
+ Helper head must load firmware on the stm mcu, after which it becomes a slave device
+ Process commands >> << to update display, run tests (ble) etc.
+ Keep logs

#  Boot Sequence
At POR, or when manually triggered, the helper head is responsible for pushing 
updated firmware to the fixture's stm32 mcu.

### POR
STM will always enter BL by default and wait for update.  
BL uart settings: 115.2k, even parity...[AN3155]  
use the boot protocol to flash new fw [AN3155]  
?maybe read known flash location to see what version is already there? Probably too much work, ug.  
change uart settings for stm: 1Mbaud, 8N1  

### Manual update
may be initiated through adb (debug), or during factory software update (flash drive inserted with new firmware?)  
if stm is running application, first send command to reset, which then enters ROM BL.  
goto POR:  

### Reference/Notes
+ AN2606: system memory boot mode ("patterns" to force into ROM BL, hardware pins etc)  
+ AN3155: USART bootloader protocol  
  * auto-baud (up to 115.2k 'tested'), start/stop bits=1, even parity...  
  * waits for 0x7F (DEL) on USARTx (any?)  
  * sends ACK 0x79 ('y') -> start cmd processing  

# Command Protocols
STM communicates with Helper using ascii line-based commands and responses.  

### General Format
a line prefixed with ">>" identifies a command to be parsed. args follow on same line.  
a line prefixed with "<<" identifies the response. command word is repeated + status/error code (#) + optional data/information.  
any unprefixed line sent from stm to helper should be added to the log (if logging is enabled).  
any unprefixed line sent from helper to stm while waiting for a cmd response "<<" is ignored.  

### Example
STM->HELPER: >>commandX arg1 "this is arg2" argN  
HELPER->STM: helper may print any info for the log/console  
HELPER->STM: info lines are ignored (great for debug tho!). stm waits for the response prefix  
HELPER->STM: <<commandX status dat1 "this is dat2" datN  

### Tags
are a relic from the mystical drive era. Tags are in the format: "[TAG:VALUE]"
tags are depreciated and have no special meaning. treat as any other non-command line.

# Commands

### Sample display format:
```
       ______________________
line1 | line 1 small text   
line2 |    _   ___   ___
line3 |   | \   |   |    
line4 |   |_/   |   |  __
line5 |   | \   |   |   |
line6 |   |_/  _|_  |___|
line7 | line 7 small text
line8 |_line 8 small text____
```

`>>lcdset line_num Small Text String`  
`<<lcdset status`  
`line_num` 0 clear all lines, accompanying text ignored  
    1-8 selects the line to set  
`text` replace the line with provided string.  
    chars exceeding display width are ignored (no wrap)  
`status` 0 success  


`>>lcdshow solo color_code Big Centered Text`  
`<<lcdshow status`  
`solo` 0=display BIG text & small text together, 1=hide small text (show only BIG)  
`color_code` [i]{r,g,b,w} = [inverted]red,green,blue,white. invert is black text on colored background.  
    e.g. 'b' -> blue text on black background, 'ig' -> black text on green background  
`text` replace big text with provided string. This text is automatically centered.  
    chars exceeding display width are ignored (truncate text before centering)  
`status` 0 success  


### IN DEVELOPMENT
start/finish/pause adding stuff to the logfile (1 big logfile?)  
long term, each log might be saved into folders/files based on name,s/n info...  

`>>logstart [TESTNAME] [fd82802dcf6f5554] [v38] [sn:1212} [hwrev:2.0.1]`  
`<<logstart 0`  
`[ ]` possible future arguments that we don't currently support. maybe to help create unique log files.  
`status` 0 success (log opened for writing)  

`>>logstop [TESTNAME] [fd82802dcf6f5554] [v38] [sn:1212} [hwrev:2.0.1]`  
`<<logstop 0`  
`[ ]` possible future arguments that we don't currently support. maybe to help create unique log files.  
`status` 0 success (log closed/written to disk)  


### IN DEVELOPMENT
delegate dut head programming/OS-update to the helper.

`>>dutprogram timeout_s [head_id(%08x)]`  
`<<dutprogram 0`  
`timeout_s` if command not finished by timeout[s], helper must kill all running operations/processes and return an error code.
`[head_id]` potential future arg, not currently supported. unique ID (S/N?) to write to the DUT Head  
`status` 0 success (DUT head programmed successfully)

