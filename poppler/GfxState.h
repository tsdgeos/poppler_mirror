//========================================================================
//
// GfxState.h
//
// Copyright 1996-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005 Kristian Høgsberg <krh@redhat.com>
// Copyright (C) 2006, 2007 Jeff Muizelaar <jeff@infidigm.net>
// Copyright (C) 2006 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2009 Koji Otani <sho@bbr.jp>
// Copyright (C) 2009-2011, 2013, 2016-2018 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2010 Christian Feuersänger <cfeuersaenger@googlemail.com>
// Copyright (C) 2011 Andrea Canciani <ranma42@gmail.com>
// Copyright (C) 2011-2014, 2016 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
// Copyright (C) 2015, 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2017 Oliver Sander <oliver.sander@tu-dresden.de>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#ifndef GFXSTATE_H
#define GFXSTATE_H

#include "poppler-config.h"

#include "goo/gtypes.h"
#include "Object.h"
#include "Function.h"

#include <assert.h>
#include <map>

class Array;
class Gfx;
class GfxFont;
class PDFRectangle;
class GfxShading;
class GooList;
class OutputDev;
class GfxState;
class GfxResources;

class Matrix {
public:
  double m[6];

  void init(double xx, double yx, double xy, double yy, double x0, double y0) {
    m[0] = xx; m[1] = yx; m[2] = xy; m[3] = yy; m[4] = x0; m[5] = y0;
  }
  GBool invertTo(Matrix *other) const;
  void translate(double tx, double ty);
  void scale(double sx, double sy);
  void transform(double x, double y, double *tx, double *ty) const;
  double determinant() const { return m[0] * m[3] - m[1] * m[2]; }
  double norm() const;
};

//------------------------------------------------------------------------
// GfxBlendMode
//------------------------------------------------------------------------
 
enum GfxBlendMode {
  gfxBlendNormal,
  gfxBlendMultiply,
  gfxBlendScreen,
  gfxBlendOverlay,
  gfxBlendDarken,
  gfxBlendLighten,
  gfxBlendColorDodge,
  gfxBlendColorBurn,
  gfxBlendHardLight,
  gfxBlendSoftLight,
  gfxBlendDifference,
  gfxBlendExclusion,
  gfxBlendHue,
  gfxBlendSaturation,
  gfxBlendColor,
  gfxBlendLuminosity
};

//------------------------------------------------------------------------
// GfxColorComp
//------------------------------------------------------------------------

// 16.16 fixed point color component
typedef int GfxColorComp;

#define gfxColorComp1 0x10000

static inline GfxColorComp dblToCol(double x) {
  return (GfxColorComp)(x * gfxColorComp1);
}

static inline double colToDbl(GfxColorComp x) {
  return (double)x / (double)gfxColorComp1;
}

static inline Guchar dblToByte(double x) {
  return (x * 255.0);
}

static inline double byteToDbl(Guchar x) {
  return (double)x / (double)255.0;
}

static inline GfxColorComp byteToCol(Guchar x) {
  // (x / 255) << 16  =  (0.0000000100000001... * x) << 16
  //                  =  ((x << 8) + (x) + (x >> 8) + ...) << 16
  //                  =  (x << 8) + (x) + (x >> 7)
  //                                      [for rounding]
  return (GfxColorComp)((x << 8) + x + (x >> 7));
}

static inline Guchar colToByte(GfxColorComp x) {
  // 255 * x + 0.5  =  256 * x - x + 0x8000
  return (Guchar)(((x << 8) - x + 0x8000) >> 16);
}

static inline Gushort colToShort(GfxColorComp x) {
  return (Gushort)(x);
}

//------------------------------------------------------------------------
// GfxColor
//------------------------------------------------------------------------

#define gfxColorMaxComps funcMaxOutputs

struct GfxColor {
  GfxColorComp c[gfxColorMaxComps];
};

//------------------------------------------------------------------------
// GfxGray
//------------------------------------------------------------------------

typedef GfxColorComp GfxGray;

//------------------------------------------------------------------------
// GfxRGB
//------------------------------------------------------------------------

struct GfxRGB {
  GfxColorComp r, g, b;
};

//------------------------------------------------------------------------
// GfxCMYK
//------------------------------------------------------------------------

struct GfxCMYK {
  GfxColorComp c, m, y, k;
};

//------------------------------------------------------------------------
// GfxColorSpace
//------------------------------------------------------------------------

// NB: The nGfxColorSpaceModes constant and the gfxColorSpaceModeNames
// array defined in GfxState.cc must match this enum.
enum GfxColorSpaceMode {
  csDeviceGray,
  csCalGray,
  csDeviceRGB,
  csCalRGB,
  csDeviceCMYK,
  csLab,
  csICCBased,
  csIndexed,
  csSeparation,
  csDeviceN,
  csPattern
};

// wrapper of cmsHTRANSFORM to copy
class GfxColorTransform {
public:
  void doTransform(void *in, void *out, unsigned int size);
  // transformA should be a cmsHTRANSFORM
  GfxColorTransform(void *transformA, int cmsIntent, unsigned int inputPixelType, unsigned int transformPixelType);
  ~GfxColorTransform();
  GfxColorTransform(const GfxColorTransform &) = delete;
  GfxColorTransform& operator=(const GfxColorTransform &) = delete;
  int getIntent() const { return cmsIntent; }
  int getInputPixelType() const { return inputPixelType; }
  int getTransformPixelType() const { return transformPixelType; }
  void ref();
  unsigned int unref();
private:
  GfxColorTransform() {}
  void *transform;
  unsigned int refCount;
  int cmsIntent;
  unsigned int inputPixelType;
  unsigned int transformPixelType;
};

class GfxColorSpace {
public:

  GfxColorSpace();
  virtual ~GfxColorSpace();

  GfxColorSpace(const GfxColorSpace &) = delete;
  GfxColorSpace& operator=(const GfxColorSpace &other) = delete;

  virtual GfxColorSpace *copy() = 0;
  virtual GfxColorSpaceMode getMode() = 0;

  // Construct a color space.  Returns nullptr if unsuccessful.
  static GfxColorSpace *parse(GfxResources *res, Object *csObj, OutputDev *out, GfxState *state, int recursion = 0);

  // Convert to gray, RGB, or CMYK.
  virtual void getGray(const GfxColor *color, GfxGray *gray) const = 0;
  virtual void getRGB(const GfxColor *color, GfxRGB *rgb) const = 0;
  virtual void getCMYK(const GfxColor *color, GfxCMYK *cmyk) const = 0;
  virtual void getDeviceN(const GfxColor *color, GfxColor *deviceN) const = 0;
  virtual void getGrayLine(Guchar * /*in*/, Guchar * /*out*/, int /*length*/) { error(errInternal, -1, "GfxColorSpace::getGrayLine this should not happen"); }
  virtual void getRGBLine(Guchar * /*in*/, unsigned int * /*out*/, int /*length*/) { error(errInternal, -1, "GfxColorSpace::getRGBLine (first variant) this should not happen"); }
  virtual void getRGBLine(Guchar * /*in*/, Guchar * /*out*/, int /*length*/) {  error(errInternal, -1, "GfxColorSpace::getRGBLine (second variant) this should not happen"); }
  virtual void getRGBXLine(Guchar * /*in*/, Guchar * /*out*/, int /*length*/) {  error(errInternal, -1, "GfxColorSpace::getRGBXLine this should not happen"); }
  virtual void getCMYKLine(Guchar * /*in*/, Guchar * /*out*/, int /*length*/) {  error(errInternal, -1, "GfxColorSpace::getCMYKLine this should not happen"); }
  virtual void getDeviceNLine(Guchar * /*in*/, Guchar * /*out*/, int /*length*/) {  error(errInternal, -1, "GfxColorSpace::getDeviceNLine this should not happen"); }

