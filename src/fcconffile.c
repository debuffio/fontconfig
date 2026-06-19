/* Copyright (C) 2025 fontconfig Authors */
/* SPDX-License-Identifier: HPND */

#include "fcint.h"

#include <json-c/json.h>

static FcBool
FcConfigFileGenerateGenericAlias (FcConfig  *config,
                                  FcPattern *pat,
                                  FcPattern *font,
                                  FcStrBuf  *buf)
{
    FcChar8              *family = NULL, *file = NULL;
    const FcChar8        *gf, *s;
    int                   generic_family = FC_FAMILY_UNKNOWN;
    FcStrSet             *lset = NULL;
    FcStrList            *list = NULL;
    FcLangSet            *ls = NULL;
    FcBool                ret = FcTrue;

    if (FcPatternObjectGetString (font, FC_FILE_OBJECT, 0, &file) != FcResultMatch) {
	fprintf (stderr, "Fontconfig warning: no file object in the font metadata\n");
	return FcFalse;
    }
    if (FcPatternObjectGetString (font, FC_FAMILY_OBJECT, 0, &family) != FcResultMatch) {
	fprintf(stderr, "Fontconfig warning: %s: no family object in the font metadata\n", file);
	return FcFalse;
    }
    FcPatternObjectGetInteger (font, FC_GENERIC_FAMILY_OBJECT, 0, &generic_family);
    if (generic_family == FC_FAMILY_UNKNOWN) {
	/* fallback to the value that is given by users */
	FcPatternObjectGetInteger (pat, FC_GENERIC_FAMILY_OBJECT, 0, &generic_family);
    }
    FcPatternObjectGetLangSet (pat, FC_LANG_OBJECT, 0, &ls);
    gf = FcNameGetConstantNameFromObject (FC_GENERIC_FAMILY_OBJECT,
                                          generic_family);
    if (!gf) {
	fprintf (stderr, "Fontconfig warning: %s: Unable to determine generic family from either of font nor pattern\n", file);
	return FcFalse;
    }

    if (ls) {
        lset = FcLangSetGetLangs (ls);
        if (!lset)
	    return FcFalse;
        list = FcStrListCreate (lset);
        if (!list) {
            ret = FcFalse;
            goto bail;
	}
	if (lset->num == 0)
	    goto nolang;
        while ((s = FcStrListNext (list))) {
	    if (!FcStrBufFormat (buf,
	                         "  <match pattern=\"pattern\">\n"
	                         "    <test name=\"lang\" compare=\"contains\">\n"
	                         "      <string>%s</string>\n"
	                         "    </test>\n"
	                         "    <test name=\"genericfamily\">\n"
	                         "      <const>%s</const>\n"
	                         "    </test>\n"
	                         "    <edit name=\"family\" mode=\"prepend\">\n"
	                         "      <string>%s</string>\n"
	                         "    </edit>\n"
	                         "  </match>\n\n",
	                         s, gf, family)) {
		ret = FcFalse;
	        goto bail;
	    }
        }
	if (!FcStrBufFormat (buf,
	                     "  <alias>\n"
	                     "    <family>%s</family>\n"
	                     "    <default><family>%s</family></default>\n"
	                     "  </alias>\n",
	                     family, gf)) {
	    ret = FcFalse;
	    goto bail;
        }
    } else {
    nolang:
	if (!FcStrBufFormat (buf,
	                     "  <match target=\"pattern\">\n"
	                     "    <test name=\"family\">\n"
	                     "      <string>%s</string>\n"
	                     "    </test>\n"
	                     "    <edit name=\"family\" mode=\"prepend\">\n"
	                     "      <string>%s</string>\n"
	                     "    </edit>\n"
	                     "  </match>\n"
	                     "  <alias>\n"
	                     "    <family>%s</family>\n"
	                     "    <default><family>%s</family></default>\n"
	                     "  </alias>\n",
	                     gf, family, family, gf)) {
	    ret = FcFalse;
	    goto bail;
        }
    }
 bail:
    if (list)
	FcStrListDone(list);
    if (lset)
	FcStrSetDestroy (lset);

    return ret;
}

