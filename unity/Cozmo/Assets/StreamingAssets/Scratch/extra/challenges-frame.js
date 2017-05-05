/**
 * API for Workspace page to open and close the challenges modal dialog
 * @author Adam Platti
 * @since 5/4/2017
 */
window.Challenges = (function(){

  var _container = null;
  var _iframe = null;

  /**
   * Shows the challenges modal
   * @returns {void}
   */
  function show() {
    if (!_container) {
      _injectFrame();
    }

    _container.style.display = 'block';
    _iframe.setAttribute('src', 'extra/challenges.html');
  }

  /**
   * Hides the challenges modal
   * @returns {void}
   */
  function hide() {
    document.querySelector('#challenges-frame-container').style.display = 'none';
    document.querySelector('#challenges-frame').setAttribute('src', '');
  }

  /**
   * Add elements to the page to create the challenges modal shell and iframe
   */
  function _injectFrame() {
    _container = document.createElement('div');
    _container.setAttribute('id', 'challenges-frame-container');

    var bg = document.createElement('div');
    bg.setAttribute('id', 'challenges-frame-modal-bg');

    _iframe = document.createElement('iframe');
    _iframe.setAttribute('id', 'challenges-frame');


    var button = document.createElement('button');
    button.setAttribute('id', 'btn-close-challenges');
    var buttonImg = document.createElement('img');
    buttonImg.setAttribute('src', 'extra/images/icon_closeButton.svg');
    button.appendChild(buttonImg);
    button.addEventListener('click', hide);

    _container.appendChild(bg);
    _container.appendChild(_iframe);
    _container.appendChild(button);

    document.body.appendChild(_container);
  }

  return {
    show: show,
    hide: hide
  };
})();