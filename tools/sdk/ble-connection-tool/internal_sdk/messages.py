'''
Messages to be passed through the state machine and connection
'''


class MessageType:
    def __repr__(self):
        values = ', '.join([f"{k}={type(v).__name__}({v})"
                            for k, v in vars(self).items()])
        return f"{self.__class__.__name__}({values})"


class Pin(MessageType):
    def __init__(self, pin):
        self.pin = str(pin)


class DoubleClick(MessageType):
    def __init__(self, clicked):
        self.clicked = clicked


class NetworkPassword(MessageType):
    def __init__(self, password):
        self.password = password


class IPAddress(MessageType):
    def __init__(self, ip):
        self.ip = ip


class StartOTA(MessageType):
    def __init__(self, url):
        self.url = url


class Handshake(MessageType):
    def __init__(self, version):
        self.version = version # int(msg[1])
    
    def as_message(self):
        hs_version = b'\x01' + self.version.to_bytes(4, 'little')
        return hs_version 

    @classmethod
    def from_buffer(cls, msg):
        if len(msg) != 5 or msg[0] != 1:
            return None
        # TODO : this will need to handle versioning more correctly
        return Handshake(int(msg[1]))