  // create mapping for spot colorants
  virtual void createMapping(GooList *separationList, int maxSepComps);

  // Does this ColorSpace support getRGBLine?
  virtual GBool useGetRGBLine() const { return gFalse; }
  // Does this ColorSpace support getGrayLine?
  virtual GBool useGetGrayLine() const { return gFalse; }
  // Does this ColorSpace support getCMYKLine?
  virtual GBool useGetCMYKLine() const { return gFalse; }
  // Does this ColorSpace support getDeviceNLine?
  virtual GBool useGetDeviceNLine() const { return gFalse; }

  // Return the number of color components.
  virtual int getNComps() const = 0;

  // Get this color space's default color.
  virtual void getDefaultColor(GfxColor *color) = 0;

  // Return the default ranges for each component, assuming an image
  // with a max pixel value of <maxImgPixel>.
  virtual void getDefaultRanges(double *decodeLow, double *decodeRange,
				int maxImgPixel);

  // Returns true if painting operations in this color space never
  // mark the page (e.g., the "None" colorant).
  virtual GBool isNonMarking() const { return gFalse; }

  // Return the color space's overprint mask.
  Guint getOverprintMask() const { return overprintMask; }

  // Return the number of color space modes
  static int getNumColorSpaceModes();

  // Return the name of the <idx>th color space mode.
  static const char *getColorSpaceModeName(int idx);

#ifdef USE_CMS
  static int setupColorProfiles();
  // displayProfileA should be a cmsHPROFILE 
  static void setDisplayProfile(void *displayProfileA);
  static void setDisplayProfileName(GooString *name);
  // result will be a cmsHPROFILE 
  static void *getRGBProfile();
  // result will be a cmsHPROFILE 
  static void *getDisplayProfile();
#endif
protected:

  Guint overprintMask;
  int *mapping;
};

//------------------------------------------------------------------------
// GfxDeviceGrayColorSpace
//------------------------------------------------------------------------

class GfxDeviceGrayColorSpace: public GfxColorSpace {
public:

  GfxDeviceGrayColorSpace();
  ~GfxDeviceGrayColorSpace();
  GfxColorSpace *copy() override;
  GfxColorSpaceMode getMode() override { return csDeviceGray; }

  void getGray(const GfxColor *color, GfxGray *gray) const override;
  void getRGB(const GfxColor *color, GfxRGB *rgb) const override;
  void getCMYK(const GfxColor *color, GfxCMYK *cmyk) const override;
  void getDeviceN(const GfxColor *color, GfxColor *deviceN) const override;
  void getGrayLine(Guchar *in, Guchar *out, int length) override;
  void getRGBLine(Guchar *in, unsigned int *out, int length) override;
  void getRGBLine(Guchar *in, Guchar *out, int length) override;
  void getRGBXLine(Guchar *in, Guchar *out, int length) override;
  void getCMYKLine(Guchar *in, Guchar *out, int length) override;
  void getDeviceNLine(Guchar *in, Guchar *out, int length) override;

  GBool useGetRGBLine() const override { return gTrue; }
  GBool useGetGrayLine() const override { return gTrue; }
  GBool useGetCMYKLine() const override { return gTrue; }
  GBool useGetDeviceNLine() const override { return gTrue; }

  int getNComps() const override { return 1; }
  void getDefaultColor(GfxColor *color) override;

private:
};

//------------------------------------------------------------------------
// GfxCalGrayColorSpace
//------------------------------------------------------------------------

class GfxCalGrayColorSpace: public GfxColorSpace {
public:

  GfxCalGrayColorSpace();
  ~GfxCalGrayColorSpace();
  GfxColorSpace *copy() override;
  GfxColorSpaceMode getMode() override { return csCalGray; }

  // Construct a CalGray color space.  Returns nullptr if unsuccessful.
  static GfxColorSpace *parse(Array *arr, GfxState *state);

  void getGray(const GfxColor *color, GfxGray *gray) const override;
  void getRGB(const GfxColor *color, GfxRGB *rgb) const override;
  void getCMYK(const GfxColor *color, GfxCMYK *cmyk) const override;
  void getDeviceN(const GfxColor *color, GfxColor *deviceN) const override;

  int getNComps() const override { return 1; }
  void getDefaultColor(GfxColor *color) override;

  // CalGray-specific access.
  double getWhiteX() { return whiteX; }
  double getWhiteY() { return whiteY; }
  double getWhiteZ() { return whiteZ; }
  double getBlackX() { return blackX; }
  double getBlackY() { return blackY; }
  double getBlackZ() { return blackZ; }
  double getGamma() { return gamma; }

private:

  double whiteX, whiteY, whiteZ;    // white point
  double blackX, blackY, blackZ;    // black point
  double gamma;			    // gamma value
  double kr, kg, kb;		    // gamut mapping mulitpliers
  void getXYZ(const GfxColor *color, double *pX, double *pY, double *pZ) const;
#ifdef USE_CMS
  GfxColorTransform *transform;
#endif
};

//------------------------------------------------------------------------
// GfxDeviceRGBColorSpace
//------------------------------------------------------------------------

class GfxDeviceRGBColorSpace: public GfxColorSpace {
public:

  GfxDeviceRGBColorSpace();
  ~GfxDeviceRGBColorSpace();
  GfxColorSpace *copy() override;
  GfxColorSpaceMode getMode() override { return csDeviceRGB; }

  void getGray(const GfxColor *color, GfxGray *gray) const override;
  void getRGB(const GfxColor *color, GfxRGB *rgb) const override;
  void getCMYK(const GfxColor *color, GfxCMYK *cmyk) const override;
  void getDeviceN(const GfxColor *color, GfxColor *deviceN) const override;
  void getGrayLine(Guchar *in, Guchar *out, int length) override;
  void getRGBLine(Guchar *in, unsigned int *out, int length) override;
  void getRGBLine(Guchar *in, Guchar *out, int length) override;
  void getRGBXLine(Guchar *in, Guchar *out, int length) override;
  void getCMYKLine(Guchar *in, Guchar *out, int length) override;
  void getDeviceNLine(Guchar *in, Guchar *out, int length) override;

  GBool useGetRGBLine() const override { return gTrue; }
  GBool useGetGrayLine() const override { return gTrue; }
  GBool useGetCMYKLine() const override { return gTrue; }
  GBool useGetDeviceNLine() const override { return gTrue; }

  int getNComps() const override { return 3; }
  void getDefaultColor(GfxColor *color) override;

private:
};

//------------------------------------------------------------------------
// GfxCalRGBColorSpace
//------------------------------------------------------------------------

class GfxCalRGBColorSpace: public GfxColorSpace {
public:

  GfxCalRGBColorSpace();
  ~GfxCalRGBColorSpace();
  GfxColorSpace *copy() override;
  GfxColorSpaceMode getMode() override { return csCalRGB; }

  // Construct a CalRGB color space.  Returns nullptr if unsuccessful.
  static GfxColorSpace *parse(Array *arr, GfxState *state);

  void getGray(const GfxColor *color, GfxGray *gray) const override;
  void getRGB(const GfxColor *color, GfxRGB *rgb) const override;
  void getCMYK(const GfxColor *color, GfxCMYK *cmyk) const override;
  void getDeviceN(const GfxColor *color, GfxColor *deviceN) const override;

  int getNComps() const override { return 3; }
  void getDefaultColor(GfxColor *color) override;

