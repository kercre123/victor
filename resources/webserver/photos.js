var dir = "/persistent/photos";
var fileextension = ".jpg";
var loadStockImages = false;

function AddImage(url) {
  var modal = document.getElementById('myModal');
  var modalImg = document.getElementById("imgModal");

  var img = $('<div class="tiles" style="background: url(\'' + url + '\'); ' +
              'background-repeat: no-repeat; background-size: 391px 220px;"></div>').appendTo($('.tile_container'))[0];
  img.onclick = function(){
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

    $(data).find("a:contains(" + fileextension + ")").each(function () {
      var filename = this.href.replace(window.location.host, "").replace("http://", "");
      if( filename.indexOf('.thm') < 0 ) {
        AddImage(filename);
        anyDisplayed = true;
      }
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
