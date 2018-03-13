(function () {
    'use strict';
    
    const Scratch = window.Scratch = window.Scratch || {};

    // If project being loaded is JSON, value is true.
    // If project being loaded is XML, value is false.
    window.currentProjectUsesJSONFormat = true;

    function addLeadingZeros(inNum, desiredLength) {
        var paddedString = "" + inNum;
        while (paddedString.length < desiredLength) {
            paddedString = "0" + paddedString;
        }
        return paddedString;
    }

    window.getTimeStamp = function() {
        var now = new Date();
        var milliseconds = now;
        // Adjust to local time (offset is in minutes)
        milliseconds -= (now.getTimezoneOffset() * 60000);

        var seconds = Math.floor(milliseconds / 1000);
        milliseconds -= (seconds * 1000);

        var minutes = Math.floor(seconds / 60);
        seconds -= (minutes * 60)

        var hours = Math.floor(minutes / 60);
        minutes -= (hours * 60);

        var days = Math.floor(hours / 24);
        hours -= (days * 24)

        var timeStamp = addLeadingZeros(hours, 2) + ":" + addLeadingZeros(minutes,2) + ":" + addLeadingZeros(seconds, 2) + "." + addLeadingZeros(milliseconds,3);
        return timeStamp;
    }

    window.cozmoDASLog = function(eventName, messageContents) {
        messageContents = "[" + window.getTimeStamp() + "] " + messageContents;
        console.log(messageContents);
        window.Unity.call({command: "cozmoDASLog", argString: eventName, argString2: messageContents});
    }

    window.cozmoDASWarn = function(eventName, messageContents) {
        messageContents = "[" + window.getTimeStamp() + "] " + messageContents;
        console.log(messageContents);
        window.Unity.call({command: "cozmoDASWarn", argString: eventName, argString2: messageContents});
    }

    window.cozmoDASError = function(eventName, messageContents) {
        messageContents = "[" + window.getTimeStamp() + "] " + messageContents;
        console.log(messageContents);
        window.Unity.call({command: "cozmoDASError", argString: eventName, argString2: messageContents});
    }

    window.startLoadingProject = function() {
        window.isLoadingProject = true;
    }

    window.notifyProjectIsLoaded = function() {
        if (window.isCozmoSampleProject || window.isCozmoFeaturedProject) {
            // Store the initial representation of the project so that
            // we can detect if the user has altered it upon exiting
            // the workspace.
            //
            // Not sure why the setTimeout is necessary but it is
            // currently, otherwise the original JSON is not set
            // correctly. - msintov, 10/12/2017
            setTimeout(function(){
                if (Scratch.vm) {
                    window.originalSampleProjectJSON = Scratch.vm.toJSON();
                } else {
                    // TODO: determine how to properly handle this error
                    window.cozmoDASError('Codelab.notifyProjectIsLoaded.ScratchVMUndefined', 'Scratch vm undefined. Page failed to load properly');
                }
            }, 100);
        }

        if (window.isVertical) {
            if (Scratch.workspace) {
                // Move the workspace to its home position after the project loads
                Scratch.workspace.scrollHome();
            } else {
                // TODO: determine how to properly handle this error
                window.cozmoDASError('Codelab.notifyProjectIsLoaded.ScratchWorkspaceUndefined', 'Scratch workspace undefined. Page failed to load properly');
            }
        }

        window.Unity.call({command: "cozmoWorkspaceLoaded"});
        window.isLoadingProject = false;
    }

    /**
     * Window "onload" handler.
     * @return {void}
     */
    function onLoad () {
        window.TABLET_WIDTH = 813; // Sam C. - increased to 813 to handle iPhone X (812x375)
        //console.log("ANKIPERFTEST common.js onLoad top, about to construct vm");

        checkForObjectAssignSupport();

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
            startScale = 0.85;
            if (window.innerWidth < window.TABLET_WIDTH) {
                startScale = 0.7;
            }
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
                minScale: 0.35,
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
        workspace.addChangeListener(vm.variableListener); // Handle a Blockly event for the variable map.

        const flyoutWorkspace = workspace.getFlyout().getWorkspace();
        flyoutWorkspace.addChangeListener(vm.flyoutBlockListener);
        flyoutWorkspace.addChangeListener(vm.monitorBlockListener);

        // Handle VM events
        vm.on('SCRIPT_GLOW_ON', function(data) {
            // TODO The Gaussian blur filter applied with glowStack sometimes
            // (especially on vertical) makes the entire program disappear
            // when applied on both iOS and Android. Need to find alternative.
            // Drop shadow also has the same problem per Adam's testing.
            if (!window.isVertical) {
                Scratch.workspace.glowStack(data.id, true);
            }
        });
        vm.on('SCRIPT_GLOW_OFF', function(data) {
            if (!window.isVertical) {
                Scratch.workspace.glowStack(data.id, false);
            }
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

        // Receipt of new block XML for the selected target.
        vm.on('workspaceUpdate', function(data) {
            //console.log("ANKIPERFTEST vm.on workspaceUpdate top");

            try {
                // Remove and reattach the workspace listener (but allow flyout events)
                Scratch.workspace.removeChangeListener(Scratch.vm.blockListener);   
                var domXML = Blockly.Xml.textToDom(data.xml);

                if (window.currentProjectUsesJSONFormat) {
                    // Project is serialized as JSON
                    //console.log("ANKIPERFTEST openBlocklyXML. After textToDom, before clearWorkspaceAndLoadFromXml");
                    Blockly.Xml.clearWorkspaceAndLoadFromXml(domXML, Scratch.workspace);
                    //console.log("ANKIPERFTEST openBlocklyXML. After clearWorkspaceAndLoadFromXml, bascially end of openBlocklyXML");
                }
                else {
                    // Project is serialized as XML
                    window.Blockly.Xml.domToWorkspace(domXML, workspace);
                }

                Scratch.workspace.addChangeListener(Scratch.vm.blockListener);
                Scratch.workspace.clearUndo();
            }
            catch (err) {
                window.cozmoDASError("Codelab.VM.WorkspaceUpdate", "Error when loading project: " + err.message);
            }

            if (window.isLoadingProject) {
                window.notifyProjectIsLoaded();
                window.startSaveProjectTimer();
            }

            if (window.isVertical) {
                workspace.toolbox_.flyout_.hide();
            }
            //console.log("ANKIPERFTEST vm.on workspaceUpdate end");
        });

        // Run threads
        vm.start();

        // DOM event handlers
        var closeButton = document.querySelector('#closebutton');
        var greenFlag = document.querySelector('#greenflag');
        var stop = document.querySelector('#stop');

        closeButton.addEventListener('click', function () {
            window.onCloseButton();
        });
        closeButton.addEventListener('touchmove', function (e) {
            e.preventDefault();
        });

        if (!window.isVertical) {
            var challengesButton = document.querySelector('#challengesbutton');
            var tutorialButton = document.querySelector('#tutorialbutton');

            challengesButton.addEventListener('click', function () {
              // show challenges dialog
              Scratch.workspace.playAudio('click');
              Challenges.show();
            });
            challengesButton.addEventListener('touchmove', function (e) {
                e.preventDefault();
            });

            tutorialButton.addEventListener('click', function () {
              // show tutorials dialog
              Scratch.workspace.playAudio('click');
              window.Tutorial.show();
            });
            tutorialButton.addEventListener('touchmove', function (e) {
                e.preventDefault();
            });
        }
        else {
            var undo = document.querySelector('#undo');
            var redo = document.querySelector('#redo');
            var zoomIn = document.querySelector('#zoomIn');
            var zoomOut = document.querySelector('#zoomOut');
            var zoomReset = document.querySelector('#zoomReset');

            undo.addEventListener('click', function () {
                Scratch.workspace.playAudio('click');
                Scratch.workspace.undo();
            });
            undo.addEventListener('touchmove', function (e) {
                e.preventDefault();
            });

            redo.addEventListener('click', function () {
                Scratch.workspace.playAudio('click');
                Scratch.workspace.undo(true);
            });
            redo.addEventListener('touchmove', function (e) {
                e.preventDefault();
            });

            // All zoom button implementation code below taken from zoom_controls.js
            zoomIn.addEventListener('click', function () {
                Scratch.workspace.playAudio('click');
                Scratch.workspace.markFocused();
                Scratch.workspace.zoomCenter(1);
                Blockly.Touch.clearTouchIdentifier();  // Don't block future drags.
            });
            zoomIn.addEventListener('touchmove', function (e) {
                e.stopPropagation();  // Don't start a workspace scroll.
                e.preventDefault();  // Stop double-clicking from selecting text.
            });

            zoomOut.addEventListener('click', function () {
                Scratch.workspace.playAudio('click');
                Scratch.workspace.markFocused();
                Scratch.workspace.zoomCenter(-1);
                Blockly.Touch.clearTouchIdentifier();  // Don't block future drags.
              });
            zoomOut.addEventListener('touchmove', function (e) {
                e.stopPropagation();  // Don't start a workspace scroll.
                e.preventDefault();  // Stop double-clicking from selecting text.
            });

            zoomReset.addEventListener('click', function () {
                Scratch.workspace.playAudio('click');
                Scratch.workspace.markFocused();
                Scratch.workspace.setScale(workspace.options.zoomOptions.startScale);
                Scratch.workspace.scrollCenter();
                Blockly.Touch.clearTouchIdentifier();  // Don't block future drags.
            });
            zoomReset.addEventListener('touchmove', function (e) {
                e.stopPropagation();  // Don't start a workspace scroll.
                e.preventDefault();  // Stop double-clicking from selecting text.
            });

            var glossaryButton = document.querySelector('#glossarybutton');
            glossaryButton.addEventListener('click', function(){
              Scratch.workspace.playAudio('click');
              Glossary.open();
            });
            glossaryButton.addEventListener('touchmove', function (e) {
              e.preventDefault();
            });

            // Move the workspace to its home position
            Scratch.workspace.scrollHome();
        }

        var exportbutton = document.querySelector('#exportbutton');
        exportbutton.addEventListener('click', function () {
            Scratch.workspace.playAudio('click');
            vm.stopAll();

            window.exportCozmoProject();
        });
        exportbutton.addEventListener('touchmove', function (e) {
            e.preventDefault();
        });

        var remixButton = document.querySelector("#remix-button");
        remixButton.addEventListener('click', function(){
            window.onRemixButton();
        });
        remixButton.addEventListener('touchmove', function (e) {
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

        var urlTutorialVar = window.getUrlVars()['showTutorial'];

        if( urlTutorialVar && urlTutorialVar == "true" ) {
            window.Tutorial.show();
        }
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
        // Set localized value of Cozmo Says horizontal and vertical default "Hi" text
        var cozmoSaysText = document.getElementById("cozmo_says_text");
        cozmoSaysText.innerHTML = $t('codeLabHorizontal.CozmoSaysBlock.DefaultText');
    };

    window.onCloseButton = function() {
        if (window.needToRemixProject()) {
            window.player.play('click');
            // a sample or featured project was changed.  Offer to save changes as a remix.
            ModalConfirm.open({
                title: $t('codeLab.saveModifiedSampleProjectAsRemixDialog.dialogTitle'),
                prompt: $t('codeLab.saveModifiedSampleProjectAsRemixDialog.saveAsRemixPrompt'),
                confirmButtonLabel: $t('codeLab.remixConfirmDialog.saveRemixButtonLabel'),
                cancelButtonLabel: $t('codeLab.saveModifiedSampleProjectAsRemixDialog.discardChangesButtonLabel'),
                confirmCallback: function(result) {
                    if (result) {
                        window.player.play('click');
                        // user wants a remix
                        var newRemixName = RenameProject.createRemixProjectTitle(window.cozmoProjectName);
                        window.remixProject(window.cozmoProjectUUID, newRemixName);
                    } else {
                        // user does not want to remix, so close the project as normal
                        window.exitWorkspace();
                    }
                }
            });
        } else {
          // This is a user project or an unmodified sample project.
          // Changes do not need to be saved.
          window.exitWorkspace();
        }
    }

    window.exitWorkspace = function() {
        Scratch.workspace.playAudio('click');
        Scratch.vm.stopAll();
        window.clearSaveProjectTimer();

        var promiseSaveProject = window.promiseWaitForSaveProject();
        promiseSaveProject.then(function(result) {
            window.Unity.call({requestId: -1, command: "cozmoLoadProjectPage"});
        });
    }

    window.onRemixButton = function() {
        Scratch.workspace.playAudio('click');
        ModalConfirm.open({
            title: $t('codeLab.remixConfirmDialog.dialogTitle'),
            prompt: $t('codeLab.remixConfirmDialog.saveProjectAsRemixPrompt'),
            confirmButtonLabel: $t('codeLab.remixConfirmDialog.saveRemixButtonLabel'),
            cancelButtonLabel: $t('codeLab.remixConfirmDialog.cancelButtonLabel'),
            confirmCallback: function(result) {
                window.player.play('click');
                if (result) {
                    // user wants a remix
                    var newRemixName = RenameProject.createRemixProjectTitle(window.cozmoProjectName);
                    window.remixProject(window.cozmoProjectUUID, newRemixName);
                }
            }
        });
    }

    // Turn on animated play button on workspace.
    window.onScriptsStarted = function() {
        document.body.classList.add('is-program-running');
    }

    // Turn off animated play button on workspace.
    window.onScriptsStopped = function() {
        // Turn off animated play button on workspace
        document.body.classList.remove('is-program-running');
    }

    // Hide all blockly dropdowns.
    window.closeBlocklyDropdowns = function() {
        if ('undefined' !== typeof Blockly) {
            if ('undefined' !== typeof Blockly.DropDownDiv && Blockly.DropDownDiv) {
                Blockly.DropDownDiv.hideWithoutAnimation();
            }
            if ('undefined' !== typeof Blockly.WidgetDiv && Blockly.WidgetDiv) {
                Blockly.WidgetDiv.hide(true);
            }
        }
    }

    /**
     * Binds the extension interface to `window.extensions`.
     * @return {void}
     */
    function bindExtensionHandler () {
        // TODO What does this method do? scratch-blocks doesn't have this code.
        if (typeof webkit === 'undefined') return;
        if (typeof webkit.messageHandlers === 'undefined') return;
        if (typeof webkit.messageHandlers.extensions === 'undefined') return;
        window.extensions = webkit.messageHandlers.extensions;
    }

    // Check for ECMAScript Object.assign() support and if not supported,
    // add polyfill implementation.
    //
    // Issue discussed here: https://forums.anki.com/t/code-lab-stopped-working-after-upated-to-v2/9456/5
    function checkForObjectAssignSupport () {
        if (typeof Object.assign != 'function') {
            window.cozmoDASWarn("Codelab.Android.OlderWebview.ObjectAssign", "ECMAScript 6 method Object.assign() does not exist. Using polyfill to support older Android webview");

            // Polyfill from https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/assign

            // Must be writable: true, enumerable: false, configurable: true
            Object.defineProperty(Object, "assign", {
                value: function assign(target, varArgs) { // .length of function is 2
                    'use strict';
                    if (target == null) { // TypeError if undefined or null
                        throw new TypeError('Cannot convert undefined or null to object');
                    }

                    var to = Object(target);

                    for (var index = 1; index < arguments.length; index++) {
                        var nextSource = arguments[index];

                        if (nextSource != null) { // Skip over if undefined or null
                            for (var nextKey in nextSource) {
                                // Avoid bugs when hasOwnProperty is shadowed
                                if (Object.prototype.hasOwnProperty.call(nextSource, nextKey)) {
                                    to[nextKey] = nextSource[nextKey];
                                }
                            }
                        }
                    }
                    return to;
                },
                writable: true,
                configurable: true
            });
        }
    }


    /**
     * Immedately show Play Now modal for featured projects in vertical workspace
     */
    function prepareForFeaturedProjectPlayNowModal() {
        if (window.isVertical && window.getUrlVar('isFeaturedProject') === 'true') {
            PlayNowModal.init();
        }
    }

    document.addEventListener('DOMContentLoaded', function(e){
        prepareForFeaturedProjectPlayNowModal();
    });

    /**
     * Bind event handlers.
     */
    window.onload = onLoad;
})();
