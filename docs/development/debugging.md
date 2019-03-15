# Victor Debugging

Debugging on Victor is provided by `lldb` and can be used via the command-line or [vscode](vscode.md).

You can also use `gdb` on the robot itself.

## Address spaces

The heap, starts at `0xadxxxxxx` and grows down
The stack, starts at `0xa7xxxxxx` and grows up

Static variables are at: `0x80xxxxxx`
Global variables are at: `0x80xxxxxx`
Code and function pointers are at: `0x7fxxxxxx`

## Running a process standalone

```bash
systemctl stop victor.target
systemctl restart vic-robot
/anki/bin/vic-anim -c /anki/etc/config/platform_config.json
```

## Running a process with the address sanitizer

```bash
systemctl stop victor.target
systemctl restart vic-robot
ASAN_OPTIONS=detect_container_overflow=0 /anki/bin/vic-anim -c /anki/etc/config/platform_config.json
ASAN_OPTIONS=detect_container_overflow=0:replace_intrin=0 /anki/bin/vic-anim -c /anki/etc/config/platform_config.json

```

## Running a process with the address sanitizer in gdb

```bash
systemctl stop victor.target
systemctl restart vic-robot
ASAN_OPTIONS=detect_container_overflow=0:replace_intrin=0 VIC_ANIM_CONFIG=/anki/etc/config/platform_config.json gdb /anki/bin/vic-anim
```

Note: setting arguments within `gdb` does not work, for example:

```bash
(gdb) set args "-c" "/anki/etc/config/platform_config.json"
(gdb) set args -c /anki/etc/config/platform_config.json
(gdb) run "-c /anki/etc/config/platform_config.json"
```

## Useful gdb commands

`info address <symbol>`

`info symbol <address>`

## Robot setup

- modify the robot filesystem to allow writes

```bash
ssh root@${ANKI_ROBOT_HOST}
mount -o remount r,w /
```

- check if lldb is running

```bash
ps | grep lldb
```

and if you're having problems kill those processes

```bash
kill -9 <process id>
```

## Development setup

- start lldb server on robot

```bash
./project/victor/scripts/start-lldb.sh
```

Note that `start-lldb.sh` will make changes to firewall settings on your robot!
These changes will remain in effect until the robot is rebooted.

- start lldb on build host

```bash
lldb
```

- connect to lldb server on robot

```lldb
platform select remote-linux
platform connect connect://victor:55001
settings set target.exec-search-paths ~/projects/victor/_build/vicos/Debug/bin ~/projects/victor/_build/vicos/Debug/lib
```

## Useful lldb commands

- attach -- Attach to a process

(but first, ssh into robot and get a list of process ids with `ps | grep vic-`)

```bash
process attach --pid 1905

Process 1905 stopped
* thread #1, name = 'logwrapper', stop reason = signal SIGSTOP
    frame #0: 0xaaa722ec libc.so.6`__poll + 28
libc.so.6`__poll:
->  0xaaa722ec <+28>: svc    #0x0
    0xaaa722f0 <+32>: ldr    r7, [sp], #4
    0xaaa722f4 <+36>: cmn    r0, #4096
    0xaaa722f8 <+40>: bxlo   lr
Target 0: (logwrapper) stopped.

Executable module set to "/Users/richard/.lldb/module_cache/remote-android/.cache/A60A8620-D112-9948-D962-C0ECAC87E4D7-98BC36BD/logwrapper".
Architecture set to: arm--linux-eabi.
```

- Show status and stop location for the current target process.

```lldb
(lldb) process status
Process 1905 stopped
* thread #1, name = 'logwrapper', stop reason = signal SIGSTOP
    frame #0: 0xaaa722ec libc.so.6`__poll + 28
libc.so.6`__poll:
->  0xaaa722ec <+28>: svc    #0x0
    0xaaa722f0 <+32>: ldr    r7, [sp], #4
    0xaaa722f4 <+36>: cmn    r0, #4096
    0xaaa722f8 <+40>: bxlo   lr
```

- Show the stack trace for everything

```lldb
(lldb) bt all
warning: (arm) /Users/richard/projects/victor/_build/vicos/Debug/lib/libcubeBleClient.so 0x91f00010526: DW_AT_specification(0x000009a9) has no decl

warning: (arm) /Users/richard/projects/victor/_build/vicos/Debug/lib/libcozmo_engine.so 0x11164f000217ea: DW_AT_specification(0x00001321) has no decl

* thread #1, name = 'vic-engine', stop reason = signal SIGSTOP
    frame #0: 0xb6be3814 libc.so.6`nanosleep + 68
    frame #1: 0xb6c172e0 libc.so.6`usleep + 76
  * frame #2: 0x7f5632d6 vic-engine
    frame #3: 0xb6b581b4 libc.so.6`__libc_start_main + 272
  thread #2, name = 'civetweb-master', stop reason = signal SIGSTOP
    frame #0: 0xb6c10314 libc.so.6`__poll + 68
    ...
  thread #3, name = 'civetweb-worker', stop reason = signal SIGSTOP
    frame #0: 0xb6b23d2c libpthread.so.0`pthread_cond_wait + 300
    ...
  thread #4, name = 'civetweb-worker', stop reason = signal SIGSTOP
    frame #0: 0xb6b23d2c libpthread.so.0`pthread_cond_wait + 300
    ...
```

- Pick a thread

```lldb
thread select 17
* thread #17, name = 'vic-anim', stop reason = signal SIGSTOP
    frame #0: 0xa7ea5854 libc.so.6`__read + 68
libc.so.6`__read:
->  0xa7ea5854 <+68>: svc    #0x0
    0xa7ea5858 <+72>: mov    r7, r0
    0xa7ea585c <+76>: mov    r0, r12
    0xa7ea5860 <+80>: bl     0xa7ec2b60
```

- Pick a stack frame

```lldb
frame select 1
frame #1: 0x7f5e916c vic-anim
    0x7f5e916c: ldmdals r5, {r0, r2, r4, r12, pc}
    0x7f5e9170: svclo  #0xfff1b0
    0x7f5e9174: .long  0xe7ffdc48                ; unknown opcode
    0x7f5e9178: stmdage pc!, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, sp, lr, pc}
```
