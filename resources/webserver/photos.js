var dir = "/persistent/photos";
var fileextension = ".jpg";
var loadStockImages = false;

function AddImage(url) {
  var modal = document.getElementById('myModal');
  var modalImg = document.getElementById("imgModal");

  var thumbnailUrl = url.replace('.jpg', '.thm.jpg');

  var img = $('<div class="tiles" style="background-image: url(\'' + thumbnailUrl + '\'); ' +
              // 'url(\'' + url + '\'); ' + // use full image url as a backup
              'background-repeat: no-repeat; background-size: 397px 223px;"></div>').appendTo($('.tile_container'))[0];

  img.onclick = function(){ // 1280x720 -> 320x180 -> * 0.31 = 396.8 x 223.2
    modal.style.display = "block";
    modalImg.src = url;
  }
}

$(function() {
  /* modal code borroed heavily from https://www.w3schools.com/howto/howto_css_modal_images.asp */

  // Get the <span> element that closes the modal
  var span = document.getElementsByClassName("close")[0];
  var modal = document.getElementById('myModal');

  // When the user clicks on <span> (x), close the modal
  span.onclick = function() {
    modal.style.display = "none";
  }

  if( loadStockImages ) {
    var getUrl = window.location;
    var baseUrl = getUrl.protocol + "//" + getUrl.host + "/" + getUrl.pathname.split('/').slice(1,-1).join("/");

    console.log('baseUrl: ' + baseUrl);

    [0, 1, 2, 3, 4, 5, 6].forEach(function(i) {
      var url = baseUrl + '/prdemo_stock_images/photo' + i + '.jpg';
      AddImage(url);
    });
  }

  $('#delete_photos').click( function() {
    if( confirm("Are you sure?") ) {
      $.post('/deleteallphotos', function(data) {
        location.reload();
      });
    }
  });
});

$.ajax({
  url: dir,
  success: function (data) {
    var anyDisplayed = false;

    var images = [];
    
    $(data).find("a:contains(" + fileextension + ")").each(function () {
      var filename = this.href.replace(window.location.host, "").replace("http://", "");
      if( filename.indexOf('.thm') < 0 ) {
        images.push(filename);
      }
    });

    // latest 20 images only
    images.sort();
    images.reverse();
    images.slice(0, 20).forEach(function(filename) {
      AddImage(filename);
      anyDisplayed = true;
    });

    if( !anyDisplayed && !loadStockImages ) {
      $(function() {
        $('.error_text').show();
      });
    }

  },
  error: function(req, status, err) {
    $(function() {
      if( !loadStockImages ) {
        $('.error_text').show();
      }
    });
  }
});