static FcBool
FcConfigFileGenerateGenericFamily (FcConfig  *config,
                                   FcPattern *pat,
                                   FcPattern *font,
                                   FcStrBuf  *buf)
{
    FcChar8              *family = NULL, *file = NULL;
    const FcChar8        *gf;
    int                   generic_family = FC_FAMILY_UNKNOWN;
    FcBool                ret = FcTrue;

    if (FcPatternObjectGetString (font, FC_FILE_OBJECT, 0, &file) != FcResultMatch) {
	fprintf (stderr, "Fontconfig warning: no file object in the font metadata\n");
	return FcFalse;
    }
    if (FcPatternObjectGetString (font, FC_FAMILY_OBJECT, 0, &family) != FcResultMatch) {
	fprintf(stderr, "Fontconfig warning: %s: no family object in the font metadata\n", file);
	return FcFalse;
    }
    FcPatternObjectGetInteger (font, FC_GENERIC_FAMILY_OBJECT, 0, &generic_family);
    if (generic_family == FC_FAMILY_UNKNOWN) {
	/* fallback to the value that is given by users */
	FcPatternObjectGetInteger (pat, FC_GENERIC_FAMILY_OBJECT, 0, &generic_family);
    }
    gf = FcNameGetConstantNameFromObject (FC_GENERIC_FAMILY_OBJECT,
                                          generic_family);
    if (!gf) {
	fprintf (stderr, "Fontconfig warning: %s: Unable to determine generic family from either of font nor pattern\n", file);
	return FcFalse;
    }

    if (!FcStrBufFormat (buf,
	                 "  <match target=\"scan\">\n"
	                 "    <test name=\"family\">\n"
	                 "      <string>%s</string>\n"
	                 "    </test>\n"
	                 "    <edit name=\"genericfamily\" mode=\"assign_replace\">\n"
	                 "      <const>%s</const>\n"
	                 "    </edit>\n"
	                 "  </match>\n",
	                 family, gf)) {
	ret = FcFalse;
	goto bail;
    }
 bail:

    return ret;
}

FcChar8 *
FcConfigFileGenerate (FcConfig      *config,
                      FcPattern     *pat,
                      const FcChar8 *font_path)
{
    FcFontSet   *fs = NULL;
    int          i;
    FcStrBuf     buf;

    FcStrBufInit (&buf, NULL, 0);
    fs = FcFontSetCreate();

    if (!FcFileIsDir (font_path)) {
        FcFileScan(fs, NULL, NULL, NULL, font_path, FcTrue);
    } else {
        FcStrSet *dirs = FcStrSetCreate();
        FcStrList *dirlist = FcStrListCreate (dirs);

        do {
	    FcDirScan (fs, dirs, NULL, NULL, font_path, FcTrue);
        } while ((font_path = FcStrListNext (dirlist)));

        FcStrListDone (dirlist);
        FcStrSetDestroy (dirs);
    }
    if (fs->nfont > 0) {
	FcHashTable *record = NULL;

        FcStrBufString (&buf,
                        (const FcChar8 *)"<?xml version=\"1.0\"?>\n"
	                                 "<!DOCTYPE fontconfig SYSTEM \"urn:fontconfig:fonts.dtd\">\n"
                                         "<fontconfig>\n");

	record = FcHashTableCreate ((FcHashFunc)FcStrHashIgnoreBlanksAndCase,
	                            (FcCompareFunc)FcStrCmpIgnoreBlanksAndCase,
	                            NULL,
	                            NULL,
	                            NULL,
	                            (FcDestroyFunc)FcPatternDestroy);
	for (i = 0; i < fs->nfont; i++) {
	    FcChar8   *family;
	    FcPattern *p = NULL;

	    p = fs->fonts[i];
	    if (FcPatternObjectGetString (fs->fonts[i],
	                                  FC_FAMILY_OBJECT,
	                                  0,
	                                  &family) != FcResultMatch) {
		continue;
	    }
	    if (!FcHashTableFind (record, family, (void **)&p)) {
		FcPatternReference (p);
		FcHashTableReplace (record, (void *)family, (void *)p);
		if (!FcConfigFileGenerateGenericFamily (config, pat, p, &buf)) {
		    FcStrBufDestroy (&buf);
		    FcStrBufInit (&buf, NULL, 0);
		    goto bail;
		}
		if (!FcConfigFileGenerateGenericAlias (config, pat, p, &buf)) {
		    FcStrBufDestroy (&buf);
		    FcStrBufInit (&buf, NULL, 0);
		    goto bail;
		}
	    }
	}
	FcHashTableDestroy (record);

        FcStrBufString (&buf, (const FcChar8 *)"</fontconfig>\n");
    }
 bail:
    FcFontSetDestroy (fs);

    return FcStrBufDone (&buf);
}

