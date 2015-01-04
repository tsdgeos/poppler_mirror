//========================================================================
//
// JPEG2000Stream.cc
//
// A JPX stream decoder using OpenJPEG
//
// Copyright 2008-2010, 2012 Albert Astals Cid <aacid@kde.org>
// Copyright 2011 Daniel Glöckner <daniel-gl@gmx.net>
// Copyright 2013,2014 Adrian Johnson <ajohnson@redneon.com>
//
// Licensed under GPLv2 or later
//
//========================================================================

#include "config.h"
#include "JPEG2000Stream.h"
#include <openjpeg.h>

#define OPENJPEG_VERSION_ENCODE(major, minor, micro) (	\
	  ((major) * 10000)				\
	+ ((minor) *   100)				\
	+ ((micro) *     1))

#ifdef USE_OPENJPEG2
#ifdef OPJ_VERSION_MAJOR
#define OPENJPEG_VERSION OPENJPEG_VERSION_ENCODE(OPJ_VERSION_MAJOR, OPJ_VERSION_MINOR, OPJ_VERSION_BUILD)
#else
// OpenJPEG started providing version macros in version 2.1.
// If the version macro is not found, set the version to 2.0.0 and
// assume there will be no API changes in 2.0.x.
#define OPENJPEG_VERSION OPENJPEG_VERSION_ENCODE(2, 0, 0)
#endif
#endif

struct JPXStreamPrivate {
  opj_image_t *image;
  int counter;
  int ccounter;
  int npixels;
  int ncomps;
  GBool inited;
#ifdef USE_OPENJPEG1
  opj_dinfo_t *dinfo;
  void init2(unsigned char *buf, int bufLen, OPJ_CODEC_FORMAT format);
#endif
#ifdef USE_OPENJPEG2
  void init2(opj_stream_t *stream, OPJ_CODEC_FORMAT format);
#endif
};

JPXStream::JPXStream(Stream *strA) : FilterStream(strA) {
  priv = new JPXStreamPrivate;
  priv->inited = gFalse;
  priv->image = NULL;
  priv->npixels = 0;
  priv->ncomps = 0;
#ifdef USE_OPENJPEG1
  priv->dinfo = NULL;
#endif
}

JPXStream::~JPXStream() {
  delete str;
  close();
  delete priv;
}

void JPXStream::reset() {
  priv->counter = 0;
  priv->ccounter = 0;
}

void JPXStream::close() {
  if (priv->image != NULL) {
    opj_image_destroy(priv->image);
    priv->image = NULL;
    priv->npixels = 0;
  }

#ifdef USE_OPENJPEG1
  if (priv->dinfo != NULL) {
    opj_destroy_decompress(priv->dinfo);
    priv->dinfo = NULL;
  }
#endif
}

Goffset JPXStream::getPos() {
  return priv->counter * priv->ncomps + priv->ccounter;
}

int JPXStream::getChars(int nChars, Guchar *buffer) {
  for (int i = 0; i < nChars; ++i) {
    const int c = doGetChar();
    if (likely(c != EOF)) buffer[i] = c;
    else return i;
  }
  return nChars;
}

int JPXStream::getChar() {
  return doGetChar();
}

int JPXStream::doLookChar() {
  if (unlikely(priv->inited == gFalse))
    init();

  if (unlikely(priv->counter >= priv->npixels))
    return EOF;

  return ((unsigned char *)priv->image->comps[priv->ccounter].data)[priv->counter];
}

int JPXStream::lookChar() {
  return doLookChar();
}

int JPXStream::doGetChar() {
  int result = doLookChar();
  if (++priv->ccounter == priv->ncomps) {
    priv->ccounter = 0;
    ++priv->counter;
  }
  return result;
}

GooString *JPXStream::getPSFilter(int psLevel, const char *indent) {
  return NULL;
}

GBool JPXStream::isBinary(GBool last) {
  return str->isBinary(gTrue);
}

void JPXStream::getImageParams(int *bitsPerComponent, StreamColorSpaceMode *csMode) {
  if (priv->inited == gFalse)
    init();

  *bitsPerComponent = 8;
  if (priv->image && priv->image->numcomps == 3)
    *csMode = streamCSDeviceRGB;
  else
    *csMode = streamCSDeviceGray;
}


