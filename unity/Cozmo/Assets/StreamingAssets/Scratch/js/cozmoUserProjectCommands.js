(function () {

    window.isCozmoSampleProject = false;
    window.cozmoProjectName = null;
    window.cozmoProjectUUID = null;

    // Put only green flag on the workspace to help the user start a script.
    // This is for newly-opened workspaces with no existing user project.
    window.putStarterGreenFlagOnWorkspace = function() {
        var greenFlagXML = '<xml xmlns="http://www.w3.org/1999/xhtml"><block type="event_whenflagclicked" id="RqohItYC/XpjZ2]xiar5" x="92" y="279"></block></xml>';
        window.openCozmoProject(null, null, greenFlagXML, false);
    }

    // Retrieve list of user projects. Sorted by date last modified, most recent first.
    // Callback method parameter is set to JSON of all Code Lab projects.
    // The callback has only been tested with a method on window, like: window.myCallback(data)
    window.getCozmoUserProjectList = function(callback) {
        window.Unity.call("{'requestId': '" + -1 + "', 'command': 'cozmoGetUserProjectList','argString': '" + callback + "'}");
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
     *
     * @param projectUUID If the project already has a projectUUID created by Unity, pass it to this method. If not, one will be created if appropriate.
     */
    window.saveCozmoUserProject = function(projectUUID) {
        var xml = Blockly.Xml.workspaceToDom(workspace);
        var xmlText = Blockly.Xml.domToText(xml);

        if (projectUUID == null) {
            projectUUID = '';
        }

        // If it's a new project, only save the project if the user has added blocks.
        // TODO For new projects, after they are first saved by Unity, need to get both the
        // project UUID and projectName from Unity and cache it, and display in upper right
        // corner of workspace.
        if (projectUUID == '' &&  !window.hasUserAddedBlocks()) {
            return;
        }

        window.Unity.call("{'requestId': '" + -1 + "', 'command': 'cozmoSaveUserProject','argString': '" + xmlText + "','argUUID': '" + projectUUID + "'}");
    }

    // Delete the project specified by the projectUUID.
    window.deleteCozmoUserProject = function(projectUUID) {
        window.Unity.call("{'requestId': '" + -1 + "', 'command': 'cozmoDeleteUserProject','argString': '" + projectUUID + "'}");
    }

    // Calls Unity with projectUUID, requesting Unity to send project data to window.openCozmoUserProject()
    window.requestToOpenCozmoUserProject = function(projectUUID) {
        window.Unity.call("{'requestId': '" + -1 + "', 'command': 'cozmoRequestToOpenUserProject','argString': '" + projectUUID + "'}");
    }

    window.openCozmoProject = function(projectUUID, projectName, projectXML, isCozmoSampleProject) {
        // Remove all existing scripts from workspace
        window.workspace.clear();

        // TODO show projectName in upper right corner of workspace
        // TODO whether the project is autosaved or not (yes for personal projects, no for sample projects) below projectName in workspace
        window.cozmoProjectUUID = projectUUID;
        window.cozmoProjectName = projectName;
        window.isCozmoSampleProject = isCozmoSampleProject;

        openBlocklyXML(projectXML);
    }

    window.openBlocklyXML = function(xml) {
        var domXML = Blockly.Xml.textToDom(xml);
        Blockly.Xml.domToWorkspace(domXML, workspace);
    }
})();
