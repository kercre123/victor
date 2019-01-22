var dir = "/persistent/photos";
var fileextension = ".jpg";
var thebox = true;

$.ajax({
  url: dir,
  success: function (data) {
    /* modal code borroed heavily from https://www.w3schools.com/howto/howto_css_modal_images.asp */
    
    // Get the modal
    var modal = document.getElementById('myModal');
    var modalImg = document.getElementById("imgModal");

    if( !thebox) {
      [0, 1, 2, 3, 4, 5, 6].forEach(function(i) {
        var url = '/prdemo_stock_images/photo' + i + '.jpg';
        var img = $('<div class="tiles" style="background: url(' + url + '); ' +
                    'background-repeat: no-repeat; background-size: 640px 360px;"></div>').appendTo($('.container'))[0];
        img.onclick = function(){
          modal.style.display = "block";
          modalImg.src = url;
        }
      });
    }
    
    $(data).find("a:contains(" + fileextension + ")").each(function () {
      var filename = this.href.replace(window.location.host, "").replace("http://", "");
      if( filename.indexOf('.thm') < 0 ) {
        var img = $('<div class="tiles" style="background: url(' + filename + '); ' +
                    'background-repeat: no-repeat; background-size: 640px 360px;"></div>').appendTo($('.container'))[0];

        img.onclick = function(){
          modal.style.display = "block";
          modalImg.src = filename;
        }
      }
    });

    // Get the <span> element that closes the modal
    var span = document.getElementsByClassName("close")[0];

    // When the user clicks on <span> (x), close the modal
    span.onclick = function() { 
      modal.style.display = "none";
    }

  }
});
