(function(){
  'use strict';

  // register onload and body click events
  registerEvents();


  /**
   * Sets static strings to localized text
   * @returns {void}
   */
  function setLocalizedText() {
    setText('#app-title', $t('Code Lab'));
    setText('#app-title-subtext', $t('based on'));
    // setText('.project-new .project-title', $t('Start A New Project'));
    setText('#tutorial-label', $t('Tutorial'));
    setText('#new-project-label', $t('New Project'));
    setText('#confirm-delete-label', $t('Are you sure you want to delete\nthis project?'));

    setText('#btn-delete-project', $t('Yes, Delete'));
    setText('#btn-cancel-delete-project', $t('No, Cancel'));
    setText('#confirm-delete-label', $t('Are you sure you want to delete this project?'));

    setText('#prototype-sample-project .project-type', $t('Sample'));
    setText('#prototype-user-project .project-type', $t('Personal'));
  }


  function registerEvents(){

    window.addEventListener('DOMContentLoaded', function() {
      // set localization strings after document body has been parsed
      setLocalizedText();

      // start rendering projects
      CozmoAPI.getProjects('window.renderProjects');
    });

    // register main click handler for the document
    document.body.addEventListener('click', _handleBodyClick);

    // activate CSS for taps by registering a touchstart event
    document.addEventListener("touchstart", function(){}, true);
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

    switch(type) {
      case 'btn-create-new-project':
        CozmoAPI.createNewProject();
        break;
      case 'load-sample-project':
        CozmoAPI.openSampleProject(typeElem.dataset.uuid);
        break;
      case 'load-user-project':
        CozmoAPI.openUserProject(typeElem.dataset.uuid);
        break;
      case 'btn-confirm-delete-project':
        showConfirmDeleteProjectModal(typeElem.parentNode);
        break;
      case 'btn-cancel-delete-project':
        hideConfirmDeleteProjectModal();
        break;
      case 'btn-delete-project':
        CozmoAPI.deleteProject(typeElem.dataset.uuid);
        break;
      case 'btn-close-page':
        CozmoAPI.closeCodeLab();
        break;
      case 'modal-background':
        hideConfirmDeleteProjectModal();
        break;
      case 'btn-tutorial':
        alert('show tutorial');
        break;
      default:
        console.log('unrecognized click data-type: ' + type);
    }
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
   * Renders cards for user and sample projects
   * @param {Array} userProjects - array of user projects to render
   * @param {Array} sampleProjects - array of sample projects to render
   */
  window.renderProjects = function(userProjectsStr, sampleProjectsStr) {
    var projectList = document.querySelector('#projects-list');
    var i, card;

    // render user projects and add them to the UI
    var userProjects = JSON.parse(userProjectsStr);
    var sampleProjects = JSON.parse(sampleProjectsStr);

    if (Array.isArray(userProjects)) {
      for(i = 0; i < userProjects.length; i++) {
        card = makeUserProjectCard(userProjects[i], i);
        projectList.appendChild(card);
      }
    }

    // render sample projects and add them to the UI
    if (Array.isArray(sampleProjects)) {
      for(i = 0; i < sampleProjects.length; i++) {
        card = makeSampleProjectCard(sampleProjects[i]);
        projectList.appendChild(card);
      }
    }
  };


  /**
   * Creates DOM elements for a card representing a sample project
   * @param {Object} projectData - fields about the project
   * @returns {HTMLElement} returns unattached DOM element for sample project card
   */
  function makeSampleProjectCard(projectData) {
    // clone the sample project card prototype
    var project = document.querySelector('#prototype-sample-project').cloneNode(true);
    project.removeAttribute('id');

    // add the project data to the element
    setProjectData(project, projectData);

    // set the project title
    project.querySelector('.project-title').textContent = projectData.ProjectName;

    // set the icon to match the content of the sample project
    var icon = projectData.ProjectIconName;
    var iconPath = '../lib/blocks/media/icons/' + icon + '.svg';
    project.querySelector('.block-icon').setAttribute('src', iconPath);

    var projectsUI = document.querySelector('#projects');

    // add a style to color the puzzle piece SVG after it loads
    var block = project.querySelector('.block');
    var type = getSampleProjectType(icon);
    block.setAttribute('src', 'images/icon_block_' + type + '.svg');
    block.addEventListener('load', function(elem) {
      project.style.visibility = 'visible';

      // show the entire Projects UI when the first sample project is ready to be shown
      projectsUI.style.visibility = 'visible';
    });
    return project;
  }


  /**
   * Creates DOM elements for a card representing a user project
   * @param {Object} projectData - fields about the project
   * @param {Number} order - the position in personal cards this card appears (used for coloring)
   * @returns {HTMLElement} returns unattached DOM element for peronal project card
   */
  function makeUserProjectCard(projectData, order) {
    // clone the user project prototype
    var project = document.querySelector('#prototype-user-project').cloneNode(true);
    project.removeAttribute('id');

    // add the project data to the element
    setProjectData(project, projectData);

    // set the project title
    project.querySelector('.project-title').textContent = projectData.ProjectName;

    // set color of puzzle pieces on background card based on order position
    var cardUrl = 'images/framing_widgetBackground_player' + ((order % 3) + 1) + '.svg';
    var card = project.querySelector('.project-card');
    card.setAttribute('src', cardUrl);
    card.addEventListener('load', function(){
      // show the card once the background has loaded
      project.style.display = 'inline-block';
    });

    return project;
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


  /**
   * Returns background color for a sample project icon category
   * @param {String} icon - name of the icon
   * @returns {String} hex color background for the icon
   */
  function getSampleProjectType(icon) {
    switch (icon) {

      // motion blocks
      case 'cozmo-backward':
      case 'cozmo-backward-fast':
      case 'cozmo-forward':
      case 'cozmo-forward-fast':
      case 'cozmo-turn-left':
      case 'cozmo-turn-right':
      case 'cozmo-dock-with-cube':
        return 'motion';

      // looks blocks
      case 'cozmo-forklift-high':
      case 'cozmo-forklift-low':
      case 'cozmo-forklift-medium':
      case 'cozmo-head-angle-high':
      case 'cozmo-head-angle-medium':
      case 'cozmo-head-angle-low':
      case 'set-led_black':
      case 'set-led_blue':
      case 'set-led_coral':
      case 'set-led_green':
      case 'set-led_mystery':
      case 'set-led_orange':
      case 'set-led_purple':
      case 'set-led_white':
      case 'set-led_yellow':
      case 'cozmo_says':
        return 'looks';

      // event blocks
      case 'cozmo-face':
      case 'cozmo-face-happy':
      case 'cozmo-face-sad':
      case 'cozmo-cube':
      case 'cozmo-cube-tap':
        return 'event';

      // control blocks
      case 'control_forever':
      case 'control_repeat':
        return 'control';

      // actions blocks
      case 'cozmo-anim-bored':
      case 'cozmo_anim-cat':
      case 'cozmo_anim-chatty':
      case 'cozmo-anim-dejected':
      case 'cozmo-anim-dog':
      case 'cozmo-anim-excited':
      case 'cozmo-anim-frustrated':
      case 'cozmo-anim-happy':
      case 'cozmo-anim-mystery':
      case 'cozmo-anim-sleep':
      case 'cozmo-anim-sneeze':
      case 'cozmo-anim-surprise':
      case 'cozmo-anim-thinking':
      case 'cozmo-anim-unhappy':
      case 'cozmo-anim-victory':
        return 'action';

      // no default to make mistakes more obvious
    }
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
    // set the title with the project name
    setText('#confirm-delete-title', $t('Delete \'{0}\'?', projectName));

    // save the UUID of the project to delete on the confirm delete button element
    document.querySelector('#btn-delete-project').dataset.uuid = uuid;

    // show the modal
    document.querySelector('#delete-project-modal').style.visibility = 'visible';
  }


  /**
   * Hides delete user project confirmation modal
   * @returns {void}
   */
  function hideConfirmDeleteProjectModal(){
    document.querySelector('#delete-project-modal').style.visibility= 'hidden';
  }


  // ***************
  // Cozmo/Unity API
  // ***************

  /**
   * CozmoAPI is a wrapper over our Unity API so that I can fake
   *  it during development.
   */
  var CozmoAPI = function(){

    function getProjects(callbackName) {
      window.getCozmoUserAndSampleProjectLists(callbackName);
    }

    function createNewProject() {
      window.requestToCreateCozmoProject();
    }

    function openUserProject(uuid) {
      window.requestToOpenCozmoUserProject(uuid);
    }

    function openSampleProject(uuid) {
      window.requestToOpenCozmoSampleProject(uuid);
    }

    function deleteProject(uuid) {
      // hide the delete project confirmation modal
      hideConfirmDeleteProjectModal();

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
      deleteProject: deleteProject,
      closeCodeLab: closeCodeLab
    };
  }();

  // *****************
  // Utility functions
  // *****************

  /**
   * Returns localized strings for display.  Accepts optional parameters
   *  to substitue into localized string.
   *
   * Example:  $t("{0} Unicorns", 5)  returns  "5 Unicorns"
   *
   * @param {String} str - localized string with possible substitution flags
   * @param
   */
  function $t(str) {
    // $$$$$ TODO: look up localized strings

    // subsitute values into the translated string if params are passed
    if (arguments.length > 1) {
      var numSubs = arguments.length - 1;
      for (var i=0; i<numSubs; i++) {
        // substitute the values in the string like "{0}"
        str = str.replace('{'+i+'}', arguments[i+1]);
      }
    }
    return str;
  }

  /**
   * Sets text inside an element described by a CSS selector
   * @param {String} selector - CSS selector of element to set text for
   * @param {String} text - text to apply to element
   * @returns {void}
   */
  function setText(selector, text) {
    document.querySelector(selector).textContent = text;
  }



  /**
   * DEVELOPMENT ONLY
   * Used to render projects into page when run from outside the Unity application.
   * @returns {void}
   */
  function _devLoadProjects() {

    function getJSON(url, callback) {
      var request = new XMLHttpRequest();
      request.open('GET', url, true);

      request.onload = function() {
        if (this.status >= 200 && this.status < 400) {
          // Success!
          var data = JSON.parse(this.response);
          callback(data);
        }
      };
      request.send();
    }

    getJSON('../sample-projects.json', function(sampleProjects) {
      // copy 3 of the sample projects and render them as user projects
      var userProjects = sampleProjects.slice(0,3);
      window.renderProjects(JSON.stringify(userProjects), JSON.stringify(sampleProjects));
    });
  }

})();

