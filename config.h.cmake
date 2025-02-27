/* config.h.  Generated from config.h.cmake by cmake.  */

/* Build against libcurl. */
#cmakedefine ENABLE_LIBCURL 1

/* Use libjpeg instead of builtin jpeg decoder. */
#cmakedefine ENABLE_LIBJPEG 1

/* Use libopenjpeg instead of builtin jpeg2000 decoder. */
#cmakedefine ENABLE_LIBOPENJPEG 1

/* Build against libtiff. */
#cmakedefine ENABLE_LIBTIFF 1

/* Build against libpng. */
#cmakedefine ENABLE_LIBPNG 1

/* Do not hardcode the library location */
#cmakedefine ENABLE_RELOCATABLE 1

/* Use zlib instead of builtin zlib decoder to uncompress flate streams. */
#cmakedefine ENABLE_ZLIB_UNCOMPRESS 1

/* Build against libnss3 for digital signature validation */
#cmakedefine ENABLE_NSS3 1

/* Build against libgpgme for digital signature validation */
#cmakedefine ENABLE_GPGME 1

/* Enable pgp signatures in GPG backend by default */
#cmakedefine ENABLE_PGP_SIGNATURES 1

/* Signatures enabled */
#cmakedefine ENABLE_SIGNATURES 1

/* Default signature backend */
#cmakedefine DEFAULT_SIGNATURE_BACKEND "${DEFAULT_SIGNATURE_BACKEND}"

/* Use cairo for rendering. */
#cmakedefine HAVE_CAIRO 1

/* Do we have any DCT decoder?. */
#cmakedefine HAVE_DCT_DECODER 1

/* Do we have any JPX decoder?. */
#cmakedefine HAVE_JPX_DECODER 1

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#cmakedefine HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#cmakedefine HAVE_FCNTL_H 1

/* Define to 1 if you have the `fseek64' function. */
#cmakedefine HAVE_FSEEK64 1

/* Define to 1 if fseeko (and presumably ftello) exists and is declared. */
#cmakedefine HAVE_FSEEKO 1

/* Define to 1 if you have the `ftell64' function. */
#cmakedefine HAVE_FTELL64 1

/* Define to 1 if you have the `pread64' function. */
#cmakedefine HAVE_PREAD64 1

/* Define to 1 if you have the `lseek64' function. */
#cmakedefine HAVE_LSEEK64 1

/* Defines if gettimeofday is available on your system */
#cmakedefine HAVE_GETTIMEOFDAY 1

/* Defines if gmtime_r is available on your system */
#cmakedefine HAVE_GMTIME_R 1

/* Defines if timegm is available on your system */
#cmakedefine HAVE_TIMEGM 1

/* Define to 1 if you have the `z' library (-lz). */
#cmakedefine HAVE_LIBZ 1

/* Defines if localtime_r is available on your system */
#cmakedefine HAVE_LOCALTIME_R 1

/* Define to 1 if you have the `mkstemp' function. */
#cmakedefine HAVE_MKSTEMP 1

/* Defines if strtok_r is available on your system */
#cmakedefine HAVE_STRTOK_R 1

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
#cmakedefine HAVE_NDIR_H 1

/* Define to 1 if you have the `popen' function. */
#cmakedefine HAVE_POPEN 1

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
#cmakedefine HAVE_SYS_DIR_H 1

/* Define to 1 if you have the <sys/mman.h> header file. */
#cmakedefine HAVE_SYS_MMAN_H 1

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
#cmakedefine HAVE_SYS_NDIR_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define to 1 if you have a big endian machine */
#cmakedefine WORDS_BIGENDIAN 1

/* Define as const if the declaration of iconv() needs const. */
#define ICONV_CONST ${ICONV_CONST}

/* Generate OPI comments in PS output. */
#cmakedefine OPI_SUPPORT 1

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

/* Support for curl based doc builder is compiled in. */
#cmakedefine POPPLER_HAS_CURL_SUPPORT 1

/* Enable word list support. */
#cmakedefine TEXTOUT_WORD_LIST 1

/* Defines if use cms */
#cmakedefine USE_CMS 1

/* Use single precision arithmetic in the Splash backend */
#cmakedefine USE_FLOAT 1

/* Version number of package */
#define VERSION "${POPPLER_VERSION}"

/* Use fontconfig font configuration backend */
#cmakedefine WITH_FONTCONFIGURATION_FONTCONFIG 1

/* Use win32 font configuration backend */
#cmakedefine WITH_FONTCONFIGURATION_WIN32 1

/* Use android font configuration backend */
#cmakedefine WITH_FONTCONFIGURATION_ANDROID 1

/* OpenJPEG with the OPJ_DPARAMETERS_IGNORE_PCLR_CMAP_CDEF_FLAG flag */
#cmakedefine WITH_OPENJPEG_IGNORE_PCLR_CMAP_CDEF_FLAG 1

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
