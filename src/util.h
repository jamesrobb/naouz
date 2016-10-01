#ifndef UTIL_H
#define UTIL_H

#include <glib.h>

#include "log.h"

#define LONG_STRING_WARNING	100000

int gchar_array_len(gchar *arr);

int gchar_array_array_len(gchar **arr);

void ghash_table_strstr_iterator(gpointer key, gpointer value, gpointer user_data);

void ghash_table_gchar_destroy(gpointer value);

void gchar_char_strip(gchar *destination, gchar* source, gchar strip_char);

void gstring_fill_with_header(gchar* key, gchar* val, GString *string_fill);

#endif