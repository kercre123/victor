const WiFiAuth = {
    AUTH_NONE_OPEN : {value: 0, name: "None" },
    AUTH_NONE_WEP :  {value: 1, name: "WEP" },
    AUTH_NONE_WEP_SHARED : {value: 2, name: "WEP Shared"},
    AUTH_IEEE8021X: {value: 3, name: "IEEE8021X"},
    AUTH_WPA_PSK: {value: 4, name: "WPA PSK"},
    AUTH_WPA_EAP: {value: 5, name: "WPA EAP"},
    AUTH_WPA2_PSK: {value: 6, name: "WPA2 PSK"},
    AUTH_WPA2_EAP: {value: 7, name: "WPA2 EAP"}
};

module.exports = WiFiAuth;
