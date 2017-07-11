window.Unity = {
    call: function(msg) {
        var iframe = document.createElement('IFRAME');
        iframe.setAttribute('src', 'unity:' + msg);
        document.documentElement.appendChild(iframe);
        iframe.parentNode.removeChild(iframe);
        iframe = null;
        
        // Support running codelab in a remote browser connected via the SDK
        if (gEnableSdkConnection) {
            var xhr = new XMLHttpRequest();
            xhr.open("POST", "sdk_call", true);
            xhr.send( JSON.stringify( {msg} ) );
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
                        eval(xhr.responseText)
                    }
                }
            }

            xhr.open("POST", "poll_sdk", true);
            xhr.send( null );
        }
    }

    setInterval(updateSdk , 10);
}
