#ifndef UTIL_H
#define UTIL_H

#include <glib.h>

#include "log.h"

#define LONG_STRING_WARNING	100000

int gchar_array_len(gchar *arr);

int gchar_array_array_len(gchar **arr);

void ghash_table_strstr_iterator(gpointer key, gpointer value, gpointer user_data);

void ghash_table_gchar_destroy(gpointer value);

#endif