
/**
 * Glossary module
 * Displays a modal popup that has reference inforomation about the blocks available
 *  to use in the workspace.
 * @author Adam Platti
 * @date 9/4/2017
 */
var Glossary = function(){
  'use strict';

  var eGlossaryModal;
  var eGlossary;
  var eCategories;
  var xml;
  var categoryMenu;
  var glossaryWS;
  var categoriesRendered = false;


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


  /**
   * Uses toolbox XML to render the list of block categories in the sidebar
   * @param {XML} xml - toolbox catalog as XML DOM
   * @returns {void}
   */
  function _renderCategorySidebar(xml) {

    // Find actual categories from the DOM tree.
    var categories = xml.querySelectorAll('category');
    eCategories.innerHTML = '';

    // create a list element for the categories
    var eList = document.createElement('ol');
    eList.className = 'category-list';

    // render the categories into the list
    for (var i=0; i < categories.length; i++) {
      var cat = categories[i];
      if (cat.children.length < 1) { continue; }

      var eCat = document.createElement('li');
      var categoryName = cat.getAttribute('name');
      eCat.className = 'category';
      eCat.setAttribute('data-category-id', categoryName);
      eCat.setAttribute('data-type', 'category');

      // create category colored bubble
      var eBubble = document.createElement('div');
      eBubble.className = 'category-bubble';
      eBubble.style.backgroundColor = cat.getAttribute('colour');
      eBubble.style.borderColor = cat.getAttribute('secondarycolour');
      eCat.appendChild(eBubble);

      // category title display
      var eCatTitle = document.createElement('div');
      eCatTitle.className = 'category-title';
      eCatTitle.textContent = $t(categoryName);
      eCat.appendChild(eCatTitle);

      eList.appendChild(eCat);
    }

    eCategories.appendChild(eList);

    categoriesRendered = true;
  }


  /**
   * Renders the blocks in a category
   * @param {String} categoryName - name of category to render blocks for
   * @returns {void}
   */
  function _renderCategoryBlocks(categoryName) {
    var blocks = xml.querySelectorAll('category[name="' + categoryName + '"] block');
    var eDefs = eGlossary.querySelector('#definitions');
    eDefs.innerHTML = '';
    var style = window.getComputedStyle(eDefs);
    var defsPadLeft = parseFloat(style.getPropertyValue('padding-left'));
    var defsPadRight = parseFloat(style.getPropertyValue('padding-right'));
    var defsPadding = defsPadLeft + defsPadRight;

    for (var i=0; i < blocks.length; i++) {
      var blockXml = blocks[i];
      var blockType = blockXml.getAttribute('type');
      var block = Blockly.Xml.domToBlock(blockXml, glossaryWS);

      var svgRoot = block.getSvgRoot();
      var blockBounds = block.getHeightWidth();
      var svg = Blockly.utils.createSvgElement('svg', {}, null);
      svg.appendChild(svgRoot);
      svg.setAttribute('class', 'block-image');

      var viewWidth = eDefs.clientWidth - defsPadding;

      var w = Math.max(blockBounds.width, viewWidth);
      var h = blockBounds.height;
      svg.setAttribute('width', viewWidth);
      svg.setAttribute('height', blockBounds.height);
      svg.setAttribute('viewBox', '0 0 ' + w + ' ' + h);

      // make room for Event blocks to have an overflow curved shape on top
      if (svgRoot.getAttribute('data-shapes') === 'hat') {
        svg.style.marginTop = '16px';
      }

      var eBlockDefContainer = document.createElement('div');
      eBlockDefContainer.className = 'block-def-container';
      eBlockDefContainer.setAttribute('data-block', blockType);

      var eBlockTitle = document.createElement('h2');
      eBlockTitle.className = 'block-def-title';
      eBlockTitle.textContent = $t('codeLabGlossary.block_definitions.title.' + blockType);

      var eBlockDef = document.createElement('p');
      eBlockDef.className = 'block-def-description';
      eBlockDef.textContent = $t('codeLabGlossary.block_definitions.description.' + blockType);

      eBlockDefContainer.appendChild(eBlockTitle);
      eBlockDefContainer.appendChild(svg);
      eBlockDefContainer.appendChild(eBlockDef);

      eDefs.appendChild(eBlockDefContainer);
    }
  }


  /**
   * Main click event handler for document body.
   * Relies on attribute "data-type" values on clickable elements to
   *  respond to the click.
   * @param {DOMEvent} event - click event on the document
   * @returns {void}
   */
  function _handleClick(event) {
    var typeElem = _getDataTypeElement(event.target);
    if (!typeElem) return;
    var type = typeElem.getAttribute('data-type');

    var playClickSound = true;

    switch(type) {
      case 'category':
        if (!typeElem.classList.contains('selected')) {
          _selectCategory(typeElem.getAttribute('data-category-id'));
        }
        break;
      case 'close-glossary':
        close();
        break;

      default:
        playClickSound = false;
    }

    if (playClickSound && window.player) {
      window.player.play('click');
    }
  }


  /**
   * Selects a category of blocks to display in the glossary
   * @param {String} categoryName - name of the category to display
   * @returns {void}
   */
  function _selectCategory(categoryName) {

    // unselect old category
    var eOldSelectedCat = eCategories.querySelector('.selected');
    if (eOldSelectedCat) {
      eOldSelectedCat.classList.remove('selected');
    }

    // select new category
    var eSelectedCat = eCategories.querySelector('.category[data-category-id="' + categoryName + '"]');
    eSelectedCat.classList.add('selected');

    _renderCategoryBlocks(categoryName);
  }



  /**
   * Initializes the glossary modal dialog
   * @returns {void}
   */
  function init(){

    setText('#glossary-title', $t('codeLabGlossary.glossary_title'));

    // register event listeners
    eGlossaryModal.removeEventListener('click', _handleClick);
    eGlossaryModal.addEventListener('click', _handleClick);

    if (!categoriesRendered) {
      _renderCategorySidebar(xml);
    }

    // create a read-only workspace for rendering the block SVGs
    glossaryWS = Blockly.inject('glossary-block-rendering-workspace', {
      collapse: true,
      comments: false,
      css: false,
      disable: true,
      grid: false,
      horizontalLayout: false,
      media: './lib/blocks/media/',
      readOnly: true,
      scrollbars: false,
      sounds: false,
      trashcan: false
    });

    // start with the Drive category
    _selectCategory('codeLab.Category.Drive');
  }


  /**
   * Closes the Glossary modal
   * @returns {void}
   */
  function close(){
    try {
      glossaryWS.dispose();
    } catch (e) {
      // catch errors thrown due to cleaning up the unusual way we are using blocks images.
      // @NOTE: a better solution might be needed here if memory is leaking.
    }

    document.body.classList.remove('show-glossary-modal');
  }


  /**
   * Opens the Glossary modal
   * @returns {void}
   */
  function open(){
    eGlossaryModal = document.querySelector('#glossary-modal');
    eGlossary = document.querySelector('#glossary');
    xml = document.querySelector('#toolbox');
    eCategories = eGlossary.querySelector('#glossary-categories');

    // hide all dropdowns
    if ('function' === typeof closeBlocklyDropdowns) {
      closeBlocklyDropdowns();
    }
    document.body.classList.add('show-glossary-modal');
    init();
  }

  return {
    init: init,
    open: open,
    close: close
  };
}();
