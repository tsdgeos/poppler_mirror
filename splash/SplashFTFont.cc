//========================================================================
//
// SplashFTFont.cc
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005, 2007-2011, 2014, 2018, 2020 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2006 Kristian Høgsberg <krh@bitplanet.net>
// Copyright (C) 2009 Petr Gajdos <pgajdos@novell.com>
// Copyright (C) 2010 Suzuki Toshiya <mpsuzuki@hiroshima-u.ac.jp>
// Copyright (C) 2011 Andreas Hartmetz <ahartmetz@gmail.com>
// Copyright (C) 2012 Thomas Freitag <Thomas.Freitag@alfa.de>
// Copyright (C) 2017 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2018 Oliver Sander <oliver.sander@tu-dresden.de>
// Copyright (C) 2025 g10 Code GmbH, Author: Sune Stolborg Vuorela <sune@vuorela.dk>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include <config.h>

#include <ft2build.h>
#include FT_OUTLINE_H
#include FT_SIZES_H
#include FT_GLYPH_H
#include "goo/gmem.h"
#include "SplashMath.h"
#include "SplashGlyphBitmap.h"
#include "SplashPath.h"
#include "SplashFTFontEngine.h"
#include "SplashFTFontFile.h"
#include "SplashFTFont.h"

#include "goo/GooLikely.h"

//------------------------------------------------------------------------

static int glyphPathMoveTo(const FT_Vector *pt, void *path);
static int glyphPathLineTo(const FT_Vector *pt, void *path);
static int glyphPathConicTo(const FT_Vector *ctrl, const FT_Vector *pt, void *path);
static int glyphPathCubicTo(const FT_Vector *ctrl1, const FT_Vector *ctrl2, const FT_Vector *pt, void *path);

//------------------------------------------------------------------------
// SplashFTFont
//------------------------------------------------------------------------

SplashFTFont::SplashFTFont(SplashFTFontFile *fontFileA, SplashCoord *matA, const SplashCoord *textMatA)
    : SplashFont(fontFileA, matA, textMatA, fontFileA->engine->aa), textScale(0), enableFreeTypeHinting(fontFileA->engine->enableFreeTypeHinting), enableSlightHinting(fontFileA->engine->enableSlightHinting), isOk(false)
{
    FT_Face face;
    int div;
    int x, y;

    face = fontFileA->face;
    if (FT_New_Size(face, &sizeObj)) {
        return;
    }
    face->size = sizeObj;
    size = splashRound(splashDist(0, 0, mat[2], mat[3]));
    if (size < 1) {
        size = 1;
    }
    if (FT_Set_Pixel_Sizes(face, 0, size)) {
        return;
    }
    // if the textMat values are too small, FreeType's fixed point
    // arithmetic doesn't work so well
    textScale = splashDist(0, 0, textMat[2], textMat[3]) / size;

    if (unlikely(textScale == 0 || face->units_per_EM == 0)) {
        return;
    }

    div = face->bbox.xMax > 20000 ? 65536 : 1;

    // transform the four corners of the font bounding box -- the min
    // and max values form the bounding box of the transformed font
    x = (int)((mat[0] * face->bbox.xMin + mat[2] * face->bbox.yMin) / (div * face->units_per_EM));
    xMin = xMax = x;
    y = (int)((mat[1] * face->bbox.xMin + mat[3] * face->bbox.yMin) / (div * face->units_per_EM));
    yMin = yMax = y;
    x = (int)((mat[0] * face->bbox.xMin + mat[2] * face->bbox.yMax) / (div * face->units_per_EM));
    if (x < xMin) {
        xMin = x;
    } else if (x > xMax) {
        xMax = x;
    }
    y = (int)((mat[1] * face->bbox.xMin + mat[3] * face->bbox.yMax) / (div * face->units_per_EM));
    if (y < yMin) {
        yMin = y;
    } else if (y > yMax) {
        yMax = y;
    }
    x = (int)((mat[0] * face->bbox.xMax + mat[2] * face->bbox.yMin) / (div * face->units_per_EM));
    if (x < xMin) {
        xMin = x;
    } else if (x > xMax) {
        xMax = x;
    }
    y = (int)((mat[1] * face->bbox.xMax + mat[3] * face->bbox.yMin) / (div * face->units_per_EM));
    if (y < yMin) {
        yMin = y;
    } else if (y > yMax) {
        yMax = y;
    }
    x = (int)((mat[0] * face->bbox.xMax + mat[2] * face->bbox.yMax) / (div * face->units_per_EM));
    if (x < xMin) {
        xMin = x;
    } else if (x > xMax) {
        xMax = x;
    }
    y = (int)((mat[1] * face->bbox.xMax + mat[3] * face->bbox.yMax) / (div * face->units_per_EM));
    if (y < yMin) {
        yMin = y;
    } else if (y > yMax) {
        yMax = y;
    }
    // This is a kludge: some buggy PDF generators embed fonts with
    // zero bounding boxes.
    if (xMax == xMin) {
        xMin = 0;
        xMax = size;
    }
    if (yMax == yMin) {
        yMin = 0;
        yMax = (int)((SplashCoord)1.2 * size);
    }

    // compute the transform matrix
    matrix.xx = (FT_Fixed)((mat[0] / size) * 65536);
    matrix.yx = (FT_Fixed)((mat[1] / size) * 65536);
    matrix.xy = (FT_Fixed)((mat[2] / size) * 65536);
    matrix.yy = (FT_Fixed)((mat[3] / size) * 65536);
    textMatrix.xx = (FT_Fixed)((textMat[0] / (textScale * size)) * 65536);
    textMatrix.yx = (FT_Fixed)((textMat[1] / (textScale * size)) * 65536);
    textMatrix.xy = (FT_Fixed)((textMat[2] / (textScale * size)) * 65536);
    textMatrix.yy = (FT_Fixed)((textMat[3] / (textScale * size)) * 65536);

    isOk = true;
}

