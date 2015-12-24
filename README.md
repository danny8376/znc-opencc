OpenCC module for ZNC
===================================

This module is a fork of znc translate module which specific to simple chinese
and traditional chinese translation.


Build
-----------------------------------

Build it with

    $ CXXFLAGS="-lopencc" znc-buildmod opencc.cpp

**This module is currently only tested with OpenCC 0.4.3. The code is supporting OpenCC 1.0, but isn't tested(even compiled).**

Install
-----------------------------------

Just like any other znc modules.

Usage
-----------------------------------

This module take none to one argument. The only argument is for opencc config path, default to "/usr/share/opencc".

For opencc translation, you will need to set translation mode.

Ex.

For channel #example, translate sent msgs from tc to sc, leaves received msgs.

    /msg *opencc set #example zhtw2zhs NONE

For user "translating" in channel #trans, translate sent msgs from sc to tc, received msgs from tc to sc.

    /msg *opencc set #trans/translating zhs2zhtw_p zhtw2zhs

For priv msg with "private", translate received msgs from sc to tc, leaves sent msgs

    /msg *opencc set private NONE zhs2zhtw_p

For more command usage, get help in this plugin:

    /msg *opencc help
