(function(){
  'use strict';

  registerEvents();

  function registerEvents() {
    window.addEventListener('DOMContentLoaded', function() {
      setText('.modal-slides #btn-back .button-label', 'Back');
      setText('.modal-slides #btn-next .button-label', 'Next');

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
    var slideWidth;
    var numSlides;

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

      // cache values
      if (slides.length) {
        slideWidth = slides[0].parentNode.offsetWidth;
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
      if (noAnimation) enableAnimation();

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
      return Math.round(-left / slideWidth) + 1;
    }

    /**
     * Updates the progress text (ie. "2 of 5") and enables/disables buttons at edges of slideshow
     * @returns {void}
     */
    function _updateDisplay() {
      if (slides.length === 0) return;

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
      back: back,
      showSlideNum: showSlideNum,
      setSlidesHTML: setSlidesHTML,
      disableAnimation: disableAnimation,
      enableAnimation: enableAnimation
    };
  }();

})();
