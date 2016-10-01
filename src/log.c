#include "log.h"

const gchar * log_level_to_string (GLogLevelFlags level) {
  
	switch (level) {
		case G_LOG_LEVEL_ERROR: return "ERROR";
		case G_LOG_LEVEL_CRITICAL: return "CRITICAL";
		case G_LOG_LEVEL_WARNING: return "WARNING";
		case G_LOG_LEVEL_MESSAGE: return "MESSAGE";
		case G_LOG_LEVEL_INFO: return "INFO";
		case G_LOG_LEVEL_DEBUG: return "DEBUG";
		default: return "UNKNOWN";

	}

	return "UNKNOWN";
}

void httpd_log_all_handler_cb (const gchar *log_domain, 
						       GLogLevelFlags log_level,
						       const gchar *message,
						       gpointer user_data) {

	// we get the current time
	GDateTime *now = g_date_time_new_now_utc();

	// format a log message given the current time and log messaged passed to this function
	GString *error_string = g_string_new(NULL);
	g_string_printf(error_string,
					"%02d-%02d-%02dT%02d:%02d:%02dZ %s: %s \n", 
					g_date_time_get_year(now),
					g_date_time_get_month(now),
					g_date_time_get_day_of_month(now),
					g_date_time_get_hour(now),
					g_date_time_get_minute(now),
					g_date_time_get_second(now),
					log_level_to_string(log_level), 
					message);
	
	// print log message out to screen
	g_print("%s", error_string->str);

	write_to_log_file(DEBUG_LOG_FILE_LOCATION, error_string);

	g_date_time_unref(now);
	g_string_free(error_string, TRUE);

	return;
}

void httpd_log_access(gchar *client_ip,
					  int client_port, 
					  gchar *req_method,
					  gchar *host_name,
					  gchar* uri, 
					  gchar *response_code) {

	// we get the current time
	GDateTime *now = g_date_time_new_now_utc();

	// format a log message given the current time and log messaged passed to this function
	GString *access_string = g_string_new(NULL);
	g_string_printf(access_string,
					"%02d-%02d-%02dT%02d:%02d:%02dZ %s:%d %s\nhttp://%s%s: %s\n", 
					g_date_time_get_year(now),
					g_date_time_get_month(now),
					g_date_time_get_day_of_month(now),
					g_date_time_get_hour(now),
					g_date_time_get_minute(now),
					g_date_time_get_second(now),
					client_ip,
					client_port,
					req_method,
					host_name,
					uri,
					response_code);
	
	// print log message out to screen
	g_print("%s", access_string->str);

	g_string_append(access_string, "\n");
	write_to_log_file(ACCESS_LOG_FILE_LOCATION, access_string);

	g_date_time_unref(now);
	g_string_free(access_string, TRUE);

	return;

}

void write_to_log_file(gchar *file_location, GString *error_string) {

	// write log message to file
	FILE *log_fp;
	log_fp = fopen(file_location, "a");
	fwrite(error_string->str, (size_t) sizeof(gchar), (size_t) error_string->len, log_fp);
	fclose(log_fp);

	return;
}