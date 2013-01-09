/**
  @file uwmf_print.h Functions for printing records from WMF files.
*/

/*
File:      uwmf_print.h
Version:   0.0.1
Date:      19-DEC-2012
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2012 David Mathog and California Institute of Technology (Caltech)
*/

#ifndef _UWMF_PRINT_
#define _UWMF_PRINT_

#ifdef __cplusplus
extern "C" {
#endif
#include "uwmf.h"
#include "uemf_print.h"

/* prototypes for objects used in WMR records (other than those defined in uemf_print.h) */
void brush_print(U_BRUSH b);
void font_print(char *font);
void pltntry_print(U_PLTNTRY pny);
void palette_print(PU_PALETTE  p, char *PalEntries);
void pen_print(U_PEN p);
void rect16_ltrb_print(U_RECT16 rect);
void rect16_brtl_print(U_RECT16 rect);
void region_print(char *region);
void bitmap16_print(U_BITMAP16 b);
void bitmapcoreheader_print(U_BITMAPCOREHEADER ch);
void logbrushw_print(U_WLOGBRUSH lb);
void polypolygon_print(uint16_t  nPolys, uint16_t *aPolyCounts, char *Points);
void scan_print(U_SCAN sc);
void dibheader_print(void *dh);

/* prototypes for WMF records */
int  wmfheader_print(char *contents, char *blimit);
void U_WMRNOTIMPLEMENTED_print(char *contents);
int  U_wmf_onerec_print(char *contents, char *blimit, int recnum,  size_t  off);

#ifdef __cplusplus
}
#endif

#endif /* _UWMF_PRINT_ */
