(function(){
  'use strict';

  registerEvents();

  function registerEvents() {
    window.addEventListener('DOMContentLoaded', function() {
      setText('.modal-slides #btn-back .button-label', 'Back');
      setText('.modal-slides #btn-next .button-label', 'Next');
      setText('.modal-slides #btn-done .button-label', 'Done');

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
      case 'btn-done':
        handleBtnDone();
        break;

      // no default
    }

    event.preventDefault();  // prevent double taps to zoom
  }

  function handleBtnDone() {
    // if the modal has an element with class 'btn-close', click it.
    var closeLink = document.querySelector('.modal-slides .btn-close');
    if (closeLink) {
      closeLink.click();  // click the close button to close the dialog
    }
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



  // Slideshow API
  window.Slides = function(){
    var strip;
    var slides;
    var modal;
    var btnNext;
    var btnBack;
    var btnDone;
    var numSlides;
    var isChallenges;

    /**
     * Initilizes slideshow
     * @returns {void}
     */
    function init() {
      // cache element references
      modal = document.querySelector('.modal-slides');  // NOTE: assumes one modal slideshow
      strip = modal.querySelector('.slide-strip');
      slides = strip.querySelectorAll('.slide');
      btnNext = modal.querySelector('#btn-next');
      btnBack = modal.querySelector('#btn-back');
      isChallenges = modal.getAttribute('id') === 'challenges-modal';
      btnDone = modal.querySelector('#btn-done');

      // cache values
      if (slides.length) {
        numSlides = slides.length;
      }
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
        showSlideNum(curSlide + 1);
      }
    }

    /**
     * Move back one slide
     * @returns {void}
     */
    function back(){
      var curSlide = _getCurrentSlideNum();
      if (curSlide - 1 >= 0) {
        showSlideNum(curSlide - 1);
      }
    }

    /**
     * Replaces slides content and reinitializes object
     * @param {String} html - html content of the slideshow
     */
    function setSlidesHTML(html) {
      strip.innerHTML = html;  // set the slides content
      init();  // reinitialize slideshow
    }

    /**
     * Displays the slide number specified
     * @param {Number} num - one based slide index to show
     * @param {Boolean} noAnimation - true to jump to slide w/o animation
     * @returns {void}
     */
    function showSlideNum(num, noAnimation) {
      var slide = slides[num - 1] || slides[0];  // go to slide, or first slide

      if (!slide) { return; } // exit if slide does not exist

      if (noAnimation) disableAnimation();
      strip.style.left = -slide.offsetLeft + 'px';

      if (noAnimation) {
        // restore page transition animations after this thread exits
        setTimeout(function(){
          enableAnimation();
        }, 0);
      }

      _updateDisplay();
    }

    /**
     * Enables animation between slides (animations are enabled by default)
     * @returns {void}
     */
    function enableAnimation() {
      modal.classList.remove('disable-slide-animation');
    }

    /**
     * Disables animation between slides
     * @returns {void}
     */
    function disableAnimation() {
      modal.classList.add('disable-slide-animation');
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
      if (slides.length === 0) return;

      var slideNum = _getCurrentSlideNum();

      // for challenges, save the slide page number in Unity to possibly reopen to later
      if (isChallenges) {
        // Save the current challenge page last viewed.
        window.Unity.call("{'requestId': '" + -1 + "', 'command': 'cozmoSetChallengeBookmark','argUInt': '" + slideNum + "'}");
      }

      // set the progress text
      setText('.progress', $t('{0} of {1}', slideNum, slides.length));

      // disable back button on first slide
      if (slideNum === 1) {
        btnBack.setAttribute('disabled', true);
      } else {
        btnBack.removeAttribute('disabled');
      }

      // disable Next button and show Done button on last slide
      if (slideNum === numSlides) {
        btnNext.setAttribute('disabled', true);
        btnDone.classList.remove('hide');
      } else {
        btnNext.removeAttribute('disabled');
        btnDone.classList.add('hide');
      }
    }

    // expose public API
    return {
      init: init,
      next: next,
      back: back,
      showSlideNum: showSlideNum,
      setSlidesHTML: setSlidesHTML,
      disableAnimation: disableAnimation,
      enableAnimation: enableAnimation
    };
  }();

})();
