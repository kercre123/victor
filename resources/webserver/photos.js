var dir = "/persistent/photos";
var fileextension = ".jpg";
var thebox = false;

$.ajax({
  url: dir,
  success: function (data) {
    if( !thebox) {
      for( var i=0; i<=6; ++i ) {
        $('.container').append(
          '<div class="tiles" style="background: url(/prdemo_stock_images/photo' + i + '.jpg); ' +
            'background-repeat: no-repeat; background-size: 391px 220px;"></div>');
      }
    }
    
    $(data).find("a:contains(" + fileextension + ")").each(function () {
      var filename = this.href.replace(window.location.host, "").replace("http://", "");
      if( filename.indexOf('.thm') < 0 ) {
        $('.container').append(
          '<div class="tiles" style="background: url(' + filename + '); ' +
            'background-repeat: no-repeat; background-size: 640px 360px;"></div>');
      }
    });
  }
});