static void libopenjpeg_error_callback(const char *msg, void * /*client_data*/) {
  error(errSyntaxError, -1, "{0:s}", msg);
}

static void libopenjpeg_warning_callback(const char *msg, void * /*client_data*/) {
  error(errSyntaxWarning, -1, "{0:s}", msg);
}

#ifdef USE_OPENJPEG1

#define BUFFER_INITIAL_SIZE 4096

void JPXStream::init()
{
  Object oLen;
  if (getDict()) getDict()->lookup("Length", &oLen);

  int bufSize = BUFFER_INITIAL_SIZE;
  if (oLen.isInt()) bufSize = oLen.getInt();
  oLen.free();

  int length = 0;
  unsigned char *buf = str->toUnsignedChars(&length, bufSize);
  priv->init2(buf, length, CODEC_JP2);
  free(buf);

  if (priv->image) {
    priv->npixels = priv->image->comps[0].w * priv->image->comps[0].h;
    priv->ncomps = priv->image->numcomps;
    for (int component = 0; component < priv->ncomps; component++) {
      if (priv->image->comps[component].data == NULL) {
        close();
        break;
      }
      unsigned char *cdata = (unsigned char *)priv->image->comps[component].data;
      int adjust = 0;
      if (priv->image->comps[component].prec > 8)
	adjust = priv->image->comps[component].prec - 8;
      int sgndcorr = 0;
      if (priv->image->comps[component].sgnd)
	sgndcorr = 1 << (priv->image->comps[0].prec - 1);
      for (int i = 0; i < priv->npixels; i++) {
	int r = priv->image->comps[component].data[i];
	r += sgndcorr;
	if (adjust) {
	  r = (r >> adjust)+((r >> (adjust-1))%2);
	  if (unlikely(r > 255))
	    r = 255;
        }
	*(cdata++) = r;
      }
    }
  } else
    priv->npixels = 0;

  priv->counter = 0;
  priv->ccounter = 0;
  priv->inited = gTrue;
}

void JPXStreamPrivate::init2(unsigned char *buf, int bufLen, OPJ_CODEC_FORMAT format)
{
  opj_cio_t *cio = NULL;

  /* Use default decompression parameters */
  opj_dparameters_t parameters;
  opj_set_default_decoder_parameters(&parameters);
#ifdef WITH_OPENJPEG_IGNORE_PCLR_CMAP_CDEF_FLAG
  parameters.flags = OPJ_DPARAMETERS_IGNORE_PCLR_CMAP_CDEF_FLAG;
#endif

  /* Configure the event manager to receive errors and warnings */
  opj_event_mgr_t event_mgr;
  memset(&event_mgr, 0, sizeof(opj_event_mgr_t));
  event_mgr.error_handler = libopenjpeg_error_callback;
  event_mgr.warning_handler = libopenjpeg_warning_callback;

  /* Get the decoder handle of the format */
  dinfo = opj_create_decompress(format);
  if (dinfo == NULL) goto error;

  /* Catch events using our callbacks */
  opj_set_event_mgr((opj_common_ptr)dinfo, &event_mgr, NULL);

  /* Setup the decoder decoding parameters */
  opj_setup_decoder(dinfo, &parameters);

  /* Open a byte stream */
  cio = opj_cio_open((opj_common_ptr)dinfo, buf, bufLen);
  if (cio == NULL) goto error;

  /* Decode the stream and fill the image structure */
  image = opj_decode(dinfo, cio);

  /* Close the byte stream */
  opj_cio_close(cio);

  if (image == NULL) goto error;
  else return;

error:
  if (format == CODEC_JP2) {
    error(errSyntaxWarning, -1, "Did no succeed opening JPX Stream as JP2, trying as J2K.");
    init2(buf, bufLen, CODEC_J2K);
  } else if (format == CODEC_J2K) {
    error(errSyntaxWarning, -1, "Did no succeed opening JPX Stream as J2K, trying as JPT.");
    init2(buf, bufLen, CODEC_JPT);
  } else {
    error(errSyntaxError, -1, "Did no succeed opening JPX Stream.");
  }
}
#endif