  // CalRGB-specific access.
  double getWhiteX() const { return whiteX; }
  double getWhiteY() const { return whiteY; }
  double getWhiteZ() const { return whiteZ; }
  double getBlackX() const { return blackX; }
  double getBlackY() const { return blackY; }
  double getBlackZ() const { return blackZ; }
  double getGammaR() const { return gammaR; }
  double getGammaG() const { return gammaG; }
  double getGammaB() const { return gammaB; }
  const double *getMatrix() const { return mat; }

private:

  double whiteX, whiteY, whiteZ;    // white point
  double blackX, blackY, blackZ;    // black point
  double gammaR, gammaG, gammaB;    // gamma values
  double mat[9];		    // ABC -> XYZ transform matrix
  double kr, kg, kb;		    // gamut mapping mulitpliers
  void getXYZ(const GfxColor *color, double *pX, double *pY, double *pZ) const;
#ifdef USE_CMS
  GfxColorTransform *transform;
#endif
};

//------------------------------------------------------------------------
// GfxDeviceCMYKColorSpace
//------------------------------------------------------------------------

class GfxDeviceCMYKColorSpace: public GfxColorSpace {
public:

  GfxDeviceCMYKColorSpace();
  ~GfxDeviceCMYKColorSpace();
  GfxColorSpace *copy() override;
  GfxColorSpaceMode getMode() override { return csDeviceCMYK; }

  void getGray(const GfxColor *color, GfxGray *gray) const override;
  void getRGB(const GfxColor *color, GfxRGB *rgb) const override;
  void getCMYK(const GfxColor *color, GfxCMYK *cmyk) const override;
  void getDeviceN(const GfxColor *color, GfxColor *deviceN) const override;
  void getRGBLine(Guchar *in, unsigned int *out, int length) override;
  void getRGBLine(Guchar *, Guchar *out, int length) override;
  void getRGBXLine(Guchar *in, Guchar *out, int length) override;
  void getCMYKLine(Guchar *in, Guchar *out, int length) override;
  void getDeviceNLine(Guchar *in, Guchar *out, int length) override;
  GBool useGetRGBLine() const override { return gTrue; }
  GBool useGetCMYKLine() const override { return gTrue; }
  GBool useGetDeviceNLine() const override { return gTrue; }

  int getNComps() const override { return 4; }
  void getDefaultColor(GfxColor *color) override;

private:
};

//------------------------------------------------------------------------
// GfxLabColorSpace
//------------------------------------------------------------------------

class GfxLabColorSpace: public GfxColorSpace {
public:

  GfxLabColorSpace();
  ~GfxLabColorSpace();
  GfxColorSpace *copy() override;
  GfxColorSpaceMode getMode() override { return csLab; }

  // Construct a Lab color space.  Returns nullptr if unsuccessful.
  static GfxColorSpace *parse(Array *arr, GfxState *state);

  void getGray(const GfxColor *color, GfxGray *gray) const override;
  void getRGB(const GfxColor *color, GfxRGB *rgb) const override;
  void getCMYK(const GfxColor *color, GfxCMYK *cmyk) const override;
  void getDeviceN(const GfxColor *color, GfxColor *deviceN) const override;

  int getNComps() const override { return 3; }
  void getDefaultColor(GfxColor *color) override;

  void getDefaultRanges(double *decodeLow, double *decodeRange,
				int maxImgPixel) override;

  // Lab-specific access.
  double getWhiteX() { return whiteX; }
  double getWhiteY() { return whiteY; }
  double getWhiteZ() { return whiteZ; }
  double getBlackX() { return blackX; }
  double getBlackY() { return blackY; }
  double getBlackZ() { return blackZ; }
  double getAMin() { return aMin; }
  double getAMax() { return aMax; }
  double getBMin() { return bMin; }
  double getBMax() { return bMax; }

private:

  double whiteX, whiteY, whiteZ;    // white point
  double blackX, blackY, blackZ;    // black point
  double aMin, aMax, bMin, bMax;    // range for the a and b components
  double kr, kg, kb;		    // gamut mapping mulitpliers
  void getXYZ(const GfxColor *color, double *pX, double *pY, double *pZ) const;
#ifdef USE_CMS
  GfxColorTransform *transform;
#endif
};

//------------------------------------------------------------------------
// GfxICCBasedColorSpace
//------------------------------------------------------------------------

class GfxICCBasedColorSpace: public GfxColorSpace {
public:

  GfxICCBasedColorSpace(int nCompsA, GfxColorSpace *altA,
			Ref *iccProfileStreamA);
  ~GfxICCBasedColorSpace();
  GfxColorSpace *copy() override;
  GfxColorSpaceMode getMode() override { return csICCBased; }

  // Construct an ICCBased color space.  Returns nullptr if unsuccessful.
  static GfxColorSpace *parse(Array *arr, OutputDev *out, GfxState *state, int recursion);

  void getGray(const GfxColor *color, GfxGray *gray) const override;
  void getRGB(const GfxColor *color, GfxRGB *rgb) const override;
  void getCMYK(const GfxColor *color, GfxCMYK *cmyk) const override;
  void getDeviceN(const GfxColor *color, GfxColor *deviceN) const override;
  void getRGBLine(Guchar *in, unsigned int *out, int length) override;
  void getRGBLine(Guchar *in, Guchar *out, int length) override;
  void getRGBXLine(Guchar *in, Guchar *out, int length) override;
  void getCMYKLine(Guchar *in, Guchar *out, int length) override;
  void getDeviceNLine(Guchar *in, Guchar *out, int length) override;

  GBool useGetRGBLine() const override;
  GBool useGetCMYKLine() const override;
  GBool useGetDeviceNLine() const override;

  int getNComps() const override { return nComps; }
  void getDefaultColor(GfxColor *color) override;

  void getDefaultRanges(double *decodeLow, double *decodeRange,
				int maxImgPixel) override;

  // ICCBased-specific access.
  GfxColorSpace *getAlt() { return alt; }

private:

  int nComps;			// number of color components (1, 3, or 4)
  GfxColorSpace *alt;		// alternate color space
  double rangeMin[4];		// min values for each component
  double rangeMax[4];		// max values for each component
  Ref iccProfileStream;		// the ICC profile
#ifdef USE_CMS
  int getIntent() { return (transform != nullptr) ? transform->getIntent() : 0; }
  GfxColorTransform *transform;
  GfxColorTransform *lineTransform; // color transform for line
  mutable std::map<unsigned int, unsigned int> cmsCache;
#endif
};
//------------------------------------------------------------------------
// GfxIndexedColorSpace
//------------------------------------------------------------------------

class GfxIndexedColorSpace: public GfxColorSpace {
public:

  GfxIndexedColorSpace(GfxColorSpace *baseA, int indexHighA);
  ~GfxIndexedColorSpace();
  GfxColorSpace *copy() override;
  GfxColorSpaceMode getMode() override { return csIndexed; }

  // Construct an Indexed color space.  Returns nullptr if unsuccessful.
  static GfxColorSpace *parse(GfxResources *res, Array *arr, OutputDev *out, GfxState *state, int recursion);

  void getGray(const GfxColor *color, GfxGray *gray) const override;
  void getRGB(const GfxColor *color, GfxRGB *rgb) const override;
  void getCMYK(const GfxColor *color, GfxCMYK *cmyk) const override;
  void getDeviceN(const GfxColor *color, GfxColor *deviceN) const override;
  void getRGBLine(Guchar *in, unsigned int *out, int length) override;
  void getRGBLine(Guchar *in, Guchar *out, int length) override;
  void getRGBXLine(Guchar *in, Guchar *out, int length) override;
  void getCMYKLine(Guchar *in, Guchar *out, int length) override;
  void getDeviceNLine(Guchar *in, Guchar *out, int length) override;

  GBool useGetRGBLine() const override { return gTrue; }
  GBool useGetCMYKLine() const override { return gTrue; }
  GBool useGetDeviceNLine() const override { return gTrue; }