/* JSON to XML conversion support */

typedef struct {
    FcChar8 *family;
    FcChar8 *generic_family;
    FcChar8 *edit_mode;
} FcScanEntry;
typedef enum _FcPriority {
    FcPriorityPrefer,
    FcPriorityAccept,
    FcPriorityDefault
} FcPriority;

struct _FcConfigFile {
    FcHashTable *scan_table;
};

static void
_fc_scan_entry_destroy (void *data)
{
    FcScanEntry *entry = (FcScanEntry *)data;
    if (entry) {
	FcStrFree (entry->family);
	FcStrFree (entry->generic_family);
	FcStrFree (entry->edit_mode);
	free (entry);
    }
}

static FcChar8 *
_fc_convert_json_make_scan_key (const FcChar8 *family, const FcChar8 *generic_family)
{
    return FcStrDupFormat ("%s|%s", family, generic_family);
}

static FcBool
_fc_convert_json_add_to_scan_table (FcHashTable *scan_table,
                                    const char  *family,
                                    const char  *generic_family,
                                    const char  *edit_mode)
{
    FcChar8     *key;
    FcScanEntry *v = NULL;

    if (!scan_table)
	return FcFalse;

    key = _fc_convert_json_make_scan_key ((FcChar8 *)family, (FcChar8 *)generic_family);
    if (!key)
	return FcFalse;

    if (!FcHashTableFind (scan_table, key, (void **)&v)) {
	FcScanEntry *entry = malloc (sizeof (FcScanEntry));

	if (!entry)
	    goto bail;
	entry->family = FcStrCopy ((const FcChar8 *)family);
	entry->generic_family = FcStrCopy ((const FcChar8 *)generic_family);
	entry->edit_mode = edit_mode ? FcStrCopy ((const FcChar8 *)edit_mode) : FcStrCopy ((const FcChar8 *)"assign_replace");
	if (!entry->family ||
	    !entry->generic_family ||
	    !entry->edit_mode) {
	    _fc_scan_entry_destroy (entry);
	    goto bail;
	}

	if (!FcHashTableAdd (scan_table, key, entry)) {
	    _fc_scan_entry_destroy (entry);
	    goto bail;
	}
    } else {
	FcStrFree (key);
    }
    return FcTrue;
bail:
    FcStrFree (key);
    return FcFalse;
}

static void
_fc_convert_json_write_comment (FcStrBuf *buf, const char *comment)
{
    FcChar8 *s = FcStrEscapeXML ((const FcChar8 *)comment);
    FcStrBufFormat (buf, "<!--\n  %s\n -->\n", s);
    FcStrFree (s);
}

static void
_fc_convert_json_write_alias (FcStrBuf   *buf,
                              const char *family,
                              const char *generic_family,
                              const char *binding,
                              FcPriority  priority)
{
    FcChar8 *escaped = NULL;

    if (binding && strcmp (binding, "weak") != 0) {
	escaped = FcStrEscapeXML ((const FcChar8 *)binding);
	FcStrBufFormat (buf, "  <alias binding=\"%s\">\n", escaped);
	FcStrFree (escaped);
    } else {
	FcStrBufString (buf, (const FcChar8 *)"  <alias>\n");
    }
    escaped = FcStrEscapeXML ((const FcChar8 *)family);
    FcStrBufFormat (buf, "    <family>%s</family>\n", escaped);
    FcStrFree (escaped);

    escaped = FcStrEscapeXML ((const FcChar8 *)generic_family);
    switch (priority) {
    case FcPriorityPrefer:
	FcStrBufFormat (buf, "    <prefer><family>%s</family></prefer>\n", escaped);
	break;
    case FcPriorityAccept:
	FcStrBufFormat (buf, "    <accept><family>%s</family></accept>\n", escaped);
	break;
    case FcPriorityDefault:
	FcStrBufFormat (buf, "    <default><family>%s</family></default>\n", escaped);
	break;
    default:
	/* unlikely */
	fprintf (stderr, "Unknown priority in JSON\n");
    }
    FcStrFree (escaped);

    FcStrBufString (buf, (const FcChar8 *)"  </alias>\n");
}

