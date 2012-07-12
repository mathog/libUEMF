/**
 Example progam used for exercising the libUEMF functions.  
 Produces a single output file: test_libuemf.emf
 Single command line parameter, hexadecimal bit flag.
   1  Disable tests that block EMF import into PowerPoint (dotted lines)
   2  Enable tests that block EMF being displayed in Windows Preview (currently, GradientFill)
   Default is 0, neither option set.

 Compile with 
 
    gcc -g -O0 -o testbed -Wall -I. testbed.c uemf.c uemf_endian.c -lm 

 or

    gcc -g -O0 -o testbed -DU_VALGRIND -Wall -I. testbed.c uemf.c uemf_endian.c -lm 

 
 The latter from enables code which lets valgrind check each record for
 uninitialized data.
 
 Takes one 
*/

/*
File:      testbed.c
Version:   0.0.2
Date:      12-JUL-2012
Author:    David Mathog, Biology Division, Caltech
email:     mathog@caltech.edu
Copyright: 2012 David Mathog and California Institute of Technology (Caltech)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "uemf.h"

#define PPT_BLOCKERS     1
#define PREVIEW_BLOCKERS 2

// noa special conditions:
// 1 then s2 is expected to have zero in the "a" channel
// 2 then s2 is expected to have zero in the "a" channel AND only top 5 bits are meaningful
int rgba_diff(char *s1, char *s2, uint32_t size, int noa){
   for( ; size ; size--, s1++, s2++){
     if(noa==1){
       if(*s1 != *s2){
         if(noa && !(*s2) && (1 == size % 4))continue;
         return(1);
       }
     }
     if(noa==2){
       if((0xF8 & *s1) != (0xF8 &*s2)){
         if(noa && !(*s2) && (1 == size % 4))continue;
         return(1);
       }
     }
     else {
       if(*s1 != *s2)return(1);
     }
   }
   return(0);
}

void taf(char *rec,EMFTRACK *et, char *text){  // Test, append, free
    if(!rec){ printf("%s failed",text);                     }
    else {    printf("%s recsize: %d",text,U_EMRSIZE(rec)); }
    (void) emf_append((PU_ENHMETARECORD)rec, et, 1);
#ifndef U_VALGRIND
    printf("\n");
#endif
}



uint32_t  makeindex(int i, int j, int w, int h, uint32_t colortype){
uint32_t index = 0;
float scale;
   switch(colortype){
      case U_BCBM_MONOCHROME: //! 2 colors.    bmiColors array has two entries 
        scale=1;               
        break;           
      case U_BCBM_COLOR4:     //! 2^4 colors.  bmiColors array has 16 entries                 
        scale=15;               
        break;           
      case U_BCBM_COLOR8:     //! 2^8 colors.  bmiColors array has 256 entries                
        scale=255;               
        break;           
      case U_BCBM_COLOR16:    //! 2^16 colors. (Several different index methods)) 
        scale=65535;               
      break;           
      case U_BCBM_COLOR24:    //! 2^24 colors. bmiColors is not used. Pixels are U_RGBTRIPLE. 
      case U_BCBM_COLOR32:    //! 2^32 colors. bmiColors is not used. Pixels are U_RGBQUAD.
      case U_BCBM_EXPLICIT:   //! Derinved from JPG or PNG compressed image or ?   
      default:
        exit(EXIT_FAILURE);     
   }
   if(scale > w*h-1)scale=w*h-1; // color table will not have more entries than the size of the image
   index = U_ROUND(scale * ( (float) i * (float)w + (float)j ) / ((float)w * (float)h));
   return(index);
}


/* 
Fill a rectangular RGBA image with gradients.  Used for testing DIB operations.
*/
void FillImage(char *px, int w, int h, int stride){
int        i,j;
int        xp,xm,yp,ym;   // color weighting factors
int        r,g,b,a;
int        pad;
U_COLORREF color;

    pad = stride - w*4;  // end of row padding in bytes (may be zero)
    for(j=0; j<h; j++){
       yp = (255 * j) / (h-1);
       ym = 255 - yp;
       for(i=0; i<w; i++, px+=4){
          xp = (255 * i)/ (w-1);
          xm = 255 - xp;
          r = (xm > ym ? ym : xm);
          g = (xp > ym ? ym : xp);
          b = (xp > yp ? yp : xp);
          a = (xm > yp ? yp : xm);
          color = U_RGBA(r,g,b,a);
          memcpy(px,&color,4);
       }
       px += pad;
    }
}

void spintext(uint32_t x, uint32_t y, uint32_t textalign, uint32_t *font, EMFTRACK *et, EMFHANDLES *eht){
    char               *rec;
    char               *rec2;
    int                 i;
    char               *string;
    uint16_t            *FontName;
    uint16_t            *FontStyle;
    uint16_t            *text16;
    U_LOGFONT            lf;
    U_LOGFONT_PANOSE     elfw;
    int                  slen;
    uint32_t            *dx;
    
    
    rec = U_EMRSETTEXTALIGN_set(textalign);
    taf(rec,et,"U_EMRSETTEXTALIGN_set");
    string = (char *) malloc(32);
    FontName = U_Utf8ToUtf16le("Courier New", 0, NULL);  // Helvetica originally, but that does not work
    FontStyle = U_Utf8ToUtf16le("Normal", 0, NULL);
    for(i=0; i<3600; i+=300){
       rec = selectobject_set(U_DEVICE_DEFAULT_FONT, eht); // Release current font
       taf(rec,et,"selectobject_set");

       if(*font){
          rec  = deleteobject_set(font, eht);  // then delete it
          taf(rec,et,"deleteobject_set");
       }

       // set escapement and orientation in tenths of a degree counter clockwise rotation
       lf   = logfont_set( -30, 0, i, i, 
                         U_FW_NORMAL, U_FW_NOITALIC, U_FW_NOUNDERLINE, U_FW_NOSTRIKEOUT,
                         U_ANSI_CHARSET, U_OUT_DEFAULT_PRECIS, U_CLIP_DEFAULT_PRECIS, 
                         U_DEFAULT_QUALITY, U_DEFAULT_PITCH, FontName);
       elfw = logfont_panose_set(lf, FontName, FontStyle, 0, U_PAN_ALL1);  // U_PAN_ALL1 is all U_PAN_NO_FIT, this is what createfont() would have made
       rec  = extcreatefontindirectw_set(font, eht,  NULL, (char *) &elfw);
       taf(rec,et,"extcreatefontindirectw_set");
       rec = selectobject_set(*font, eht); // make this font active
       taf(rec,et,"selectobject_set");

       sprintf(string,"....Degrees:%d",i/10);
       text16 = U_Utf8ToUtf16le(string, 0, NULL);
       slen   = wchar16len(text16);
       dx = dx_set(-30,  U_FW_NORMAL, slen);
       rec2 = emrtext_set( pointl_set(x,y), slen, 2, text16, U_ETO_NONE, U_RCL_DEF, dx);
       free(text16);
       free(dx);
       rec = U_EMREXTTEXTOUTW_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,1.0,(PU_EMRTEXT)rec2); 
       taf(rec,et,"U_EMREXTTEXTOUTW_set");
       free(rec2);
    }
    free(FontName);
    free(FontStyle);
    free(string);
}
    
