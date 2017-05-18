/**
 * Sets static strings to localized text
 * @returns {void}
 */
window.addEventListener('DOMContentLoaded', function() {
  setText('#tutorial-modal .modal-title', 'Code Lab Tutorial');

  setText('#tutorial-slide-1 .slide-label', 'Code Lab lets you program Cozmo.');
  setText('#tutorial-slide-2 .slide-label', 'You can tap a code block to see what it does.');
  setText('#tutorial-slide-2 #text-tap-to-learn-more', 'Turn left' );
  setText('#tutorial-slide-3 .slide-label', 'Drag code blocks together to make a program.');
  setText('#tutorial-slide-4 .slide-label', 'Always place a green flag BLOCK first. That way, the green flag BUTTON starts your program!');

});
