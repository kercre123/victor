(function () {
    const Scratch = window.Scratch = window.Scratch || {};

    /**
     * Window "onload" handler.
     * @return {void}
     */
    function onLoad () {
        window.TABLET_WIDTH = 800;

        // Instantiate the VM and create an empty project
        const vm = new window.VirtualMachine();
        Scratch.vm = vm;

        // Provided by Scratch team and reduced to remove unused assets.
        var emptyProjectJSONString = '{' +
            '"objName": "Stage",' +
            '"currentCostumeIndex": 0,' +
            '"penLayerID": -1,' +
            '"tempoBPM": 60,' +
            '"videoAlpha": 0.5,' +
            '"children": [{' +
                    '"objName": "Sprite1",' +
                    '"currentCostumeIndex": 0,' +
                    '"scratchX": 0,' +
                    '"scratchY": 0,' +
                    '"scale": 1,' +
                    '"direction": 90,' +
                    '"rotationStyle": "normal",' +
                    '"isDraggable": false,' +
                    '"indexInLibrary": 1,' +
                    '"visible": true,' +
                    '"spriteInfo": {' +
                    '}' +
            '}]' +
        '}';

        vm.loadProject(emptyProjectJSONString);

        // Get XML toolbox definition
        var toolbox = document.getElementById('toolbox');
        window.toolbox = toolbox;

        var toolboxPosition = 'end';
        var controls = false;
        var startScale = 1.2;
        if (window.isVertical) {
            toolboxPosition = 'start';
            controls = true;
            startScale = 0.80;
        }

        // Instantiate scratch-blocks and attach it to the DOM.
        var workspace = window.Blockly.inject('blocks', {
            media: './lib/blocks/media/',
            scrollbars: true,
            trashcan: false,
            horizontalLayout: !window.isVertical,
            toolbox: window.toolbox,
            toolboxPosition: toolboxPosition,
            sounds: true,
            zoom: {
                controls: controls,
                wheel: false,
                startScale: startScale,
                maxScale: 2,
                minScale: 0.5,
                scaleSpeed: 1.1
            }
        });
        Scratch.workspace = workspace;

        // Create an array which stores resolve functions indexed by requestId.
        // These will be used as a means of communication between the JavaScript
        // and C# code to know when the robot is done with his work and the vm
        // should proceed to the next block.
        window.resolveCommands = [];

        // Attach scratch-blocks events to VM.
        workspace.addChangeListener(vm.blockListener);
        const flyoutWorkspace = workspace.getFlyout().getWorkspace();
        flyoutWorkspace.addChangeListener(vm.flyoutBlockListener);
        flyoutWorkspace.addChangeListener(vm.monitorBlockListener);

        // Handle VM events
        vm.on('SCRIPT_GLOW_ON', function(data) {
            Scratch.workspace.glowStack(data.id, true);
        });
        vm.on('SCRIPT_GLOW_OFF', function(data) {
            Scratch.workspace.glowStack(data.id, false);
        });
        vm.on('BLOCK_GLOW_ON', function(data) {
            Scratch.workspace.glowBlock(data.id, true);
        });
        vm.on('BLOCK_GLOW_OFF', function(data) {
            Scratch.workspace.glowBlock(data.id, false);
        });
        vm.on('VISUAL_REPORT', function(data) {
            Scratch.workspace.reportValue(data.id, data.value);
        });

        // Run threads
        vm.start();

        // DOM event handlers
        var closeButton = document.querySelector('#closebutton');
        var challengesButton = document.querySelector('#challengesbutton');
        var greenFlag = document.querySelector('#greenflag');
        var stop = document.querySelector('#stop');

        // TODO Temporary hack to make close button visible
        if (window.isVertical) {
            var close = document.querySelector('#close');
            close.style.left = 290 + "px";
        }

        window.resolvePromiseWaitForSaveProject = null;
        window.saveProjectCompleted = function (unityIsWaitingForCallback) {
            // If we have a Promise to resolve, resolve it
            if (window.resolvePromiseWaitForSaveProject) {
                window.resolvePromiseWaitForSaveProject();
                window.resolvePromiseWaitForSaveProject = null;
            }

            if (unityIsWaitingForCallback) {
                window.Unity.call('{"requestId": "-1", "command": "cozmoSaveOnQuitCompleted"}');
            }
        }

        window.promiseWaitForSaveProject = function () {
            return new Promise(function (resolve) {
                window.resolvePromiseWaitForSaveProject = resolve;
                window.saveCozmoUserProject(false);
            });
        };

        closeButton.addEventListener('click', function () {
            Scratch.workspace.playAudio('click');
            vm.stopAll();
            clearInterval(window.saveProjectTimerId);

            var promiseSaveProject = window.promiseWaitForSaveProject();
            promiseSaveProject.then(function(result) {
                window.Unity.call('{"requestId": "-1", "command": "cozmoLoadProjectPage"}');
            });
        });
        closeButton.addEventListener('touchmove', function (e) {
            e.preventDefault();
        });
        challengesButton.addEventListener('click', function () {
          // show challenges dialog
          Scratch.workspace.playAudio('click');
          Challenges.show();
        });
        challengesButton.addEventListener('touchmove', function (e) {
            e.preventDefault();
        });
        greenFlag.addEventListener('click', function () {
            Scratch.workspace.playAudio('click');
            vm.greenFlag();
        });
        greenFlag.addEventListener('touchmove', function (e) {
            e.preventDefault();
        });
        stop.addEventListener('click', function () {
            Scratch.workspace.playAudio('click');
            vm.stopAll();
        });
        stop.addEventListener('touchmove', function (e) {
            e.preventDefault();
        });

        // Extension event handlers
        bindExtensionHandler();

        window.setLocalizedValues();
        window.onresize();
    }

    window.onresize = function(event) {
        // innerWidth values -
        //     iPhone 5: 568
        //     iPhone 6: 667
        //     iPhone 6 Plus: 736
        //     iPad: 1024
        //     iPad Pro: 1366
        //     Nexus 6: 732
        //     Galaxy Note 5: 700
        //     Pixel: 652
        //
        // Also see CSS height column here: https://mydevice.io/devices/

        if (!window.isVertical) {
            if (window.innerWidth < window.TABLET_WIDTH) {
                Scratch.workspace.setScale(0.70);
                //document.getElementById('navigation').style.zoom = "80%"
            }else{
                Scratch.workspace.setScale(1.2);
            }
        }
    };

    window.setLocalizedValues = function() {
        if (!window.isVertical) {
            // Set localized value of Cozmo Says horizontal default "Hi" text
            var cozmoSaysText = document.getElementById("cozmo_says_text");
            cozmoSaysText.innerHTML = $t('codeLabHorizontal.CozmoSaysBlock.DefaultText');
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
     * Bind event handlers.
     */
    window.onload = onLoad;
})();
