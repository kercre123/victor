/**
 *
 */
var RenameProject = function(){
  'use strict';

  var modal;
  var btnUpdateTitle;
  var origProjectName;
  var projectNameTextbox;
  var saveButton;

  /**
   * Intialize dialog
   * @returns {void}
   */
  function init() {
    // set translations
    setText('#btn-save-rename', $t('codeLab.renameProjectModal.saveButtonLabel'));
    setText('#btn-cancel-rename', $t('codeLab.renameProjectModal.cancelButtonLabel'));
    setText('#rename-project-prompt', $t('codeLab.renameProjectModal.renameProjectPrompt'));

    // cache relevant elements
    modal = document.querySelector('#rename-project-modal');
    btnUpdateTitle = modal.querySelector('#btn-update-title');
    projectNameTextbox = modal.querySelector('#rename-project-text-box');
    saveButton = modal.querySelector('#btn-save-rename');

    // register event handlers
    _registerEvents();
  }

  /**
   * Show the Rename Modal dialog
   */
  function _show() {
    // set the current proejct name in the title and edit box
    origProjectName = window.cozmoProjectName || 'Project Name Placeholder';
    document.querySelector('#rename-project-modal .original-project-name').textContent = origProjectName;
    document.querySelector('#rename-project-modal #rename-project-text-box').value = origProjectName;

    // disable the save button until the user changes the title
    saveButton.setAttribute('disabled', 'disabled');

    // show the dialog and focus on the text box
    document.body.classList.add('show-rename-project-modal');
  }

  /**
   * Hide the Rename Modal dialog
   */
  function _hide() {
    document.body.classList.remove('show-rename-project-modal');
  }

  /**
   * Register event handlers for the dialog
   */
  function _registerEvents() {
    // form submit handler (for when pressing "Go" on mobile keyboard)
    var form = modal.querySelector('#rename-project-form');
    form.removeEventListener('submit', _handleFormSubmit);
    form.addEventListener('submit', _handleFormSubmit);

    // Textbox keypres - to enable "Save" button when title changes
    projectNameTextbox.removeEventListener('keyup', _handleTextboxKeypress);
    projectNameTextbox.addEventListener('keyup', _handleTextboxKeypress);

    // Save button
    saveButton.removeEventListener('click', _handleSaveButton);
    saveButton.addEventListener('click', _handleSaveButton);

    // Cancel button - closes
    var cancelButton = modal.querySelector('#btn-cancel-rename');
    cancelButton.removeEventListener('click', _handleCancelButton);
    cancelButton.addEventListener('click', _handleCancelButton);

    // Close button
    var closeButton = modal.querySelector('#btn-close-rename-project');
    closeButton.removeEventListener('click', _handleCancelButton);
    closeButton.addEventListener('click', _handleCancelButton);

    // Project title
    var projectTitle = document.querySelector('#app-title');
    if (projectTitle && !window.isCozmoSampleProject) {
      projectTitle.removeEventListener('click', _show);
      projectTitle.addEventListener('click', _show);
    }
  }

  /**
   * Form submit handler
   * Saves new project if the user submits the form from the keyboard with return key
   * @param {DOMEvent} e - submit event
   * @returns {void}
   */
  function _handleFormSubmit(e) {
    // prevent the browser from changing the page
    e.preventDefault();

    // save project if the name changed
    if (origProjectName !== projectNameTextbox.value) {
      saveButton.click();
    }
  }

  /**
   * Handle keyboard events when the textbox is in focus
   * @param {DOMEvent} e - submit event
   * @returns {void}
   */
  function _handleTextboxKeypress(e) {
    if (projectNameTextbox.value !== origProjectName) {
      saveButton.removeAttribute('disabled');
    } else {
      saveButton.setAttribute('disabled', 'disabled');
    }
  }

  /**
   * Handle clicks on the cancel button
   * @param {DOMEvent} e - submit event
   * @returns {void}
   */
  function _handleCancelButton(e) {
    _hide();
    return false;
  }

  /**
   * Handle click on save button
   * @param {DOMEvent} e - submit event
   * @returns {void}
   */
  function _handleSaveButton(e) {
    window.renameProject("RenameProject.callbackAfterProjectRename", window.cozmoProjectUUID, projectNameTextbox.value);
  }

  /**
   * Callback function after Unity changes project name
   * @param {String} newProjectName - new project name just saved
   * @returns {void}
   */
  function callbackAfterProjectRename(newProjectName) {
    // update the display of the main project name
    var appTitle = document.querySelector('#app-title');
    if (appTitle) {
      appTitle.textContent = newProjectName;
    }
    _hide();
  }

  // public functions
  return {
    init: init,
    callbackAfterProjectRename: callbackAfterProjectRename
  };
}();