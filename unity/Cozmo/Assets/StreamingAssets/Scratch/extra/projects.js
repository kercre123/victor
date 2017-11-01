(function(){
  'use strict';

  // Prevent duplicate featured project tile clicks.
  // Could expand to use for all project tiles if desired.
  var featuredProjectTileClicked = false;

  // register onload and body click events
  registerEvents();


  /**
   * Sets static strings to localized text
   * @returns {void}
   */
  function setLocalizedText() {
    setText('#app-title', $t('codeLab.projects.modalTitle'));

    setText('#tutorial-label', $t('codeLab.projects.tutorialLabel'));
    setText('#new-project-label-horizontal', $t('codeLab.projects.newHorizontalProjectButtonLabel'));
    setText('#new-project-label-vertical', $t('codeLab.projects.newVerticalProjectButtonLabel'));

    setText('#prototype-sample-project .project-type', $t('codeLab.projects.projectType.sampleProject'));
    setText('#prototype-user-project .project-type', $t('codeLab.projects.projectType.myProject'));

    setText('#featured-projects-tab', $t('codeLab.projects.featuredTab.title'));
    setText('#horizontal-projects-tab', $t('codeLab.projects.horizontalTab.title'));
    setText('#vertical-projects-tab', $t('codeLab.projects.verticalTab.title'));

    setText('#horizontal-tab-hero .tab-hero-title', $t('codeLab.projects.horizontalTab.horizontalTabHeroTitle'));
    setText('#horizontal-tab-hero .tab-hero-detail', $t('codeLab.projects.horizontalTab.horizontalTabHeroDetail'));

    setText('#vertical-tab-hero .tab-hero-title', $t('codeLab.projects.verticalTab.verticalTabHeroTitle'));
    setText('#vertical-tab-hero .tab-hero-detail', $t('codeLab.projects.verticalTab.verticalTabHeroDetail'));

    setText('#featured-tab-hero .tab-hero-title', $t('codeLab.projects.featuredTab.featuredTabHeroTitle'));
    setText('#featured-tab-hero .tab-hero-detail', $t('codeLab.projects.featuredTab.featuredTabHeroDetail'));
  }


  /**
   * Registers page event handlers
   * @returns {void}
   */
  function registerEvents(){

    window.addEventListener('DOMContentLoaded', function() {
      // set localization strings after document body has been parsed
      setLocalizedText();

      // decide which projects tab to display first and load it
      setDefaultTab();
    });

    // register main click handler for the document
    document.body.addEventListener('click', _handleBodyClick);

    // activate CSS for taps by registering a touchstart event
    document.addEventListener("touchstart", function(){}, true);

    // prevent page from elastic scrolling to reveal whitespace, but
    //  allow the project list to scroll
    document.addEventListener('touchmove', function(event){
      event.preventDefault();
    });
    document.querySelector('#projects-list').addEventListener('touchmove', function(event){
      // allow the project list to scroll by touch
      event.stopPropagation();
    });
  }

  /**
   * Main click event handler for document body.
   * Relies on attribute "data-type" values on clickable elements to
   *  respond to the click.
   * @param {DOMEvent} event - click event on the document
   * @returns {void}
   */
  function _handleBodyClick(event) {
    var typeElem = _getDataTypeElement(event.target);
    if (!typeElem) return;
    var type = typeElem.getAttribute('data-type');

    var playClickSound = true;

    switch(type) {
      case 'btn-create-new-project':
        // we play the click sound before calling unity to make sure sound plays on android
        playClickSound = false;
        handleCreateNewProjectClick();
        break;
      case 'load-sample-project':
        // we play the click sound before calling unity to make sure sound plays on android
        playClickSound = false;
        handleSampleProjectClick(typeElem.dataset.uuid);
        break;
      case 'load-featured-project':
        if (!featuredProjectTileClicked) {
          featuredProjectTileClicked = true;

          // we play the click sound before calling unity to make sure sound plays on android
          playClickSound = false;
          handleFeaturedProjectClick(typeElem.dataset.uuid);
        }
        break;
      case 'load-user-project':
        // we play click the sound before calling unity to make sure sound plays on android
        playClickSound = false;
        handleUserProjectClick(typeElem.dataset.uuid);
        break;
      case 'btn-confirm-delete-project':
        showConfirmDeleteProjectModal(typeElem.parentNode);
        break;
      case 'btn-close-page':
        handleClosePage();
        break;
      case 'tutorial-link':
        playClickSound = false;
        handleTutorialLinkClick(typeElem);
        break;
      case 'project-tab':
        switchProjectTab(typeElem);
        break;

      default:
        playClickSound = false;
    }

    if (playClickSound && window.player) {
      window.player.play('click');
    }
  }

  /**
   * Chooses first projects tab to display based on value of "projects" URL parameter
   * @returns {void}
   */
  function setDefaultTab() {
    var param = window.getUrlVar('projects');
    var tabElem = null;

    if (param) {
      var tabId = param + '-projects-tab';
      tabElem = document.querySelector('#' + tabId);
    }

    // default to vertical for now
    // TODO Change default to featured
    if (!tabElem) {
      tabElem = document.querySelector('#vertical-projects-tab');
    }

    switchProjectTab(tabElem);
  }


  /**
   * Switches between views of featured, horizontal, and vertical projects
   * @param {HTMLElement} tabElem - the element representing the tab to be selected
   * @returns {void}
   */
  function switchProjectTab(tabElem) {
    var projectsElem = document.querySelector('#projects');

    var tabId = tabElem.getAttribute('id').replace("#", "").replace("-projects-tab", "");
    window.Unity.call({command: "cozmoSwitchProjectTab", argString: tabId});

    if (tabElem.classList.contains('tab-selected')) {
      return;
    } else {
      // clear out old projects to prevent flashes of old and new cards together
      clearProjects();

      // update selected state of tabs
      var oldSelected = tabElem.parentNode.querySelector('.tab-selected');
      if (oldSelected) {
        oldSelected.classList.remove('tab-selected');
      }
      tabElem.classList.add('tab-selected');

      projectsElem.className = tabElem.getAttribute('id');

      // load projects for new tab
      loadProjects();
    }
  }


  /**
   * Loads project data from unity app or as simulated unity webview in a normal browser
   * @returns {void}
   */
  function loadProjects() {

    if (window.isDev()){
      // dev: simulate this page as a webview in Unity
      _devLoadProjects();
    } else {
      // request project list from Unity
      CozmoAPI.getProjects('window.renderProjects');
    }
  }

  function resetProjectsScroll() {
    var bdElem = document.querySelector('#projects .bd');
    bdElem.scrollLeft = 0;
  }


  /**
   * Fetches the name of the selected projects tab
   * @returns {String|null} 'featured', 'horizontal', 'vertical'
   */
  function getSelectedProjectTabName() {
    var selectedProjectTab = document.querySelector('#project-tabs .tab-selected');
    if (selectedProjectTab) {
      return selectedProjectTab.getAttribute('id').split('-')[0];
    } else {
      return null;
    }
  }


  /**
   * Opens confirmation modal when user signals to close the projects page
   * @returns {void}
   */
  function handleClosePage() {
    // open a dialog confirming that they want to exit the Code Lab
    ModalConfirm.open({
      title: $t('codeLab.projects.confirmQuit.promptText'),
      prompt: '',
      cancelButtonLabel: $t('codeLab.projects.confirmQuit.cancelButton.labelText'),
      confirmButtonLabel: $t('codeLab.projects.confirmQuit.confirmButton.labelText'),
      confirmCallback: function(result) {
        window.player.play('click');
        if (result) {
          CozmoAPI.closeCodeLab();
        }
      }
    });
  }

  /**
   * Handles notifying Unity that the user wants to create a new project
   * @returns {void}
   */
  function handleCreateNewProjectClick() {
    // Play the click sound before calling unity to insure it is played on Android
    if (window.player) {
      window.player.play('click');
    }
    CozmoAPI.createNewProject();
  }

  /**
   * Handles notifying Unity that the user wants to load a sample project
   * @returns {void}
   */
  function handleSampleProjectClick(uuid) {
    // Play the click sound before calling unity to insure it is played on Android
    if (window.player) {
      window.player.play('click');
    }
    CozmoAPI.openSampleProject(uuid);
  }

  /**
   * Handles notifying Unity that the user wants to load a featured project
   * @returns {void}
   */
  function handleFeaturedProjectClick(uuid) {
    // Play the click sound before calling unity to insure it is played on Android
    if (window.player) {
      window.player.play('click');
    }
    CozmoAPI.openFeaturedProject(uuid);
  }

  /**
   * Handles notifying Unity that the user wants to load a user's personal project
   * @returns {void}
   */
  function handleUserProjectClick(uuid) {
    // Play the click sound before calling unity to insure it is played on Android
    if (window.player) {
      window.player.play('click');
    }
    CozmoAPI.openUserProject(uuid);
  }

  /**
   * Plays sound before opening the tutorial
   * @param {HTMLElement} elem - tutorial link button element (assuming <a> (anchor) tag)
   * @returns {void}
   */
  function handleTutorialLinkClick(elem) {
    // if sound player is present, play a click sound before closing
    if (window.player) {
      window.player.play('click');
    }

    // to ensure the sound has time to play before navigating, intercept the
    //  anchor navigation and wait until after the sound plays to change pages

    // prevent the anchor tag from navigating to the next page until there is enough time for sound to play
    event.preventDefault();

    // save the URL of the anchor tag and navigate there after sound should finish playing
    var url = elem.getAttribute('href');
    setTimeout(function(){
      // navigate to anchor tag's url after sound finishes
      window.location.href = url + '?locale=' + LOCALE;
    }, 50);
  }

  /**
   * returns closest element with a "data-type" attribute
   * @param {HTMLElement} elem - elem to search for data-type attribute on self or ancestors
   * @returns {HTMLElement|null} closest element with data-type attribute, or null
   */
  function _getDataTypeElement(elem){
    while (elem && elem !== document) {
      var type = elem.getAttribute('data-type');
      if (type) {
        return elem;  // return element with data-type attribute
      } else {
        elem = elem.parentNode;  // search next ancestor
      }
    }
    return null;  // no element with data-type attribute found
  }


  // **********************
  // Project Card Rendering
  // **********************

  /**
   * Removes user and sample project cards from display
   * @returns {void}
   */
  function clearProjects() {
    var projectList = document.querySelector('#projects-list');
    var projects = projectList.children;

    resetProjectsScroll();

    // skip the "create new project" and divider elements
    var nextProject = projects[0];
    while (nextProject) {
      var curProject = nextProject;
      nextProject = curProject.nextSibling;

      // do not remove th new project button
      if (curProject.nodeType === Node.TEXT_NODE || !curProject.classList.contains('project-new')) {
        curProject.parentNode.removeChild(curProject);
      }
    }
  }


  /**
   * Renders cards for user and sample projects
   * @param {Array} userProjects - array of user projects to render
   * @param {Array} sampleProjects - array of sample projects to render
   */
  window.renderProjects = function(userProjectsStr, sampleProjectsStr) {
    // clear any already rendered projects
    clearProjects();

    var projectTabName = getSelectedProjectTabName();
    var projectList = document.querySelector('#projects-list');
    var i, card;

    // render user projects and add them to the UI

    if (projectTabName === 'featured') {
      // featured projects
      var featuredProjects = JSON.parse(userProjectsStr);
      if (Array.isArray(featuredProjects)) {
        for(i = 0; i < featuredProjects.length; i++) {
          card = makeFeaturedProjectCard(featuredProjects[i]);
          projectList.appendChild(card);
        }
      }
    } else {
      // horizontal or vertical projects
      var userProjects = JSON.parse(userProjectsStr);
      var sampleProjects = JSON.parse(sampleProjectsStr);

      // render user projects and add them to UI
      if (Array.isArray(userProjects)) {
        for(i = 0; i < userProjects.length; i++) {
          card = makeUserProjectCard(userProjects[i], i, projectTabName);
          projectList.appendChild(card);
        }
      }

      // render sample projects and add them to the UI
      if (Array.isArray(sampleProjects)) {
        for(i = 0; i < sampleProjects.length; i++) {
          card = makeSampleProjectCard(sampleProjects[i], projectTabName);
          projectList.appendChild(card);
        }
      } else {
        // no sample projects.
        // make UI visible right away instead of waiting for sample project images to load
        var projectsUI = document.querySelector('#projects');
        projectsUI.style.visibility = 'visible';
      }
    }
  };



  /**
   * Creates DOM elements for a card representing a sample project
   * @param {Object} projectData - fields about the project
   * @param {String} tabName - name of the tab the sample proejct is for ('horizontal' or 'vertical')
   * @returns {HTMLElement} returns unattached DOM element for sample project card
   */
  function makeSampleProjectCard(projectData, tabName) {
    // clone the sample project card prototype
    var project = document.querySelector('#prototype-sample-project').cloneNode(true);
    project.removeAttribute('id');
    project.classList.add(tabName);

    // add the project data to the element
    setProjectData(project, projectData);

    // set the project title to the localized name
    var title = project.querySelector('.project-title');
    title.textContent = $t(projectData.ProjectName);

    // set the icon to match the content of the sample project
    var icon = projectData.ProjectIconName;
    var iconPath = '../lib/blocks/media/icons/' + icon + '.svg';
    project.querySelector('.block-icon').setAttribute('src', iconPath);

    var projectsUI = document.querySelector('#projects');

    // add a style to color the puzzle piece SVG after it loads
    var block = project.querySelector('.block');
    var type = getBlockIconColor(icon);
    if (tabName === 'vertical') {
      block.setAttribute('src', 'images/icon_bars_' + type + '.svg');
    } else {
      block.setAttribute('src', 'images/icon_block_' + type + '.svg');
    }

    block.addEventListener('load', function(elem) {
      project.style.visibility = 'visible';

      // if title is unusually large, add a class to reduce the font
      _shrinkLongProjectTitle(title, project);

      // show the entire Projects UI when the first sample project is ready to be shown
      projectsUI.style.visibility = 'visible';
    });
    return project;
  }


  /**
   * Creates DOM elements for a card representing a user project
   * @param {Object} projectData - fields about the project
   * @param {Number} order - the position in personal cards this card appears (used for coloring)
   * @param {String} tabName - name of the tab the sample proejct is for ('horizontal' or 'vertical')
   * @returns {HTMLElement} returns unattached DOM element for peronal project card
   */
  function makeUserProjectCard(projectData, order, tabName) {
    // clone the user project prototype
    var project = document.querySelector('#prototype-user-project').cloneNode(true);
    project.removeAttribute('id');
    project.classList.add(tabName);

    // add the project data to the element
    setProjectData(project, projectData);

    // set the project title
    var title = project.querySelector('.project-title');
    title.textContent = projectData.ProjectName;

    // set color of puzzle pieces on background card based on order position
    var tabName = getSelectedProjectTabName();
    var cardUrl = 'images/framing_cardMyProject0' + ((order % 3) + 1) + '_'+tabName+'.svg';
    var card = project.querySelector('.project-card');
    card.setAttribute('src', cardUrl);
    card.addEventListener('load', function(){
      // show the card once the background has loaded
      project.style.display = 'inline-block';

      // if title is unusually large, add a class to reduce the font
      _shrinkLongProjectTitle(title, project);
    });

    return project;
  }


  /**
   * Creates DOM element for a featured project card
   * @param {Object} projectData - fields about the project
   * @returns {HTMLElement} returns unattached DOM element for featured project card
   */
  function makeFeaturedProjectCard(projectData) {
    var project = document.querySelector('#prototype-featured-project').cloneNode(true);
    project.removeAttribute('id');

    // add the project data to the element
    setProjectData(project, projectData);

    // set the project title to the localized name
    var title = project.querySelector('.project-title');
    title.textContent = $t(projectData.ProjectName);
    title.style.color = projectData.FeaturedProjectTitleTextColor;

    // set the project description to the localized name
    var description = project.querySelector('.project-description');
    description.textContent = $t(projectData.FeaturedProjectDescription);

    var projectsUI = document.querySelector('#projects');

    // set color of puzzle pieces on background card based on order position
    var cardUrl = 'images/featured/framing_card' + projectData.FeaturedProjectImageName + '_feat.png';
    var card = project.querySelector('.project-card');
    card.setAttribute('src', cardUrl);
    card.addEventListener('load', function(){
      // show the card once the background has loaded
      project.style.display = 'inline-block';

      // if title is unusually large, add a class to reduce the font
      _shrinkLongProjectTitle(description, project);

      // show the entire Projects UI when the first featured project is ready to be shown
      projectsUI.style.visibility = 'visible';
    });

    return project;
  }


  /**
   * Applies a CSS class to project title container if it is too large to fit on the card
   * @param titleElem {HTMLElement} element on project card containing the project title text
   * @param projectElem {HTMLElement} container element for the project card
   * @returns {void}
   */
  function _shrinkLongProjectTitle(titleElem, projectElem) {
    // title is approximately 30% of the entire project container
    var approxTitleAreaHeight = parseInt(projectElem.clientHeight, 10) * 0.3;

    if (titleElem.clientHeight > approxTitleAreaHeight) {
      // title exceedes the size given so apply class to reduce the font size
      titleElem.classList.add('long-title');
    }
  }

  /**
   * Copies relevent project data fields to data attributes of project DOM element
   * @param {HTMLElement} projectElem - project container element to copy fields to
   * @param {Object} projectData - project fields to copy to project's dataset
   * @returns {void}
   */
  function setProjectData(projectElem, projectData) {
    projectElem.setAttribute('data-uuid', projectData.ProjectUUID);
    projectElem.setAttribute('data-project-name', projectData.ProjectName);
  }




  // *********************************
  // Confirm Delete User Project Modal
  // *********************************

  /**
   * Displays modal asking user to confirm deleting a user project
   * @param {HTMLElement} projectElem - element containing the project
   * @returns {void}
   */
  function showConfirmDeleteProjectModal(projectElem){
    var uuid = projectElem.getAttribute('data-uuid');
    var projectName = projectElem.getAttribute('data-project-name');


    // @NOTE: translation tags for confirm/cancel button strings are intentionally switched.
    //        the keys should have been chosen based on their content, and not whether they
    //        were confirms or cancels.
    ModalConfirm.open({
      title: $t('codeLab.projects.confirmDeleteProject.confirmPromptTitle', projectName),
      cancelButtonLabel: $t('codeLab.projects.confirmDeleteProject.cancelButton.labelText'),
      confirmButtonLabel: $t('codeLab.projects.confirmDeleteProject.confirmButton.labelText'),
      prompt: $t('codeLab.projects.confirmDeleteProject.confirmPrompt'),
      confirmCallback: function(result) {
        if (!result) {
          window.player.play('click');
        } else {
          if (window.player) {
            window.player.play('delete');
          }
          CozmoAPI.deleteProject(uuid);
        }
      }
    });
  }




  // ***************
  // Cozmo/Unity API
  // ***************

  /**
   * CozmoAPI is a wrapper over our Unity API so that I can fake
   *  it during development.
   */
  var CozmoAPI = function(){

    function _getIsVertical() {
      var projectTabName = getSelectedProjectTabName();
      var isVertical = (projectTabName === 'vertical');
      return isVertical;
    }

    function getProjects(callbackName) {
      var projectTabName = getSelectedProjectTabName();
      if (projectTabName == 'featured') {
        // render nothing until there are featured projects
        window.getCozmoFeaturedProjectList(callbackName);
      } else {
        var isVertical = _getIsVertical();
        window.getCozmoUserAndSampleProjectLists(callbackName, isVertical);
      }
    }

    function createNewProject() {
      var isVertical = _getIsVertical();
      window.requestToCreateCozmoProject(isVertical);
    }

    function openUserProject(uuid) {
      var isVertical = _getIsVertical();
      window.requestToOpenCozmoUserProject(uuid, isVertical);
    }

    function openSampleProject(uuid) {
      var isVertical = _getIsVertical();
      window.requestToOpenCozmoSampleProject(uuid, isVertical);
    }

    function openFeaturedProject(uuid) {
      window.requestToOpenCozmoFeaturedProject(uuid);
    }

    function deleteProject(uuid) {
      // remove the project from the display
      var projectElem = document.querySelector('.project[data-uuid="'+uuid+'"]');
      projectElem.parentNode.removeChild(projectElem);

      // notify Unity to actually delete the project
      window.deleteCozmoUserProject(uuid);
    }

    function closeCodeLab() {
      window.closeCodeLab();
    }

    return {
      getProjects: getProjects,
      createNewProject: createNewProject,
      openUserProject: openUserProject,
      openSampleProject: openSampleProject,
      openFeaturedProject: openFeaturedProject,
      deleteProject: deleteProject,
      closeCodeLab: closeCodeLab
    };
  }();

  // *****************
  // Utility functions
  // *****************

  /**
   * DEVELOPMENT ONLY
   * Used to render projects into page when run from outside the Unity application.
   * @returns {void}
   */
  function _devLoadProjects() {

    var tabName = getSelectedProjectTabName();
    if (tabName === 'featured') {
      // renderFeaturedProjects();

      getJSON('../featured-projects.json', function(featuredProjects) {

        var projects = JSON.parse(JSON.stringify(featuredProjects));

        for (var i=0; i < projects.length; i++) {
          var project = projects[i];
          project.ProjectName = $t(project.ProjectName);
          project.FeaturedProjectDescription = $t(project.FeaturedProjectDescription);
        }

        window.renderProjects(JSON.stringify(projects));
      });

    } else {
      getJSON('../sample-projects.json', function(sampleProjects) {
        // copy 3 of the sample projects and render them as user projects
        var userProjects = sampleProjects.slice(0,3);

        // deep copy the fake user projects so that translations of sample projects are not effected
        userProjects = JSON.parse(JSON.stringify(userProjects));

        // translate the sample project names
        for (var i=0; i < userProjects.length; i++) {
          userProjects[i].ProjectName = $t(userProjects[i].ProjectName);
        }

        window.renderProjects(JSON.stringify(userProjects), JSON.stringify(sampleProjects));
      });
    }
  }

})();

