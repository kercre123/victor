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
        html += _populateTemplate(questionTemplate, challenge);
      } else {
        // "Answer" category
        html += _populateTemplate(answerTemplate, challenge);
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
   * Render template and substitute keys
   * @param {String} template - simple template language
   * @param {Object} map - key/values to substitute into the template
   */
  function _populateTemplate(template, map) {
    var rendered = template;

    // replace keys in template text surrounded by two curly braces, like {{KeyName}}
    for (var key in map) {
      var regex = new RegExp('\\{\\{' + key + '\\}\\}', 'g');

      // if key ends in "Key", assume it's a translation key and look up the translation
      var value = (key.match("Key$")) ? $t(map[key]) : map[key];
      if (typeof value === 'object') {
        // replace value with translation
        value = value.translation;
      }

      // substitute values into the template
      rendered = rendered.replace(regex, value);
    }

    // return the rendered template with the substituted values
    return rendered;
  }

  /**
   * register event handlders
   * @return {void}
   */
  function registerEvents() {

    // load translations when DOM is ready for filling in values
    window.addEventListener('DOMContentLoaded', function() {
      setText('#challenges-modal .modal-title', $t('Challenges'));
      renderChallenges(window._CodeLabChallengesData);
    });
  }

  registerEvents();
})();
