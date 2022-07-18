// original doc at:
// https://docs.google.com/spreadsheet/pub?key=0AgvB-IYnTjzRdElpazB4UDFQejZ4MHR1bThzUVVJX1E&single=true&gid=0&output=html&widget=true'

var yesterday = new Date(new Date().valueOf()-24*60*60*1000);
var nextweek = new Date(new Date().valueOf()+7*24*60*60*1000);

$(document).ready(function() {

$('#flood-frame').contents().find('table td.s0').each(function(){
 var d = new Date($(this).html());
 if (d < yesterday || d > nextweek) $(this).parent('tr').hide();
});

});
