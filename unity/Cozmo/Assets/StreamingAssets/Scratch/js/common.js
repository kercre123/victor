(function () {
 
    /**
     * Window "onload" handler.
     * @return {void}
     */
    function onLoad () {
        // Instantiate the VM and create an empty project
        var vm = new window.VirtualMachine();
        window.vm = vm;
        window.vm.createEmptyProject();

        // Create audio player and load sounds
        var useWebAudioAPI = true;
        var ua = navigator.userAgent;
        if( ua.indexOf("Android") >= 0 ) {
            // Web Audio API is on Android 5.0 and higher, not on Android 4.4 and lower.
            var androidversion = parseFloat(ua.slice(ua.indexOf("Android")+8)); 
            if (androidversion < 5.0)
            {
                useWebAudioAPI = false;
            }
        }
        else if ( ua.indexOf("iPhone OS 7") >= 0 ) {
            // Turn sound off for Unity Editor WebView, otherwise webview content
            // will fail to load, as apparently the Unity Editor WebView
            // doesn't support the sound code in our simpleWebAudioPlayer.
            useWebAudioAPI = false;
        }

        if (useWebAudioAPI) {
            window.player = simpleWebAudioPlayer();
            window.player.load({name: "click", src: click_wav});
            window.player.load({name: "delete", src: delete_wav});
        }
 
        // Get XML toolbox definition
        var toolbox = document.getElementById('toolbox');
        window.toolbox = toolbox;
 
        // Instantiate scratch-blocks and attach it to the DOM.
        var workspace = window.Blockly.inject('blocks', {
            media: './lib/blocks/media/',
            scrollbars: false,
            trashcan: true,
            horizontalLayout: true,
            toolbox: window.toolbox,
            sounds: true,
            zoom: {
                controls: false,
                wheel: false,
                startScale: 1.2
            },
            colours: {
                workspace: '#334771',
                flyout: '#283856',
                scrollbar: '#24324D',
                scrollbarHover: '#0C111A',
                insertionMarker: '#FFFFFF',
                insertionMarkerOpacity: 0.3,
                fieldShadow: 'rgba(255, 255, 255, 0.3)',
                dragShadowOpacity: 0.6
            }
        });
        window.workspace = workspace;
 
        // Disable long-press
        // TODO Need to turn this off for tool tips?
        Blockly.longStart_ = function() {};
 
        // Attach blocks to the VM
        workspace.addChangeListener(vm.blockListener);
        var flyoutWorkspace = workspace.getFlyout().getWorkspace();
        flyoutWorkspace.addChangeListener(vm.flyoutBlockListener);
 
        // Handle VM events
        vm.on('STACK_GLOW_ON', function(data) {
            workspace.glowStack(data.id, true);
        });
        vm.on('STACK_GLOW_OFF', function(data) {
            workspace.glowStack(data.id, false);
        });
        vm.on('BLOCK_GLOW_ON', function(data) {
            workspace.glowBlock(data.id, true);
        });
        vm.on('BLOCK_GLOW_OFF', function(data) {
            workspace.glowBlock(data.id, false);
        });
        vm.on('VISUAL_REPORT', function(data) {
            workspace.reportValue(data.id, data.value);
        });
 
        // Run threads
        vm.start();
 
        // DOM event handlers
        var greenFlag = document.querySelector('#greenflag');
        var stop = document.querySelector('#stop');
        greenFlag.addEventListener('click', function () {
            vm.greenFlag();
        });
        greenFlag.addEventListener('touchmove', function (e) {
            e.preventDefault();
        });
        stop.addEventListener('click', function () {
            vm.stopAll();
            if (typeof window.extensions !== 'undefined') {
                window.extensions.postMessage({
                    extension: 'wedo',
                    method: 'motorStop',
                    args: []
                });
            }
        });
        stop.addEventListener('touchmove', function (e) {
            e.preventDefault();
        });
 
        // Extension event handlers
        bindExtensionHandler();
 
        // Handle platform-specific style adjustments
        if (window.isIphone) {
            workspace.setScale(0.85);
            document.getElementById('navigation').style.zoom = "80%"
        }
    }
 
    /**
     * Binds the extension interface to `window.extensions`.
     * @return {void}
     */
    function bindExtensionHandler () {
        if (typeof webkit === 'undefined') return;
        if (typeof webkit.messageHandlers === 'undefined') return;
        if (typeof webkit.messageHandlers.extensions === 'undefined') return;
        window.extensions = webkit.messageHandlers.extensions;
    }
 
    /**
     * Extension "connect" handler.
     * @return {void}
     */
    function onConnect () {
        var di = document.querySelector('#navigation .device button');
        di.classList.add('connected');
    }
 
    /**
     * Extension "disconnect" handler.
     * @return {void}
     */
    function onDisconnect () {
        // Original code
        //var di = document.querySelector('#navigation .device button');
        //di.classList.remove('connected');
 
        // TODO Quick and dirty: hide a UI element so the end user can see it is disconnected (green flag in lower right).
        // If Cozmo actually disconnects, what should happen? Dialog to user? Save scripts and exit webview?
        var greenFlagLittle = document.getElementById("greenflag");
        greenFlagLittle.style.display='none';
 
        vm.stopAll();
    }
 
    /**
     * Bind event handlers.
     */
    window.onload = onLoad;
    window.onExtensionConnect = onConnect;
    window.onExtensionDisconnect = onDisconnect;
})();
