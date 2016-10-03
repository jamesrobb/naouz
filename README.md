# naouz - A Simple HTTP Server

naouz is a simple http server written in c.

# Requirements

* glib2.0

# Building

Move into the `src` directory and run:

    make

# Usage

    httpd -port N

# End Points

* /headers - displays list HTTP request headers that were sent over to naouz
* /test - displays list of queries (get variables) used in the request
* /colour?bg=<colour> - displays a blank page with the background set to <colour> where <colour> is an html color (red, blue, etc.)
* /favicon.ico - displays the favicon of the server
* anything else defaults to a page that shows you some connection information, and will display anything you POST to it

GET and HEAD is defined for the endpoints listed above. The POST behavour described for the catchall page applies to endpoints listed above aswell.
