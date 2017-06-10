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

    window.Unity.call("{'requestId': '-1', 'command': 'cozmoChallengesOpen'}");

    // Request a slide number to open to from Unity, to restore the same place
    // in the challenges if the user reopens challenges in this session. Retrieves
    // the last viewed challenge page.
    //
    // callback method will receive a parameter that is an int set to the the last viewed challenge page.
    // The callback has only been tested with a method on window.
    // Example: window.myCallback(lastChallengePageViewed);
    window.Unity.call("{'requestId': '" + -1 + "', 'command': 'cozmoGetChallengeBookmark','argString': 'window.callback_openToSlideNum'}");

    // FOR DEV ONLY - DO NOT TURN ON IN COMMIT
    //window.callback_openToSlideNum(6);
  }

  /**
   * Hides the challenges modal
   * @returns {void}
   */
  function hide() {
    Scratch.workspace.playAudio('click');
    window.Unity.call("{'requestId': '-1', 'command': 'cozmoChallengesClose'}");
    document.querySelector('#challenges-frame-container').style.display = 'none';
    document.querySelector('#challenges-frame').setAttribute('src', '');
  }


  /**
   * ONLY INTENDED TO BE CALLED BY UNITY!
   * Callback function passed to unity that gets called with the page number
   *  in the challenges slides to display first.
   * @returns {void}
   */
  function openToSlideNum(number) {
    // put a hash fragement itentifier on the URL to tell the
    //  challenges which page to start with
    var iframeUrl = 'extra/challenges.html#' + (number || 1);

    _container.style.display = 'block';
    _iframe.setAttribute('src', iframeUrl);
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
    hide: hide,
    openToSlideNum: openToSlideNum
  };
})();

window.callback_openToSlideNum = function(number) {
  window.Challenges.openToSlideNum(number);
};