  int getNComps() const override { return 1; }
  void getDefaultColor(GfxColor *color) override;

  void getDefaultRanges(double *decodeLow, double *decodeRange,
				int maxImgPixel) override;

  // Indexed-specific access.
  GfxColorSpace *getBase() { return base; }
  int getIndexHigh() { return indexHigh; }
  Guchar *getLookup() { return lookup; }
  GfxColor *mapColorToBase(const GfxColor *color, GfxColor *baseColor) const;
  Guint getOverprintMask() { return base->getOverprintMask(); }
  void createMapping(GooList *separationList, int maxSepComps) override
    { base->createMapping(separationList, maxSepComps); }


private:

  GfxColorSpace *base;		// base color space
  int indexHigh;		// max pixel value
  Guchar *lookup;		// lookup table
};

//------------------------------------------------------------------------
// GfxSeparationColorSpace
//------------------------------------------------------------------------

class GfxSeparationColorSpace: public GfxColorSpace {
public:

  GfxSeparationColorSpace(GooString *nameA, GfxColorSpace *altA,
			  Function *funcA);
  ~GfxSeparationColorSpace();
  GfxColorSpace *copy() override;
  GfxColorSpaceMode getMode() override { return csSeparation; }

  // Construct a Separation color space.  Returns nullptr if unsuccessful.
  static GfxColorSpace *parse(GfxResources *res, Array *arr, OutputDev *out, GfxState *state, int recursion);

  void getGray(const GfxColor *color, GfxGray *gray) const override;
  void getRGB(const GfxColor *color, GfxRGB *rgb) const override;
  void getCMYK(const GfxColor *color, GfxCMYK *cmyk) const override;
  void getDeviceN(const GfxColor *color, GfxColor *deviceN) const override;

  void createMapping(GooList *separationList, int maxSepComps) override;

  int getNComps() const override { return 1; }
  void getDefaultColor(GfxColor *color) override;

  GBool isNonMarking() const override { return nonMarking; }

  // Separation-specific access.
  GooString *getName() { return name; }
  GfxColorSpace *getAlt() { return alt; }
  const Function *getFunc() const { return func; }

private:

  GfxSeparationColorSpace(GooString *nameA, GfxColorSpace *altA,
			  Function *funcA, GBool nonMarkingA,
			  Guint overprintMaskA, int *mappingA);

  GooString *name;		// colorant name
  GfxColorSpace *alt;		// alternate color space
  Function *func;		// tint transform (into alternate color space)
  GBool nonMarking;
};

//------------------------------------------------------------------------
// GfxDeviceNColorSpace
//------------------------------------------------------------------------

class GfxDeviceNColorSpace: public GfxColorSpace {
public:

  GfxDeviceNColorSpace(int nCompsA, GooString **namesA,
		       GfxColorSpace *alt, Function *func, GooList *sepsCS);
  ~GfxDeviceNColorSpace();
  GfxColorSpace *copy() override;
  GfxColorSpaceMode getMode() override { return csDeviceN; }

  // Construct a DeviceN color space.  Returns nullptr if unsuccessful.
  static GfxColorSpace *parse(GfxResources *res, Array *arr, OutputDev *out, GfxState *state, int recursion);

  void getGray(const GfxColor *color, GfxGray *gray) const override;
  void getRGB(const GfxColor *color, GfxRGB *rgb) const override;
  void getCMYK(const GfxColor *color, GfxCMYK *cmyk) const override;
  void getDeviceN(const GfxColor *color, GfxColor *deviceN) const override;

  void createMapping(GooList *separationList, int maxSepComps) override;

  int getNComps() const override { return nComps; }
  void getDefaultColor(GfxColor *color) override;

  GBool isNonMarking() const override { return nonMarking; }

  // DeviceN-specific access.
  const GooString *getColorantName(int i) const { return names[i]; }
  GfxColorSpace *getAlt() { return alt; }
  Function *getTintTransformFunc() { return func; }

private:

  GfxDeviceNColorSpace(int nCompsA, GooString **namesA,
		       GfxColorSpace *alt, Function *func, GooList *sepsCSA,
		       int *mappingA, GBool nonMarkingA, Guint overprintMaskA);

  int nComps;			// number of components
  GooString			// colorant names
    *names[gfxColorMaxComps];
  GfxColorSpace *alt;		// alternate color space
  Function *func;		// tint transform (into alternate color space)
  GBool nonMarking;
  GooList *sepsCS; // list of separation cs for spot colorants;
};

//------------------------------------------------------------------------
// GfxPatternColorSpace
//------------------------------------------------------------------------

class GfxPatternColorSpace: public GfxColorSpace {
public:

  GfxPatternColorSpace(GfxColorSpace *underA);
  ~GfxPatternColorSpace();
  GfxColorSpace *copy() override;
  GfxColorSpaceMode getMode() override { return csPattern; }

  // Construct a Pattern color space.  Returns nullptr if unsuccessful.
  static GfxColorSpace *parse(GfxResources *res, Array *arr, OutputDev *out, GfxState *state, int recursion);

  void getGray(const GfxColor *color, GfxGray *gray) const override;
  void getRGB(const GfxColor *color, GfxRGB *rgb) const override;
  void getCMYK(const GfxColor *color, GfxCMYK *cmyk) const override;
  void getDeviceN(const GfxColor *color, GfxColor *deviceN) const override;

  int getNComps() const override { return 0; }
  void getDefaultColor(GfxColor *color) override;

  // Pattern-specific access.
  GfxColorSpace *getUnder() { return under; }

private:

  GfxColorSpace *under;		// underlying color space (for uncolored
				//   patterns)
};

//------------------------------------------------------------------------
// GfxPattern
//------------------------------------------------------------------------

class GfxPattern {
public:

  GfxPattern(int typeA, int patternRefNumA);
  virtual ~GfxPattern();

  GfxPattern(const GfxPattern &) = delete;
  GfxPattern& operator=(const GfxPattern &other) = delete;

  static GfxPattern *parse(GfxResources *res, Object *obj, OutputDev *out, GfxState *state, int patternRefNum);

  virtual GfxPattern *copy() = 0;

  int getType() const { return type; }

  int getPatternRefNum() const { return patternRefNum; }

private:

  int type;
  int patternRefNum;
};

//------------------------------------------------------------------------
// GfxTilingPattern
//------------------------------------------------------------------------

class GfxTilingPattern: public GfxPattern {
public:

  static GfxTilingPattern *parse(Object *patObj, int patternRefNum);
  ~GfxTilingPattern();

  GfxPattern *copy() override;

  int getPaintType() const { return paintType; }
  int getTilingType() const { return tilingType; }
  const double *getBBox() const { return bbox; }
  double getXStep() const { return xStep; }
  double getYStep() const { return yStep; }
  Dict *getResDict()
    { return resDict.isDict() ? resDict.getDict() : (Dict *)nullptr; }
  const double *getMatrix() const { return matrix; }
  Object *getContentStream() { return &contentStream; }

private:

  GfxTilingPattern(int paintTypeA, int tilingTypeA,
		   double *bboxA, double xStepA, double yStepA,
		   Object *resDictA, double *matrixA,
		   Object *contentStreamA, int patternRefNumA);

  int paintType;
  int tilingType;
  double bbox[4];
  double xStep, yStep;
  Object resDict;
  double matrix[6];
  Object contentStream;
};

//------------------------------------------------------------------------
// GfxShadingPattern
//------------------------------------------------------------------------

class GfxShadingPattern: public GfxPattern {
public:

  static GfxShadingPattern *parse(GfxResources *res, Object *patObj, OutputDev *out, GfxState *state, int patternRefNum);
  ~GfxShadingPattern();

