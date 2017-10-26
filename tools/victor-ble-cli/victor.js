class Victor {
    static get MSG_MAX_SIZE() {
        return 20;
    }
    static get MSG_PAYLOAD_MAX_SIZE() {
        return 18;
    }
    static get MSG_BASE_SIZE() {
        return 1;
    }
    static get MSG_B2V_BTLE_DISCONNECT() {
        return 0x0D;
    }
    static get MSG_B2V_CORE_PING_REQUEST() {
        return 0x16;
    }
    static get MSG_V2B_CORE_PING_RESPONSE() {
        return 0x17;
    }
    static get MSG_B2V_HEARTBEAT() {
        return 0x18;
    }
    static get MSG_V2B_HEARTBEAT() {
        return 0x19;
    }
    static get MSG_B2V_WIFI_START() {
        return 0x1A;
    }
    static get MSG_B2V_WIFI_STOP() {
        return 0x1B;
    }
    static get MSG_B2V_DEV_PING_WITH_DATA_REQUEST() {
        return 0x91;
    }
    static get MSG_V2B_DEV_PING_WITH_DATA_RESPONSE() {
        return 0x92;
    }
    static get MSG_B2V_DEV_RESTART_ADBD() {
        return 0x93;
    }
    static get MSG_B2V_DEV_EXEC_CMD_LINE() {
        return 0x94;
    }
    static get MSG_V2B_DEV_EXEC_CMD_LINE_RESPONSE() {
        return 0x95;
    }
    static get MSG_B2V_MULTIPART_START() {
        return 0xF0;
    }
    static get MSG_B2V_MULTIPART_CONTINUE() {
        return 0xF1;
    }
    static get MSG_B2V_MULTIPART_FINAL() {
        return 0xF2;
    }
    static get MSG_V2B_MULTIPART_START() {
        return 0xF3;
    }
    static get MSG_V2B_MULTIPART_CONTINUE() {
        return 0xF4;
    }
    static get MSG_V2B_MULTIPART_FINAL() {
        return 0xF5;
    }
    constructor(peripheral, service, send, read, outputCallback) {

        this._peripheral = peripheral;
        this._service = service;
        this._send_char = send;
        this._read_char = read;
        this._output = outputCallback;
        this._outgoing_packets = [];
        this._incoming_packets = [];
        this._heartbeat_counter = 0;

        this._peripheral.on('disconnect', () => {
            clearInterval(this._interval);
        });

        this._interval = setInterval(() => {
            if (this._outgoing_packets.length == 0) { return ;}

            var packet = this._outgoing_packets.shift();

            this._send_char.write(packet);
        }, 10);

        var handleMessage = function(data) {
            if (!data) {
                return;
            }
            if (data.length < 2) {
                return;
            }
            var size = data[0];
            var msgID = data[1];
            switch (msgID) {
            case Victor.MSG_V2B_CORE_PING_RESPONSE:
                this._output("Ping Response");
                return;
            case Victor.MSG_V2B_HEARTBEAT:
                this._heartbeat_counter = data[2];
                this._output("Heartbeat " + this._heartbeat_counter);
                return;
            case Victor.MSG_V2B_DEV_EXEC_CMD_LINE_RESPONSE:
                this._output(data.toString('utf8', 2, data.length));
                return;
            case Victor.MSG_V2B_MULTIPART_START:
                this._incoming_packets = [];
                this._incoming_packets.push(Buffer.from(data));
                return;
            case Victor.MSG_V2B_MULTIPART_CONTINUE:
                this._incoming_packets.push(Buffer.from(data));
                return;
            case Victor.MSG_V2B_MULTIPART_FINAL:
                this._incoming_packets.push(Buffer.from(data));
                var totalLength = 0;
                for (var i = 0 ; i < this._incoming_packets.length ; i++) {
                    totalLength += (this._incoming_packets[i].length - 2);
                }
                var buf = Buffer.alloc(totalLength);
                var targetStart = 0;
                for (var i = 0 ; i < this._incoming_packets.length ; i++) {
                    targetStart += this._incoming_packets[i].copy(buf, targetStart, 2);
                }
                handleMessage.bind(this)(buf);
                this._incoming_packets = [];
                return;
            default:
                return;
            };

        };
        read.on('data', handleMessage.bind(this));
        read.subscribe();
    };

    _send (buffer) {
        this._outgoing_packets.push(buffer);
    };

    send (msgID, body) {
        var size = Victor.MSG_BASE_SIZE;
        if (body) {
            size += body.length;
        }
        const hdr = Buffer.from([size, msgID]);
        const buf = Buffer.concat([hdr, body]);
        if (buf.length > Victor.MSG_MAX_SIZE) {
            var off = 0;
            while (off < buf.length) {
                var msgSize = Victor.MSG_BASE_SIZE;
                var id;
                if (off == 0) {
                    id = Victor.MSG_B2V_MULTIPART_START;
                    msgSize += Victor.MSG_PAYLOAD_MAX_SIZE;
                } else if ((buf.length - off) > Victor.MSG_PAYLOAD_MAX_SIZE) {
                    id = Victor.MSG_B2V_MULTIPART_CONTINUE;
                    msgSize += Victor.MSG_PAYLOAD_MAX_SIZE;
                } else {
                    id = Victor.MSG_B2V_MULTIPART_FINAL;
                    msgSize += (buf.length - off);
                }
                const mhdr = Buffer.from([msgSize, id]);
                const mbuf = Buffer.concat([mhdr, buf.slice(off, off + msgSize - 1)]);
                this._send(mbuf);
                off += (msgSize - Victor.MSG_BASE_SIZE);
            }
        } else {
            this._send(buf);
        }
    };


    disconnect () {
        this._peripheral.disconnect();
    };
}

module.exports = Victor;
