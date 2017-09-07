window.Unity = {
    call: function(msg) {
        var jsonMsg = JSON.stringify(msg);

        // Encode the stringified JSON object so that special chars like ', " and \ to make it all the way to unity.
        var encodedJsonMsg = encodeURIComponent(jsonMsg);

        var iframe = document.createElement('IFRAME');

        // The src attribute of an iframe is a URL. If we don't encode the JSON, this call will replace \" with /", which will break when Unity tries to read the string.
        iframe.setAttribute('src', 'unity:' + encodedJsonMsg); 

        document.documentElement.appendChild(iframe);
        iframe.parentNode.removeChild(iframe);
        iframe = null;
        
        // Support running codelab in a remote browser connected via the SDK
        if (gEnableSdkConnection) {
            var xhr = new XMLHttpRequest();
            xhr.open("POST", "sdk_call", true);
            xhr.send(encodedJsonMsg);
        }
    }
}


if (gEnableSdkConnection) {
    var _updateInterval = null;

    function startUpdateSdk() {
        if (_updateInterval == null) {
            console.log("UpdateSDK.startUpdateSdk.Starting")
            _updateInterval = setInterval(updateSdk , 10);
        }
        else {
            console.log("UpdateSDK.startUpdateSdk.AlreadyRunning!")
        }
    }

    function endUpdateSdk()
    {
        if (_updateInterval != null) {
            console.log("UpdateSDK.endUpdateSdk.Ending")
            clearInterval(_updateInterval);
            _updateInterval = null;
        }
    }

    function updateSdk()
    {
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function() {
            if (xhr.readyState == XMLHttpRequest.DONE) {
                if (xhr.status != 200) {
                    console.log("UpdateSDK.BadStatus: '" + xhr.status + "' - cancelling update");
                    endUpdateSdk();
                }
                else if (xhr.responseText != "OK") {
                    // TODO - encode multiple commands in one response to lower latency / allow higher command rate
                    eval(xhr.responseText);
                }
            }
        }

        xhr.onerror = function(e) {
            console.log("UpdateSDK.OnError: Error - no/bad response from server");
            endUpdateSdk();
        }

        if (gEnableSdkConnection && (_updateInterval != null)) {
            xhr.open("POST", "poll_sdk", true);
            xhr.send( null );
        }
    }

    startUpdateSdk();

    var xhr = new XMLHttpRequest();
    xhr.open("POST", "sdk_page_loaded", true);
    xhr.send( null );
}