  GfxPattern *copy() override;

  GfxShading *getShading() { return shading; }
  const double *getMatrix() const { return matrix; }

private:

  GfxShadingPattern(GfxShading *shadingA, double *matrixA, int patternRefNumA);

  GfxShading *shading;
  double matrix[6];
};

//------------------------------------------------------------------------
// GfxShading
//------------------------------------------------------------------------

class GfxShading {
public:

  GfxShading(int typeA);
  GfxShading(GfxShading *shading);
  virtual ~GfxShading();

  GfxShading(const GfxShading &) = delete;
  GfxShading& operator=(const GfxShading &other) = delete;

  static GfxShading *parse(GfxResources *res, Object *obj, OutputDev *out, GfxState *state);

  virtual GfxShading *copy() = 0;

  int getType() const { return type; }
  GfxColorSpace *getColorSpace() { return colorSpace; }
  const GfxColor *getBackground() const { return &background; }
  GBool getHasBackground() const { return hasBackground; }
  void getBBox(double *xMinA, double *yMinA, double *xMaxA, double *yMaxA) const
    { *xMinA = xMin; *yMinA = yMin; *xMaxA = xMax; *yMaxA = yMax; }
  GBool getHasBBox() const { return hasBBox; }

protected:

  GBool init(GfxResources *res, Dict *dict, OutputDev *out, GfxState *state);

  // 1: Function-based shading
  // 2: Axial shading
  // 3: Radial shading
  // 4: Free-form Gouraud-shaded triangle mesh
  // 5: Lattice-form Gouraud-shaded triangle mesh
  // 6: Coons patch mesh
  // 7: Tensor-product patch mesh
  int type;
  GBool hasBackground;
  GBool hasBBox;
  GfxColorSpace *colorSpace;
  GfxColor background;
  double xMin, yMin, xMax, yMax;
};

//------------------------------------------------------------------------
// GfxUnivariateShading
//------------------------------------------------------------------------

class GfxUnivariateShading: public GfxShading {
public:

  GfxUnivariateShading(int typeA,
		       double t0A, double t1A,
		       Function **funcsA, int nFuncsA,
		       GBool extend0A, GBool extend1A);
  GfxUnivariateShading(GfxUnivariateShading *shading);
  ~GfxUnivariateShading();

  double getDomain0() const { return t0; }
  double getDomain1() const { return t1; }
  GBool getExtend0() const { return extend0; }
  GBool getExtend1() const { return extend1; }
  int getNFuncs() const { return nFuncs; }
  const Function *getFunc(int i) const { return funcs[i]; }
  // returns the nComps of the shading
  // i.e. how many positions of color have been set
  int getColor(double t, GfxColor *color);

  void setupCache(const Matrix *ctm,
		  double xMin, double yMin,
		  double xMax, double yMax);

  virtual void getParameterRange(double *lower, double *upper,
				 double xMin, double yMin,
				 double xMax, double yMax) = 0;

  virtual double getDistance(double tMin, double tMax) = 0;

private:

  double t0, t1;
  Function *funcs[gfxColorMaxComps];
  int nFuncs;
  GBool extend0, extend1;

  int cacheSize, lastMatch;
  double *cacheBounds;
  double *cacheCoeff;
  double *cacheValues;
};

//------------------------------------------------------------------------
// GfxFunctionShading
//------------------------------------------------------------------------

class GfxFunctionShading: public GfxShading {
public:

  GfxFunctionShading(double x0A, double y0A,
		     double x1A, double y1A,
		     double *matrixA,
		     Function **funcsA, int nFuncsA);
  GfxFunctionShading(GfxFunctionShading *shading);
  ~GfxFunctionShading();

  static GfxFunctionShading *parse(GfxResources *res, Dict *dict, OutputDev *out, GfxState *state);

  GfxShading *copy() override;

  void getDomain(double *x0A, double *y0A, double *x1A, double *y1A) const
    { *x0A = x0; *y0A = y0; *x1A = x1; *y1A = y1; }
  const double *getMatrix() const { return matrix; }
  int getNFuncs() const { return nFuncs; }
  const Function *getFunc(int i) const { return funcs[i]; }
  void getColor(double x, double y, GfxColor *color) const;

private:

  double x0, y0, x1, y1;
  double matrix[6];
  Function *funcs[gfxColorMaxComps];
  int nFuncs;
};

//------------------------------------------------------------------------
// GfxAxialShading
//------------------------------------------------------------------------

class GfxAxialShading: public GfxUnivariateShading {
public:

  GfxAxialShading(double x0A, double y0A,
		  double x1A, double y1A,
		  double t0A, double t1A,
		  Function **funcsA, int nFuncsA,
		  GBool extend0A, GBool extend1A);
  GfxAxialShading(GfxAxialShading *shading);
  ~GfxAxialShading();

  static GfxAxialShading *parse(GfxResources *res, Dict *dict, OutputDev *out, GfxState *state);

  GfxShading *copy() override;

  void getCoords(double *x0A, double *y0A, double *x1A, double *y1A) const
    { *x0A = x0; *y0A = y0; *x1A = x1; *y1A = y1; }

  void getParameterRange(double *lower, double *upper,
				 double xMin, double yMin,
				 double xMax, double yMax) override;

  double getDistance(double tMin, double tMax) override;

private:

  double x0, y0, x1, y1;
};

//------------------------------------------------------------------------
// GfxRadialShading
//------------------------------------------------------------------------

class GfxRadialShading: public GfxUnivariateShading {
public:

  GfxRadialShading(double x0A, double y0A, double r0A,
		   double x1A, double y1A, double r1A,
		   double t0A, double t1A,
		   Function **funcsA, int nFuncsA,
		   GBool extend0A, GBool extend1A);
  GfxRadialShading(GfxRadialShading *shading);
  ~GfxRadialShading();

  static GfxRadialShading *parse(GfxResources *res, Dict *dict, OutputDev *out, GfxState *state);

  GfxShading *copy() override;

  void getCoords(double *x0A, double *y0A, double *r0A,
		 double *x1A, double *y1A, double *r1A) const
    { *x0A = x0; *y0A = y0; *r0A = r0; *x1A = x1; *y1A = y1; *r1A = r1; }

  void getParameterRange(double *lower, double *upper,
				 double xMin, double yMin,
				 double xMax, double yMax) override;

  double getDistance(double tMin, double tMax) override;

private:

  double x0, y0, r0, x1, y1, r1;
};

//------------------------------------------------------------------------
// GfxGouraudTriangleShading
//------------------------------------------------------------------------

struct GfxGouraudVertex {
  double x, y;
  GfxColor color;
};

class GfxGouraudTriangleShading: public GfxShading {
public:

  GfxGouraudTriangleShading(int typeA,
			    GfxGouraudVertex *verticesA, int nVerticesA,
			    int (*trianglesA)[3], int nTrianglesA,
			    Function **funcsA, int nFuncsA);
  GfxGouraudTriangleShading(GfxGouraudTriangleShading *shading);
  ~GfxGouraudTriangleShading();

  static GfxGouraudTriangleShading *parse(GfxResources *res, int typeA, Dict *dict, Stream *str, OutputDev *out, GfxState *state);

  GfxShading *copy() override;

  int getNTriangles() const { return nTriangles; }

  bool isParameterized() const { return nFuncs > 0; }

  /**
   * @precondition isParameterized() == true
   */
  double getParameterDomainMin() const { assert(isParameterized()); return funcs[0]->getDomainMin(0); }

  /**
   * @precondition isParameterized() == true
   */
  double getParameterDomainMax() const { assert(isParameterized()); return funcs[0]->getDomainMax(0); }