static FcBool
_fc_convert_json_process_add_font_as_item (FcStrBuf           *buf,
                                           struct json_object *item,
                                           FcHashTable        *scan_table)
{
    struct json_object *generic_family_obj = NULL;
    struct json_object *binding_obj = NULL;
    struct json_object *priority_obj = NULL;
    struct json_object *comment_obj = NULL;
    struct json_object *edit_mode_obj = NULL;
    const char         *binding = "weak";
    const char         *priority = NULL;
    const char         *edit_mode = NULL;
    FcPriority          prior = FcPriorityDefault;

    if (!item)
	return FcTrue;

    if (json_object_object_get_ex (item, "$comment", &comment_obj)) {
	const char *comment = json_object_get_string (comment_obj);

	if (comment && strlen (comment) > 0)
	    _fc_convert_json_write_comment (buf, comment);
    }

    if (json_object_object_get_ex (item, "binding", &binding_obj)) {
	const char *s = json_object_get_string (binding_obj);
	if (s) {
	    if (strcmp (s, "weak") != 0 &&
	        strcmp (s, "strong") != 0 &&
	        strcmp (s, "same") != 0)
		return FcFalse;
	    binding = s;
	}
    }

    if (json_object_object_get_ex (item, "edit_mode_for_scan", &edit_mode_obj))
	edit_mode = json_object_get_string (edit_mode_obj);

    if (json_object_object_get_ex (item, "priority", &priority_obj)) {
	priority = json_object_get_string (priority_obj);

	if (!priority)
	    return FcFalse;
	else if (strcmp (priority, "default") == 0)
	    prior = FcPriorityDefault;
	else if (strcmp (priority, "prefer") == 0)
	    prior = FcPriorityPrefer;
	else if (strcmp (priority, "accept") == 0)
	    prior = FcPriorityAccept;
	else
	    return FcFalse;
    }

    if (!json_object_object_get_ex (item, "generic_family", &generic_family_obj))
	return FcFalse;

    json_object_object_foreach (generic_family_obj, generic_family, family_array)
    {
	if (!json_object_is_type (family_array, json_type_array))
	    continue;

	size_t n_families = json_object_array_length (family_array);
	if (n_families == 0)
	    continue;

	for (size_t i = 0; i < n_families; i++) {
	    struct json_object *family_obj = json_object_array_get_idx (family_array, i);
	    const char         *family_name = json_object_get_string (family_obj);

	    if (!family_name || strlen (family_name) == 0)
		continue;

	    if (!_fc_convert_json_add_to_scan_table (scan_table, family_name, generic_family, edit_mode)) {
		return FcFalse;
	    }
	}
    }

    FcStrBufString (buf, (const FcChar8 *)"\n");

    json_object_object_foreach (generic_family_obj, gf2, fam_array2)
    {
	FcChar8 *s;

	if (!json_object_is_type (fam_array2, json_type_array))
	    continue;

	size_t n_families2 = json_object_array_length (fam_array2);
	if (n_families2 == 0)
	    continue;

	s = FcStrEscapeXML ((const FcChar8 *)gf2);
	FcStrBufFormat (buf, "  <!--\n    %s faces\n   -->\n", s);
	FcStrFree (s);

	for (size_t j = 0; j < n_families2; j++) {
	    struct json_object *family_obj2 = json_object_array_get_idx (fam_array2, j);
	    const char         *family_name2 = json_object_get_string (family_obj2);

	    if (!family_name2 || strlen (family_name2) == 0)
		continue;

	    _fc_convert_json_write_alias (buf, family_name2, gf2, binding, prior);
	}
    }

    return FcTrue;
}

