/**
 * Sets static strings to localized text
 * @returns {void}
 */
window.addEventListener('DOMContentLoaded', function() {
  setText('#tutorial-modal .modal-title', $t('codeLab.tutorial.modalTitle'));

  setText('#tutorial-slide-1 .slide-label', $t('codeLab.tutorial.slide_letsYouProgramCozmo'));
  setText('#tutorial-slide-2 .slide-label', $t('codeLab.tutorial.slide_tapBlockToSeeFunction'));
  setText('#tutorial-slide-2 #text-tap-to-learn-more', $t('codeLab.tutorial.slide_tapBlockToSeeFunction.sampleTooltip.turnLeft'));
  setText('#tutorial-slide-3 .slide-label', $t('codeLab.tutorial.slide_dragBlocksToMakePrograms'));
  setText('#tutorial-slide-4 .slide-label', $t('codeLab.tutorial.slide_greenBlockFirst'));

});
