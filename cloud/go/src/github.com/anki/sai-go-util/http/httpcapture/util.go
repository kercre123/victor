package httpcapture

import (
	"net"
	"strings"
)

func getIP() string {
	ifaces, err := net.Interfaces()
	if err != nil {
		return "unknown"
	}
	for _, iface := range ifaces {
		if (iface.Flags&net.FlagUp > 0) && (iface.Flags&net.FlagLoopback == 0) && (strings.HasPrefix(iface.Name, "en") || strings.HasPrefix(iface.Name, "eth")) {
			addrs, _ := iface.Addrs()
			if addr := findv4(addrs); addr != "" {
				return addr
			}
		}
	}
	return "unknown"
}

func findv4(addrs []net.Addr) (result string) {
	for _, addr := range addrs {
		switch ip := addr.(type) {
		case *net.IPNet:
			if v4 := ip.IP.To4(); v4 != nil {
				return v4.String()
			}
			result = ip.IP.String()
		case *net.IPAddr:
			if v4 := ip.IP.To4(); v4 != nil {
				return v4.String()
			}
			result = ip.IP.String()
		}
	}
	return result
}
