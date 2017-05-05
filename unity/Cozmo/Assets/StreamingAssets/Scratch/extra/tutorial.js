/**
 * Sets static strings to localized text
 * @returns {void}
 */
window.addEventListener('DOMContentLoaded', function() {
  setText('#tutorial-modal .modal-title', 'Code Lab Tutorial');

  setText('#tutorial-slide-1 .slide-label', 'Code Lab lets you code Cozmo.');
  setText('#tutorial-slide-2 .slide-label', 'Tap to preview code blocks.\nTap to learn more.');
  setText('#tutorial-slide-3 .slide-label', 'Drag code blocks together.');
  setText('#tutorial-slide-4 .slide-label', 'Tap flag button to start flag block.');

});
