(function () {

    window.isCozmoSampleProject = false;
    window.cozmoProjectName = null;
    window.cozmoProjectUUID = null;
    window.previouslySavedProjectXML = null;
    window.saveProjectTimerId = null;

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
        var point = new CLPoint(120, 200);
        if (window.innerWidth <= 799) {
            point.x = 100;
            point.y = 110;
        }
        return point;
    }

    // Put only green flag on the workspace to help the user start a script.
    // This is for newly-opened workspaces with no existing user project.
    window.putStarterGreenFlagOnWorkspace = function() {
        var point = window.getScriptStartingPoint();
        var greenFlagXML = '<xml xmlns="http://www.w3.org/1999/xhtml"><block type="event_whenflagclicked" id="RqohItYC/XpjZ2]xiar5" x="' + point.x + '" y="' + point.y +'"></block></xml>';
        window.openCozmoProject(null, null, greenFlagXML, false);
    }

    // Check that there is a script on the workspace and it contains
    // more than just the green flag.
    window.hasUserAddedBlocks = function() {
        var xml = Blockly.Xml.workspaceToDom(workspace);
        var nodes = xml.getElementsByTagName('block');
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

    /* Save all scripts currently on the workspace into a Cozmo Code Lab user project in the Unity user profile.
     * Call this method to save both new and existing projects.
     */
    window.saveCozmoUserProject = function() {
        if (window.isSampleProject) {
            window.saveProjectCompleted();
            return;
        }

        var xml = Blockly.Xml.workspaceToDom(workspace);
        var xmlText = Blockly.Xml.domToText(xml);

        if (window.cozmoProjectUUID == null) {
            window.cozmoProjectUUID = '';
        }
        
        if (window.cozmoProjectUUID != '' && window.previouslySavedProjectXML == xmlText) {
            // No changes to save
            window.saveProjectCompleted();
            return;
        }

        // If it's a new project, only save the project if the user has added blocks.
        // TODO For new projects, after they are first saved by Unity, need to get both the
        // project UUID and projectName from Unity and cache it, and display in upper right
        // corner of workspace.
        if (window.cozmoProjectUUID == '' &&  !window.hasUserAddedBlocks()) {
            window.saveProjectCompleted();
            return;
        }

        window.previouslySavedProjectXML = xmlText;

        // If we reached this far, Unity will take care of resolving the promise.
        window.Unity.call("{'requestId': '" + -1 + "', 'command': 'cozmoSaveUserProject','argString': '" + xmlText + "','argUUID': '" + window.cozmoProjectUUID + "'}");
    }

    window.openCozmoProject = function(projectUUID, projectName, projectXML, isCozmoSampleProjectStr) {
        var isCozmoSampleProject = (isCozmoSampleProjectStr == 'true');

        // Remove all existing scripts from workspace
        window.workspace.clear();

        // TODO show projectName in upper right corner of workspace
        // TODO whether the project is autosaved or not (yes for personal projects, no for sample projects) below projectName in workspace
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

        var autosavedText = 'Autosaved';
        if (isSampleProject) {
            autosavedText = 'Sample projects not saved';
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
                window.saveCozmoUserProject();
            }
        }
    }

    window.openBlocklyXML = function(xml) {
        var domXML = Blockly.Xml.textToDom(xml);
        Blockly.Xml.domToWorkspace(domXML, workspace);
    }
})();