static FcBool
_fc_convert_json_process_add_font_for_item (FcStrBuf           *buf,
                                            struct json_object *item,
                                            FcHashTable        *scan_table)
{
    struct json_object *generic_family_obj = NULL;
    struct json_object *binding_obj = NULL;
    struct json_object *priority_obj = NULL;
    struct json_object *comment_obj = NULL;
    struct json_object *edit_mode_obj = NULL;
    const char         *binding = "weak";
    const char         *priority = NULL;
    const char         *edit_mode = NULL;
    FcPriority          prior = FcPriorityPrefer;

    if (!item)
	return FcTrue;

    if (json_object_object_get_ex (item, "$comment", &comment_obj)) {
	const char *comment = json_object_get_string (comment_obj);

	if (comment && strlen (comment) > 0)
	    _fc_convert_json_write_comment (buf, comment);
    }

    if (json_object_object_get_ex (item, "binding", &binding_obj)) {
	const char *s = json_object_get_string (binding_obj);

	if (s) {
	    if (strcmp (s, "weak") != 0 &&
	        strcmp (s, "strong") != 0 &&
	        strcmp (s, "same") != 0)
		return FcFalse;
	    binding = s;
	}
    }

    if (json_object_object_get_ex (item, "edit_mode_for_scan", &edit_mode_obj))
	edit_mode = json_object_get_string (edit_mode_obj);

    if (json_object_object_get_ex (item, "priority", &priority_obj)) {
	priority = json_object_get_string (priority_obj);

	if (!priority)
	    return FcFalse;
	else if (strcmp (priority, "default") == 0)
	    prior = FcPriorityDefault;
	else if (strcmp (priority, "prefer") == 0)
	    prior = FcPriorityPrefer;
	else if (strcmp (priority, "accept") == 0)
	    prior = FcPriorityAccept;
	else
	    return FcFalse;
    }

    if (!json_object_object_get_ex (item, "generic_family", &generic_family_obj))
	return FcFalse;

    json_object_object_foreach (generic_family_obj, generic_family, family_array)
    {
	size_t   i;
	FcChar8 *escaped_gf, *escaped_bind;

	if (!json_object_is_type (family_array, json_type_array))
	    continue;

	size_t n_families = json_object_array_length (family_array);
	if (n_families == 0)
	    continue;

	for (i = 0; i < n_families; i++) {
	    struct json_object *family_obj = json_object_array_get_idx (family_array, i);
	    const char         *family_name = json_object_get_string (family_obj);

	    if (!family_name || strlen (family_name) == 0)
		continue;

	    if (!_fc_convert_json_add_to_scan_table (scan_table, family_name, generic_family, edit_mode)) {
		return FcFalse;
	    }
	}

	escaped_gf = FcStrEscapeXML ((const FcChar8 *)generic_family);
	FcStrBufFormat (buf, "  <!--\n    %s faces\n   -->\n", escaped_gf);

	escaped_bind = FcStrEscapeXML ((const FcChar8 *)binding);
	if (binding && strcmp (binding, "weak") != 0) {
	    FcStrBufFormat (buf, "  <alias binding=\"%s\">\n", escaped_bind);
	} else {
	    FcStrBufString (buf, (const FcChar8 *)"  <alias>\n");
	}
	FcStrFree (escaped_bind);
	FcStrBufFormat (buf, "    <family>%s</family>\n", escaped_gf);
	FcStrFree (escaped_gf);

	switch (prior) {
	case FcPriorityPrefer:
	    FcStrBufString (buf, (const FcChar8 *)"    <prefer>\n");
	    break;
	case FcPriorityAccept:
	    FcStrBufString (buf, (const FcChar8 *)"    <accept>\n");
	    break;
	case FcPriorityDefault:
	    FcStrBufString (buf, (const FcChar8 *)"    <default>\n");
	    break;
	default:
	    /* unlikely */
	    return FcFalse;
	}

	for (i = 0; i < n_families; i++) {
	    struct json_object *family_obj = json_object_array_get_idx (family_array, i);
	    const char         *family_name = json_object_get_string (family_obj);
	    FcChar8            *s;

	    if (!family_name || strlen (family_name) == 0)
		continue;

	    s = FcStrEscapeXML ((const FcChar8 *)family_name);
	    FcStrBufFormat (buf, "      <family>%s</family>\n", s);
	    FcStrFree (s);
	}

	switch (prior) {
	case FcPriorityPrefer:
	    FcStrBufString (buf, (const FcChar8 *)"    </prefer>\n");
	    break;
	case FcPriorityAccept:
	    FcStrBufString (buf, (const FcChar8 *)"    </accept>\n");
	    break;
	case FcPriorityDefault:
	    FcStrBufString (buf, (const FcChar8 *)"    </default>\n");
	    break;
	default:
	    /* unlikely */
	    return FcFalse;
	}

	FcStrBufString (buf, (const FcChar8 *)"  </alias>\n");
    }

    return FcTrue;
}

