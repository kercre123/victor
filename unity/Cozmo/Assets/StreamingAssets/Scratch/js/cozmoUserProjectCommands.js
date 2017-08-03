(function () {

    window.isCozmoSampleProject = false;
    window.cozmoProjectName = null;
    window.cozmoProjectUUID = null;
    window.previouslySavedProjectXML = null;
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
      var point = window.getScriptStartingPoint();
      var greenFlagXML = '<block type="event_whenflagclicked" id="RqohItYC/XpjZ2]xiar5" x="' + point.x + '" y="' + point.y +'"></block>';

      var xmlStart = '<xml xmlns="http://www.w3.org/1999/xhtml">';
      var xmlEnd = '</xml>';

      if (window.getNodes().length <= 0) {
        // No other blocks are on the workspace so put green flag back on workspace by itself.
        var xmlTextWithGreenFlag = xmlStart + greenFlagXML + xmlEnd;
        window.openCozmoProject(window.cozmoProjectUUID, window.cozmoProjectName, xmlTextWithGreenFlag, window.isCozmoSampleProject);
      }
      else {
        if (!window.isGreenFlagOnWorkspace()) {
          // There is a program on the workspace but no green flag. Get existing program and append green flag xml to beginning.
          var xml = Blockly.Xml.workspaceToDom(Scratch.workspace);
          var xmlText = Blockly.Xml.domToText(xml);
          var xmlTextWithGreenFlag = xmlStart + greenFlagXML + xmlText.substring(xmlStart.length, xmlText.length);

          window.openCozmoProject(window.cozmoProjectUUID, window.cozmoProjectName, xmlTextWithGreenFlag, window.isCozmoSampleProject);
        }
      }
    }

    // Check that there is a script on the workspace and it contains
    // more than just the green flag.
    window.hasUserAddedBlocks = function() {
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
            window.Unity.call('{"requestId": "-1", "command": "cozmoSaveOnQuitCompleted"}');
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

        var xml = Blockly.Xml.workspaceToDom(Scratch.workspace);
        var xmlText = Blockly.Xml.domToText(xml);

        if (window.cozmoProjectUUID == null) {
            window.cozmoProjectUUID = '';
        }
        
        if (window.cozmoProjectUUID != '' && window.previouslySavedProjectXML == xmlText) {
            // No changes to save
            window.saveProjectCompleted(unityIsWaitingForCallback);
            return;
        }

        // If it's a new project, only save the project if the user has added blocks.
        if (window.cozmoProjectUUID == '' &&  !window.hasUserAddedBlocks()) {
            window.saveProjectCompleted(unityIsWaitingForCallback);
            return;
        }

        window.previouslySavedProjectXML = xmlText;

        // If we reached this far, Unity will take care of resolving the promise.
        window.Unity.call({requestId: -1, command: "cozmoSaveUserProject", argString: xmlText, argUUID: window.cozmoProjectUUID});
    }

    window.openCozmoProject = function(projectUUID, projectName, projectXML, isCozmoSampleProjectStr) {
        var isCozmoSampleProject = (isCozmoSampleProjectStr == 'true');

        // Remove all existing scripts from workspace
        Scratch.workspace.clear();

        window.cozmoProjectUUID = projectUUID;
        window.cozmoProjectName = projectName;
        window.isCozmoSampleProject = isCozmoSampleProject;
        window.previouslySavedProjectXML = null;

        if (window.saveProjectTimerId) {
            clearInterval(window.saveProjectTimerId);
        }

        if (isCozmoSampleProject) {
            // Set the coordinate setting in the sample project xml to our desired location on-screen.
            var startingPoint = window.getScriptStartingPoint();
            projectXML = projectXML.replace("REPLACE_X_COORD", startingPoint.x);
            projectXML = projectXML.replace("REPLACE_Y_COORD", startingPoint.y);
        }

        openBlocklyXML(projectXML);
        setProjectNameAndSavedText(projectName, isCozmoSampleProject);

        window.startSaveProjectTimer();
    }

    window.setProjectNameAndSavedText = function(projectName, isSampleProject) {
        setText('#app-title', $t(projectName));

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
})();
