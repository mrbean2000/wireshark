/*
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
 *
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include "ftypes/ftypes.h"
#include "syntax-tree.h"
#include <epan/proto.h> // For BASE_NONE

static void
sttype_fvalue_free(gpointer value)
{
	fvalue_t *fvalue = value;

	/* If the data was not claimed with stnode_steal_data(), free it. */
	if (fvalue) {
		fvalue_free(fvalue);
	}
}

static void
pcre_free(gpointer value)
{
	ws_regex_t *pcre = value;

	/* If the data was not claimed with stnode_steal_data(), free it. */
	if (pcre) {
		/*
		 * They're reference-counted, so just drop the reference
		 * count; it'll get freed when the reference count drops
		 * to 0.
		 */
		ws_regex_free(pcre);
	}
}

static char *
sttype_fvalue_tostr(const void *data, gboolean pretty)
{
	const fvalue_t *fvalue = data;

	char *s, *repr;

	s = fvalue_to_string_repr(NULL, fvalue, FTREPR_DFILTER, BASE_NONE);
	if (pretty)
		repr = g_strdup(s);
	else
		repr = g_strdup_printf("%s[%s]", fvalue_type_name(fvalue), s);
	g_free(s);
	return repr;
}

static char *
field_tostr(const void *data, gboolean pretty _U_)
{
	const header_field_info *hfinfo = data;

	return g_strdup(hfinfo->abbrev);
}

static char *
pcre_tostr(const void *data, gboolean pretty _U_)
{
	return g_strdup(ws_regex_pattern(data));
}

void
sttype_register_pointer(void)
{
	static sttype_t field_type = {
		STTYPE_FIELD,
		"FIELD",
		NULL,
		NULL,
		NULL,
		field_tostr
	};
	static sttype_t fvalue_type = {
		STTYPE_FVALUE,
		"FVALUE",
		NULL,
		sttype_fvalue_free,
		NULL,
		sttype_fvalue_tostr
	};
	static sttype_t pcre_type = {
		STTYPE_PCRE,
		"PCRE",
		NULL,
		pcre_free,
		NULL,
		pcre_tostr
	};

	sttype_register(&field_type);
	sttype_register(&fvalue_type);
	sttype_register(&pcre_type);
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