SplashFTFont::~SplashFTFont() = default;

bool SplashFTFont::getGlyph(int c, int xFrac, int yFrac, SplashGlyphBitmap *bitmap, int x0, int y0, SplashClip *clip, SplashClipResult *clipRes)
{
    return SplashFont::getGlyph(c, xFrac, 0, bitmap, x0, y0, clip, clipRes);
}

static FT_Int32 getFTLoadFlags(bool type1, bool trueType, bool aa, bool enableFreeTypeHinting, bool enableSlightHinting)
{
    int ret = FT_LOAD_DEFAULT;
    if (aa) {
        ret |= FT_LOAD_NO_BITMAP;
    }

    if (enableFreeTypeHinting) {
        if (enableSlightHinting) {
            ret |= FT_LOAD_TARGET_LIGHT;
        } else {
            if (trueType) {
                // FT2's autohinting doesn't always work very well (especially with
                // font subsets), so turn it off if anti-aliasing is enabled; if
                // anti-aliasing is disabled, this seems to be a tossup - some fonts
                // look better with hinting, some without, so leave hinting on
                if (aa) {
                    ret |= FT_LOAD_NO_AUTOHINT;
                }
            } else if (type1) {
                // Type 1 fonts seem to look better with 'light' hinting mode
                ret |= FT_LOAD_TARGET_LIGHT;
            }
        }
    } else {
        ret |= FT_LOAD_NO_HINTING;
    }
    return ret;
}

