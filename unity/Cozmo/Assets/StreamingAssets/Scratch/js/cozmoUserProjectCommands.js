(function () {

    window.isCozmoSampleProject = false;
    window.cozmoProjectName = null;
    window.cozmoProjectUUID = null;
    window.previouslySavedProjectJSON = null;
    window.saveProjectTimerId = null;
    window.resolvePromiseWaitForSaveProject = null;

    // Prevent saveCozmoUserProject from being called more than once for a project, which can
    // cause more than one project to be created if Unity UUID has not yet been created.
    window.isSavingProject = false;

    function $t(str) {
        return str;
    }
    function setText(query, text) {
        document.querySelector(query).textContent = text;
    }

    function CLPoint (x, y) {
        this.x = x;
        this.y = y;
    }

    // Starting point for one script to appear on phones and tablets.
    window.getScriptStartingPoint = function() {
        var point = null;
        if (window.isVertical) {
            point = new CLPoint(480, 200);
        }
        else {
            point = new CLPoint(120, 200);
        }

        if (window.innerWidth <= 799) {
            point.x = 100;
            point.y = 110;
        }
        return point;
    }

    // Put green flag in its location in the upper left corner of the workspace if no green flag is on workspace.
    window.ensureGreenFlagIsOnWorkspace = function () {
      // TODO This is currently in XML, but we could migrate to use JSON instead throughout this method.
      // These two lines should help:
      //var projectJSON = '{"targets":[{"id":"9I=:fGU6_w`eoI3X`=J!","name":"Stage","isStage":true,"x":0,"y":0,"size":100,"direction":90,"draggable":false,"currentCostume":0,"costumeCount":0,"visible":true,"rotationStyle":"all around","blocks":{},"variables":{},"lists":{},"costumes":[],"sounds":[]},{"id":"D:hj2q3qXn53^HJG4b,d","name":"Sprite1","isStage":false,"x":0,"y":0,"size":100,"direction":90,"draggable":false,"currentCostume":0,"costumeCount":0,"visible":true,"rotationStyle":"all around","blocks":{"g68)-/+Er8xO7[moRW8J":{"id":"g68)-/+Er8xO7[moRW8J","opcode":"event_whenflagclicked","inputs":{},"fields":{},"next":null,"topLevel":true,"parent":null,"shadow":false,"x":662.4705882352941,"y":426.7058823529412}},"variables":{},"lists":{},"costumes":[],"sounds":[]}],"meta":{"semver":"3.0.0","vm":"0.1.0","agent":"Mozilla/5.0 (iPad; CPU OS 9_1 like Mac OS X) AppleWebKit/601.1.46 (KHTML, like Gecko) Version/9.0 Mobile/13B143 Safari/601.1"}}';
      //window.Scratch.vm.fromJSON(projectJSON);

      var point = window.getScriptStartingPoint();
      var greenFlagXML = '<block type="event_whenflagclicked" id="RqohItYC/XpjZ2]xiar5" x="' + point.x + '" y="' + point.y +'"></block>';

      var xmlStart = '<xml xmlns="http://www.w3.org/1999/xhtml">';
      var xmlEnd = '</xml>';

      if (window.getNodes().length <= 0) {
        // No other blocks are on the workspace so put green flag back on workspace by itself.
        var xmlTextWithGreenFlag = xmlStart + greenFlagXML + xmlEnd;
        window.openCozmoProjectXML(window.cozmoProjectUUID, window.cozmoProjectName, xmlTextWithGreenFlag, window.isCozmoSampleProject);
      }
      else {
        if (!window.isGreenFlagOnWorkspace()) {
          // There is a program on the workspace but no green flag. Get existing program and append green flag xml to beginning.
          var xml = Blockly.Xml.workspaceToDom(Scratch.workspace);
          var xmlText = Blockly.Xml.domToText(xml);
          var xmlTextWithGreenFlag = xmlStart + greenFlagXML + xmlText.substring(xmlStart.length, xmlText.length);

          window.openCozmoProjectXML(window.cozmoProjectUUID, window.cozmoProjectName, xmlTextWithGreenFlag, window.isCozmoSampleProject);
        }
      }
    }

    // Check that there is a script on the workspace and it contains
    // more than just the green flag.
    window.hasUserAddedBlocks = function() {
        // TODO Update to use JSON?
        var nodes = window.getNodes();
        var greenFlagType = 'event_whenflagclicked';
        var hasUserAddedBlocks = false;
        if (nodes.length > 0) {
            for(var i = 0; i < nodes.length; i++) { //loop thru the nodes
                if (nodes[i].getAttribute("type") != greenFlagType) {
                    hasUserAddedBlocks = true;
                    break;
                }
            }
        }

        return hasUserAddedBlocks;
    }

    window.isGreenFlagOnWorkspace = function() {
        // TODO Update to use JSON?
        var nodes = window.getNodes();
        var greenFlagType = 'event_whenflagclicked';
        for(var i = 0; i < nodes.length; i++) { //loop thru the nodes
            if (nodes[i].getAttribute("type") == greenFlagType) {
                return true;
            }
        }

        return false;
    }

    // A node is a block representation on the workspace.
    window.getNodes = function() {
        // TODO Update to use JSON?
        var xml = Blockly.Xml.workspaceToDom(Scratch.workspace);
        var nodes = xml.getElementsByTagName('block');
        return nodes;
    }

    window.saveProjectCompleted = function (unityIsWaitingForCallback) {
        window.isSavingProject = false;

        // If we have a Promise to resolve, resolve it
        if (window.resolvePromiseWaitForSaveProject) {
            window.resolvePromiseWaitForSaveProject();
            window.resolvePromiseWaitForSaveProject = null;
        }

        if (unityIsWaitingForCallback) {
            window.Unity.call({requestId: -1, command: "cozmoSaveOnQuitCompleted"});
        }
    }

    window.promiseWaitForSaveProject = function () {
        return new Promise(function (resolve) {
            window.resolvePromiseWaitForSaveProject = resolve;
            window.saveCozmoUserProject(false);
        });
    };

    /* Save all scripts currently on the workspace into a Cozmo Code Lab user project in the Unity user profile.
     * Call this method to save both new and existing projects.
     */
    window.saveCozmoUserProject = function(unityIsWaitingForCallback) {
        if (window.isSavingProject) {
            return;
        }

        window.isSavingProject = true;

        if (window.isCozmoSampleProject) {
            window.saveProjectCompleted(unityIsWaitingForCallback);
            return;
        }

        var json = Scratch.vm.toJSON();

        if (window.cozmoProjectUUID == null) {
            window.cozmoProjectUUID = '';
        }

        if (window.cozmoProjectUUID != '' && window.previouslySavedProjectJSON == json) {
            // No changes to save
            window.saveProjectCompleted(unityIsWaitingForCallback);
            return;
        }

        // If it's a new project, only save the project if the user has added blocks.
        if (window.cozmoProjectUUID == '' &&  !window.hasUserAddedBlocks()) {
            window.saveProjectCompleted(unityIsWaitingForCallback);
            return;
        }

        window.previouslySavedProjectJSON = json;

        // If we reached this far, Unity will take care of resolving the promise.
        window.Unity.call({requestId: -1, command: "cozmoSaveUserProject", argString: json, argUUID: window.cozmoProjectUUID});
    }

    window.exportCozmoProject = function() {
        var promiseSaveProject = window.promiseWaitForSaveProject();
        promiseSaveProject.then(function(result) {
            window.Unity.call({requestId: -1, command: "cozmoExportProject", argUUID: window.cozmoProjectUUID});
        });
    }

    window.openCozmoProjectJSON = function(projectUUID, projectName, projectJSON, isCozmoSampleProjectStr) {
        window.openCozmoProject(projectUUID, projectName, projectJSON, null, isCozmoSampleProjectStr);
    }

    // DEPRECATED - only used to open user projects that were created before 2.1 and are still in XML
    window.openCozmoProjectXML = function(projectUUID, projectName, projectXML, isCozmoSampleProjectStr) {
        window.openCozmoProject(projectUUID, projectName, null, projectXML, isCozmoSampleProjectStr);
    }

    // Don't call this method directly. Please call openCozmoProjectJSON instead.
    // This method takes a projectXML parameter to support pre-2.1 builds.
    window.openCozmoProject = function(projectUUID, projectName, projectJSON, projectXML, isCozmoSampleProjectStr) {
        var startTime = performance.now()

        var isCozmoSampleProject = (isCozmoSampleProjectStr == 'true');

        // TODO: Special case to fix localized text for intruder sample project. Rip out and revisit post-2.0.0.
        // TODO After sample projects are converted to JSON, must revisit this.
        if (isCozmoSampleProject && projectUUID == "4bb7eb61-99c4-44a2-8295-f0f94ddeaf62" && projectXML != null) {
            projectXML = window.replaceSampleProjectTextForIntruder(projectXML);
        }

        // Remove all existing scripts from workspace.
        Scratch.workspace.clear();

        window.cozmoProjectUUID = projectUUID;
        window.cozmoProjectName = projectName;
        window.isCozmoSampleProject = isCozmoSampleProject;
        window.previouslySavedProjectJSON = null;

        if (window.saveProjectTimerId) {
            clearInterval(window.saveProjectTimerId);
        }


        if (isCozmoSampleProject && projectXML != null) {
            // Set the coordinate setting in the sample project xml to our desired location on-screen.
            var startingPoint = window.getScriptStartingPoint();
            projectXML = projectXML.replace("REPLACE_X_COORD", startingPoint.x); // TODO need JSON solution
            projectXML = projectXML.replace("REPLACE_Y_COORD", startingPoint.y); // TODO need JSON solution
        }

        var startBlocklyTime = performance.now()
        var methodCalledForDAS;
        if (projectJSON != null) {
            window.Scratch.vm.fromJSON(projectJSON);
            methodCalledForDAS = "Scratch.vm.fromJSON";
        }
        else {
            // User project was build pre-Cozmo app 2.1 release. Open projectXML.
            openBlocklyXML(projectXML);
            methodCalledForDAS = "openBlocklyXML";
        }
        var blocklyXmlTime = (performance.now() - startBlocklyTime) * 0.001;
        setProjectNameAndSavedText(projectName, isCozmoSampleProject);

        window.startSaveProjectTimer();
        
        var loadTime = (performance.now() - startTime) * 0.001;
        window.cozmoDASLog("openCozmoProject", "Took: " + loadTime.toFixed(3) + "s - " + blocklyXmlTime.toFixed(3) + "s in " + methodCalledForDAS);
    }

    window.setProjectNameAndSavedText = function(projectName, isSampleProject) {
        setText('#app-title', $t(projectName));

        // if the title overflows the container, reduce the font size to make it fit
        var title = document.querySelector('#app-title');
        if (title.scrollWidth > title.clientWidth) {
          title.classList.add('long-title');
        }

        var autosavedText = window.$t('codeLab.SaveProject.Autosaved');
        if (isSampleProject) {
            autosavedText = window.$t('codeLab.SaveProject.NotSaved');
        }
        else if (projectName == '' || projectName == null) {
            // When user selects 'Create New Project' from save/load UI, at first project has no name and is not yet autosaved.
            autosavedText = '';
        }
        setText('#app-title-subtext', $t(autosavedText));
    }

    window.newProjectCreated = function(projectUUID, projectName) {
        window.cozmoProjectUUID = projectUUID;
        window.cozmoProjectName = projectName;

        window.setProjectNameAndSavedText(projectName, false);
    }

    window.startSaveProjectTimer = function() {
        // Start timer for intervals at which we'll check if the user project should be saved.
        if (!window.isCozmoSampleProject) {
            var timeInterval_ms = 3000;
            window.saveProjectTimerId = setInterval(saveProjectTimer, timeInterval_ms);
            function saveProjectTimer() {
                window.saveCozmoUserProject(false);
            }
        }
    }

    window.openBlocklyXML = function(xml) {
        var domXML = Blockly.Xml.textToDom(xml);
        Blockly.Xml.domToWorkspace(domXML, Scratch.workspace);
    }

    // TODO Replace this method used for the very particular case of replacing intruder localized text in sample project.
    window.replaceSampleProjectTextForIntruder = function(projectXML) {
        var intruderTranslation = window.$t('codeLabChallenge_intruder.CozmoSaysBlock.DefaultText');
        return projectXML.replace("Intruder!", intruderTranslation);
    }
})();