  /**
   * @precondition isParameterized() == false
   */
  void getTriangle(int i, double *x0, double *y0, GfxColor *color0,
		   double *x1, double *y1, GfxColor *color1,
		   double *x2, double *y2, GfxColor *color2);

  /**
   * Variant for functions.
   *
   * @precondition isParameterized() == true
   */
  void getTriangle(int i, double *x0, double *y0, double *color0,
		   double *x1, double *y1, double *color1,
		   double *x2, double *y2, double *color2);

  void getParameterizedColor(double t, GfxColor *color) const;

private:

  GfxGouraudVertex *vertices;
  int nVertices;
  int (*triangles)[3];
  int nTriangles;
  Function *funcs[gfxColorMaxComps];
  int nFuncs;
};

//------------------------------------------------------------------------
// GfxPatchMeshShading
//------------------------------------------------------------------------

/**
 * A tensor product cubic bezier patch consisting of 4x4 points and 4 color
 * values.
 *
 * See the Shading Type 7 specifications. Note that Shading Type 6 is also
 * represented using GfxPatch.
 */
struct GfxPatch {
  /**
   * Represents a single color value for the patch.
   */
  struct ColorValue {
    /**
     * For parameterized patches, only element 0 is valid; it contains
     * the single parameter.
     *
     * For non-parameterized patches, c contains all color components
     * as decoded from the input stream. In this case, you will need to
     * use dblToCol() before assigning them to GfxColor.
     */
    double c[gfxColorMaxComps];
  };

  double x[4][4];
  double y[4][4];
  ColorValue color[2][2];
};

class GfxPatchMeshShading: public GfxShading {
public:

  GfxPatchMeshShading(int typeA, GfxPatch *patchesA, int nPatchesA,
		      Function **funcsA, int nFuncsA);
  GfxPatchMeshShading(GfxPatchMeshShading *shading);
  ~GfxPatchMeshShading();

  static GfxPatchMeshShading *parse(GfxResources *res, int typeA, Dict *dict, Stream *str, OutputDev *out, GfxState *state);

  GfxShading *copy() override;

  int getNPatches() const { return nPatches; }
  const GfxPatch *getPatch(int i) const { return &patches[i]; }

  bool isParameterized() const { return nFuncs > 0; }

  /**
   * @precondition isParameterized() == true
   */
  double getParameterDomainMin() const { assert(isParameterized()); return funcs[0]->getDomainMin(0); }

  /**
   * @precondition isParameterized() == true
   */
  double getParameterDomainMax() const { assert(isParameterized()); return funcs[0]->getDomainMax(0); }

  void getParameterizedColor(double t, GfxColor *color) const;

private:

  GfxPatch *patches;
  int nPatches;
  Function *funcs[gfxColorMaxComps];
  int nFuncs;
};

//------------------------------------------------------------------------
// GfxImageColorMap
//------------------------------------------------------------------------

class GfxImageColorMap {
public:

  // Constructor.
  GfxImageColorMap(int bitsA, Object *decode, GfxColorSpace *colorSpaceA);

  // Destructor.
  ~GfxImageColorMap();

  GfxImageColorMap(const GfxImageColorMap &) = delete;
  GfxImageColorMap& operator=(const GfxImageColorMap &) = delete;

  // Return a copy of this color map.
  GfxImageColorMap *copy() { return new GfxImageColorMap(this); }

  // Is color map valid?
  GBool isOk() const { return ok; }

  // Get the color space.
  GfxColorSpace *getColorSpace() { return colorSpace; }

  // Get stream decoding info.
  int getNumPixelComps() const { return nComps; }
  int getBits() const { return bits; }

  // Get decode table.
  double getDecodeLow(int i) const { return decodeLow[i]; }
  double getDecodeHigh(int i) const { return decodeLow[i] + decodeRange[i]; }
  
  bool useRGBLine() const { return (colorSpace2 && colorSpace2->useGetRGBLine ()) || (!colorSpace2 && colorSpace->useGetRGBLine ()); }
  bool useCMYKLine() const { return (colorSpace2 && colorSpace2->useGetCMYKLine ()) || (!colorSpace2 && colorSpace->useGetCMYKLine ()); }
  bool useDeviceNLine() const { return (colorSpace2 && colorSpace2->useGetDeviceNLine ()) || (!colorSpace2 && colorSpace->useGetDeviceNLine ()); }

  // Convert an image pixel to a color.
  void getGray(Guchar *x, GfxGray *gray);
  void getRGB(Guchar *x, GfxRGB *rgb);
  void getRGBLine(Guchar *in, unsigned int *out, int length);
  void getRGBLine(Guchar *in, Guchar *out, int length);
  void getRGBXLine(Guchar *in, Guchar *out, int length);
  void getGrayLine(Guchar *in, Guchar *out, int length);
  void getCMYKLine(Guchar *in, Guchar *out, int length);
  void getDeviceNLine(Guchar *in, Guchar *out, int length);
  void getCMYK(Guchar *x, GfxCMYK *cmyk);
  void getDeviceN(Guchar *x, GfxColor *deviceN);
  void getColor(Guchar *x, GfxColor *color);

  // Matte color ops
  void setMatteColor(const GfxColor *color) { useMatte = gTrue; matteColor = *color; }
  const GfxColor *getMatteColor() const { return (useMatte) ? &matteColor : nullptr; }
private:

  GfxImageColorMap(GfxImageColorMap *colorMap);

  GfxColorSpace *colorSpace;	// the image color space
  int bits;			// bits per component
  int nComps;			// number of components in a pixel
  GfxColorSpace *colorSpace2;	// secondary color space
  int nComps2;			// number of components in colorSpace2
  GfxColorComp *		// lookup table
    lookup[gfxColorMaxComps];
  GfxColorComp *		// optimized case lookup table
    lookup2[gfxColorMaxComps];
  Guchar *byte_lookup;
  double			// minimum values for each component
    decodeLow[gfxColorMaxComps];
  double			// max - min value for each component
    decodeRange[gfxColorMaxComps];
  GBool useMatte;
  GfxColor matteColor;
  GBool ok;
};

//------------------------------------------------------------------------
// GfxSubpath and GfxPath
//------------------------------------------------------------------------

class GfxSubpath {
public:

  // Constructor.
  GfxSubpath(double x1, double y1);

  // Destructor.
  ~GfxSubpath();

  GfxSubpath(const GfxSubpath &) = delete;
  GfxSubpath& operator=(const GfxSubpath &) = delete;

  // Copy.
  GfxSubpath *copy() const { return new GfxSubpath(this); }

  // Get points.
  int getNumPoints() const { return n; }
  double getX(int i) const { return x[i]; }
  double getY(int i) const { return y[i]; }
  GBool getCurve(int i) const { return curve[i]; }

  void setX(int i, double a) { x[i] = a; }
  void setY(int i, double a) { y[i] = a; }

  // Get last point.
  double getLastX() const { return x[n-1]; }
  double getLastY() const { return y[n-1]; }

  // Add a line segment.
  void lineTo(double x1, double y1);

  // Add a Bezier curve.
  void curveTo(double x1, double y1, double x2, double y2,
	       double x3, double y3);

  // Close the subpath.
  void close();
  GBool isClosed() const { return closed; }

  // Add (<dx>, <dy>) to each point in the subpath.
  void offset(double dx, double dy);

private:

  double *x, *y;		// points
  GBool *curve;			// curve[i] => point i is a control point
				//   for a Bezier curve
  int n;			// number of points
  int size;			// size of x/y arrays
  GBool closed;			// set if path is closed

  GfxSubpath(const GfxSubpath *subpath);
};

class GfxPath {
public:

  // Constructor.
  GfxPath();

  // Destructor.
  ~GfxPath();

