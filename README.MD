GSoC 2020: Implementing a MPD Module in VLC
=============================================
## Introduction
* This repository contains supplement files for the [VLC MPD Server Proposal](https://docs.google.com/document/d/1HHBeUhZwo73mbgj_cNPHwifbSYW9LP9x_NRQUlRTvKY/edit?usp=sharing).
* Read more about MPD Protocol here: [Music Player Daemon](https://mpd.readthedocs.io/en/stable/)
* Basic schema of MPD Architecture to be implemented as module discussed in [this repository](https://github.com/knightwayne/MPD-Architecture).


### How to Use MPD Module with VLC Media Player

*   Step 1: Install VLC Media Player on your system, and along with this, the mpd module will also be installed.
*   Step 2: Using a Command Line, initialise the MPD server within VLC Media Player with command **./vlc --mpd**.
    This loads the MPD module along with running the core _vlc_main()_ thread in the background, waiting for mpd clients requests to come and service them.
*   Step 3: Using mpc-clients, send requests to the server to control playback and queue.
*   Step 4: Stop the server to kill the thread.

### Summary Of Proposed Tasks

* Creating a basic MPD server-client architecture
* Implementing the MPD Protocols
* Parsing Client Side Request
* Connecting it with VLC media library
* Implementing Different MPD features
* Configuring Files Option
* Command line Access to the server
* Documentation & Bug Fixes

### Summary of Completed Work

* Created MPD Architecture based on Socket Programming Communication Model and implemented MPD protocol to service clients.
* Polling for sampling a set of clients
* Parsing Input Commands from different Music Player Daemon-Clients.(MPC)
* Implemented two ways of sending data.
* Implemented the following sets of Commands for controlling VLC through MPD Protocol ([Complete Set of Commands Available Here](https://www.musicpd.org/doc/mpc/html/)):
    1. Playback Options: consume, repeat, random, single, volume, etc
    2. Controlling Playback: play, pause, next, previous, stop, seek, etc
    3. Queue Control: add (files, folders recursively), delete, move, swap, shuffle, playlist information, playlist information metadata, etc.
    4. Playlists: Saving, loading and modifying playlists from MPD.
    5. Music Database: Not very stable, but basic listing and search/search+add features work fine.
    6. Handling other misc commands: Ping, Idle, Close, etc.
* Documentation 

### Remaining Features & Known Issues:

* Currently only one filter can be applied through _vlc_query_params_ for searching through music database. Multiple tag filters for searching needs to be added, in Advanced Find/Search commands.
* _input_item_t_ Parameter can't extract current playback/database information when Music is being played.
* _status_ and _stats_ commands: can't extract information, _input_item_t_ doesn't grab value as it should.
* Making the music database more verbose. Adding more tags to be queried by MPD clients.