int main(int argc, char *argv[]){
    EMFTRACK            *et;
    EMFHANDLES          *eht;
    U_POINT              pl1;
    U_POINT              pl12[12];
    U_POINT              plarray[7]   = { { 200, 0 }, { 400, 200 }, { 400, 400 }, { 200, 600 } , { 0, 400 }, { 0, 200 }, { 200, 0 }};
    U_POINT16            plarray16[7] = { { 200, 0 }, { 400, 200 }, { 400, 400 }, { 200, 600 } , { 0, 400 }, { 0, 200 }, { 200, 0 }};
    U_POINT              star5[5]     = { { 100, 0 }, { 200, 300 }, { 0, 100 }, { 200, 100 } , { 0, 300 }};
    PU_POINT             points;
    PU_POINT16           point16;
    int                  status;
    char                *rec;
    char                *rec2;
    char                *string;
    char                *px;
    char                *rgba_px;
    char                *rgba_px2;
    U_RECTL              rclBounds,rclFrame,rclBox;
    U_SIZEL              szlDev,szlMm;
    uint16_t            *Description;
    uint16_t            *FontStyle;
    uint16_t            *FontName;
    uint16_t            *text16;
    uint32_t            *dx;
    size_t               slen;
    size_t               len;
    uint32_t             pen,brush,font;
    U_XFORM              xform;
    U_LOGBRUSH           lb;
    U_EXTLOGPEN         *elp;
    U_COLORREF           cr;
    U_POINTL             ul,lr;
    U_BITMAPINFOHEADER   Bmih;
    PU_BITMAPINFO        Bmi;
    U_LOGFONT            lf;
    U_LOGFONT_PANOSE     elfw;
    uint32_t             cbDesc;
    uint32_t             cbPx;
    uint32_t             colortype;
    PU_RGBQUAD           ct;         //color table
    int                  numCt;      //number of entries in the color table
    int                  i;

    int                  mode = 0;   // conditional for some tests
    
    if(argc<2){                                              mode = -1; }
    else if(argc==2){
       if(1 != sscanf(argv[1],"%X",&mode)){ mode = -1; }// note, undefined bits may be set without generating warnings
    }
    else {                                                   mode = -1; }
    if(mode<0){
      printf("Syntax: testbed flags\n");
      printf("   Flags is a hexadecimal number composed of the following optional bits (use 0 for a standard run):\n");
      printf("   1  Disable tests that block EMF import into PowerPoint (dotted lines)\n");
      printf("   2  Enable tests that block EMF being displayed in Windows Preview (currently, GradientFill)\n");
      exit(EXIT_FAILURE);
    }

    char     *step0 = U_strdup("Ever play telegraph?");
    uint16_t *step1 = U_Utf8ToUtf16le(     step0, 0, NULL);
    uint32_t *step2 = U_Utf16leToUtf32le(  step1, 0, NULL); 
    uint16_t *step3 = U_Utf32leToUtf16le(  step2, 0, NULL);  
    char     *step4 = U_Utf16leToUtf8(     step3, 0, NULL); 
    uint32_t *step5 = U_Utf8ToUtf32le(     step4, 0, NULL); 
    char     *step6 = U_Utf32leToUtf8(     step5, 0, &len); 
    printf("telegraph size: %d, string:<%s>\n",len,step6);
    free(step0);  
    free(step1);  
    free(step2);  
    free(step3);  
    free(step4);  
    free(step5);  
    free(step6);  

    step0 = U_strdup("Ever play telegraph?");
    step1 = U_Utf8ToUtf16le(     step0,   4, &len);
    step2 = U_Utf16leToUtf32le(  step1, len, &len); 
    step3 = U_Utf32leToUtf16le(  step2, len, &len); 
    step4 = U_Utf16leToUtf8(     step3, len, &len); 
    step5 = U_Utf8ToUtf32le(     step4,   4, &len); 
    step6 = U_Utf32leToUtf8(     step5, len, &len); 
    printf("telegraph size: %d, string (should be just \"Ever\"):<%s>\n",len,step6);
    free(step0);  
    free(step1);  
    free(step2);  
    free(step3);  
    free(step4);  
    free(step5);  
    free(step6);  
 
 
    /* ********************************************************************** */
    // set up and begin the EMF
 
    status=emf_start("test_libuemf.emf",1000000, 250000, &et);  // space allocation initial and increment 
    status=htable_create(128, 128, &eht);

    (void) device_size(216, 279, 47.244094, &szlDev, &szlMm); // Example: device is Letter vertical, 1200 dpi = 47.244 DPmm
    (void) drawing_size(297, 210, 47.244094, &rclBounds, &rclFrame);  // Example: drawing is A4 horizontal,  1200 dpi = 47.244 DPmm
    Description = U_Utf8ToUtf16le("Test EMF\1produced by libUEMF testbed program\1",0, NULL); 
    cbDesc = 2 + wchar16len(Description);  // also count the final terminator
    (void) U_Utf16leEdit(Description, U_Utf16le(1), 0);
    rec = U_EMRHEADER_set( rclBounds,  rclFrame,  NULL, cbDesc, Description, szlDev, szlMm, 0);
    taf(rec,et,"U_EMRHEADER_set");
    free(Description);

    rec = textcomment_set("First comment");
    taf(rec,et,"textcomment_set");

    rec = textcomment_set("Second comment");
    taf(rec,et,"textcomment_set");

    rec = U_EMRSETMAPMODE_set(U_MM_TEXT);
    taf(rec,et,"U_EMRSETMAPMODE_set");

    xform = xform_set(1, 0, 0, 1, 0, 0);
    rec = U_EMRMODIFYWORLDTRANSFORM_set(xform, U_MWT_LEFTMULTIPLY);
    taf(rec,et,"U_EMRMODIFYWORLDTRANSFORM_set");

    rec = U_EMRSETBKMODE_set(U_TRANSPARENT); //! iMode uses BackgroundMode Enumeration
    taf(rec,et,"U_EMRSETBKMODE_set");

    rec = U_EMRSETMITERLIMIT_set(8);
    taf(rec,et,"U_EMRSETMITERLIMIT_set");

    rec = U_EMRSETPOLYFILLMODE_set(U_WINDING); //! iMode uses PolygonFillMode Enumeration
    taf(rec,et,"U_EMRSETPOLYFILLMODE_set");


    cr = colorref_set(196, 127, 255);
    lb = logbrush_set(U_BS_SOLID, cr, U_HS_SOLIDCLR);
    rec = createbrushindirect_set(&brush, eht,lb);                 taf(rec,et,"createbrushindirect_set");
/* tested and works
    (void) htable_insert(&ih, eht);
    rec = U_EMRCREATEBRUSHINDIRECT_set(ih,lb);
    taf(rec,et,"U_EMRCREATEBRUSHINDIRECT_set");
*/

    rec = selectobject_set(brush, eht); // make brush which is at handle 1 active
    taf(rec,et,"selectobject_set");

    rec=U_EMRSETPOLYFILLMODE_set(U_WINDING);
    taf(rec,et,"U_EMRSETPOLYFILLMODE_set");

    cr = colorref_set(127, 255, 196);
    elp = extlogpen_set(U_PS_SOLID|U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER|U_PS_GEOMETRIC, 50, U_BS_SOLID, cr, U_HS_HORIZONTAL,0,NULL);
    rec = extcreatepen_set(&pen, eht,  NULL, 0, NULL, elp );        taf(rec,et,"emrextcreatepen_set");
    free(elp);

    rec = selectobject_set(pen, eht); // make pen just created active
    taf(rec,et,"selectobject_set");

    /* ********************************************************************** */
    // basic drawing operations

    ul     = pointl_set(100,1300);
    lr     = pointl_set(400,1600);
    rclBox = rectl_set(ul,lr);
    rec = U_EMRELLIPSE_set(rclBox);
    taf(rec,et,"U_EMRELLIPSE_set");

    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);
    taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");


    ul     = pointl_set(500,1300);
    lr     = pointl_set(800,1600);
    rclBox = rectl_set(ul,lr);
    rec = U_EMRRECTANGLE_set(rclBox);
    taf(rec,et,"U_EMRRECTANGLE_set");

    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);
    taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");


    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRMOVETOEX_set(pointl_set(900,1300));                taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRLINETO_set(pointl_set(1200,1500));                 taf(rec,et,"U_EMRLINETO_set");
    rec = U_EMRLINETO_set(pointl_set(1200,1700));                 taf(rec,et,"U_EMRLINETO_set");
    rec = U_EMRLINETO_set(pointl_set(900,1500));                  taf(rec,et,"U_EMRLINETO_set");
    rec = U_EMRCLOSEFIGURE_set();                                 taf(rec,et,"U_EMRCLOSEFIGURE_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");


    ul     = pointl_set(1300,1300);
    lr     = pointl_set(1600,1600);
    rclBox = rectl_set(ul,lr);
    rec = U_EMRARC_set(rclBox, ul, pointl_set(1450,1600));     taf(rec,et,"U_EMRARC_set");
 
    rec = U_EMRSETARCDIRECTION_set(U_AD_COUNTERCLOCKWISE);
    taf(rec,et,"U_SETARCDIRECTION_set");


    ul     = pointl_set(1600,1300);
    lr     = pointl_set(1900,1600);
    rclBox = rectl_set(ul,lr);
    // arcto initially draws a line from current position to its start.
    rec = U_EMRMOVETOEX_set(pointl_set(1800,1300));              taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRARCTO_set(rclBox, ul, pointl_set(1750,1600));     taf(rec,et,"U_EMRARCTO_set");
    rec = U_EMRLINETO_set(pointl_set(1750,1450));                taf(rec,et,"U_EMRLINETO_set");




    ul     = pointl_set(1900,1300);
    lr     = pointl_set(2200,1600);
    rclBox = rectl_set(ul,lr);
    rec = U_EMRCHORD_set(rclBox, ul,  pointl_set(2050,1600));   taf(rec,et,"U_EMRCHORD_set");

    rec = U_EMRSETARCDIRECTION_set(U_AD_CLOCKWISE);
    taf(rec,et,"U_SETARCDIRECTION_set");
    ul     = pointl_set(2200,1300);
    lr     = pointl_set(2500,1600);
    rclBox = rectl_set(ul,lr);
    rec = U_EMRPIE_set(rclBox, ul,  pointl_set(2350,1600));     taf(rec,et,"U_EMRPIE_set"); 



    rec = selectobject_set(U_BLACK_PEN, eht); // make stock object BLACK_PEN active
    taf(rec,et,"selectobject_set");

    rec = selectobject_set(U_GRAY_BRUSH, eht); // make stock object GREY_BRUSH active
    taf(rec,et,"selectobject_set");

    if(brush){
       rec = deleteobject_set(&brush, eht); // disable brush which is at handle 1, it should not be selected when this happens!!!
       taf(rec,et,"deleteobject_set");
    }

    if(pen){
       rec = deleteobject_set(&pen, eht); // disable pen which is at handle 2, it should not be selected when this happens!!!
       taf(rec,et,"deleteobject_set");
    }

    points = points_transform(star5, 5, xform_alt_set(1.0, 1.0, 0.0, 0.0, 3000, 1300));
    rec = U_EMRSETPOLYFILLMODE_set(U_WINDING);                    taf(rec,et,"U_EMRSETPOLYFILLMODE_set");
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYGON_set(findbounds(5, points, 0), 5, points);  taf(rec,et,"U_EMRPOLYGON_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    points = points_transform(star5, 5, xform_alt_set(1.0, 1.0, 0.0, 0.0, 3300, 1300));
    rec = U_EMRSETPOLYFILLMODE_set(U_ALTERNATE);                  taf(rec,et,"U_EMRSETPOLYFILLMODE_set");
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYGON_set(findbounds(5, points, 0), 5, points);  taf(rec,et,"U_EMRPOLYGON_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    // various types of draw

    cr = colorref_set(255, 0, 0);
    elp = extlogpen_set(U_PS_SOLID|U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER|U_PS_GEOMETRIC, 10, U_BS_SOLID, cr, U_HS_HORIZONTAL,0,NULL);
    rec = extcreatepen_set(&pen, eht,  NULL, 0, NULL, elp );       taf(rec,et,"emrextcreatepen_set");
    rec = selectobject_set(pen, eht);                              taf(rec,et,"selectobject_set");   // make this pen active
    free(elp);

    points = points_transform(plarray, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 100, 1800));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYBEZIER_set(U_RCL_DEF, 7, points);              taf(rec,et,"U_EMRPOLYBEZIER_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 600, 1800));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYGON_set(findbounds(6, points, 0), 6, points);  taf(rec,et,"U_EMRPOLYGON_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);
 
    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 1100, 1800));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYLINE_set(findbounds(6, points, 0), 6, points); taf(rec,et,"U_EMRPOLYLINE_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 1600, 1800));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRMOVETOEX_set(pointl_set(1600,1800));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYBEZIERTO_set(U_RCL_DEF, 6, points);            taf(rec,et,"U_EMRPOLYBEZIERTO_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 2100, 1800));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRMOVETOEX_set(pointl_set(2100,1800));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYLINETO_set(U_RCL_DEF, 6, points);              taf(rec,et,"U_EMRPOLYLINETO_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    rec = U_EMRABORTPATH_set();                                   taf(rec,et,"U_EMRABORTPATH_set");

    // same without begin/end path

    points = points_transform(plarray, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 3100, 1800));
    rec = U_EMRPOLYBEZIER_set(U_RCL_DEF, 7, points);              taf(rec,et,"U_EMRPOLYBEZIER_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 3600, 1800));
    rec = U_EMRPOLYGON_set(findbounds(6, points, 0), 6, points);  taf(rec,et,"U_EMRPOLYGON_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);
 
    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 4100, 1800));
    rec = U_EMRPOLYLINE_set(findbounds(6, points, 0), 6, points); taf(rec,et,"U_EMRPOLYLINE_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 4600, 1800));
    rec = U_EMRMOVETOEX_set(pointl_set(4600,1800));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYBEZIERTO_set(U_RCL_DEF, 6, points);            taf(rec,et,"U_EMRPOLYBEZIERTO_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 5100, 1800));
    rec = U_EMRMOVETOEX_set(pointl_set(5100,1800));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYLINETO_set(U_RCL_DEF, 6, points);              taf(rec,et,"U_EMRPOLYLINETO_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(points);

    // same without begin/end path or strokefillpath

    points = points_transform(plarray, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 6100, 1800));
    rec = U_EMRPOLYBEZIER_set(U_RCL_DEF, 7, points);              taf(rec,et,"U_EMRPOLYBEZIER_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 6600, 1800));
    rec = U_EMRPOLYGON_set(findbounds(6, points, 0), 6, points);  taf(rec,et,"U_EMRPOLYGON_set");
    free(points);
 
    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 7100, 1800));
    rec = U_EMRPOLYLINE_set(findbounds(6, points, 0), 6, points); taf(rec,et,"U_EMRPOLYLINE_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 7600, 1800));
    rec = U_EMRMOVETOEX_set(pointl_set(7600,1800));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYBEZIERTO_set(U_RCL_DEF, 6, points);            taf(rec,et,"U_EMRPOLYBEZIERTO_set");
    free(points);

    points = points_transform(plarray, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 8100, 1800));
    rec = U_EMRMOVETOEX_set(pointl_set(8100,1800));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYLINETO_set(U_RCL_DEF, 6, points);              taf(rec,et,"U_EMRPOLYLINETO_set");
    free(points);

    // 16 bit versions of the preceding
 

    point16 = point16_transform(plarray16, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 100, 2300));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYBEZIER16_set(U_RCL_DEF, 7, point16);           taf(rec,et,"U_EMRPOLYBEZIER16_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0,  600, 2300));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYGON16_set(findbounds16(6, point16, 0), 6, point16);  
                                                                  taf(rec,et,"U_EMRPOLYGON16_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);
 
    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 1100, 2300));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRPOLYLINE16_set(findbounds16(6, point16, 0), 6, point16); 
                                                                  taf(rec,et,"U_EMRPOLYLINE16_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 1600, 2300));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRMOVETOEX_set(pointl_set(1600,2300));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYBEZIERTO16_set(U_RCL_DEF, 6, point16);         taf(rec,et,"U_EMRPOLYBEZIERTO16_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 2100, 2300));
    rec = U_EMRBEGINPATH_set();                                   taf(rec,et,"U_EMRBEGINPATH_set");
    rec = U_EMRMOVETOEX_set(pointl_set(2100,2300));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYLINETO16_set(U_RCL_DEF, 6, point16);           taf(rec,et,"U_EMRPOLYLINETO16_set");
    rec = U_EMRENDPATH_set();                                     taf(rec,et,"U_EMRENDPATH_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);


    // same but without begin/end path

    point16 = point16_transform(plarray16, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 3100, 2300));
    rec = U_EMRPOLYBEZIER16_set(U_RCL_DEF, 7, point16);           taf(rec,et,"U_EMRPOLYBEZIER16_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 3600, 2300));
    rec = U_EMRPOLYGON16_set(findbounds16(6, point16, 0), 6, point16);  
                                                                  taf(rec,et,"U_EMRPOLYGON16_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);
 
    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 4100, 2300));
    rec = U_EMRPOLYLINE16_set(findbounds16(6, point16, 0), 6, point16); 
                                                                  taf(rec,et,"U_EMRPOLYLINE16_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 4600, 2300));
    rec = U_EMRMOVETOEX_set(pointl_set(4600,2300));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYBEZIERTO16_set(U_RCL_DEF, 6, point16);         taf(rec,et,"U_EMRPOLYBEZIERTO16_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 5100, 2300));
    rec = U_EMRMOVETOEX_set(pointl_set(5100,2300));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYLINETO16_set(U_RCL_DEF, 6, point16);           taf(rec,et,"U_EMRPOLYLINETO16_set");
    rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                   taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
    free(point16);

    // same but without begin/end path or strokeandfillpath

    point16 = point16_transform(plarray16, 7, xform_alt_set(0.5, 1.0, 0.0, 0.0, 6100, 2300));
    rec = U_EMRPOLYBEZIER16_set(U_RCL_DEF, 7, point16);           taf(rec,et,"U_EMRPOLYBEZIER16_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 6600, 2300));
    rec = U_EMRPOLYGON16_set(findbounds16(6, point16, 0), 6, point16);  
                                                                  taf(rec,et,"U_EMRPOLYGON16_set");
    free(point16);
 
    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 7100, 2300));
    rec = U_EMRPOLYLINE16_set(findbounds16(6, point16, 0), 6, point16); 
                                                                  taf(rec,et,"U_EMRPOLYLINE16_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 7600, 2300));
    rec = U_EMRMOVETOEX_set(pointl_set(7600,2300));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYBEZIERTO16_set(U_RCL_DEF, 6, point16);         taf(rec,et,"U_EMRPOLYBEZIERTO16_set");
    free(point16);

    point16 = point16_transform(plarray16, 6, xform_alt_set(0.5, 1.0, 0.0, 0.0, 8100, 2300));
    rec = U_EMRMOVETOEX_set(pointl_set(8100,2300));               taf(rec,et,"U_EMRMOVETOEX_set");
    rec = U_EMRPOLYLINETO16_set(U_RCL_DEF, 6, point16);           taf(rec,et,"U_EMRPOLYLINETO16_set");
    free(point16);


     /* ********************************************************************** */
    // test transform routines, first generate a circle of points

    for(i=0; i<12; i++){
       pl1     = point_set(1,0);
       points  = points_transform(&pl1,1,xform_alt_set(100.0, 1.0, (float)(i*30), 0.0, 0.0, 0.0));
       pl12[i] = *points;
       free(points);
    }
    pl12[0].x = U_ROUND((float)pl12[0].x) * 1.5; // make one points stick out a bit

    // test scale (range 1->2) and axis ratio (range 1->0)
    for(i=0; i<12; i++){
       points = points_transform(pl12, 12, xform_alt_set(1.0 + ((float)i)/11.0, ((float)(11-i))/11.0, 0.0, 30.0*((float)i), 200 + i*300, 2800));
       rec = U_EMRBEGINPATH_set();                                     taf(rec,et,"U_EMRBEGINPATH_set");
       rec = U_EMRPOLYGON_set(findbounds(12, points, 0), 12, points);  taf(rec,et,"U_EMRPOLYGON_set");
       rec = U_EMRENDPATH_set();                                       taf(rec,et,"U_EMRENDPATH_set");
       rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                     taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
       free(points);
    }

    // test scale (range 1->2) and axis ratio (range 1->0), rotate major axis to vertical (point lies on positive Y axis)
    for(i=0; i<12; i++){
       points = points_transform(pl12, 12, xform_alt_set(1.0 + ((float)i)/11.0, ((float)(11-i))/11.0, 90.0, 30.0*((float)i), 200 + i*300, 3300));
       rec = U_EMRBEGINPATH_set();                                     taf(rec,et,"U_EMRBEGINPATH_set");
       rec = U_EMRPOLYGON_set(findbounds(12, points, 0), 12, points);  taf(rec,et,"U_EMRPOLYGON_set");
       rec = U_EMRENDPATH_set();                                       taf(rec,et,"U_EMRENDPATH_set");
       rec = U_EMRSTROKEANDFILLPATH_set(rclFrame);                     taf(rec,et,"U_EMRSTROKEANDFILLPATH_set");
       free(points);
    }

    if(mode & PREVIEW_BLOCKERS){
    /*  gradientfill
        These appear to create the proper binary records, but XP cannot use them and they poison the EMF so that nothing shows.
    */
        
        PU_TRIVERTEX         tvs;
        U_TRIVERTEX          tvrect[2]    = {{0,0,0xFFFF,0,0,0}, {100,100,0,0xFFFF,0,0}};
        U_GRADIENT4          grrect[1]    = {{0,1}};
        U_TRIVERTEX          tvtrig[3]    = {{0,0,0xFFFF,0,0,0}, {50,100,0,0,0xFFFF,0}, {100,0,0,0xFFFF,0,0}};
        U_GRADIENT3          grtrig[1]    = {{0,1,2}};

        tvs = trivertex_transform(tvrect, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 1900, 1300));
        rec = U_EMRGRADIENTFILL_set(U_RCL_DEF, 2, 1, U_GRADIENT_FILL_RECT_H, tvs, (uint32_t *) grrect );
        taf(rec,et,"U_EMRGRADIENTFILL_set");
        free(tvs);

        tvs = trivertex_transform(tvrect, 2, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2200, 1300));
        rec = U_EMRGRADIENTFILL_set(U_RCL_DEF, 2, 1, U_GRADIENT_FILL_RECT_V, tvs, (uint32_t *) grrect );
        taf(rec,et,"U_EMRGRADIENTFILL_set");
        free(tvs);

        tvs = trivertex_transform(tvtrig, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, 2500, 1300));
        rec = U_EMRGRADIENTFILL_set(U_RCL_DEF, 3, 1, U_GRADIENT_FILL_TRIANGLE, tvs, (uint32_t *) grtrig );
        taf(rec,et,"U_EMRGRADIENTFILL_set");
        free(tvs);
    } // PREVIEW_BLOCKERS
    /* ********************************************************************** */
    // line types

 
    if(!(mode & PPT_BLOCKERS)){
       pl12[0] = point_set(0,   0  );
       pl12[1] = point_set(200, 200);
       pl12[2] = point_set(0,   400);
       pl12[3] = point_set(2,   1 ); // these next two are actually used as the pattern for U_PS_USERSTYLE
       pl12[4] = point_set(4,   3 ); // dash =2, gap=1, dash=4, gap =3
       for(i=0; i<=U_PS_DASHDOTDOT; i++){
          rec = selectobject_set(U_BLACK_PEN, eht); // make pen a stock object
          taf(rec,et,"deleteobject_set");
          if(pen){
             rec = deleteobject_set(&pen, eht); // delete current pen
             taf(rec,et,"deleteobject_set");
          }

          rec = createpen_set(&pen, eht,        
                   logpen_set(i, point_set(4+i,0),colorref_set(255, 0, 255))
   // all solid    logpen_set( U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER|i, point_set(1,1),colorref_set(255, 0, 255))
   // all solid    logpen_set( U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER|i, point_set(2+i,0),colorref_set(255, 0, 255))
   // all solid    logpen_set( U_PS_GEOMETRIC|U_PS_ENDCAP_SQUARE|U_PS_JOIN_MITER|i, point_set(2+i,0),colorref_set(255, 0, 255))
   // all solid    logpen_set( U_PS_GEOMETRIC | i, point_set(2+i,0),colorref_set(255, 0, 255))
   // all solid    logpen_set( U_PS_ENDCAP_SQUARE|i, point_set(2+i,0),colorref_set(255, 0, 255))
                );
          taf(rec,et,"createpen_set");

          rec = selectobject_set(pen, eht); // make pen just created active
          taf(rec,et,"selectobject_set");

          points = points_transform(pl12, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, 200 + i*100, 3700));
          rec = U_EMRPOLYLINE_set(findbounds(3, points, 0), 3, points); taf(rec,et,"U_EMRPOLYLINE_set");
          free(points);
       }
    } //PPT_BLOCKERS
    rec = selectobject_set(U_BLACK_PEN, eht); // make pen a stock object
    taf(rec,et,"selectobject_set");
    if(pen){
       rec = deleteobject_set(&pen, eht); // delete current pen
       taf(rec,et,"deleteobject_set");
    }

    // if width isn't 1 in the following then it is drawn solid, at width==1 anyway.
    elp = extlogpen_set(U_PS_USERSTYLE, 1, U_BS_SOLID, colorref_set(0, 0, 255), U_HS_HORIZONTAL,4,(uint32_t *)&(pl12[3]));
    rec = extcreatepen_set(&pen, eht,  NULL, 0, NULL, elp );        taf(rec,et,"emrextcreatepen_set");
    free(elp);

    rec = selectobject_set(pen, eht); // make pen just created active
    taf(rec,et,"selectobject_set");

    points = points_transform(pl12, 3, xform_alt_set(1.0, 1.0, 0.0, 0.0, 200 + i*100, 3700));
    rec = U_EMRPOLYLINE_set(findbounds(3, points, 0), 3, points); taf(rec,et,"U_EMRPOLYLINE_set");
    free(points);


    rec = selectobject_set(U_BLACK_PEN, eht); // make pen a stock object
    taf(rec,et,"selectobject_set");
    if(pen){
       rec = deleteobject_set(&pen, eht); // delete current pen
       taf(rec,et,"deleteobject_set");
    }


    /* ********************************************************************** */
    // bitmaps
    
    // Make the first test image, it is 10 x 10 and has various colors, R,G,B in each of 3 corners
    rgba_px = (char *) malloc(10*10*4);
    FillImage(rgba_px,10,10,40);



    rec =  U_EMRSETSTRETCHBLTMODE_set(U_STRETCH_DELETESCANS);
    taf(rec,et,"U_EMRSETSTRETCHBLTMODE_set");

    colortype = U_BCBM_COLOR32;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, 0, 1);
       //  Test the inverse operation - this does not affect the EMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 10, 10,    colortype,  0, 1);
    if(rgba_diff(rgba_px, rgba_px2, 10 * 10 * 4, 0))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR32\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(5000,5000),
       pointl_set(2000,2000),
       pointl_set(0,0), 
       pointl_set(10,10),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");

    // we are going to step on this one with little rectangles using different binary raster operations
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(2900,5000),
       pointl_set(2000,2000),
       pointl_set(0,0), 
       pointl_set(10,10),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");

    free(Bmi);
    free(px);



    colortype = U_BCBM_COLOR24;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, 0, 1);
       //  Test the inverse operation - this does not affect the EMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 10, 10,     colortype, 0, 1);
    if(rgba_diff(rgba_px, rgba_px2, 10 * 10 * 4, 1))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR24\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, NULL);
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(7020,5000),
       pointl_set(2000,2000),
       pointl_set(0,0), 
       pointl_set(10,10),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    free(Bmi);
    free(px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");
    // 16 bit, 5 bits per color, no table
    colortype = U_BCBM_COLOR16;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, 0, 1);
       //  Test the inverse operation - this does not affect the EMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 10, 10,     colortype, 0, 1);
    if(rgba_diff(rgba_px, rgba_px2, 10 * 10 * 4,2))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR16\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(9040,5000),
       pointl_set(2000,2000),
       pointl_set(0,0), 
       pointl_set(10,10),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");
       
    // write a second copy next to it using the negative height method to indicate it should be upside down
    Bmi->bmiHeader.biHeight *= -1;
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(11060,5000),
       pointl_set(2000,2000),
       pointl_set(0,0), 
       pointl_set(10,10),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");       
    free(Bmi);
    free(px);


    colortype = U_BCBM_COLOR8;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  10, 10, 40, colortype, 1, 0);
       //  Test the inverse operation - this does not affect the EMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 10, 10,     colortype, 1, 0);
    if(rgba_diff(rgba_px, rgba_px2, 10 * 10 * 4, 0))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR8\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(10, 10, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    free(ct);
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(5000,7020),
       pointl_set(2000,2000),
       pointl_set(0,0), 
       pointl_set(10,10),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    free(Bmi);
    free(px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");

    // done with the first test image, make the 2nd

    free(rgba_px); 
    rgba_px = (char *) malloc(4*4*4);
    FillImage(rgba_px,4,4,16);

    colortype = U_BCBM_COLOR4;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  4, 4, 16, colortype, 1, 0);
       //  Test the inverse operation - this does not affect the EMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 4, 4,     colortype, 1, 0);
    if(rgba_diff(rgba_px, rgba_px2, 4 * 4 * 4, 0))printf("Error in RGBA->DIB->RGBA for U_BCBM_COLOR4\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(4, 4, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    free(ct);
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(7020,7020),
       pointl_set(2000,2000),
       pointl_set(0,0), 
       pointl_set(4,4),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    free(Bmi);
    free(px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");

    // make a two color image in the existing RGBA array
    memset(rgba_px, 0x55, 4*4*4);
    memset(rgba_px, 0xAA, 4*4*2);

    colortype = U_BCBM_MONOCHROME;
    status = RGBA_to_DIB(&px, &cbPx, &ct, &numCt,  rgba_px,  4, 4, 16, colortype, 1, 0);
       //  Test the inverse operation - this does not affect the EMF contents
    status = DIB_to_RGBA( px,         ct,  numCt, &rgba_px2, 4, 4,     colortype, 1, 0);
    if(rgba_diff(rgba_px, rgba_px2, 4 * 4 * 4, 0))printf("Error in RGBA->DIB->RGBA for U_BCBM_MONOCHROME\n"); fflush(stdout);
    free(rgba_px2);
    Bmih = bitmapinfoheader_set(4, 4, 1, colortype, U_BI_RGB, 0, 47244, 47244, numCt, 0);
    Bmi = bitmapinfo_set(Bmih, ct);
    
    free(ct);
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(9040,7020),
       pointl_set(2000,2000),
       pointl_set(0,0), 
       pointl_set(4,4),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");
    // write a second copy next to it using the negative height method to indicate it should be upside down
    Bmi->bmiHeader.biHeight *= -1;
    rec = U_EMRSTRETCHDIBITS_set(
       U_RCL_DEF,
       pointl_set(11060,7020),
       pointl_set(2000,2000),
       pointl_set(0,0), 
       pointl_set(4,4),
       U_DIB_RGB_COLORS, 
       U_SRCCOPY,
       Bmi, 
       cbPx, 
       px);
    taf(rec,et,"U_EMRSTRETCHDIBITS_set");
       
    free(Bmi);
    free(px);

    free(rgba_px); 

    // Make another sacrificial image, this one all black
    rec =  U_EMRBITBLT_set(
      U_RCL_DEF,
      pointl_set(2900,7020),
      pointl_set(2000,2000),
      pointl_set(5000,5000),
      xform_set(1.0, 0.0, 0.0, 1.0, 0.0, 0.0),
      colorref_set(0,0,0),
      U_DIB_RGB_COLORS,
      U_DSTINVERT,
      NULL,
      0,
      NULL);
    taf(rec,et,"U_EMRBITBLT_set");

    // make a series of rectangle draws with white rectangles under various binary raster operations

    for(i=0;i<16;i++){
       rec = U_EMRSETROP2_set(i+1);
       taf(rec,et,"U_EMRSETROP2_set");
       ul     = pointl_set(2900 + i*100,5000);
       lr     = pointl_set(2900 + i*100+90,9019);
       rclBox = rectl_set(ul,lr);
       rec = U_EMRRECTANGLE_set(rclBox);
       taf(rec,et,"U_EMRRECTANGLE_set");
    }
    //restore the default
    rec = U_EMRSETROP2_set(U_R2_COPYPEN);
    taf(rec,et,"U_EMRSETROP2_set");



    FontName = U_Utf8ToUtf16le("Arial", 0, NULL);  // Helvetica originally, but that does not work
    lf   = logfont_set( -300, 0, 0, 0, 
                      U_FW_BOLD, U_FW_NOITALIC, U_FW_NOUNDERLINE, U_FW_NOSTRIKEOUT,
                      U_ANSI_CHARSET, U_OUT_DEFAULT_PRECIS, U_CLIP_DEFAULT_PRECIS, 
                      U_DEFAULT_QUALITY, U_DEFAULT_PITCH, FontName);
    FontStyle = U_Utf8ToUtf16le("Bold", 0, NULL);
    elfw = logfont_panose_set(lf, FontName, FontStyle, 0, U_PAN_ALL1);  // U_PAN_ALL1 is all U_PAN_NO_FIT, this is what createfont() would have made
    free(FontName);
    free(FontStyle);
    rec  = extcreatefontindirectw_set(&font, eht,  NULL, (char *) &elfw);
    taf(rec,et,"extcreatefontindirectw_set");

    rec  = selectobject_set(font, eht);
    taf(rec,et,"selectobject_set");

    rec = U_EMRSETTEXTCOLOR_set(colorref_set(255,0,0));
    taf(rec,et,"U_EMRSETTEXTCOLOR_set");

    // On XP this apparently ignores the active font UNLESS it is followed by a U_EMREXTTEXTOUTA/W.  Strange.
    // PowerPoint 2003 drops this on input from EMF in any case
    
    string = U_strdup("Text8 from U_EMRSMALLTEXTOUT");
    slen = strlen(string);
    rec = U_EMRSMALLTEXTOUT_set( pointl_set(100,50),  slen,  U_ETO_SMALL_CHARS,  
       U_GM_COMPATIBLE,  1.0, 1.0,  U_RCL_DEF, string);
    taf(rec,et,"U_EMRSMALLTEXTOUT_set");
    free(string);

    rec = U_EMRSETTEXTCOLOR_set(colorref_set(0, 255,0));
    taf(rec,et,"U_EMRSETTEXTCOLOR_set");

    text16 = U_Utf8ToUtf16le("Text16 from U_EMRSMALLTEXTOUT", 0, NULL);
    slen   = wchar16len(text16);
    rec = U_EMRSMALLTEXTOUT_set( pointl_set(100,350),  slen,  U_ETO_NONE,
       U_GM_COMPATIBLE,  1.0, 1.0,  U_RCL_DEF, (char *) text16);
    taf(rec,et,"U_EMRSMALLTEXTOUT_set");
    free(text16);

    rec = U_EMRSETTEXTCOLOR_set(colorref_set(0,0,255));
    taf(rec,et,"U_EMRSETTEXTCOLOR_set");

    string = U_strdup("Text8 from U_EMREXTTEXTOUTA");
    slen = strlen(string);
    dx = dx_set(-300,  U_FW_NORMAL, slen);
    rec2 = emrtext_set( pointl_set(100,650), slen, 1, string, U_ETO_NONE, (U_RECTL){0,0,-1,-1}, dx);
    rec = U_EMREXTTEXTOUTA_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,1.0,(PU_EMRTEXT)rec2); 
    taf(rec,et,"U_EMREXTTEXTOUTA_set");
    free(dx);
    free(rec2);
    free(string);

    rec = U_EMRSETTEXTCOLOR_set(colorref_set(0,0,0));
    taf(rec,et,"U_EMRSETTEXTCOLOR_set");

    text16 = U_Utf8ToUtf16le("Text16 from U_EMREXTTEXTOUTW", 0, NULL);
    slen   = wchar16len(text16);
    dx = dx_set(-300,  U_FW_NORMAL, slen);
    rec2 = emrtext_set( pointl_set(100,950), slen, 2, text16, U_ETO_NONE, U_RCL_DEF, dx);
    free(text16);
    rec = U_EMREXTTEXTOUTW_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,1.0,(PU_EMRTEXT)rec2); 
    taf(rec,et,"U_EMREXTTEXTOUTW_set");
    free(dx);
    free(rec2);

    /* ***************    test all text alignments  *************** */
    /* WNF documentation (section 2.1.2.3) says that the bounding rectangle should supply edges which are used as
       reference points.  This appears to not be true.  The following example draws two sets of aligned text,
       with the bounding rectangles indicated with a grey rectangle, using two different size bounding rectangles,
       and the text is in the same position in both cases.
       
       EMF documentation (section 2.2.5) says that the same rectangle is used for "clipping or opaquing" by ExtTextOutA/W.
       That does not seem to occur either.
    */
    
    // get rid of big font
    if(font){
       rec  = deleteobject_set(&font, eht);
       taf(rec,et,"deleteobject_set");
    }

    // Use a smaller font for text alignment tests.  Note that if the target system does
    // not have "Courier New" it will use some default font, which will not look right
    FontName = U_Utf8ToUtf16le("Courier New", 0, NULL);  // Helvetica originally, but that does not work
    lf   = logfont_set( -30, 0, 0, 0, 
                      U_FW_BOLD, U_FW_NOITALIC, U_FW_NOUNDERLINE, U_FW_NOSTRIKEOUT,
                      U_ANSI_CHARSET, U_OUT_DEFAULT_PRECIS, U_CLIP_DEFAULT_PRECIS, 
                      U_DEFAULT_QUALITY, U_DEFAULT_PITCH, FontName);
    FontStyle = U_Utf8ToUtf16le("Bold", 0, NULL);
    elfw = logfont_panose_set(lf, FontName, FontStyle, 0, U_PAN_ALL1);  // U_PAN_ALL1 is all U_PAN_NO_FIT, this is what createfont() would have made
    free(FontName);
    free(FontStyle);
    rec  = extcreatefontindirectw_set(&font, eht,  NULL, (char *) &elfw);
    taf(rec,et,"extcreatefontindirectw_set");
    rec  = selectobject_set(font, eht);
    taf(rec,et,"selectobject_set");

    // use NULL_PEN (no edge on drawn rectangle)
    rec = selectobject_set(U_NULL_PEN, eht); // make stock object NULL_PEN active
    taf(rec,et,"selectobject_set");
    // get rid of current pen, if defined
    if(pen){
        rec  = deleteobject_set(&pen, eht);
        taf(rec,et,"deleteobject_set");
     }

    text16 = U_Utf8ToUtf16le("textalignment:default", 0, NULL);
    slen   = wchar16len(text16);
    dx = dx_set(-30,  U_FW_NORMAL, slen);
    rec = U_EMRRECTANGLE_set(rectl_set(pointl_set(5500-20,100-20),pointl_set(5500+20,100+20)));
    taf(rec,et,"U_EMRRECTANGLE_set");
    rec2 = emrtext_set( pointl_set(5500,100), slen, 2, text16, U_ETO_NONE, U_RCL_DEF, dx);
    free(text16);
    rec = U_EMREXTTEXTOUTW_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,1.0,(PU_EMRTEXT)rec2); 
    taf(rec,et,"U_EMREXTTEXTOUTW_set");
    free(dx);
    free(rec2);

    string = (char *) malloc(32);
    for(i=0;i<=0x18; i+=2){
       rec = U_EMRSETTEXTALIGN_set(i);
       taf(rec,et,"U_EMRSETTEXTALIGN_set");
       sprintf(string,"textalignment:0x%2.2X",i);
       text16 = U_Utf8ToUtf16le(string, 0, NULL);
       slen   = wchar16len(text16);
       dx = dx_set(-30,  U_FW_NORMAL, slen);
       rec = U_EMRRECTANGLE_set(rectl_set(pointl_set(6000-20,100+ i*50-20),pointl_set(6000+20,100+ i*50+20)));
       taf(rec,et,"U_EMRRECTANGLE_set");
       rec2 = emrtext_set( pointl_set(6000,100+ i*50), slen, 2, text16, U_ETO_NONE, U_RCL_DEF, dx);
       free(text16);
       rec = U_EMREXTTEXTOUTW_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,1.0,(PU_EMRTEXT)rec2); 
       taf(rec,et,"U_EMREXTTEXTOUTW_set");
       free(dx);
       free(rec2);
    }
 
    for(i=0;i<=0x18; i+=2){
       rec = U_EMRSETTEXTALIGN_set(i);
       taf(rec,et,"U_EMRSETTEXTALIGN_set");
       sprintf(string,"textalignment:0x%2.2X",i);
       text16 = U_Utf8ToUtf16le(string, 0, NULL);
       slen   = wchar16len(text16);
       dx = dx_set(-30,  U_FW_NORMAL, slen);
       rec = U_EMRRECTANGLE_set(rectl_set(pointl_set(7000-40,100+ i*50-40),pointl_set(7000+40,100+ i*50+40)));
       taf(rec,et,"U_EMRRECTANGLE_set");
       rec2 = emrtext_set( pointl_set(7000,100+ i*50), slen, 2, text16, U_ETO_NONE, rectl_set(pointl_set(7000-40,100+ i*50-40),pointl_set(7000+40,100+ i*50+40)), dx);
       free(text16);
       rec = U_EMREXTTEXTOUTW_set(U_RCL_DEF,U_GM_COMPATIBLE,1.0,1.0,(PU_EMRTEXT)rec2); 
       taf(rec,et,"U_EMREXTTEXTOUTW_set");
       free(dx);
       free(rec2);
    }
    free(string);
    
    // restore the default text alignment
    rec = U_EMRSETTEXTALIGN_set(U_TA_DEFAULT);
    taf(rec,et,"U_EMRSETTEXTALIGN_set");
    
    /* ***************    test rotated text  *************** */
    

    spintext(8000, 300,U_TA_BASELINE,&font,et,eht);
    spintext(8600, 300,U_TA_TOP,     &font,et,eht);
    spintext(9200, 300,U_TA_BOTTOM,  &font,et,eht);
    rec = U_EMRSETTEXTALIGN_set(U_TA_BASELINE); // go back to baseline
    taf(rec,et,"U_EMRSETTEXTALIGN_set");


