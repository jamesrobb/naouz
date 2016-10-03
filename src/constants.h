#ifndef _CONSTANTS_H
#define _CONSTANTS_H

#define MAX_CLIENT_CONNS    		50
#define MAX_CONN_BACKLOG    		10
#define DATA_BUFFER_LENGTH  		77056 // 64KB
#define CONN_FREE           		-1
#define NAOUZ_VERSION       		((gchar *) "0.2")
#define FAVICON_LOCATION			((gchar *) "./favicon.ico")
#define CONNECTION_TIMEOUT			5
#define DEBUG_LOG_FILE_LOCATION		((gchar*) "./debug.log")
#define ACCESS_LOG_FILE_LOCATION 	((gchar*) "./access.log")

#endif