  GfxPath(const GfxPath &) = delete;
  GfxPath& operator=(const GfxPath &) = delete;

  // Copy.
  GfxPath *copy() const
    { return new GfxPath(justMoved, firstX, firstY, subpaths, n, size); }

  // Is there a current point?
  GBool isCurPt() const { return n > 0 || justMoved; }

  // Is the path non-empty, i.e., is there at least one segment?
  GBool isPath() const { return n > 0; }

  // Get subpaths.
  int getNumSubpaths() const { return n; }
  GfxSubpath *getSubpath(int i) { return subpaths[i]; }

  // Get last point on last subpath.
  double getLastX() const { return subpaths[n-1]->getLastX(); }
  double getLastY() const { return subpaths[n-1]->getLastY(); }

  // Move the current point.
  void moveTo(double x, double y);

  // Add a segment to the last subpath.
  void lineTo(double x, double y);

  // Add a Bezier curve to the last subpath
  void curveTo(double x1, double y1, double x2, double y2,
	       double x3, double y3);

  // Close the last subpath.
  void close();

  // Append <path> to <this>.
  void append(GfxPath *path);

  // Add (<dx>, <dy>) to each point in the path.
  void offset(double dx, double dy);

private:

  GBool justMoved;		// set if a new subpath was just started
  double firstX, firstY;	// first point in new subpath
  GfxSubpath **subpaths;	// subpaths
  int n;			// number of subpaths
  int size;			// size of subpaths array

  GfxPath(GBool justMoved1, double firstX1, double firstY1,
	  GfxSubpath **subpaths1, int n1, int size1);
};

//------------------------------------------------------------------------
// GfxState
//------------------------------------------------------------------------

class GfxState {
public:
  /**
   * When GfxState::getReusablePath() is invoked, the currently active
   * path is taken per reference and its coordinates can be re-edited.
   *
   * A ReusablePathIterator is intented to reduce overhead when the same
   * path type is used a lot of times, only with different coordinates. It
   * allows just to update the coordinates (occuring in the same order as
   * in the original path).
   */
  class ReusablePathIterator {
  public:
    /**
     * Creates the ReusablePathIterator. This should only be done from
     * GfxState::getReusablePath().
     *
     * @param path the path as it is used so far. Changing this path,
     * deleting it or starting a new path from scratch will most likely
     * invalidate the iterator (and may cause serious problems). Make
     * sure the path's memory structure is not changed during the
     * lifetime of the ReusablePathIterator.
     */
    ReusablePathIterator( GfxPath* path );

    /**
     * Returns true if and only if the current iterator position is
     * beyond the last valid point.
     *
     * A call to setCoord() will be undefined.
     */
    bool isEnd() const;

    /**
     * Advances the iterator.
     */
    void next();

     /**
     * Updates the coordinates associated to the current iterator
     * position.
     */
     void setCoord( double x, double y );

    /**
     * Resets the iterator.
     */
    void reset();
  private:
    GfxPath *path;
    int subPathOff;

    int coordOff;
    int numCoords;

    GfxSubpath *curSubPath;
  };

  // Construct a default GfxState, for a device with resolution <hDPI>
  // x <vDPI>, page box <pageBox>, page rotation <rotateA>, and
  // coordinate system specified by <upsideDown>.
  GfxState(double hDPIA, double vDPIA, const PDFRectangle *pageBox,
	   int rotateA, GBool upsideDown);

  // Destructor.
  ~GfxState();

  GfxState(const GfxState &) = delete;
  GfxState& operator=(const GfxState &) = delete;

  // Copy.
  GfxState *copy(GBool copyPath = gFalse) const
    { return new GfxState(this, copyPath); }

  // Accessors.
  double getHDPI() const { return hDPI; }
  double getVDPI() const { return vDPI; }
  const double *getCTM() const { return ctm; }
  void getCTM(Matrix *m) const { memcpy (m->m, ctm, sizeof m->m); }
  double getX1() const { return px1; }
  double getY1() const { return py1; }
  double getX2() const { return px2; }
  double getY2() const { return py2; }
  double getPageWidth() const { return pageWidth; }
  double getPageHeight() const { return pageHeight; }
  int getRotate() const { return rotate; }
  const GfxColor *getFillColor() const { return &fillColor; }
  const GfxColor *getStrokeColor() const { return &strokeColor; }
  void getFillGray(GfxGray *gray)
    { fillColorSpace->getGray(&fillColor, gray); }
  void getStrokeGray(GfxGray *gray)
    { strokeColorSpace->getGray(&strokeColor, gray); }
  void getFillRGB(GfxRGB *rgb) const
    { fillColorSpace->getRGB(&fillColor, rgb); }
  void getStrokeRGB(GfxRGB *rgb) const
    { strokeColorSpace->getRGB(&strokeColor, rgb); }
  void getFillCMYK(GfxCMYK *cmyk)
    { fillColorSpace->getCMYK(&fillColor, cmyk); }
  void getFillDeviceN(GfxColor *deviceN)
    { fillColorSpace->getDeviceN(&fillColor, deviceN); }
  void getStrokeCMYK(GfxCMYK *cmyk)
    { strokeColorSpace->getCMYK(&strokeColor, cmyk); }
  void getStrokeDeviceN(GfxColor *deviceN)
    { strokeColorSpace->getDeviceN(&strokeColor, deviceN); }
  GfxColorSpace *getFillColorSpace() { return fillColorSpace; }
  GfxColorSpace *getStrokeColorSpace() { return strokeColorSpace; }
  GfxPattern *getFillPattern() { return fillPattern; }
  GfxPattern *getStrokePattern() { return strokePattern; }
  GfxBlendMode getBlendMode() const { return blendMode; }
  double getFillOpacity() const { return fillOpacity; }
  double getStrokeOpacity() const { return strokeOpacity; }
  GBool getFillOverprint() const { return fillOverprint; }
  GBool getStrokeOverprint() const { return strokeOverprint; }
  int getOverprintMode() const { return overprintMode; }
  Function **getTransfer() { return transfer; }
  double getLineWidth() const { return lineWidth; }
  void getLineDash(double **dash, int *length, double *start)
    { *dash = lineDash; *length = lineDashLength; *start = lineDashStart; }
  int getFlatness() const { return flatness; }
  int getLineJoin() const { return lineJoin; }
  int getLineCap() const { return lineCap; }
  double getMiterLimit() const { return miterLimit; }
  GBool getStrokeAdjust() const { return strokeAdjust; }
  GBool getAlphaIsShape() const { return alphaIsShape; }
  GBool getTextKnockout() const { return textKnockout; }
  GfxFont *getFont() { return font; }
  double getFontSize() const { return fontSize; }
  const double *getTextMat() const { return textMat; }
  double getCharSpace() const { return charSpace; }
  double getWordSpace() const { return wordSpace; }
  double getHorizScaling() const { return horizScaling; }
  double getLeading() const { return leading; }
  double getRise() const { return rise; }
  int getRender() const { return render; }
  char *getRenderingIntent() { return renderingIntent; }
  GfxPath *getPath() { return path; }
  void setPath(GfxPath *pathA);
  double getCurX() const { return curX; }
  double getCurY() const { return curY; }
  void getClipBBox(double *xMin, double *yMin, double *xMax, double *yMax)
    { *xMin = clipXMin; *yMin = clipYMin; *xMax = clipXMax; *yMax = clipYMax; }
  void getUserClipBBox(double *xMin, double *yMin, double *xMax, double *yMax);
  double getLineX() const { return lineX; }
  double getLineY() const { return lineY; }

  // Is there a current point/path?
  GBool isCurPt() const { return path->isCurPt(); }
  GBool isPath() const { return path->isPath(); }

