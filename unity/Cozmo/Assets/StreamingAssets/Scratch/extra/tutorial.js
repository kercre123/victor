(function(){
  'use strict';

  registerEvents();


  /**
   * Sets static strings to localized text
   * @returns {void}
   */
  function setLocalizedText() {
    setText('#tutorial-modal .modal-title', 'Code Lab Tutorial');

    setText('#tutorial-slide-1 .slide-label', 'Code Lab lets you code Cozmo.');
    setText('#tutorial-slide-2 .slide-label', 'Tap to preview code blocks.\nTap to learn more.');
    setText('#tutorial-slide-3 .slide-label', 'Drag code blocks together.');
    setText('#tutorial-slide-4 .slide-label', 'Tap flag button to start flag block.');

    setText('#tutorial-modal #btn-back .button-label', 'Back');
    setText('#tutorial-modal #btn-next .button-label', 'Next');
  }


  function registerEvents() {
    window.addEventListener('DOMContentLoaded', function() {
      // set localization strings after document body has been parsed
      setLocalizedText();
      Slides.init();
    });

    // register main click handler for the document
    document.body.addEventListener('click', _handleBodyClick);

    // activate CSS for taps by registering a touchstart event
    document.addEventListener("touchstart", function(){}, true);
  }


  /**
   * Main click event handler for document body.
   * Relies on attribute "data-type" values on clickable elements to
   *  respond to the click.
   * @param {DOMEvent} event - click event on the document
   * @returns {void}
   */
  function _handleBodyClick(event) {
    var typeElem = _getDataTypeElement(event.target);
    if (!typeElem) return;
    var type = typeElem.getAttribute('data-type');

    switch(type) {
      case 'btn-next':
        Slides.next();
        break;
      case 'btn-back':
        Slides.back();
        break;

      // no default
    }

    event.preventDefault();  // prevent double taps to zoom
  }


  // Slideshow API
  var Slides = function(){
    var strip;
    var slides;
    var modal;
    var btnNext;
    var btnBack;
    var numSlides;

    /**
     * Initilizes slideshow
     * @returns {void}
     */
    function init() {
      // cache element references
      modal = document.querySelector('#tutorial-modal');
      strip = modal.querySelector('.slide-strip');
      slides = strip.querySelectorAll('.slide');
      btnNext = modal.querySelector('#btn-next');
      btnBack = modal.querySelector('#btn-back');

      // cache values
      numSlides = slides.length;
      strip.style.left = 0; // allows transition animation to second page

      // set the slide count text and disable the back button
      _updateDisplay();
    }


    /**
     * Advance to next slide
     * @returns {void}
     */
    function next() {
      var curSlide = _getCurrentSlideNum();
      if (curSlide + 1 <= numSlides) {
        _showSlideNum(curSlide + 1);
      }
      _updateDisplay();
    }

    /**
     * Move back one slide
     * @returns {void}
     */
    function back(){
      var curSlide = _getCurrentSlideNum();
      if (curSlide - 1 >= 0) {
        _showSlideNum(curSlide - 1);
      }
      _updateDisplay();
    }

    /**
     * Displays the slide number specified
     * @param {Number} num - one based slide index to show
     * @returns {void}
     */
    function _showSlideNum(num) {
      var slide = slides[num - 1];
      strip.style.left = -slide.offsetLeft + 'px';
    }

    /**
     * Returns the number of the current slide
     * @param {Number}
     */
    function _getCurrentSlideNum() {
      var left = parseInt(strip.style.left, 10) || 0;
      var slideWidth = strip.offsetWidth;
      var slideNum = Math.round(-left / slideWidth) + 1;

      if (isNaN(slideNum)) {
        // on older android devices, slideWidth might be null early in page life
        return 1;
      } else {
        return slideNum;
      }
    }

    /**
     * Updates the progress text (ie. "2 of 5") and enables/disables buttons at edges of slideshow
     * @returns {void}
     */
    function _updateDisplay() {
      var slideNum = _getCurrentSlideNum();

      setText('.progress', $t('{0} of {1}', slideNum, slides.length));

      // disable back button on first slide
      if (slideNum === 1) {
        btnBack.setAttribute('disabled', true);
      } else {
        btnBack.removeAttribute('disabled');
      }

      // disable Next button on last slide
      if (slideNum === numSlides) {
        btnNext.setAttribute('disabled', true);
      } else {
        btnNext.removeAttribute('disabled');
      }
    }

    // expose public API
    return {
      init: init,
      next: next,
      back: back
    };
  }();







  // *****************
  // Utility functions
  // *****************

  /**
   * Returns localized strings for display.  Accepts optional parameters
   *  to substitue into localized string.
   *
   * Example:  $t("{0} Unicorns", 5)  returns  "5 Unicorns"
   *
   * @param {String} str - localized string with possible substitution flags
   * @param
   */
  function $t(str) {
    // $$$$$ TODO: look up localized strings

    // subsitute values into the translated string if params are passed
    if (arguments.length > 1) {
      var numSubs = arguments.length - 1;
      for (var i=0; i<numSubs; i++) {
        // substitute the values in the string like "{0}"
        str = str.replace('{'+i+'}', arguments[i+1]);
      }
    }
    return str;
  }

  /**
   * Sets text inside an element described by a CSS selector
   * @param {String} selector - CSS selector of element to set text for
   * @param {String} text - text to apply to element
   * @returns {void}
   */
  function setText(selector, text) {
    document.querySelector(selector).textContent = text;
  }

  /**
   * returns closest element with a "data-type" attribute
   * @param {HTMLElement} elem - elem to search for data-type attribute on self or ancestors
   * @returns {HTMLElement|null} closest element with data-type attribute, or null
   */
  function _getDataTypeElement(elem){
    while (elem && elem !== document) {
      var type = elem.getAttribute('data-type');
      if (type) {
        return elem;  // return element with data-type attribute
      } else {
        elem = elem.parentNode;  // search next ancestor
      }
    }
    return null;  // no element with data-type attribute found
  }

})();
