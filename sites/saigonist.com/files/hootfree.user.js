// ==UserScript==
// @name        hootfree, HootSuite Ad-Blocker
// @namespace   http://www.saigonist.com
// @description Filters hootsuite.com's "promoted" tweets
// @match       http://hootsuite.com/*
// @include     http://hootsuite.com/*
// @version     2011.5.3
// ==/UserScript==

// Copyright (c) 2011 Tomo Huynh
// http://www.saigonist.com/hootfree


var styletext = "._messages ._promoted { background: black; } " +
                "._messages ._promoted p { color: black; }";

var newstyle = document.createElement('style');
newstyle.type = 'text/css';
newstyle.media = 'screen';
newstyle.appendChild(document.createTextNode(styletext));
// Note: if we ever needed IE support: http://yuiblog.com/blog/2007/06/07/style/
document.getElementsByTagName('head')[0].appendChild(newstyle);

