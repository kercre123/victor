(function () {
    'use strict';
    
    function unescapedText(str) {
        if (str === null || str === undefined) {
            return str;
        }
        var resultStr = str.replaceAll('\\\'', '\'');
        resultStr = resultStr.replaceAll('\\\"', '\"');
        return resultStr;
    }

    window.isCozmoFeaturedProject = (window.getUrlVar('isFeaturedProject') == 'true') || false;
    window.isCozmoSampleProject = false;

    window.cozmoProjectName = null;
    window.cozmoProjectUUID = window.getUrlVar('projectID') || null;
    window.cozmoProjectVersionNum = null;
    window.previouslySavedProjectJSON = null;
    window.originalSampleProjectJSON = null;
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

    window.ensureGreenFlagIsOnWorkspaceRequest = function () {
        // TODO HACK Adding delay as calling this too early during workspace loading is causing the
        // green flag not to be recognized by the vm, and the project not to be saved.
        setTimeout(function(){
          window.ensureGreenFlagIsOnWorkspace();
        }, 750);    
    }

    // Put green flag in its location in the upper left corner of the workspace if no green flag is on workspace.
    // This method serves to kick off the save timer when openCozmoProjectXML and thus openCozmoProject is called.
    window.ensureGreenFlagIsOnWorkspace = function () {
      if (window.isLoadingProject) return;

      var point = window.getScriptStartingPoint();
      var greenFlagXML = '<block type="event_whenflagclicked" id="RqohItYC/XpjZ2]xiar5" x="' + point.x + '" y="' + point.y +'"></block>';

      var xmlStart = '<xml xmlns="http://www.w3.org/1999/xhtml">';
      var xmlEnd = '</xml>';

      var blocks = Scratch.workspace.getTopBlocks(false);
      if (blocks.length <= 0) {
        // No other blocks are on the workspace so put green flag back on workspace by itself.
        var xmlTextWithGreenFlag = xmlStart + greenFlagXML + xmlEnd;
        window.openCozmoProjectXML(window.cozmoProjectUUID, window.cozmoProjectName, window.cozmoProjectVersionNum, xmlTextWithGreenFlag, window.isCozmoSampleProject);
        if (window.isVertical) {
            Blockly.getMainWorkspace().scrollHome(true);
        }
      }

      // Turning off this logic for now as we iron out the green flag causing problems at project launch time.
      /*
      else {
        if (!window.isGreenFlagOnWorkspace()) {
          // There is a program on the workspace but no green flag. Get existing program and append green flag xml to beginning.
          var xml = Blockly.Xml.workspaceToDom(Scratch.workspace);
          var xmlText = Blockly.Xml.domToText(xml);
          var xmlTextWithGreenFlag = xmlStart + greenFlagXML + xmlText.substring(xmlStart.length, xmlText.length);

          window.openCozmoProjectXML(window.cozmoProjectUUID, window.cozmoProjectName, window.cozmoProjectVersionNum, xmlTextWithGreenFlag, window.isCozmoSampleProject);
        }
      }
      */
    }

    window.isGreenFlagBlock = function(block) {
        var localizedGreenFlagString = 'When green flag clicked';
        if (window.isVertical) {
            var locale = LOCALE;
            switch(locale){
                case 'en-US':
                    localizedGreenFlagString = 'when flag clicked';
                    break;
                case 'de-DE':
                    localizedGreenFlagString = 'Wenn flag angeklickt';
                    break;
                case 'fr-FR':
                    localizedGreenFlagString = 'quand flag est cliqué';
                    break;
                case 'ja-JP':
                    localizedGreenFlagString = 'flag がクリックされたとき';
                    break;
                default:
                    window.cozmoDASError("CodeLab.IsGreenFlagBlock.LocaleError", "Unknown locale in window.isGreenFlagBlock");
                    break;
            }
        }

        if (block == localizedGreenFlagString) {
            return true;
        }

        return false;
    }

    // Check that there is a script on the workspace and it contains
    // more than just the green flag.
    window.hasUserAddedBlocks = function() {
        var blocks = Scratch.workspace.getAllBlocks();
        var hasUserAddedBlocks = false;
        for (var i = 0, block; block = blocks[i]; i++) {
            if (!window.isGreenFlagBlock(block)) {
                hasUserAddedBlocks = true;
                break;
            }
        }

        return hasUserAddedBlocks;
    }

    window.isGreenFlagOnWorkspace = function() {
        var blocks = Scratch.workspace.getTopBlocks(false);
        for (var i = 0, block; block = blocks[i]; i++) {
            if (window.isGreenFlagBlock(block)) {
                return true;
            }
        }

        return false;
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

        if (window.isCozmoSampleProject || window.isCozmoFeaturedProject) {
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

    window.onCozmoProjectExportConfirmation = function() {
        var promiseSaveProject = window.promiseWaitForSaveProject();

        var projectType = "user";
        if(window.isCozmoSampleProject) {
            projectType = "sample";
        }
        if(window.isCozmoFeaturedProject) {
            projectType = "featured";
        }
        // @todo: inject a new check for featured projects

        promiseSaveProject.then(function(result) {
            // Pass the projectJSON along to cozmoExportProject in case this project has not yet been saved as JSON (such as with horizontal sample projects).
            var projectJSON = Scratch.vm.toJSON();
            window.Unity.call({requestId: -1, command: "cozmoExportProject", argUUID: window.cozmoProjectUUID, argString: projectType, argString2: projectJSON});
        });
    }

    window.exportCozmoProject = function() {
        var title = window.$t('codeLab.export_modal.title');
        var body = window.$t('codeLab.export_modal.body');
        var cancelText = Blockly.Msg.IOS_CANCEL;
        var confirmText = window.$t('codeLab.export_modal.button.copy_to_drive');

        // On Kindle, use alternate text since Google Drive app is unavailable for Kindle.
        var isKindle = window.getUrlVar('isKindle') == 'true';
        if (isKindle) {
            body = window.$t('codeLab.export_modal_kindle.body');
            confirmText = window.$t('codeLab.export_modal_kindle.button.copy_to_drive');
        }

        ModalConfirm.open({
            title: title,
            prompt: body,
            confirmButtonLabel: confirmText,
            cancelButtonLabel: cancelText,
            confirmCallback: function(result) {
                window.player.play('click');
                if (result) {
                    window.onCozmoProjectExportConfirmation();
                }
            }
        });
    }

    window.openCozmoProjectJSON = function (cozmoProjectJSON) {
        //console.log("ANKIPERFTEST top openCozmoProjectJSON");
        window.currentProjectUsesJSONFormat = true;
        try {
            window.openCozmoProjectRequest(cozmoProjectJSON.projectUUID, cozmoProjectJSON.projectName, cozmoProjectJSON.versionNum, cozmoProjectJSON.projectJSON, null, cozmoProjectJSON.isSampleStr);
        }
        catch(err) {
            window.cozmoDASError("Codelab.OpenCozmoProjectJSON.JavaScriptError", err.message);
            window.notifyProjectIsLoaded();
        }
    }

    // DEPRECATED - only used to open user projects that were created before 2.1 and are still in XML
    window.openCozmoProjectXML = function(projectUUID, projectName, projectVersionNum, projectXML, isCozmoSampleProjectStr) {
        //console.log("ANKIPERFTEST top openCozmoProjectXML");
        window.currentProjectUsesJSONFormat = false;
        window.openCozmoProjectRequest(projectUUID, projectName, projectVersionNum, null, projectXML, isCozmoSampleProjectStr);
    }

    // Don't call this method directly. Please call openCozmoProjectJSON instead.
    // This method takes a projectXML parameter to support pre-2.1 builds.
    window.openCozmoProject = function(projectUUID, projectName, projectVersionNum, projectJSON, projectXML, isCozmoSampleProjectStr) {
        //console.log("ANKIPERFTEST top openCozmoProject for projectName = " + projectName);

        var isCozmoSampleProject = (isCozmoSampleProjectStr == 'true');
        window.isCozmoSampleProject = isCozmoSampleProject;

        // TODO: Special case to fix localized text for intruder sample project. Rip out and revisit post-2.0.0.
        // TODO After sample projects are converted to JSON, must revisit this.
        if (isCozmoSampleProject && projectUUID == "4bb7eb61-99c4-44a2-8295-f0f94ddeaf62" && projectXML != null) {
            projectXML = window.replaceSampleProjectTextForIntruder(projectXML);
        }

        window.cozmoProjectUUID = projectUUID;
        window.cozmoProjectVersionNum = projectVersionNum;
        window.previouslySavedProjectJSON = null;

        window.setProjectNameAndSavedText(projectName, isCozmoSampleProject);

        window.clearSaveProjectTimer();

        if (isCozmoSampleProject && projectXML != null) {
            // Set the coordinate setting in the sample project xml to our desired location on-screen.
            // Note that for projects serialized as JSON (not XML), Blockly.WorkspaceSvg.prototype.scrollHome
            // is used to position the workspace correctly at project load time.
            var startingPoint = window.getScriptStartingPoint();
            projectXML = projectXML.replace("REPLACE_X_COORD", startingPoint.x);
            projectXML = projectXML.replace("REPLACE_Y_COORD", startingPoint.y);
        }

        window.startLoadingProject();

        if (projectJSON != null) {            
            window.Scratch.vm.fromJSON(JSON.stringify(projectJSON));
        }
        else {
            // User project was build pre-Cozmo app 2.1 release. Open projectXML.
        
            try {
                // Remove all existing scripts from workspace.
                Scratch.workspace.clear();

                var domXML = Blockly.Xml.textToDom(projectXML);
                Blockly.Xml.domToWorkspace(domXML, Scratch.workspace);
            }
            catch (err) {
                window.cozmoDASError("Codelab.OpenCozmoProject.JavaScriptError", "Error when loading XML project: " + err.message);
            }

            window.notifyProjectIsLoaded();
            window.startSaveProjectTimer();
        }

        //console.log("ANKIPERFTEST end openCozmoProject for projectName = " + projectName);
    }

    // TODO HACK Adding delay as calling this too early during workspace loading is causing the green flag
    // not to be recognized for horizontal and vertical sample projects.
    window.openCozmoProjectRequest = function(projectUUID, projectName, projectVersionNum, projectJSON, projectXML, isCozmoSampleProjectStr) {
        setTimeout(function(){
          window.openCozmoProject(projectUUID, projectName, projectVersionNum, projectJSON, projectXML, isCozmoSampleProjectStr);
        }, 250);
    }

    window.newProjectCreated = function(projectUUID, projectName) {
        window.cozmoProjectUUID = projectUUID;

        window.setProjectNameAndSavedText(projectName, false);
    }

    // Sets window.cozmoProjectName and sets name on workspace.
    // Makes rename and remix UI visible and tappable, if appropriate.
    window.setProjectNameAndSavedText = function(projectName, isSampleProject) {
        var unencodedProjectName = unescapedText( projectName );

        window.cozmoProjectName = unencodedProjectName;

        setText('#app-title', $t(unencodedProjectName));

        // if the title overflows the container, reduce the font size to make it fit
        var title = document.querySelector('#app-title');
        if (title.scrollWidth > title.clientWidth) {
          title.classList.add('long-title');
        }

        var autosavedText = window.$t('codeLab.SaveProject.Autosaved');
        if (isSampleProject) {
            autosavedText = window.$t('codeLab.SaveProject.NotSaved');
        }
        else if (unencodedProjectName == '' || unencodedProjectName == null) {
            // When user selects 'Create New Project' from save/load UI, at first project has no name and is not yet autosaved.
            autosavedText = '';
        }
        setText('#app-title-subtext', $t(autosavedText));

        if (window.cozmoProjectName != null && window.cozmoProjectUUID != null && window.cozmoProjectUUID != '') {
            // set class that title is set so that remix button is shown
            document.querySelector('#projecttext').classList.add('is-title-set');

            // initialize renames of user projects
            RenameProject.init();
        }
    }

    window.onRemixedProject = function(projectUUID, newProjectName, isFirstRemix) {
        window.cozmoProjectUUID = projectUUID;
        window.isCozmoSampleProject = false;
        window.isCozmoFeaturedProject = false;

        window.previouslySavedProjectJSON = null;
        window.changeMadeToSampleProject = false;

        if (isFirstRemix == "True") {
            ModalAlert.open({
              title: window.$t('codeLab.firstRemixSavedConfirmation.saved_first_remix_title'),
              prompt: window.$t('codeLab.firstRemixSavedConfirmation.remixing_blocks_jumpstart_prompt'),
              confirmButtonLabel: window.$t('codeLab.firstRemixSavedConfirmation.continue_button_label')
            });
        }

        window.setProjectNameAndSavedText(newProjectName, false);

        // Turn save timer back on
        window.startSaveProjectTimer();
    }

    window.clearSaveProjectTimer = function() {
        if (window.saveProjectTimerId != null) {
            clearInterval(window.saveProjectTimerId);
            window.saveProjectTimerId = null;
        }
    }

    window.startSaveProjectTimer = function() {
        // Start timer for intervals at which we'll check if the user project should be saved.
        if (!window.isCozmoSampleProject && !window.isCozmoFeaturedProject) {
            window.clearSaveProjectTimer();  
            var timeInterval_ms = 3000;
            window.saveProjectTimerId = setInterval(saveProjectTimer, timeInterval_ms);
            function saveProjectTimer() {
                window.saveCozmoUserProject(false);
            }
        }
    }

    // Removes variable values from serialized project string so that projects can be compared
    // without considering the variable values.
    //
    // Returns project as string.
    function cleanSerializedProjectStringForComparison(serializedProjectAsString) {
        var serializedProjectJSONObject = JSON.parse(serializedProjectAsString);
        if (serializedProjectJSONObject.targets[0]) {
             for (var key in serializedProjectJSONObject.targets[0].variables) {
                if (serializedProjectJSONObject.targets[0].variables.hasOwnProperty(key)) {
                    serializedProjectJSONObject.targets[0].variables[key].value = "";
                }
            }
        }

        return JSON.stringify(serializedProjectJSONObject)
    }

    // Check if sample or featured project blocks have been altered since the project was loaded.
    window.needToRemixProject = function() {
        if (window.isCozmoSampleProject || window.isCozmoFeaturedProject) {
            // For the featured and sample projects, if the user merely runs a project and tries to
            // exit, we don't want to prompt them to remix. If the project have a variable, and
            // upon running the project the var setting gets changed by the blocks, then technically
            // the project has changed (in its serialization), but we don't want to prompt the user
            // to remix in this case.
            //
            // So, for both current and original JSON, we will set all var values to empty string ("")
            // so we can ignore the var differences when we decide to prompt for remix.
            var currentSampleProjectJSONString = cleanSerializedProjectStringForComparison(Scratch.vm.toJSON());
            var originalSampleProjectJSONString = cleanSerializedProjectStringForComparison(window.originalSampleProjectJSON);
            if (currentSampleProjectJSONString != originalSampleProjectJSONString) {
                return true;
            }
        }

        return false;
    }

    // TODO Replace this method used for the very particular case of replacing intruder localized text in sample project.
    window.replaceSampleProjectTextForIntruder = function(projectXML) {
        var intruderTranslation = window.$t('codeLabChallenge_intruder.CozmoSaysBlock.DefaultText');
        return projectXML.replace("Intruder!", intruderTranslation);
    }
})();
