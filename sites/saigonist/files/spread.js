var imax = 3;
var jmax = 3;
function saigonist_spread_stringify() {
    var ary = [];
    for (var j = 1; j <= jmax; j++) {
        var row = [];
        for (var i = 1; i <= imax; i++) {
            var val = $('input[name="saigonist-spread-cell-'+j+'-'+i+'"]')[0].value;
            row.push(val);
        }
        ary.push(row);
    }

    return JSON.stringify(ary);
}

function saigonist_spread_add_row() {
    // add row
    jmax += 1;
    var id = 'saigonist-spread-row-'+jmax;
    $('.saigonist-spread-rows').append('<div id="'+id+'">');
    $('#'+id).attr('class', 'saigonist-spread-row');
    $('#'+id).append('<div class="saigonist-spread-col-0">' + jmax + '</div>');
    for (var i = 1; i <= imax; i++) {
        $('#'+id).append('<div class="saigonist-spread-col-' + i + '"><input type="text" length="10" name="saigonist-spread-cell-' + jmax + '-' + i + '"></div>');
    }
}

function saigonist_spread_add_col() {
    imax += 1;
    $('.saigonist-spread-col-' + (imax - 1)).after('<div class="saigonist-spread-col-' + imax + '">');
    var letter = 'A'.charCodeAt(0) + imax - 1;
    if (letter >= 'Z'.charCodeAt(0)) 
        letter = 'Z' + letter; // XXX lazy
    else
        letter = String.fromCharCode(letter);
    $('.saigonist-spread-row-head .saigonist-spread-col-' + imax).append('<span>' + letter + '</span>');
    for (var j = 1; j <= jmax; j++) {
        $('#saigonist-spread-row-' + j + ' .saigonist-spread-col-' + imax).append('<input type="text" length="20" name="saigonist-spread-cell-' + j + '-' + imax + '">');
    }
}

$(document).ready(function() {
    $('#saigonist-spread input').live('click', function(e) { $('#saigonist-spread-value-json').html(saigonist_spread_stringify()); });
});

