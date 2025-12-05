/* config.h.  Generated from config.h.cmake by cmake.  */

#include "poppler-config.h"

/* Build against libcurl. */
#cmakedefine01 ENABLE_LIBCURL

/* Use libopenjpeg instead of builtin jpeg2000 decoder. */
#cmakedefine01 ENABLE_LIBOPENJPEG

/* Do not hardcode the library location */
#cmakedefine01 ENABLE_RELOCATABLE

/* Build against libnss3 for digital signature validation */
#cmakedefine01 ENABLE_NSS3

/* Build against libgpgme for digital signature validation */
#cmakedefine01 ENABLE_GPGME

/* Enable pgp signatures in GPG backend by default */
#cmakedefine01 ENABLE_PGP_SIGNATURES

/* Signatures enabled */
#cmakedefine01 ENABLE_SIGNATURES

/* Default signature backend */
#cmakedefine DEFAULT_SIGNATURE_BACKEND "${DEFAULT_SIGNATURE_BACKEND}"

/* Do we have any DCT decoder?. */
#cmakedefine01 HAVE_DCT_DECODER

/* Do we have any JPX decoder?. */
#cmakedefine01 HAVE_JPX_DECODER

/* Define to 1 if you have the `fseek64' function. */
#cmakedefine01 HAVE_FSEEK64

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#cmakedefine01 HAVE_FSEEKO

/* Define to 1 if you have the `pread64' function. */
#cmakedefine01 HAVE_PREAD64

/* Define to 1 if you have the `lseek64' function. */
#cmakedefine01 HAVE_LSEEK64

/* Defines if gmtime_r is available on your system */
#cmakedefine01 HAVE_GMTIME_R

/* Defines if timegm is available on your system */
#cmakedefine01 HAVE_TIMEGM

/* Defines if localtime_r is available on your system */
#cmakedefine01 HAVE_LOCALTIME_R

/* Defines if strtok_r is available on your system */
#cmakedefine01 HAVE_STRTOK_R

/* Define to 1 if you have the `popen' function. */
#cmakedefine01 HAVE_POPEN

/* Define to 1 if you have a big endian machine */
#cmakedefine01 WORDS_BIGENDIAN

/* Define as const if the declaration of iconv() needs const. */
#define ICONV_CONST ${ICONV_CONST}

/* Name of package */
#define PACKAGE "poppler"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "https://bugs.freedesktop.org/enter_bug.cgi?product=poppler"

/* Define to the full name of this package. */
#define PACKAGE_NAME "poppler"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "poppler ${POPPLER_VERSION}"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "poppler"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "${POPPLER_VERSION}"

/* Poppler data dir */
#define POPPLER_DATADIR "${POPPLER_DATADIR}"

/* Version number of package */
#define VERSION "${POPPLER_VERSION}"

/* Use fontconfig font configuration backend */
#cmakedefine01 WITH_FONTCONFIGURATION_FONTCONFIG

/* Use win32 font configuration backend */
#cmakedefine01 WITH_FONTCONFIGURATION_WIN32

/* Use android font configuration backend */
#cmakedefine01 WITH_FONTCONFIGURATION_ANDROID

/* MS defined snprintf as deprecated but then added it in Visual Studio 2015. */
#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf
#endif

//------------------------------------------------------------------------
// popen
//------------------------------------------------------------------------
#if defined(_MSC_VER) || defined(__BORLANDC__)
#define popen _popen
#define pclose _pclose
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

/* Number of bits in a file offset, on hosts where this is settable. */
#cmakedefine _FILE_OFFSET_BITS @_FILE_OFFSET_BITS@

/* Define to 1 to make fseeko visible on some hosts (e.g. glibc 2.2). */
/* TODO This is wrong, port if needed #undef _LARGEFILE_SOURCE */

/* Define for large files, on AIX-style hosts. */
/* TODO This is wrong, port if needed #undef _LARGE_FILES */
