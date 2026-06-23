/* Copyright (C) 2025 fontconfig Authors */
/* SPDX-License-Identifier: HPND */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <fontconfig/fontconfig.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif

#ifdef ENABLE_NLS
#  include <libintl.h>
#  define _(x) (dgettext (GETTEXT_PACKAGE, x))
#else
#  define dgettext(d, s) (s)
#  define _(x)           (x)
#endif

static void
usage (const char *program, int error)
{
    FILE *file = error ? stderr : stdout;
    fprintf (file, _("usage: %s [-h] [-o OUTPUT_DIR] [-s SCAN_FILE] INPUT...\n"), program);
    fprintf (file, _("Convert JSON config file(s) to XML format\n"));
    fprintf (file, "\n");
    fprintf (file, _("  -h               display this help and exit\n"));
    fprintf (file, _("  -o OUTPUT_DIR    write output files to OUTPUT_DIR (default: current dir)\n"));
    fprintf (file, _("  -s SCAN_FILE     write scan matches to SCAN_FILE (default: 80-scan.conf)\n"));
    fprintf (file, _("  INPUT...         input JSON file(s)\n"));
    exit (error);
}

int
main (int argc, char **argv)
{
    const FcChar8 *output_dir = NULL;
    const FcChar8 *scan_file = (const FcChar8 *)"80-scan.conf";
    FcConfigFile  *file = NULL;
    int            c, ret = 0;

    extern char *optarg;
    extern int   optind;

    while ((c = getopt (argc, argv, "ho:s:")) != -1) {
	switch (c) {
	case 'h':
	    usage (argv[0], 0);
	    break;
	case 'o':
	    output_dir = (const FcChar8 *)optarg;
	    break;
	case 's':
	    scan_file = (const FcChar8 *)optarg;
	    break;
	default:
	    usage (argv[0], 1);
	}
    }

    if (optind >= argc) {
	fprintf (stderr, "Error: Missing input file(s)\n");
	usage (argv[0], 1);
    }

    /* Create FcConfigFile */
    file = FcConfigFileCreate();
    if (!file) {
	fprintf (stderr, "Error: Failed to create FcConfigFile\n");
	return 1;
    }

    /* Process all input files */
    for (int i = optind; i < argc; i++) {
	if (!FcConfigFileConvertJSONtoXML (file,
	                                   (const FcChar8 *)argv[i],
	                                   output_dir)) {
	    fprintf (stderr, "Error: Failed to convert %s\n", argv[i]);
	    ret = 1;
	}
    }

    /* Write scan file */
    if (!FcConfigFileWriteScanPattern (file, scan_file, output_dir)) {
	fprintf (stderr, "Error: Failed to write scan file\n");
	ret = 1;
    }

    FcConfigFileDestroy (file);

    return ret;
}
