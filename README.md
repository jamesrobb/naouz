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
* anything else defaults to a page that shows you some connection information, and will display anything you POST to it