/*
 * Public APIs
 */

FcConfigFile *
FcConfigFileCreate (void)
{
    FcConfigFile *file = malloc (sizeof (FcConfigFile));
    if (!file)
	return NULL;

    file->scan_table = FcHashTableCreate ((FcHashFunc)FcStrHashIgnoreBlanksAndCase,
                                          (FcCompareFunc)FcStrCmpIgnoreBlanksAndCase,
                                          NULL,
                                          NULL,
                                          (FcDestroyFunc)FcStrFree,
                                          (FcDestroyFunc)_fc_scan_entry_destroy);
    if (!file->scan_table) {
	free (file);
	return NULL;
    }

    return file;
}

void
FcConfigFileDestroy (FcConfigFile *file)
{
    if (file) {
	if (file->scan_table)
	    FcHashTableDestroy (file->scan_table);
	free (file);
    }
}

FcBool
FcConfigFileConvertJSONtoXML (FcConfigFile  *file,
                              const FcChar8 *json_file,
                              const FcChar8 *output_dir)
{
    struct json_object *root = NULL;
    struct json_object *desc_obj = NULL;
    struct json_object *comment_obj = NULL;
    struct json_object *add_font_as = NULL;
    struct json_object *add_font_for = NULL;
    FcChar8            *output_file = NULL;
    FcChar8            *base = NULL;
    FcStrBuf            buf;
    FcBool              ret = FcFalse;
    FILE               *fp = NULL;

    if (!file || !file->scan_table)
	return FcFalse;

    root = json_object_from_file ((const char *)json_file);
    if (!root)
	return FcFalse;

    FcStrBufInit (&buf, NULL, 0);

    base = FcStrBasename (json_file);
    if (base) {
	char    *dot = strrchr ((char *)base, '.');
	FcChar8 *conffile;

	if (dot && strcmp (dot, ".json") == 0)
	    *dot = '\0';
	conffile = FcStrPlus (base, (const FcChar8 *)".conf");
	if (!conffile)
	    goto cleanup;

	if (output_dir) {
	    output_file = FcStrBuildFilename (output_dir, conffile, NULL);
	} else {
	    output_file = FcStrCopy (conffile);
	}
	FcStrFree (conffile);
    } else {
	goto cleanup;
    }

    FcStrBufString (&buf, (const FcChar8 *)"<?xml version=\"1.0\"?>\n");
    FcStrBufString (&buf, (const FcChar8 *)"<!DOCTYPE fontconfig SYSTEM \"urn:fontconfig:fonts.dtd\">\n");
    FcStrBufString (&buf, (const FcChar8 *)"<fontconfig>\n");

    if (json_object_object_get_ex (root, "description", &desc_obj)) {
	const char *description = json_object_get_string (desc_obj);
	if (description && strlen (description) > 0) {
	    FcChar8 *s = FcStrEscapeXML ((const FcChar8 *)description);

	    FcStrBufFormat (&buf, "  <description>%s</description>\n", s);
	    FcStrFree (s);
	}
    }

    if (json_object_object_get_ex (root, "$comment", &comment_obj)) {
	const char *comment = json_object_get_string (comment_obj);
	if (comment && strlen (comment) > 0)
	    _fc_convert_json_write_comment (&buf, comment);
    }

    if (json_object_object_get_ex (root, "add_font_as", &add_font_as)) {
	if (json_object_is_type (add_font_as, json_type_array)) {
	    size_t n_items = json_object_array_length (add_font_as);
	    for (size_t i = 0; i < n_items; i++) {
		struct json_object *item = json_object_array_get_idx (add_font_as, i);
		if (!_fc_convert_json_process_add_font_as_item (&buf, item, file->scan_table))
		    goto cleanup;
	    }
	}
    }

    if (json_object_object_get_ex (root, "add_font_for", &add_font_for)) {
	if (json_object_is_type (add_font_for, json_type_array)) {
	    size_t n_items = json_object_array_length (add_font_for);
	    for (size_t i = 0; i < n_items; i++) {
		struct json_object *item = json_object_array_get_idx (add_font_for, i);
		if (!_fc_convert_json_process_add_font_for_item (&buf, item, file->scan_table))
		    goto cleanup;
	    }
	}
    }

    FcStrBufString (&buf, (const FcChar8 *)"\n</fontconfig>\n");

    if (!output_file)
	goto cleanup;
    fp = fopen ((const char *)output_file, "w");
    if (!fp)
	goto cleanup;

    FcChar8 *xml = FcStrBufDone (&buf);
    if (xml) {
	size_t flen = strlen ((const char *)xml);
	if (fwrite (xml, 1, flen, fp) == flen)
	    ret = FcTrue;
	FcStrFree (xml);
    }

cleanup:
    FcStrBufDestroy (&buf);
    if (fp)
	fclose (fp);
    if (root)
	json_object_put (root);
    FcStrFree (base);
    FcStrFree (output_file);

    return ret;
}

