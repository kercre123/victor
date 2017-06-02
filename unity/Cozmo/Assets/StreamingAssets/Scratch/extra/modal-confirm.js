/**
 * Confirm Modal Dialog
 * Provides a modal dialog to ask the use confirm/cancel questions with custom text.
 *
 * @author Adam Platti
 * @since 6/1/2017
 */
var ModalConfirm = (function(){

  var _dialog = null;   // cached reference to modal dialog container element
  var _options = null;  // options the dialog was opened with, including callback function

  /**
   * Opens the confirmation dialog
   * @param {Object} options - paramters for text labels in the dialog and callback function
   * @param {String} options.title - title to display for dialog
   * @param {String} options.prompt - prompt text the user is asked to confirm
   * @param {String} options.confirmButtonLabel - label text for confirm button
   * @param {String} options.cancelButtonLabel - label text for cancel button
   * @param {Function} options.confirmCallback - function called with result of the confirmation
   * @returns {void}
   */
  function open(options) {

    // add the dialog elements to the page if not already done
    if (!_dialog) {
      _dialog = _render();
      document.body.appendChild(_dialog);
      _dialog.addEventListener('click', _handleClick);
    }

    // save the options, so we can call the callback function later
    _options = options;

    // set the text lables for dialog title, prompt, and buttons
    _dialog.querySelector('.btn-confirm').textContent = options.confirmButtonLabel;
    _dialog.querySelector('.btn-cancel').textContent = options.cancelButtonLabel;
    _dialog.querySelector('.modal-confirm-title').textContent = options.title;
    _dialog.querySelector('.modal-confirm-prompt').textContent = options.prompt;

    // show the dialog
    _dialog.style.visibility = 'visible';

    // HACK!!
    // Using a double timeout here to prevent a flash of the dialog taking up the full width
    //  of the screen and then shrinking to the right size on iOS.  This is the only thing I
    //  found which worked.
    var inner = _dialog.querySelector('.modal-inner');
    setTimeout(function(){
      setTimeout(function(){
        inner.style.top = 0;
      });
    },0);
  }

  /**
   * Hides the confirm dialog and clears any state
   * @returns {void}
   */
  function _close() {
    _dialog.style.visibility = 'hidden';
    _options = null;
  }


  /**
   * Handles clicks on the confirm dialog
   * @param {Event} browser click event
   * @returns {void}
   */
  function _handleClick(event) {
    var target = event.target;
    var classes = target.classList;

    if (classes.contains('modal-bg')) {
      window.player.play('click');
    } else if (classes.contains('btn-confirm')) {
      // @NOTE: let the caller's callback decide if a click sound should be played
      _options.confirmCallback(true);
    } else if (classes.contains('btn-cancel')) {
      // @NOTE: let the caller's callback decide if a click sound should be played
      _options.confirmCallback(false);
    }

    _close();
  }


  /**
   * Renders the HTML for the confirm dialog and retusn the unattached container element
   * @returns {HTMLElement} container element of the modal dialog
   */
  function _render() {
    // render the HTML into a template and return the element
    var template = document.createElement('template');
    var html = '<div class="modal-confirm">' +
               '  <div class="modal-bg" data-type="modal-background"></div>' +
               '  <div class="modal-inner">' +
               '    <h2 class="hd modal-confirm-title"></h2>' +
               '    <p class="bd modal-confirm-prompt"></p>' +
               '    <div class="ft">' +
               '      <button class="btn-cancel button-pill button-blue-pill"></button>' +
               '      <button class="btn-confirm button-pill button-red-pill"></button>' +
               '    </div>' +
               '  </div>' +
               '</div>';

    template.innerHTML = html;
    return template.content.firstChild;
  }

  return {
    open : open
  };

})();
