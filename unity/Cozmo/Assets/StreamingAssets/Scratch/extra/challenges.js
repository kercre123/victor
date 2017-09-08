/**
 * Functions used for the Challenges page
 * @author Adam Platti
 * @since 5/4/2017
 */
(function(){

  /**
   * Renders challenge slides
   * @param {Array} challenges - array of JSON objects for each challenge slide
   * @returns {void}
   */
  function renderChallenges(challenges) {
    // load the templates for questions and answers
    var questionTemplate = document.querySelector('#template-challenge-question').textContent;
    var answerTemplate = document.querySelector('#template-challenge-answer').textContent;

    // render each slide using the question or answer templates
    var html = '';
    for (var i=0; i < challenges.length; i++) {
      var challenge = challenges[i];

      // add the block category to color the icon background
      challenge['BlockCategory'] = getBlockIconColor(challenge['BlockIcon']);

      if (challenge.Template === "Question") {
        html += window._populateTemplate(questionTemplate, challenge);
      } else {
        // "Answer" category
        html += window._populateTemplate(answerTemplate, challenge);
      }
    }

    // set the rendered content into the modal window
    Slides.setSlidesHTML(html);

    // if there is a slide number in the URL hash, load that slide
    if (window.location.hash) {
      var slideNum = parseInt(window.location.hash.substring(1), 10) || 1;
      Slides.showSlideNum(slideNum, true);
    }
  }


  /**
   * register event handlders
   * @return {void}
   */
  function registerEvents() {

    // load translations when DOM is ready for filling in values
    window.addEventListener('DOMContentLoaded', function() {
      setText('#challenges-modal .modal-title', $t('codeLabChallenge_challengesUI.challengesModalTitle'));
      renderChallenges(window._CodeLabChallengesData);
    });
  }

  registerEvents();
})();