static FcBool
_fc_convert_json_write_scan_entry (FcHashTable *table,
                                   void        *key,
                                   void        *value,
                                   void        *data)
{
    FcStrBuf    *buf = (FcStrBuf *)data;
    FcScanEntry *entry = (FcScanEntry *)value;
    FcChar8     *f, *e, *g;

    f = FcStrEscapeXML (entry->family);
    e = FcStrEscapeXML (entry->edit_mode);
    g = FcStrEscapeXML (entry->generic_family);
    FcStrBufFormat (buf,
                    "  <match target=\"scan\">\n"
                    "    <test name=\"family\">\n"
                    "      <string>%s</string>\n"
                    "    </test>\n"
                    "    <edit name=\"genericfamily\" mode=\"%s\">\n"
                    "      <const>%s</const>\n"
                    "    </edit>\n"
                    "  </match>\n",
                    f, e, g);
    FcStrFree (f);
    FcStrFree (e);
    FcStrFree (g);

    return FcTrue;
}

FcBool
FcConfigFileWriteScanPattern (FcConfigFile  *file,
                              const FcChar8 *scan_file,
                              const FcChar8 *output_dir)
{
    FcChar8 *scan_path;
    FcStrBuf buf;
    FILE    *fp;
    FcBool   ret = FcFalse;

    if (!file || !file->scan_table || !scan_file)
	return FcFalse;

    if (output_dir) {
	scan_path = FcStrBuildFilename (output_dir, scan_file, NULL);
    } else {
	scan_path = FcStrCopy (scan_file);
    }
    if (!scan_path)
	return FcFalse;

    fp = fopen ((const char *)scan_path, "w");
    if (!fp) {
	FcStrFree (scan_path);
	return FcFalse;
    }

    FcStrBufInit (&buf, NULL, 0);

    FcStrBufString (&buf, (const FcChar8 *)"<?xml version=\"1.0\"?>\n");
    FcStrBufString (&buf, (const FcChar8 *)"<!DOCTYPE fontconfig SYSTEM \"urn:fontconfig:fonts.dtd\">\n");
    FcStrBufString (&buf, (const FcChar8 *)"<fontconfig>\n");
    FcStrBufString (&buf, (const FcChar8 *)"  <description>Set genericfamily for fonts</description>\n");
    FcStrBufString (&buf, (const FcChar8 *)"\n");

    FcHashTableForEach (file->scan_table, _fc_convert_json_write_scan_entry, &buf);

    FcStrBufString (&buf, (const FcChar8 *)"\n</fontconfig>\n");

    FcChar8 *xml = FcStrBufDone (&buf);
    if (xml) {
	size_t flen = strlen ((const char *)xml);
	if (fwrite (xml, 1, flen, fp) == flen)
	    ret = FcTrue;
	FcStrFree (xml);
    }

    fclose (fp);
    FcStrFree (scan_path);

    return ret;
}

#define __fcconffile__
#include "fcaliastail.h"
#include "fcftaliastail.h"
#undef __fcconffile__
