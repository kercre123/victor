# Espressif RAM program to load SPI flash faster

## Protocol
Once booted, the flasher will send `'0'` on the UART.

Sending a `'0'` to the Espressif will cause an `'0'` to be echoed back.

Otherwise commands are of the form:
* Command (1 byte)
* Address (3 bytes)
* Payload (0 or 64 bytes)

Replies are of the form:
* Result (1 byte) which is `'0'` on success or another number on error.
* Payload (0 or 64 bytes)

### Commands

#### Erase:
* Command: `'e'`
* Address, the sector of flash to erase. (Sectors are 4096 bytes)
* No payload

#### Read:
* Command: `'r'`
* Address: Byte offset in flash to read from, always reads 64 bytes.
* No payload
* On success, the response will be followed by the 64 bytes following the address specified.

#### Flash:
* Command: `'f'`
* Address: Byte offset in flash to write to, always writes 64 bytes.
* 64 bytes of data to write to address.
* On success, the response will include the 64 bytes read back from the same address to allow confirmation of successful write.

### Error codes

See [`flasher_error.h`](./flasher_error.h)
