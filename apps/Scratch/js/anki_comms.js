String.prototype.replaceAll = function(search, replacement) {
    var target = this;
    return target.split(search).join(replacement);
};

window.Unity = {
    _firstMsgSentTime: null,
    _numMsgSentThisPeriod: 0,
    _pendingMessages: [],
    _pendingMessagesQueueTime: null,
    _flushTimeoutId: null,
    call: function(msg, forceFlush) {
        var kMaxPendingMessages = 250;
        var kMaxTimeQueued_ms = 15;

        var now = new Date().getTime();

        // Always add the message (as the Json structure is now always a list of messages, even for 1 message)
        this._pendingMessages.push( msg );
        if (this._pendingMessagesQueueTime == null) {
            this._pendingMessagesQueueTime = now;
        }
                
        // If this message has a valid requestId, then it will wait on a promise, and we should flush + send this immediately
        // Otherwise the caller can specifically request the messages are flushed immediately, or the flush will occur when
        // either (a) sufficient messages are queued or (b) the queue is too old
        var hasValidRequestId = (msg.requestId != "undefined") && (msg.requestId >= 0);
        var timeQueued = (this._pendingMessagesQueueTime != null) ? (now - this._pendingMessagesQueueTime) : 0;
        var flushNow = forceFlush || hasValidRequestId || (this._pendingMessages.length > kMaxPendingMessages) || (timeQueued > kMaxTimeQueued_ms);

        if (flushNow) {
            this.flushMessageList();
        }
        else {
            // If there isn't already a timer running (to ensure this is flushed within a reasonable time) then add one now
            if (this._flushTimeoutId == null) {
                this._flushTimeoutId = setTimeout( function() { window.Unity.flushMessageList(); }, kMaxTimeQueued_ms );
            }
        }     
    },
    flushMessageList: function() {
        // If there's a timer then clear it now
        if (this._flushTimeoutId != null) {
            clearTimeout(this._flushTimeoutId);
            this._flushTimeoutId = null;
        }
        // If there are any messages to send then send them now and clear the queue
        if (this._pendingMessages.length > 0) {
            this.callWithMessageList({messages: this._pendingMessages});
            // Clear the queue
            this._pendingMessages = [];
            this._pendingMessagesQueueTime = null;
        }
    },
    callWithMessageList: function(msg) {
        var jsonMsg = JSON.stringify(msg);

        // Encode the stringified JSON object so that special chars like ', " and \ to make it all the way to unity.
        var encodedJsonMsg = encodeURIComponent(jsonMsg);

        // Support running codelab in a remote browser connected via the SDK
        if (gEnableSdkConnection) {
            var xhr = new XMLHttpRequest();
            xhr.open("POST", "sdk_call", true);
            xhr.send(encodedJsonMsg);
        }
        else {
            var iframe = document.createElement('IFRAME');
    
            // The src attribute of an iframe is a URL. If we don't encode the JSON, this call will replace \" with /", which will break when Unity tries to read the string.
            iframe.setAttribute('src', 'unity:' + encodedJsonMsg); 
    
            document.documentElement.appendChild(iframe);
            iframe.parentNode.removeChild(iframe);
            iframe = null;    
        }

        if (this._firstMsgSentTime == null) {
            this._firstMsgSentTime = new Date().getTime();
            this._numMsgSentThisPeriod = 0;
        }
        else {
            ++this._numMsgSentThisPeriod;
        }
    },
    resetMsgTracking: function() {
        // Reset - start a new period with the next message
        this._firstMsgSentTime = null;
        this._numMsgSentThisPeriod = 0;
    },
    calculateSleepRequiredToThrottle: function() {
        if (this._numMsgSentThisPeriod > 30) {
            var duration = new Date().getTime() - this._firstMsgSentTime;
            // Cap calls to 30 per 100ms, e.g ~10 calls per 33ms (0.033s) Unity tick
            // Note: Clock was doing ~9 calls in 0.2s = 200ms
            var minDuration = 100;
            if (duration < minDuration) {
                return (minDuration - duration);
            }
            else {
                // Return negative value to indicate evaluated period was
                // fine and it should reset.
                return -1;
            }     
        }

        return 0;
    },
    sleepPromiseIfNecessary: function() {
        // For blocks that don't create a promise (or other means of sleeping), this automatically
        // creates one if necessary to throttle the message rate
        // (and prevent Scratch locking up in busy-loops that call into Unity a lot)
        var sleepRequired_ms = this.calculateSleepRequiredToThrottle();
        if (sleepRequired_ms > 0) {
            // Don't reset the tracking - that's done as soon as it's slept for long enough
            return new Promise(function (resolve) {
                setTimeout(function(){
                    resolve();                    
                }, sleepRequired_ms);
            });
        }
        else if (sleepRequired_ms < 0) {
            // Reset it so that we can respond to future spikes
            // without previous pauses making the average look OK
            this.resetMsgTracking();
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
                    try {
                        eval(xhr.responseText);
                    }
                    catch(err) {
                        if (err.message == "window.setCozmoState is not a function") {
                            // Ignore - this happens whenever on e.g. projects page in Chrome
                            // but the app is in the workspace (and Unity is sending setCozmoState)
                        }
                        else {
                            console.log("updateSdk.eval.error: " + err.message);
                            console.log("" + xhr.responseText);
                        }
                    }
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
