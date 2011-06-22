//========================================================================
//
// ImageOutputDev.cc
//
// Copyright 1998-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005, 2007 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2006 Rainer Keller <class321@gmx.de>
// Copyright (C) 2008 Timothy Lee <timothy.lee@siriushk.com>
// Copyright (C) 2008 Vasile Gaburici <gaburici@cs.umd.edu>
// Copyright (C) 2009 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2009 William Bader <williambader@hotmail.com>
// Copyright (C) 2010 Jakob Voss <jakob.voss@gbv.de>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include "config.h"
#include <poppler-config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include "goo/gmem.h"
#include "Error.h"
#include "GfxState.h"
#include "Object.h"
#include "Stream.h"
#ifdef ENABLE_LIBJPEG
#include "DCTStream.h"
#endif
#include "ImageOutputDev.h"

ImageOutputDev::ImageOutputDev(double phisical_width, PopplerImageMapping* mapping, char* path, bool* is_saved ) {
  fileName = path;
  this->mapping = mapping;
  this->phisical_width = phisical_width;
  this->is_saved = is_saved;
  *is_saved = false;
  dumpJPEG = true;
  imgNum = 0;
  pageNum = 0;
  ok = gTrue;
}

ImageOutputDev::~ImageOutputDev() {
}

void ImageOutputDev::setFilename(const char *fileExt) {
  strcpy( strchr(fileName, '\0'), fileExt);
}

void ImageOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
				   int width, int height, GBool invert,
				   GBool interpolate, GBool inlineImg) {
  FILE *f;
  int c;
  int size, i;
  double PPI = (mapping->area.x2 - mapping->area.x1) / phisical_width;

  // dump JPEG file
  if (dumpJPEG && 71.0 < PPI && PPI < 121.0 && str->getKind() == strDCT && !inlineImg ) {

    // open the image file
    setFilename(".jpg");
    ++imgNum;
    if (!(f = fopen(fileName, "wb"))) {
      error(-1, (char *)"Couldn't open image file '%s'", fileName);
      return;
    }

    // initialize stream
    str = ((DCTStream *)str)->getRawStream();
    str->reset();

    // copy the stream
    while ((c = str->getChar()) != EOF)
      fputc(c, f);

    str->close();
    fclose(f);
    *is_saved = true;

  // dump PBM file
  } else {

    // open the image file and write the PBM header
    setFilename(".pbm");
    ++imgNum;
    if (!(f = fopen(fileName, "wb"))) {
      error(-1, (char *)"Couldn't open image file '%s'", fileName);
      return;
    }
    fprintf(f, "P4\n");
    fprintf(f, "%d %d\n", width, height);

    // initialize stream
    str->reset();

    // copy the stream
    size = height * ((width + 7) / 8);
    for (i = 0; i < size; ++i) {
      fputc(str->getChar(), f);
    }

    str->close();
    fclose(f);
    *is_saved = true;
  }
}

void ImageOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
			       int width, int height,
			       GfxImageColorMap *colorMap,
			       GBool interpolate, int *maskColors, GBool inlineImg) {
  FILE *f;
  ImageStream *imgStr;
  Guchar *p;
  Guchar zero = 0;
  GfxGray gray;
  GfxRGB rgb;
  int x, y;
  int c;
  int size, i;
  int pbm_mask = 0xff;
  double PPI = (mapping->area.x2 - mapping->area.x1) / phisical_width;

  // dump JPEG file
  if (dumpJPEG && str->getKind() == strDCT &&
      71.0 < PPI && PPI < 121.0 &&
      (colorMap->getNumPixelComps() == 1 ||
       colorMap->getNumPixelComps() == 3) &&
      !inlineImg  ) {

    // open the image file
    setFilename(".jpg");
    ++imgNum;
    if (!(f = fopen(fileName, "wb"))) {
      error(-1, (char *)"Couldn't open image file '%s'", fileName);
      return;
    }

    // initialize stream
    str = ((DCTStream *)str)->getRawStream();
    str->reset();

    // copy the stream
    while ((c = str->getChar()) != EOF)
      fputc(c, f);

    str->close();
    fclose(f);
    *is_saved = true;

  // dump PBM file
  } else if (colorMap->getNumPixelComps() == 1 &&
	     colorMap->getBits() == 1) {

    // open the image file and write the PBM header
    setFilename(".pbm");
    ++imgNum;
    if (!(f = fopen(fileName, "wb"))) {
      error(-1, (char *)"Couldn't open image file '%s'", fileName);
      return;
    }
    fprintf(f, "P4\n");
    fprintf(f, "%d %d\n", width, height);

    // initialize stream
    str->reset();

    // if 0 comes out as 0 in the color map, the we _flip_ stream bits
    // otherwise we pass through stream bits unmolested
    colorMap->getGray(&zero, &gray);
    if(colToByte(gray))
      pbm_mask = 0;

    // copy the stream
    size = height * ((width + 7) / 8);
    for (i = 0; i < size; ++i) {
      fputc(str->getChar() ^ pbm_mask, f);
    }

    str->close();
    fclose(f);
    *is_saved = true;

  // dump PPM file
  }
}

void ImageOutputDev::drawMaskedImage(
  GfxState *state, Object *ref, Stream *str,
  int width, int height, GfxImageColorMap *colorMap, GBool interpolate,
  Stream *maskStr, int maskWidth, int maskHeight, GBool maskInvert, GBool maskInterpolate) {
  drawImage(state, ref, str, width, height, colorMap, interpolate, NULL, gFalse);
  drawImageMask(state, ref, maskStr, maskWidth, maskHeight, maskInvert,
		maskInterpolate, gFalse);
}

void ImageOutputDev::drawSoftMaskedImage(
  GfxState *state, Object *ref, Stream *str,
  int width, int height, GfxImageColorMap *colorMap, GBool interpolate,
  Stream *maskStr, int maskWidth, int maskHeight,
  GfxImageColorMap *maskColorMap, GBool maskInterpolate) {
  drawImage(state, ref, str, width, height, colorMap, interpolate, NULL, gFalse);
  drawImage(state, ref, maskStr, maskWidth, maskHeight,
	    maskColorMap, maskInterpolate, NULL, gFalse);
}
