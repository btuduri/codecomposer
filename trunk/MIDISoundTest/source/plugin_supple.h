#ifndef _PLUGIN_SUPPLE_H
#define _PLUGIN_SUPPLE_H

static int msp_fopen(const char *fn);
static bool msp_fclose(int fh);
static char *GetINIData(void);
static int GetINISize(void);
static void *GetBINData(void);
static int GetBINSize(void);
static int GetBINFileHandle(void);

#endif
