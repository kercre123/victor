(function () {

    // cozmoSaveLoadAPI.js is a list of calls for the save/load UI page and should not contain any references to
    // code in Scratch/lib (i.e., Blockly, scratch-blocks or scratch-vm code).

    // Retrieve two lists of projects: user projects and sample projects. Each is sorted by the order
    // it should be displayed. For user projects, the sorting order is by date last modified, most recent first.
    //
    // Callback method has two parameters, both JSON arrays: userProjectList and sampleProjectList.
    // The callback has only been tested with a method on window.
    // Example: window.myCallback(userProjectList, sampleProjectList)
    window.getCozmoUserAndSampleProjectLists = function(callback) {
        window.Unity.call("{'requestId': '" + -1 + "', 'command': 'getCozmoUserAndSampleProjectLists','argString': '" + callback + "'}");
    }

    // Delete the project specified by the projectUUID.
    window.deleteCozmoUserProject = function(projectUUID) {
        window.Unity.call("{'requestId': '" + -1 + "', 'command': 'cozmoDeleteUserProject','argString': '" + projectUUID + "'}");
    }

    // Calls Unity with projectUUID, requesting Unity to send project data to window.openCozmoUserProject()
    window.requestToOpenCozmoUserProject = function(projectUUID) {
        window.Unity.call("{'requestId': '" + -1 + "', 'command': 'cozmoRequestToOpenUserProject','argString': '" + projectUUID + "'}");
    }

    // Calls Unity with projectUUID, requesting Unity to send project data to window.openCozmoSampleProject()
    window.requestToOpenCozmoSampleProject = function(projectUUID) {
        window.Unity.call("{'requestId': '" + -1 + "', 'command': 'cozmoRequestToOpenSampleProject','argString': '" + projectUUID + "'}");
    }

    // Request that workspace be presented with only a green flag on the workspace.
    window.requestToCreateCozmoProject = function() {
        window.Unity.call("{'requestId': '" + -1 + "', 'command': 'cozmoRequestToCreateProject'}");
    }

    // Close minigame.
    window.closeCodeLab = function() {
        window.Unity.call('{"requestId": "' + -1 + '", "command": "cozmoCloseCodeLab"}');
    }
})();