#ifdef USE_OPENJPEG2
static OPJ_SIZE_T readStream_callback(void *buffer, OPJ_SIZE_T nBytes, void *userData)
{
  int len;
  JPXStream *p = (JPXStream *)userData;

  len = p->readStream(nBytes, (Guchar*)buffer);
  if (len == 0)
    return (OPJ_SIZE_T)-1;
  else
    return len;
}

void JPXStream::init()
{
  opj_stream_t *stream;

  str->reset();
  stream = opj_stream_default_create(OPJ_TRUE);

#if OPENJPEG_VERSION >= OPENJPEG_VERSION_ENCODE(2, 1, 0)
  opj_stream_set_user_data (stream, this, NULL);
#else
  opj_stream_set_user_data (stream, this);
#endif

  opj_stream_set_read_function(stream, readStream_callback);
  priv->init2(stream, OPJ_CODEC_JP2);

  opj_stream_destroy(stream);

  if (priv->image) {
    priv->npixels = priv->image->comps[0].w * priv->image->comps[0].h;
    priv->ncomps = priv->image->numcomps;
    for (int component = 0; component < priv->ncomps; component++) {
      if (priv->image->comps[component].data == NULL) {
        close();
        break;
      }
      unsigned char *cdata = (unsigned char *)priv->image->comps[component].data;
      int adjust = 0;
      if (priv->image->comps[component].prec > 8)
	adjust = priv->image->comps[component].prec - 8;
      int sgndcorr = 0;
      if (priv->image->comps[component].sgnd)
	sgndcorr = 1 << (priv->image->comps[0].prec - 1);
      for (int i = 0; i < priv->npixels; i++) {
	int r = priv->image->comps[component].data[i];
	r += sgndcorr;
	if (adjust) {
	  r = (r >> adjust)+((r >> (adjust-1))%2);
	  if (unlikely(r > 255))
	    r = 255;
        }
	*(cdata++) = r;
      }
    }
  } else {
    priv->npixels = 0;
  }

  priv->counter = 0;
  priv->ccounter = 0;
  priv->inited = gTrue;
}

void JPXStreamPrivate::init2(opj_stream_t *stream, OPJ_CODEC_FORMAT format)
{
  opj_codec_t *decoder;

  /* Use default decompression parameters */
  opj_dparameters_t parameters;
  opj_set_default_decoder_parameters(&parameters);
  parameters.flags |= OPJ_DPARAMETERS_IGNORE_PCLR_CMAP_CDEF_FLAG;

  /* Get the decoder handle of the format */
  decoder = opj_create_decompress(format);
  if (decoder == NULL) {
    error(errSyntaxWarning, -1, "Unable to create decoder");
    goto error;
  }

  /* Catch events using our callbacks */
  opj_set_warning_handler(decoder, libopenjpeg_warning_callback, NULL);
  opj_set_error_handler(decoder, libopenjpeg_error_callback, NULL);

  /* Setup the decoder decoding parameters */
  if (!opj_setup_decoder(decoder, &parameters)) {
    error(errSyntaxWarning, -1, "Unable to set decoder parameters");
    goto error;
  }

  /* Decode the stream and fill the image structure */
  image = NULL;
  if (!opj_read_header(stream, decoder, &image)) {
    error(errSyntaxWarning, -1, "Unable to read header");
    goto error;
  }

  /* Optional if you want decode the entire image */
  if (!opj_set_decode_area(decoder, image, parameters.DA_x0,
                           parameters.DA_y0, parameters.DA_x1, parameters.DA_y1)){
    error(errSyntaxWarning, -1, "X2");
    goto error;
  }

  /* Get the decoded image */
  if (!(opj_decode(decoder, stream, image) && opj_end_decompress(decoder, stream))) {
    error(errSyntaxWarning, -1, "Unable to decode image");
    goto error;
  }

  opj_destroy_codec(decoder);

  if (image != NULL)
    return;

error:
  if (format == OPJ_CODEC_JP2) {
    error(errSyntaxWarning, -1, "Did no succeed opening JPX Stream as JP2, trying as J2K.");
    init2(stream, OPJ_CODEC_J2K);
  } else if (format == OPJ_CODEC_J2K) {
    error(errSyntaxWarning, -1, "Did no succeed opening JPX Stream as J2K, trying as JPT.");
    init2(stream, OPJ_CODEC_JPT);
  } else {
    error(errSyntaxError, -1, "Did no succeed opening JPX Stream.");
  }
}
#endif
