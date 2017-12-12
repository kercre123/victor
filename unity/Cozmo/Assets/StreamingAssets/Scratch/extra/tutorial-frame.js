/**
 * API for Workspace page to open and close the tutorial modal dialog
 * @author Nicolas Kent
 * @since 9/22/2017
 */
window.Tutorial = (function(){

  var _container = null;
  var _iframe = null;


  /**
   * Shows the tutorial modal
   * @returns {void}
   */
  function show() {
    if (!_container) {
      _injectFrame();
    }

    // hide all dropdowns
    if ('function' === typeof closeBlocklyDropdowns) {
      closeBlocklyDropdowns();
    }

    window.Unity.call({requestId: -1, command: "cozmoTutorialOpen"});

    var locale = window.getUrlVars()['locale'];
    var iframeUrl = 'extra/tutorial.html?locale=' + locale;

    _container.style.display = 'block';
    _iframe.setAttribute('src', iframeUrl);
  }

  /**
   * Hides the tutorial modal
   * @returns {void}
   */
  function hide() {
    Scratch.workspace.playAudio('click');
    window.Unity.call({requestId: -1, command: "cozmoTutorialClose"});
    document.querySelector('#tutorial-frame-container').style.display = 'none';
    document.querySelector('#tutorial-frame').setAttribute('src', '');
  }

  /**
   * Add elements to the page to create the tutorial modal shell and iframe
   */
  function _injectFrame() {
    _container = document.createElement('div');
    _container.setAttribute('id', 'tutorial-frame-container');

    var bg = document.createElement('div');
    bg.setAttribute('id', 'tutorial-frame-modal-bg');

    _iframe = document.createElement('iframe');
    _iframe.setAttribute('id', 'tutorial-frame');


    var button = document.createElement('button');
    button.setAttribute('id', 'btn-close-tutorial');
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
