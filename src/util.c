#include "util.h"

int gchar_array_len(gchar *arr) {
	// this function assumes arr is null terminated
	int len = 0;

	while(arr[len] != '\0') {
		len++;

		if(len >= LONG_STRING_WARNING) {
			g_warning("string passed to gchar_array_len is very long");
		}
	}

	return len+1;
}

int gchar_array_array_len(gchar **arr) {
	// returns the length of an array of gchar arrays
	int len = 0;

	while(arr[len]) {
		len++;
	}

	return len;
}

void ghash_table_strstr_iterator(gpointer key, gpointer value, G_GNUC_UNUSED gpointer user_data) {
	g_print("field: %s, value: %s\n", (gchar*) key, (gchar*) value);
	return;
}

void ghash_table_gchar_destroy(gpointer value) {
	g_free((gchar *) value);
}

// strips strip_char from the source and saves the stripped array to the destination.
void gchar_char_strip(gchar *destination, gchar* source, gchar strip_char) {
	int length = gchar_array_len(source);
	int destpos = 0;
	for(int i = 0; i < length; i++) {
		if(source[i] != strip_char) {
			destination[destpos] = source[i];
			destpos++;
		}
	}
}