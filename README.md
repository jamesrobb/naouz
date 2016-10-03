# naouz - A Simple HTTP Server

naouz is a simple http server written in c. This was written for a computer networking assignment at Reykjavik University.

The server makes the assumption that it can read what a particular client is sending it in one `recv()` call. The limit on how many bytes that is, is a constant set in `constants.h`.

# Requirements

* glib2.0

# Building

Move into the `src` directory and run:

    make

# Usage

    httpd -port N

# End Points

* /headers - Displays a list HTTP request headers that were sent over to naouz in a request
* /test - Displays a list of queries (get variables) used in the request
* /colour?bg=HTML_COLOUR - Displays a blank page with the background set to HTML_COLOUR where HTML_COLOUR is an html colour (red, blue, etc.). A cookie is also set to remember HTML_COLOUR
* /colour - Displays a blank page with the background set to HTML_COLOUR if HTML_COLOUR was stored in a cookie by calling the preceeding end point
* /favicon.ico - Displays the favicon of the server. The location of the favicon is assumed to be in the same directory the executable is run in.
* Anything else defaults to a page that displays some connection information, and will display anything you POST to it

GET and HEAD are defined for the end points listed above. The POST behavour described for the catch-all page applies to the end points listed above aswell.

# Logging

Both log files will be populated in the directory the executable is run in.

* debug.log - debugging information about naouz during execution
* access.log - information about end points requested, http status, and requesting client

# Fairness

naouz is relatively "fair" when serving its clients. The server uses `select()` to handle incoming connections and requests. Once `select()` populates an `fd_set` of file descriptors to work with, the server loops through them and responds to all of the requests before checking for more incoming connections and requests. This means that a client usually is not served again before all other requests known at that time are served.

To make naouz more fair, it would be possible to check the ip address of the incoming connections such that a subsequent request with the same ip address as an already received request would not be served until all other requests known at that time had been served first. This would prevent a host from using multiple parallel connections to receive more time from the server than other hosts. However, this mechanism alone assumes that each host has a unique ip address (i.e. there aren't multiple hosts sitting behind a home router, for example), and so in itself is not a complete solution.
