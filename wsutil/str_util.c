/* str_util.c
 * String utility routines
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"
#include "str_util.h"

int
ws_xton(char ch)
{
	switch (ch) {
		case '0': return 0;
		case '1': return 1;
		case '2': return 2;
		case '3': return 3;
		case '4': return 4;
		case '5': return 5;
		case '6': return 6;
		case '7': return 7;
		case '8': return 8;
		case '9': return 9;
		case 'a':  case 'A': return 10;
		case 'b':  case 'B': return 11;
		case 'c':  case 'C': return 12;
		case 'd':  case 'D': return 13;
		case 'e':  case 'E': return 14;
		case 'f':  case 'F': return 15;
		default: return -1;
	}
}

/* Convert all ASCII letters to lower case, in place. */
gchar *
ascii_strdown_inplace(gchar *str)
{
	gchar *s;

	for (s = str; *s; s++)
		/* What 'g_ascii_tolower (gchar c)' does, this should be slightly more efficient */
		*s = g_ascii_isupper (*s) ? *s - 'A' + 'a' : *s;

	return (str);
}

/* Convert all ASCII letters to upper case, in place. */
gchar *
ascii_strup_inplace(gchar *str)
{
	gchar *s;

	for (s = str; *s; s++)
		/* What 'g_ascii_toupper (gchar c)' does, this should be slightly more efficient */
		*s = g_ascii_islower (*s) ? *s - 'a' + 'A' : *s;

	return (str);
}

/* Check if an entire string is printable. */
gboolean
isprint_string(const gchar *str)
{
	guint pos;

	/* Loop until we reach the end of the string (a null) */
	for(pos = 0; str[pos] != '\0'; pos++){
		if(!g_ascii_isprint(str[pos])){
			/* The string contains a non-printable character */
			return FALSE;
		}
	}

	/* The string contains only printable characters */
	return TRUE;
}

/* Check if an entire UTF-8 string is printable. */
gboolean
isprint_utf8_string(const gchar *str, guint length)
{
	const char *c;

	if (!g_utf8_validate (str, length, NULL)) {
		return FALSE;
	}

	for (c = str; *c; c = g_utf8_next_char(c)) {
		if (!g_unichar_isprint(g_utf8_get_char(c))) {
			return FALSE;
		}
	}

	return TRUE;
}

/* Check if an entire string is digits. */
gboolean
isdigit_string(const guchar *str)
{
	guint pos;

	/* Loop until we reach the end of the string (a null) */
	for(pos = 0; str[pos] != '\0'; pos++){
		if(!g_ascii_isdigit(str[pos])){
			/* The string contains a non-digit character */
			return FALSE;
		}
	}

	/* The string contains only digits */
	return TRUE;
}

#define FORMAT_SIZE_UNIT_MASK 0x00ff
#define FORMAT_SIZE_PFX_MASK 0xff00

static const char *thousands_grouping_fmt = NULL;

DIAG_OFF(format)
static void test_printf_thousands_grouping(void) {
	/* test whether wmem_strbuf works with "'" flag character */
	wmem_strbuf_t *buf = wmem_strbuf_new(NULL, NULL);
	wmem_strbuf_append_printf(buf, "%'d", 22);
	if (g_strcmp0(wmem_strbuf_get_str(buf), "22") == 0) {
		thousands_grouping_fmt = "%'"G_GINT64_MODIFIER"d";
	} else {
		/* Don't use */
		thousands_grouping_fmt = "%"G_GINT64_MODIFIER"d";
	}
	wmem_strbuf_destroy(buf);
}
DIAG_ON(format)

/* Given a size, return its value in a human-readable format */
/* This doesn't handle fractional values. We might want to make size a double. */
gchar *
format_size_wmem(wmem_allocator_t *allocator, gint64 size, format_size_flags_e flags)
{
	wmem_strbuf_t *human_str = wmem_strbuf_new(allocator, NULL);
	int power = 1000;
	int pfx_off = 0;
	gboolean is_small = FALSE;
	static const gchar *prefix[] = {" T", " G", " M", " k", " Ti", " Gi", " Mi", " Ki"};
	gchar *ret_val;

	if (thousands_grouping_fmt == NULL)
		test_printf_thousands_grouping();

	if ((flags & FORMAT_SIZE_PFX_MASK) == format_size_prefix_iec) {
		pfx_off = 4;
		power = 1024;
	}

	if (size / power / power / power / power >= 10) {
		wmem_strbuf_append_printf(human_str, thousands_grouping_fmt, size / power / power / power / power);
		wmem_strbuf_append(human_str, prefix[pfx_off]);
	} else if (size / power / power / power >= 10) {
		wmem_strbuf_append_printf(human_str, thousands_grouping_fmt, size / power / power / power);
		wmem_strbuf_append(human_str, prefix[pfx_off+1]);
	} else if (size / power / power >= 10) {
		wmem_strbuf_append_printf(human_str, thousands_grouping_fmt, size / power / power);
		wmem_strbuf_append(human_str, prefix[pfx_off+2]);
	} else if (size / power >= 10) {
		wmem_strbuf_append_printf(human_str, thousands_grouping_fmt, size / power);
		wmem_strbuf_append(human_str, prefix[pfx_off+3]);
	} else {
		wmem_strbuf_append_printf(human_str, thousands_grouping_fmt, size);
		is_small = TRUE;
	}

	switch (flags & FORMAT_SIZE_UNIT_MASK) {
		case format_size_unit_none:
			break;
		case format_size_unit_bytes:
			wmem_strbuf_append(human_str, is_small ? " bytes" : "B");
			break;
		case format_size_unit_bits:
			wmem_strbuf_append(human_str, is_small ? " bits" : "b");
			break;
		case format_size_unit_bits_s:
			wmem_strbuf_append(human_str, is_small ? " bits/s" : "bps");
			break;
		case format_size_unit_bytes_s:
			wmem_strbuf_append(human_str, is_small ? " bytes/s" : "Bps");
			break;
		case format_size_unit_packets:
			wmem_strbuf_append(human_str, is_small ? " packets" : "packets");
			break;
		case format_size_unit_packets_s:
			wmem_strbuf_append(human_str, is_small ? " packets/s" : "packets/s");
			break;
		default:
			ws_assert_not_reached();
	}

	ret_val = wmem_strbuf_finalize(human_str);
	return g_strchomp(ret_val);
}

gchar
printable_char_or_period(gchar c)
{
	return g_ascii_isprint(c) ? c : '.';
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