bool SplashFTFont::makeGlyph(int c, int xFrac, int yFrac, SplashGlyphBitmap *bitmap, int x0, int y0, SplashClip *clip, SplashClipResult *clipRes)
{
    SplashFTFontFile *ff;
    FT_Vector offset;
    FT_GlyphSlot slot;
    FT_UInt gid;
    int rowSize;
    unsigned char *p, *q;
    int i;

    if (unlikely(!isOk)) {
        return false;
    }

    ff = (SplashFTFontFile *)fontFile;

    ff->face->size = sizeObj;
    offset.x = (FT_Pos)(int)((SplashCoord)xFrac * splashFontFractionMul * 64);
    offset.y = 0;
    FT_Set_Transform(ff->face, &matrix, &offset);
    slot = ff->face->glyph;

    if (c < int(ff->codeToGID.size()) && c >= 0) {
        gid = (FT_UInt)ff->codeToGID[c];
    } else {
        gid = (FT_UInt)c;
    }

    if (FT_Load_Glyph(ff->face, gid, getFTLoadFlags(ff->type1, ff->trueType, aa, enableFreeTypeHinting, enableSlightHinting))) {
        return false;
    }

    // prelimirary values based on FT_Outline_Get_CBox
    // we add two pixels to each side to be in the safe side
    FT_BBox cbox;
    FT_Outline_Get_CBox(&ff->face->glyph->outline, &cbox);
    bitmap->x = -(cbox.xMin / 64) + 2;
    bitmap->y = (cbox.yMax / 64) + 2;
    bitmap->w = ((cbox.xMax - cbox.xMin) / 64) + 4;
    bitmap->h = ((cbox.yMax - cbox.yMin) / 64) + 4;

    *clipRes = clip->testRect(x0 - bitmap->x, y0 - bitmap->y, x0 - bitmap->x + bitmap->w, y0 - bitmap->y + bitmap->h);
    if (*clipRes == splashClipAllOutside) {
        bitmap->freeData = false;
        return true;
    }

    if (FT_Render_Glyph(slot, aa ? ft_render_mode_normal : ft_render_mode_mono)) {
        return false;
    }

    if (slot->bitmap.width == 0 || slot->bitmap.rows == 0) {
        // this can happen if (a) the glyph is really tiny or (b) the
        // metrics in the TrueType file are broken
        return false;
    }

    bitmap->x = -slot->bitmap_left;
    bitmap->y = slot->bitmap_top;
    bitmap->w = slot->bitmap.width;
    bitmap->h = slot->bitmap.rows;
    bitmap->aa = aa;
    if (aa) {
        rowSize = bitmap->w;
    } else {
        rowSize = (bitmap->w + 7) >> 3;
    }
    bitmap->data = (unsigned char *)gmallocn_checkoverflow(rowSize, bitmap->h);
    if (!bitmap->data) {
        return false;
    }
    bitmap->freeData = true;
    for (i = 0, p = bitmap->data, q = slot->bitmap.buffer; i < bitmap->h; ++i, p += rowSize, q += slot->bitmap.pitch) {
        memcpy(p, q, rowSize);
    }

    return true;
}

double SplashFTFont::getGlyphAdvance(int c)
{
    SplashFTFontFile *ff;
    FT_Vector offset;
    FT_UInt gid;
    FT_Matrix identityMatrix;

    ff = (SplashFTFontFile *)fontFile;

    // init the matrix
    identityMatrix.xx = 65536; // 1 in 16.16 format
    identityMatrix.xy = 0;
    identityMatrix.yx = 0;
    identityMatrix.yy = 65536; // 1 in 16.16 format

    // init the offset
    offset.x = 0;
    offset.y = 0;

    ff->face->size = sizeObj;
    FT_Set_Transform(ff->face, &identityMatrix, &offset);

    if (c < int(ff->codeToGID.size())) {
        gid = (FT_UInt)ff->codeToGID[c];
    } else {
        gid = (FT_UInt)c;
    }

    if (FT_Load_Glyph(ff->face, gid, getFTLoadFlags(ff->type1, ff->trueType, aa, enableFreeTypeHinting, enableSlightHinting))) {
        return -1;
    }

    // 64.0 is 1 in 26.6 format
    return ff->face->glyph->metrics.horiAdvance / 64.0 / size;
}

struct SplashFTFontPath
{
    SplashPath *path;
    SplashCoord textScale;
    bool needClose;
};

SplashPath *SplashFTFont::getGlyphPath(int c)
{
    static const FT_Outline_Funcs outlineFuncs = {
#if FREETYPE_MINOR <= 1
        (int (*)(FT_Vector *, void *))&glyphPathMoveTo,
        (int (*)(FT_Vector *, void *))&glyphPathLineTo,
        (int (*)(FT_Vector *, FT_Vector *, void *))&glyphPathConicTo,
        (int (*)(FT_Vector *, FT_Vector *, FT_Vector *, void *))&glyphPathCubicTo,
#else
        &glyphPathMoveTo,
        &glyphPathLineTo,
        &glyphPathConicTo,
        &glyphPathCubicTo,
#endif
        0,
        0
    };
    SplashFTFontFile *ff;
    SplashFTFontPath path;
    FT_GlyphSlot slot;
    FT_UInt gid;
    FT_Glyph glyph;

    if (unlikely(textScale == 0)) {
        return nullptr;
    }

    ff = (SplashFTFontFile *)fontFile;
    ff->face->size = sizeObj;
    FT_Set_Transform(ff->face, &textMatrix, nullptr);
    slot = ff->face->glyph;
    if (c < int(ff->codeToGID.size()) && c >= 0) {
        gid = ff->codeToGID[c];
    } else {
        gid = (FT_UInt)c;
    }
    if (FT_Load_Glyph(ff->face, gid, getFTLoadFlags(ff->type1, ff->trueType, aa, enableFreeTypeHinting, enableSlightHinting))) {
        return nullptr;
    }
    if (FT_Get_Glyph(slot, &glyph)) {
        return nullptr;
    }
    if (FT_Outline_Check(&((FT_OutlineGlyph)glyph)->outline)) {
        return nullptr;
    }
    path.path = new SplashPath();
    path.textScale = textScale;
    path.needClose = false;
    FT_Outline_Decompose(&((FT_OutlineGlyph)glyph)->outline, &outlineFuncs, &path);
    if (path.needClose) {
        path.path->close();
    }
    FT_Done_Glyph(glyph);
    return path.path;
}

