(function(){
  'use strict';

  registerEvents();

  function registerEvents() {
    window.addEventListener('DOMContentLoaded', function() {
      setText('.modal-slides #btn-back .button-label', $t('codeLab.modalSlideshow.backButtonLabel'));
      setText('.modal-slides #btn-next .button-label', $t('codeLab.modalSlideshow.nextButtonLabel'));
      setText('.modal-slides #btn-done .button-label', $t('codeLab.modalSlideshow.doneButtonLabel'));

      Slides.init();
    });

    // register main click handler for the document
    document.body.addEventListener('click', _handleBodyClick);

    // activate CSS for taps by registering a touchstart event
    document.addEventListener("touchstart", function(){}, true);

    // prevent elastic scrolling on the page that reveals whitespace and bounces back
    document.addEventListener('touchmove', function(event){
      event.preventDefault();
    });
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

    var playClickSound = true;

    switch(type) {
      case 'btn-next':
        Slides.next();
        break;
      case 'btn-back':
        Slides.back();
        break;
      case 'btn-done':
        playClickSound = false;  // prevents two click sounds: one for done button, and one for close button
        handleBtnDone();
        break;
      case 'btn-close':
        playClickSound = false;  // click sound is made in handleBtnClose
        handleBtnClose(typeElem, event);
        break;

      default:
        playClickSound = false;
        break;
    }

    if (playClickSound && window.player) {
      window.player.play('click');
    }

    event.preventDefault();  // prevent double taps to zoom
  }

  /**
   * Plays sound before closing the dialog
   * @param {HTMLElement} elem - close button element (assuming <a> (anchor) tag)
   * @returns {void}
   */
  function handleBtnClose(elem) {
    // if sound player is present, play a click sound before closing
    if (window.player) {
      window.player.play('click');
    }

    // to ensure the sound has time to play before navigating, intercept the
    //  anchor navigation and wait until after the sound plays to change pages

    // prevent the anchor tag from navigating to the next page until there is enough time for sound to play
    event.preventDefault();

    // save the URL of the anchor tag and navigate there after sound should finish playing
    var url = elem.getAttribute('href');
    setTimeout(function(){
      // navigate to anchor tag's url after sound finishes
      window.location.href = url;
    }, 50);
  }

  /**
   * "Done" button click handler.  Finds the close button element and clicks it.
   * @returns void
   */
  function handleBtnDone() {
    // if the modal has an element with class 'btn-close', click it.
    var closeLink = document.querySelector('.modal-slides .btn-close');
    if (closeLink) {
      handleBtnClose(closeLink);
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
      strip.style.webkitTransform = 'translate3d(-' + slide.offsetLeft + 'px, 0, 0)';

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
      // parse xyz int values out of string 'translate3d(0,0,0)'
      var parsedXYZ = strip.style.webkitTransform.replace('translate3d(','').replace(')','').split(',');

      var left = parseInt(parsedXYZ[0], 10) || 0;

      var slideWidth = strip.offsetWidth;
      var slideNum = Math.floor(-left / slideWidth) + 1;

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
      setText('.progress', $t('codeLabChallenge_challengesUI.slideXofY', slideNum, slides.length));

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
