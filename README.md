OMXPlayer
=========

OMXPlayer is a commandline OMX player for the Raspberry Pi. It was developed as
a testbed for the XBMC Raspberry PI implementation and is quite handy to use
standalone. 

About this project
------------------

All credits go to popcornmix/huceke for creating omxplayer. 
See the original project here: https://github.com/popcornmix/omxplayer/

    This project uses modified Makefiles that compiles the player on the Pi!

My main goals were to:

    - Provide console input with --console argument (e.g. for SSH)
    - Implement a simple interface to control the player (RemoteBridge)

Remote bridge
-------------

This extension can connect to a given TCP port on localhost and
respond to simple commands like query playpack position, pause, seek,
set volume, etc.
There is a Python project I wrote which is able to create this TCP port
and control the player with commands received from an Android client.

See the Python project here:

    https://github.com/rycus86/omxremote-py

See the Android client project here:

    https://github.com/rycus86/omxremote-android

Installation and usage
----------------------

Should be installed like the original project.

    make ffmpeg
    make
    make dist
    (optionally) make install

