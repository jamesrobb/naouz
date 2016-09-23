#ifndef LOG_H

#include <glib.h>
#include <stdio.h>
#include <stdbool.h>

#define LOG_FILE_LOCATION	((gchar*) "debug.log")

const gchar * log_level_to_string (GLogLevelFlags level);

void httpd_log_all_handler_cb (const gchar *log_domain, 
							   GLogLevelFlags log_level, 
							   const gchar *message,
							   gpointer user_data);


#endif