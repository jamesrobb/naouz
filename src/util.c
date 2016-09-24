#include "util.h"

int gchar_array_len(gchar *arr) {
	// this function assumes arr is null terminated
	int len = 1;

	while(arr[len] != '\0') {
		len++;

		if(len >= LONG_STRING_WARNING) {
			g_warning("string passed to gchar_array_len is very long");
		}
	}

	return len;
}