static int glyphPathMoveTo(const FT_Vector *pt, void *path)
{
    SplashFTFontPath *p = (SplashFTFontPath *)path;

    if (p->needClose) {
        p->path->close();
        p->needClose = false;
    }
    p->path->moveTo((SplashCoord)pt->x * p->textScale / 64.0, (SplashCoord)pt->y * p->textScale / 64.0);
    return 0;
}

static int glyphPathLineTo(const FT_Vector *pt, void *path)
{
    SplashFTFontPath *p = (SplashFTFontPath *)path;

    p->path->lineTo((SplashCoord)pt->x * p->textScale / 64.0, (SplashCoord)pt->y * p->textScale / 64.0);
    p->needClose = true;
    return 0;
}

static int glyphPathConicTo(const FT_Vector *ctrl, const FT_Vector *pt, void *path)
{
    SplashFTFontPath *p = (SplashFTFontPath *)path;
    SplashCoord x0, y0, x1, y1, x2, y2, x3, y3, xc, yc;

    if (!p->path->getCurPt(&x0, &y0)) {
        return 0;
    }
    xc = (SplashCoord)ctrl->x * p->textScale / 64.0;
    yc = (SplashCoord)ctrl->y * p->textScale / 64.0;
    x3 = (SplashCoord)pt->x * p->textScale / 64.0;
    y3 = (SplashCoord)pt->y * p->textScale / 64.0;

    // A second-order Bezier curve is defined by two endpoints, p0 and
    // p3, and one control point, pc:
    //
    //     p(t) = (1-t)^2*p0 + t*(1-t)*pc + t^2*p3
    //
    // A third-order Bezier curve is defined by the same two endpoints,
    // p0 and p3, and two control points, p1 and p2:
    //
    //     p(t) = (1-t)^3*p0 + 3t*(1-t)^2*p1 + 3t^2*(1-t)*p2 + t^3*p3
    //
    // Applying some algebra, we can convert a second-order curve to a
    // third-order curve:
    //
    //     p1 = (1/3) * (p0 + 2pc)
    //     p2 = (1/3) * (2pc + p3)

    x1 = (SplashCoord)(1.0 / 3.0) * (x0 + (SplashCoord)2 * xc);
    y1 = (SplashCoord)(1.0 / 3.0) * (y0 + (SplashCoord)2 * yc);
    x2 = (SplashCoord)(1.0 / 3.0) * ((SplashCoord)2 * xc + x3);
    y2 = (SplashCoord)(1.0 / 3.0) * ((SplashCoord)2 * yc + y3);

    p->path->curveTo(x1, y1, x2, y2, x3, y3);
    p->needClose = true;
    return 0;
}

static int glyphPathCubicTo(const FT_Vector *ctrl1, const FT_Vector *ctrl2, const FT_Vector *pt, void *path)
{
    SplashFTFontPath *p = (SplashFTFontPath *)path;

    p->path->curveTo((SplashCoord)ctrl1->x * p->textScale / 64.0, (SplashCoord)ctrl1->y * p->textScale / 64.0, (SplashCoord)ctrl2->x * p->textScale / 64.0, (SplashCoord)ctrl2->y * p->textScale / 64.0,
                     (SplashCoord)pt->x * p->textScale / 64.0, (SplashCoord)pt->y * p->textScale / 64.0);
    p->needClose = true;
    return 0;
}