  // Transforms.
  void transform(double x1, double y1, double *x2, double *y2)
    { *x2 = ctm[0] * x1 + ctm[2] * y1 + ctm[4];
      *y2 = ctm[1] * x1 + ctm[3] * y1 + ctm[5]; }
  void transformDelta(double x1, double y1, double *x2, double *y2)
    { *x2 = ctm[0] * x1 + ctm[2] * y1;
      *y2 = ctm[1] * x1 + ctm[3] * y1; }
  void textTransform(double x1, double y1, double *x2, double *y2)
    { *x2 = textMat[0] * x1 + textMat[2] * y1 + textMat[4];
      *y2 = textMat[1] * x1 + textMat[3] * y1 + textMat[5]; }
  void textTransformDelta(double x1, double y1, double *x2, double *y2)
    { *x2 = textMat[0] * x1 + textMat[2] * y1;
      *y2 = textMat[1] * x1 + textMat[3] * y1; }
  double transformWidth(double w);
  double getTransformedLineWidth()
    { return transformWidth(lineWidth); }
  double getTransformedFontSize();
  void getFontTransMat(double *m11, double *m12, double *m21, double *m22);

  // Change state parameters.
  void setCTM(double a, double b, double c,
	      double d, double e, double f);
  void concatCTM(double a, double b, double c,
		 double d, double e, double f);
  void shiftCTMAndClip(double tx, double ty);
  void setFillColorSpace(GfxColorSpace *colorSpace);
  void setStrokeColorSpace(GfxColorSpace *colorSpace);
  void setFillColor(const GfxColor *color) { fillColor = *color; }
  void setStrokeColor(const GfxColor *color) { strokeColor = *color; }
  void setFillPattern(GfxPattern *pattern);
  void setStrokePattern(GfxPattern *pattern);
  void setBlendMode(GfxBlendMode mode) { blendMode = mode; }
  void setFillOpacity(double opac) { fillOpacity = opac; }
  void setStrokeOpacity(double opac) { strokeOpacity = opac; }
  void setFillOverprint(GBool op) { fillOverprint = op; }
  void setStrokeOverprint(GBool op) { strokeOverprint = op; }
  void setOverprintMode(int op) { overprintMode = op; }
  void setTransfer(Function **funcs);
  void setLineWidth(double width) { lineWidth = width; }
  void setLineDash(double *dash, int length, double start);
  void setFlatness(int flatness1) { flatness = flatness1; }
  void setLineJoin(int lineJoin1) { lineJoin = lineJoin1; }
  void setLineCap(int lineCap1) { lineCap = lineCap1; }
  void setMiterLimit(double limit) { miterLimit = limit; }
  void setStrokeAdjust(GBool sa) { strokeAdjust = sa; }
  void setAlphaIsShape(GBool ais) { alphaIsShape = ais; }
  void setTextKnockout(GBool tk) { textKnockout = tk; }
  void setFont(GfxFont *fontA, double fontSizeA);
  void setTextMat(double a, double b, double c,
		  double d, double e, double f)
    { textMat[0] = a; textMat[1] = b; textMat[2] = c;
      textMat[3] = d; textMat[4] = e; textMat[5] = f; }
  void setCharSpace(double space)
    { charSpace = space; }
  void setWordSpace(double space)
    { wordSpace = space; }
  void setHorizScaling(double scale)
    { horizScaling = 0.01 * scale; }
  void setLeading(double leadingA)
    { leading = leadingA; }
  void setRise(double riseA)
    { rise = riseA; }
  void setRender(int renderA)
    { render = renderA; }
  void setRenderingIntent(const char *intent)
    { strncpy(renderingIntent, intent, 31); }

#ifdef USE_CMS
  void setDisplayProfile(void *localDisplayProfileA);
  void *getDisplayProfile() { return localDisplayProfile; }
  GfxColorTransform *getXYZ2DisplayTransform();
#endif

  // Add to path.
  void moveTo(double x, double y)
    { path->moveTo(curX = x, curY = y); }
  void lineTo(double x, double y)
    { path->lineTo(curX = x, curY = y); }
  void curveTo(double x1, double y1, double x2, double y2,
	       double x3, double y3)
    { path->curveTo(x1, y1, x2, y2, curX = x3, curY = y3); }
  void closePath()
    { path->close(); curX = path->getLastX(); curY = path->getLastY(); }
  void clearPath();

  // Update clip region.
  void clip();
  void clipToStrokePath();
  void clipToRect(double xMin, double yMin, double xMax, double yMax);

  // Text position.
  void textSetPos(double tx, double ty) { lineX = tx; lineY = ty; }
  void textMoveTo(double tx, double ty)
    { lineX = tx; lineY = ty; textTransform(tx, ty, &curX, &curY); }
  void textShift(double tx, double ty);
  void shift(double dx, double dy);

  // Push/pop GfxState on/off stack.
  GfxState *save();
  GfxState *restore();
  GBool hasSaves() const { return saved != nullptr; }
  GBool isParentState(GfxState *state) { return saved == state || (saved && saved->isParentState(state)); }

  // Misc
  GBool parseBlendMode(Object *obj, GfxBlendMode *mode);

  ReusablePathIterator *getReusablePath() { return new ReusablePathIterator(path); }
private:

  double hDPI, vDPI;		// resolution
  double ctm[6];		// coord transform matrix
  double px1, py1, px2, py2;	// page corners (user coords)
  double pageWidth, pageHeight;	// page size (pixels)
  int rotate;			// page rotation angle

  GfxColorSpace *fillColorSpace;   // fill color space
  GfxColorSpace *strokeColorSpace; // stroke color space
  GfxColor fillColor;		// fill color
  GfxColor strokeColor;		// stroke color
  GfxPattern *fillPattern;	// fill pattern
  GfxPattern *strokePattern;	// stroke pattern
  GfxBlendMode blendMode;	// transparency blend mode
  double fillOpacity;		// fill opacity
  double strokeOpacity;		// stroke opacity
  GBool fillOverprint;		// fill overprint
  GBool strokeOverprint;	// stroke overprint
  int overprintMode;		// overprint mode
  Function *transfer[4];	// transfer function (entries may be: all
				//   nullptr = identity; last three nullptr =
				//   single function; all four non-nullptr =
				//   R,G,B,gray functions)

  double lineWidth;		// line width
  double *lineDash;		// line dash
  int lineDashLength;
  double lineDashStart;
  int flatness;			// curve flatness
  int lineJoin;			// line join style
  int lineCap;			// line cap style
  double miterLimit;		// line miter limit
  GBool strokeAdjust;		// stroke adjustment
  GBool alphaIsShape;		// alpha is shape
  GBool textKnockout;		// text knockout

  GfxFont *font;		// font
  double fontSize;		// font size
  double textMat[6];		// text matrix
  double charSpace;		// character spacing
  double wordSpace;		// word spacing
  double horizScaling;		// horizontal scaling
  double leading;		// text leading
  double rise;			// text rise
  int render;			// text rendering mode

  GfxPath *path;		// array of path elements
  double curX, curY;		// current point (user coords)
  double lineX, lineY;		// start of current text line (text coords)

  double clipXMin, clipYMin,	// bounding box for clip region
         clipXMax, clipYMax;
  char renderingIntent[32];

  GfxState *saved;		// next GfxState on stack

  GfxState(const GfxState *state, GBool copyPath);

#ifdef USE_CMS
  void *localDisplayProfile;
  int displayProfileRef;
  GfxColorTransform *XYZ2DisplayTransformRelCol;
  GfxColorTransform *XYZ2DisplayTransformAbsCol;
  GfxColorTransform *XYZ2DisplayTransformSat;
  GfxColorTransform *XYZ2DisplayTransformPerc;
#endif
};

#endif