/* ************************************************* */
//  careful not to call anything after here twice!!! 

    rec = U_EMREOF_set(0,NULL,et);
    taf(rec,et,"U_EMREOF_set");

    /* Test the endian routines (on either Big or Little Endian machines).
    This must be done befoe the call to emf_finish, as that will swap the byte
    order of the EMF data before it writes it out on a BE machine.  */
    
#if 1
    string = (char *) malloc(et->used);
    if(!string){
       printf("Could not allocate enough memory to test u_emf_endian() function\n");
    }
    else {
       memcpy(string,et->buf,et->used);
       status = 0;
       if(!U_emf_endian(et->buf,et->used,1)){
          printf("Error in byte swapping of completed EMF, native -> reverse\n");
       }
       if(!U_emf_endian(et->buf,et->used,0)){
          printf("Error in byte swapping of completed EMF, reverse -> native\n");
       }
       if(rgba_diff(string, et->buf, et->used, 0)){ 
          printf("Error in u_emf_endian() function, round trip byte swapping does not match original\n");
       }
       free(string);
    }
#endif // swap testing

    status=emf_finish(et, eht);
    if(status){ printf("emf_finish failed: %d\n", status); }
    else {      printf("emf_finish sucess\n");             }

    emf_free(&et);
    htable_free(&eht);
/*
    status=htable_delete(uint32_t ih, EMFHANDLES *eht);
    status=htable_insert(uint32_t *ih, EMFHANDLES *eht);
*/

  exit(EXIT_SUCCESS);
}
