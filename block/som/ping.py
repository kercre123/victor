import sys, time, socket
import messages

def ping(addr, count, interval):
    "Send a single ping message to an address"
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    for i in range(count):
        s.sendto(messages.PingMessage().serialize(), addr)
        time.sleep(interval)
    s.close()

if __name__ == '__main__':
    """Test mode: send a single ping message"""
    try:
        count = int(sys.argv[2])
    except:
        count = 4
    try:
        interval = float(sys.argv[3])
    except:
        interval = 1.0
    ping((sys.argv[1], 5551), count, interval)
