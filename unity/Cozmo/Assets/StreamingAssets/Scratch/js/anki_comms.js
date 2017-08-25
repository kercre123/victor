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
    function updateSdk()
    {
        if (gEnableSdkConnection) {
            var xhr = new XMLHttpRequest();
            xhr.onreadystatechange = function() {
                if (xhr.readyState == XMLHttpRequest.DONE) {
                    // Handle each response in turn?
                    if (xhr.responseText != "OK") {
                        eval(xhr.responseText);
                    }
                }
            }

            xhr.open("POST", "poll_sdk", true);
            xhr.send( null );
        }
    }

    setInterval(updateSdk , 10);
}
