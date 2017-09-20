(function () {

    // cozmoSaveLoadAPI.js is a list of calls for the save/load UI page and should not contain any references to
    // code in Scratch/lib (i.e., Blockly, scratch-blocks or scratch-vm code).

    // Retrieve one list of projects: featured projects that are all vertical, sorted by the order
    // it should be displayed.
    //
    // Callback method has one parameters, a JSON array of the featured projects.
    // The callback has only been tested with a method on window.
    // Example: window.myCallback(featuredProjectList)
    window.getCozmoFeaturedProjectList = function(callback) {
        window.Unity.call({requestId: -1, command: "getCozmoFeaturedProjectList", argString: callback});
    }

    // For horizontal or vertical: retrieve two lists of projects: user projects and sample projects. Each is sorted by the order
    // it should be displayed. For user projects, the sorting order is by date last modified, most recent first.
    //
    // Callback method has two parameters, both JSON arrays: userProjectList and sampleProjectList.
    // The callback has only been tested with a method on window.
    // Example: window.myCallback(userProjectList, sampleProjectList)
    window.getCozmoUserAndSampleProjectLists = function(callback, isVertical) {
        window.Unity.call({requestId: -1, command: "getCozmoUserAndSampleProjectLists", argString: callback, argBool: isVertical});
    }

    // Delete the project specified by the projectUUID.
    window.deleteCozmoUserProject = function(projectUUID) {
        window.Unity.call({requestId: -1, command: "cozmoDeleteUserProject", argString: projectUUID});
    }

    // For horizontal or vertical user project, calls Unity with projectUUID, requesting Unity to send project data to window.openCozmoUserProject()
    window.requestToOpenCozmoUserProject = function(projectUUID, isVertical) {
        window.Unity.call({requestId: -1, command: "cozmoRequestToOpenUserProject", argString: projectUUID, argBool: isVertical});
    }

    // For horizontal or vertical sample project, calls Unity with projectUUID, requesting Unity to send project data to window.openCozmoSampleProject()
    window.requestToOpenCozmoSampleProject = function(projectUUID, isVertical) {
        window.Unity.call({requestId: -1, command: "cozmoRequestToOpenSampleProject", argString: projectUUID, argBool: isVertical});
    }

    // Request that horizontal or vertical workspace be presented with only a green flag on the workspace.
    window.requestToCreateCozmoProject = function(isVertical) {
        window.Unity.call({requestId: -1, command: "cozmoRequestToCreateProject", argBool: isVertical});
    }

    // Close minigame.
    window.closeCodeLab = function() {
        window.Unity.call({requestId: -1, command: "cozmoCloseCodeLab"});
    }
})();
