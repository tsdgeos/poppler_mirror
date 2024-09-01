cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)

project(poppler)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

include(PopplerDefaults)
include(PopplerMacros)
# Ensure that the user-provided C_FLAGS are used for try_compile() calls.
# This is needed since PopplerMacros.cmake clears CMAKE_C_FLAGS and if
# CMAKE_TRY_COMPILE_CONFIGURATION is empty CMake only uses the flags
# specified in CMAKE_C_FLAGS (https://gitlab.kitware.com/cmake/cmake/-/issues/22414
# and https://gitlab.kitware.com/cmake/cmake/-/issues/19512).
# We therefore have to explicitly set CMAKE_TRY_COMPILE_CONFIGURATION until we
# depend on a CMake release that includes a fix for those issues.
# This is set after including PopplerMacros since that sets the default
# CMAKE_BUILD_TYPE and also sets _CMAKE_BUILD_TYPE_UPPER.
set(CMAKE_TRY_COMPILE_CONFIGURATION "${_CMAKE_BUILD_TYPE_UPPER}")

include(MacroOptionalFindPackage)
find_package(PkgConfig)
include(TestBigEndian)
test_big_endian(WORDS_BIGENDIAN)
include(CheckFileOffsetBits)
CHECK_FILE_OFFSET_BITS()

include(GenerateExportHeader)
include(GNUInstallDirs)
include(CMakePushCheckState)

set(ENABLE_FUZZER FALSE)

find_package (ECM 1.6.0 QUIET NO_MODULE)
if (ECM_FOUND)
    include("${ECM_MODULE_DIR}/ECMEnableSanitizers.cmake")

    if(ECM_ENABLE_SANITIZERS MATCHES fuzzer)
        set(ENABLE_FUZZER TRUE)
    endif()
endif()

set(POPPLER_MAJOR_VERSION "24")
set(POPPLER_MINOR_VERSION_STRING "09")
# We want the string version to have 08 but the integer version can't have a leading 0 since otherwise it's considered octal
# So strip a leading 0 if found in POPPLER_MINOR_VERSION_STRING and store the result in POPPLER_MINOR_VERSION
string(REGEX REPLACE "^0?(.+)$" "\\1" POPPLER_MINOR_VERSION "${POPPLER_MINOR_VERSION_STRING}")
set(POPPLER_MICRO_VERSION "0")
set(POPPLER_VERSION "${POPPLER_MAJOR_VERSION}.${POPPLER_MINOR_VERSION_STRING}.${POPPLER_MICRO_VERSION}")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_LINK_DEPENDS_NO_SHARED TRUE)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)


# command line switches
option(ENABLE_UNSTABLE_API_ABI_HEADERS "Install API/ABI unstable xpdf headers." OFF)
option(BUILD_GTK_TESTS "Whether to compile the GTK+ test programs." ON)
option(BUILD_QT5_TESTS "Whether to compile the Qt5 test programs." ON)
option(BUILD_QT6_TESTS "Whether to compile the Qt6 test programs." ON)
option(BUILD_CPP_TESTS "Whether to compile the CPP test programs." ON)
option(BUILD_MANUAL_TESTS "Whether to compile manual test programs." ON)
option(ENABLE_BOOST "Use boost (for Splash backend performance)." ON)
option(ENABLE_UTILS "Compile poppler command line utils." ON)
option(ENABLE_CPP "Compile poppler cpp wrapper." ON)
option(ENABLE_GLIB "Compile poppler glib wrapper." ON)
option(ENABLE_GOBJECT_INTROSPECTION "Whether to generate GObject introspection." ON)
option(ENABLE_GTK_DOC "Whether to generate glib API documentation." OFF)
option(ENABLE_QT5 "Compile poppler qt5 wrapper." ON)
option(ENABLE_QT6 "Compile poppler qt6 wrapper." ON)
set(ENABLE_LIBOPENJPEG "openjpeg2" CACHE STRING "Use libopenjpeg for JPX streams. Possible values: openjpeg2, unmaintained, none. 'unmaintained' gives you the internal unmaintained decoder. Use at your own risk. 'none' compiles no JPX decoder at all. Default: openjpeg2")
set(ENABLE_DCTDECODER "libjpeg" CACHE STRING "Use libjpeg for DCT streams. Possible values: libjpeg, unmaintained, none. will use libjpeg if available or fail if not. 'unmaintained' gives you the internal unmaintained decoder. Use at your own risk. 'none' compiles no DCT decoder at all. Default: libjpeg")
option(ENABLE_LCMS "Use LCMS for color management." ON)
option(ENABLE_LIBCURL "Build libcurl based HTTP support." ON)
option(ENABLE_LIBTIFF "Build code to write images as TIFF (pdfimages/pdftocairo/etc)." ON)
option(ENABLE_NSS3 "Build the NSS backend for cryptographic support" ON)
option(ENABLE_GPGME "Build the GPG backend for cryptographic support" ON)
option(ENABLE_ZLIB_UNCOMPRESS "Use zlib to uncompress flate streams (not totally safe)." OFF)
option(USE_FLOAT "Use single precision arithmetic in the Splash backend" OFF)
option(BUILD_SHARED_LIBS "Build poppler as a shared library" ON)
option(RUN_GPERF_IF_PRESENT "Run gperf if it is found" ON)
if(WIN32)
  option(ENABLE_RELOCATABLE "Do not hardcode the poppler library location (on Windows)." ON)
else()
  set(ENABLE_RELOCATABLE OFF)
endif()
option(EXTRA_WARN "Enable extra compile warnings" OFF)

set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(TESTDATADIR "${CMAKE_SOURCE_DIR}/../test" CACHE STRING "Specify test data dir.")
if(NOT (EXISTS ${TESTDATADIR} AND EXISTS ${TESTDATADIR}/test-poppler.c))
  message(WARNING "
 No test data found in $testdatadir.
 You will not be able to run 'make test' successfully.

 The test data is not included in the source packages
 and is also not part of the main git repository. Instead,
 you can checkout the test data from its own git
 repository with:

   git clone git://git.freedesktop.org/git/poppler/test

 You should checkout the test data as a sibling of your
 poppler source folder or specify the location of your
 checkout with -DTESTDATADIR=/path/to/checkoutdir/test.
  ")
endif()

if(WIN32)
  set(_default_fontconfiguration "win32")
elseif(ANDROID)
  set(_default_fontconfiguration "android")
else()
  set(_default_fontconfiguration "fontconfig")
endif()
set(FONT_CONFIGURATION "${_default_fontconfiguration}" CACHE STRING "The font configuration backend (win32|android|generic|fontconfig).")
string(TOLOWER "${FONT_CONFIGURATION}" font_configuration)
set(WITH_FONTCONFIGURATION_WIN32 OFF)
set(WITH_FONTCONFIGURATION_FONTCONFIG OFF)
set(WITH_FONTCONFIGURATION_ANDROID OFF)
if(font_configuration STREQUAL "win32")
  set(WITH_FONTCONFIGURATION_WIN32 ON)
elseif(font_configuration STREQUAL "fontconfig")
  set(WITH_FONTCONFIGURATION_FONTCONFIG ON)
elseif(font_configuration STREQUAL "android")
  set(WITH_FONTCONFIGURATION_ANDROID ON)
elseif(font_configuration STREQUAL "generic")
  message(STATUS "no win32, android, or fontconfig specific code")
else()
  message(FATAL_ERROR "Invalid font configuration setting: ${FONT_CONFIGURATION}")
endif()

# Enable these unconditionally.
set(OPI_SUPPORT ON)
set(TEXTOUT_WORD_LIST ON)

# setting the minimum required versions for some components
set(CAIRO_VERSION "1.16.0")
set(GLIB_REQUIRED "2.72")
set(GTK_REQUIRED "3.24")
set(GDK_PIXBUF_REQUIRED "2.40")
set(FREETYPE_VERSION "2.11")
set(FONTCONFIG_VERSION "2.13")

find_package(Freetype ${FREETYPE_VERSION} REQUIRED)
if(WITH_FONTCONFIGURATION_FONTCONFIG)
  find_package(Fontconfig ${FONTCONFIG_VERSION} REQUIRED)
elseif(WITH_FONTCONFIGURATION_ANDROID)
  find_library(Androidlib NAMES android REQUIRED)
endif()

macro(find_soft_mandatory_package _enable_option _package_name _package_version)
  if(${_enable_option})
    find_package(${_package_name} ${_package_version})
    if(NOT ${_package_name}_FOUND)
      MESSAGE(FATAL_ERROR "Could not find the ${_package_version} version of ${_package_name}. If you're not interested in the features it provides set the cmake ${_enable_option} option to OFF")
    endif()
  endif()
endmacro()

find_soft_mandatory_package(ENABLE_NSS3 NSS3 3.68)
find_soft_mandatory_package(ENABLE_GPGME Gpgmepp 1.19)
find_soft_mandatory_package(ENABLE_LIBTIFF TIFF 4.3)

macro_optional_find_package(JPEG)
macro_optional_find_package(PNG)
if(ENABLE_DCTDECODER STREQUAL "libjpeg")
  if(JPEG_FOUND)
    include(CheckCSourceCompiles)
    set(_save_CMAKE_REQUIRED_LIBRARIES "${CMAKE_REQUIRED_LIBRARIES}")
    set(CMAKE_REQUIRED_LIBRARIES JPEG::JPEG)
    check_c_source_compiles("
    #include <stddef.h>
    #include <stdio.h>
    #include <jpeglib.h>
    int main() { struct jpeg_decompress_struct info; jpeg_mem_src(&info, 0, 0); return 0; }" HAVE_JPEG_MEM_SRC)
    set(CMAKE_REQUIRED_LIBRARIES "${_save_CMAKE_REQUIRED_LIBRARIES}")

    if(NOT HAVE_JPEG_MEM_SRC)
      message(FATAL_ERROR "Your libjpeg is too old. Poppler needs one that provides jpeg_mem_src. That is provided in libjpeg >= 8 or libjpeg-turbo >= 1.1.0. You can \
also decide to use the internal unmaintained DCT decoder or none at all.\n\
Possible options are: -DENABLE_DCTDECODER=libjpeg, -DENABLE_DCTDECODER=none, \
-DENABLE_DCTDECODER=unmaintained")
    endif()

    set(ENABLE_LIBJPEG ${JPEG_FOUND})
  else()
    message(STATUS "Could NOT find libjpeg.")
    message(FATAL_ERROR "Install libjpeg before trying to build poppler. You can \
also decide to use the internal unmaintained DCT decoder or none at all.\n\
Possible options are: -DENABLE_DCTDECODER=libjpeg, -DENABLE_DCTDECODER=none, \
-DENABLE_DCTDECODER=unmaintained")
  endif()
  set(HAVE_DCT_DECODER ON)
elseif(ENABLE_DCTDECODER STREQUAL "unmaintained")
  set(ENABLE_LIBJPEG OFF)
  set(HAVE_DCT_DECODER ON)
elseif(ENABLE_DCTDECODER STREQUAL "none")
  set(ENABLE_LIBJPEG OFF)
  set(HAVE_DCT_DECODER OFF)
else()
  message(FATAL_ERROR "Invalid ENABLE_DCTDECODER value.")
endif()

set(QT5_VERSION "5.15")
find_soft_mandatory_package(ENABLE_QT5 Qt5Core ${QT5_VERSION})
find_soft_mandatory_package(ENABLE_QT5 Qt5Gui ${QT5_VERSION})
find_soft_mandatory_package(ENABLE_QT5 Qt5Xml ${QT5_VERSION})
find_soft_mandatory_package(ENABLE_QT5 Qt5Widgets ${QT5_VERSION})
find_soft_mandatory_package(ENABLE_QT5 Qt5Test ${QT5_VERSION})

set(QT6_VERSION "6.2")
SET(QT_NO_CREATE_VERSIONLESS_TARGETS ON)
find_soft_mandatory_package(ENABLE_QT6 Qt6Core ${QT6_VERSION})
find_soft_mandatory_package(ENABLE_QT6 Qt6Gui ${QT6_VERSION})
find_soft_mandatory_package(ENABLE_QT6 Qt6Widgets ${QT6_VERSION})
find_soft_mandatory_package(ENABLE_QT6 Qt6Test ${QT6_VERSION})

# Check for Cairo rendering backend
macro_optional_find_package(Cairo ${CAIRO_VERSION})

find_package(Boost 1.74.0)
if(Boost_FOUND)
  set(USE_BOOST_HEADERS ON)
elseif(ENABLE_BOOST)
  message(FATAL_ERROR "-- Boost recommended for Splash. Use ENABLE_BOOST=OFF to skip.")
endif()

if(CAIRO_FOUND)
  set(HAVE_CAIRO ${CAIRO_FOUND})
  set(CAIRO_FEATURE "#define POPPLER_HAS_CAIRO 1")
  set(CAIRO_REQ "cairo")
  set(POPPLER_GLIB_DISABLE_DEPRECATED "")
  set(POPPLER_GLIB_DISABLE_SINGLE_INCLUDES "")
  if(ENABLE_GLIB)
    macro_optional_find_package(GLIB)
    if(NOT GLIB_FOUND)
      set(ENABLE_GLIB OFF)
    endif()
  endif()
  if(ENABLE_GLIB)
    if(ENABLE_GOBJECT_INTROSPECTION)
      # Check for introspection
      macro_optional_find_package(GObjectIntrospection 1.72.0)
      set(HAVE_INTROSPECTION ${INTROSPECTION_FOUND})
    endif()
    set(POPPLER_GLIB_DISABLE_DEPRECATED "${POPPLER_GLIB_DISABLE_DEPRECATED} -DG_DISABLE_DEPRECATED")
    set(POPPLER_GLIB_DISABLE_SINGLE_INCLUDES "${POPPLER_GLIB_DISABLE_SINGLE_INCLUDES} -DG_DISABLE_SINGLE_INCLUDES")
    macro_optional_find_package(GTK)
  endif()
else()
  set(CAIRO_FEATURE "#undef POPPLER_HAS_CAIRO")
  set(ENABLE_GLIB OFF)
endif()

# GTK API docs require both the gtk-doc package & python3 support
if(ENABLE_GTK_DOC)
  # Stop the build & raise an error if the package is missing
  find_package(GtkDoc)
  if(NOT GtkDoc_FOUND)
    message(FATAL_ERROR "Install the gtk-doc package to generate GTK API documentation, or set ENABLE_GTK_DOC to Off.")
  endif()
  # NOTE: The FindPythonInterp module is deprecated, but the newer FindPython3 module requires CMake >=3.12
  find_package(PythonInterp 3)
  # Also bail out with an error if Python3 is missing
  if(NOT PYTHONINTERP_FOUND)
    message(FATAL_ERROR "Install python3 in order to generate GTK API documentation, or set ENABLE_GTK_DOC to Off.")
  endif()
endif()

if(ENABLE_CPP)
  cmake_push_check_state()
  find_package(Iconv REQUIRED)
  set(CMAKE_REQUIRED_LIBRARIES Iconv::Iconv)
  check_cxx_source_compiles("
    #include <iconv.h>
    int main(){
      iconv_t conv = 0;
      const char* in = 0;
      size_t ilen = 0;
      char* out = 0;
      size_t olen = 0;
      iconv(conv, &in, &ilen, &out, &olen);
      return 0;
    }
  " ICONV_SECOND_ARGUMENT_IS_CONST)
  cmake_pop_check_state()
  if(ICONV_SECOND_ARGUMENT_IS_CONST)
    set(ICONV_CONST "const")
  endif()
endif()
find_package(ZLIB REQUIRED)

set(WITH_OPENJPEG FALSE)
if(ENABLE_LIBOPENJPEG STREQUAL "openjpeg2")
  find_package(OpenJPEG)
  set(WITH_OPENJPEG ${OpenJPEG_FOUND})
  if(NOT OpenJPEG_FOUND OR OPENJPEG_MAJOR_VERSION VERSION_LESS 2)
    message(STATUS "Could NOT find openjpeg2.")
    message(FATAL_ERROR "Install libopenjpeg2 before trying to build poppler. You \
can also decide to use the internal unmaintained JPX decoder or none at all.\n\
Possible options are: -DENABLE_LIBOPENJPEG=openjpeg2, -DENABLE_LIBOPENJPEG=none, \
-DENABLE_LIBOPENJPEG=unmaintained,")
  endif()
  set(HAVE_JPX_DECODER ON)
elseif(ENABLE_LIBOPENJPEG STREQUAL "unmaintained")
  set(WITH_OPENJPEG OFF)
  set(HAVE_JPX_DECODER ON)
elseif(ENABLE_LIBOPENJPEG STREQUAL "none")
  set(WITH_OPENJPEG OFF)
  set(HAVE_JPX_DECODER OFF)
else()
  message(FATAL_ERROR "Invalid ENABLE_LIBOPENJPEG value: ${ENABLE_LIBOPENJPEG}")
endif()
set(ENABLE_LIBOPENJPEG "${WITH_OPENJPEG}")

find_soft_mandatory_package(ENABLE_LCMS LCMS2 2.9)
set(USE_CMS ${ENABLE_LCMS})

find_soft_mandatory_package(ENABLE_LIBCURL CURL 7.68)
set(POPPLER_HAS_CURL_SUPPORT ${ENABLE_LIBCURL})

if(MINGW)
  # Use mingw's ansi stdio extensions
  add_definitions(-D__USE_MINGW_ANSI_STDIO=1)
endif()
if(WITH_FONTCONFIGURATION_WIN32)
  if(MINGW)
    # Set the minimum required Internet Explorer version to 5.0
    add_definitions(-D_WIN32_IE=0x0500)
  endif()
endif()
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/fofi
  ${CMAKE_CURRENT_SOURCE_DIR}/goo
  ${CMAKE_CURRENT_SOURCE_DIR}/poppler
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/poppler
)

if(PNG_FOUND)
  set(ENABLE_LIBPNG ON)
endif()

set(SIGNATURE_BACKENDS "")
if(ENABLE_NSS3)
  list(APPEND SIGNATURE_BACKENDS "NSS")
endif()
if(ENABLE_GPGME)
  list(APPEND SIGNATURE_BACKENDS "GPG")
endif()

list(LENGTH SIGNATURE_BACKENDS _signing_backends_count)
if (_signing_backends_count GREATER 0)
  if (NOT DEFAULT_SIGNATURE_BACKEND)
    # If not specified at compiletime, we take the first one added.
    # This means that the order we append them to the list is significant
    list(GET SIGNATURE_BACKENDS 0 DEFAULT_SIGNATURE_BACKEND)
  endif()
  set(ENABLE_SIGNATURES ON)
endif()
if (NOT DEFAULT_SIGNATURE_BACKEND)
  set(DEFAULT_SIGNATURE_BACKEND "None")
endif()

# Recent versions of poppler-data install a .pc file.
# Use it to determine the encoding data path, if available.
# Default to the same prefix otherwise.
pkg_check_modules(POPPLER_DATA poppler-data)
if(POPPLER_DATA_FOUND)
  execute_process(COMMAND "${PKG_CONFIG_EXECUTABLE}" --variable=poppler_datadir poppler-data
                  RESULT_VARIABLE _result_var
                  OUTPUT_VARIABLE _output_var OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(_result_var STREQUAL "0" AND NOT _output_var STREQUAL "")
    set(POPPLER_DATADIR "${_output_var}")
  endif()
endif()
if(NOT DEFINED POPPLER_DATADIR)
  set(POPPLER_DATADIR "${CMAKE_INSTALL_PREFIX}/share/poppler")
endif()

if(EXTRA_WARN)
  set(CMAKE_C_FLAGS "-Wall ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${DEFAULT_COMPILE_WARNINGS_EXTRA} ${CMAKE_CXX_FLAGS}")
else()
  set(CMAKE_C_FLAGS "-Wall ${CMAKE_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${DEFAULT_COMPILE_WARNINGS} ${CMAKE_CXX_FLAGS}")
endif()

include(ConfigureChecks.cmake)
configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(poppler/poppler-config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/poppler/poppler-config.h)

find_program(GPERF gperf)
find_program(SED sed)

set(poppler_SRCS
  goo/GooString.cc
  goo/GooTimer.cc
  goo/ImgWriter.cc
  goo/JpegWriter.cc
  goo/NetPBMWriter.cc
  goo/PNGWriter.cc
  goo/TiffWriter.cc
  goo/ft_utils.cc
  goo/gbase64.cc
  goo/gbasename.cc
  goo/gfile.cc
  goo/glibc.cc
  goo/glibc_strtok_r.cc
  goo/grandom.cc
  goo/gstrtod.cc
  fofi/FoFiBase.cc
  fofi/FoFiEncodings.cc
  fofi/FoFiTrueType.cc
  fofi/FoFiType1.cc
  fofi/FoFiType1C.cc
  fofi/FoFiIdentifier.cc
  poppler/Annot.cc
  poppler/AnnotStampImageHelper.cc
  poppler/Array.cc
  poppler/CachedFile.cc
  poppler/Catalog.cc
  poppler/CharCodeToUnicode.cc
  poppler/CMap.cc
  poppler/CryptoSignBackend.cc
  poppler/DateInfo.cc
  poppler/Decrypt.cc
  poppler/Dict.cc
  poppler/Error.cc
  poppler/FDPDFDocBuilder.cc
  poppler/FILECacheLoader.cc
  poppler/FileSpec.cc
  poppler/FlateEncoder.cc
  poppler/FontEncodingTables.cc
  poppler/Form.cc
  poppler/FontInfo.cc
  poppler/Function.cc
  poppler/Gfx.cc
  poppler/GfxFont.cc
  poppler/GfxState.cc
  poppler/GlobalParams.cc
  poppler/Hints.cc
  poppler/ImageEmbeddingUtils.cc
  poppler/JArithmeticDecoder.cc
  poppler/JBIG2Stream.cc
  poppler/JSInfo.cc
  poppler/Lexer.cc
  poppler/Link.cc
  poppler/Linearization.cc
  poppler/LocalPDFDocBuilder.cc
  poppler/MarkedContentOutputDev.cc
  poppler/NameToCharCode.cc
  poppler/Object.cc
  poppler/OptionalContent.cc
  poppler/Outline.cc
  poppler/OutputDev.cc
  poppler/Page.cc
  poppler/PageTransition.cc
  poppler/Parser.cc
  poppler/PDFDoc.cc
  poppler/PDFDocBuilder.cc
  poppler/PDFDocEncoding.cc
  poppler/PDFDocFactory.cc
  poppler/ProfileData.cc
  poppler/PreScanOutputDev.cc
  poppler/PSTokenizer.cc
  poppler/SignatureInfo.cc
  poppler/Stream.cc
  poppler/StructTreeRoot.cc
  poppler/StructElement.cc
  poppler/UnicodeMap.cc
  poppler/UnicodeMapFuncs.cc
  poppler/UnicodeTypeTable.cc
  poppler/UTF.cc
  poppler/XRef.cc
  poppler/PSOutputDev.cc
  poppler/TextOutputDev.cc
  poppler/PageLabelInfo.cc
  poppler/SecurityHandler.cc
  poppler/Sound.cc
  poppler/ViewerPreferences.cc
  poppler/Movie.cc
  poppler/Rendition.cc
  poppler/CertificateInfo.cc
  poppler/BBoxOutputDev.cc
  poppler/SplashOutputDev.cc
  splash/Splash.cc
  splash/SplashBitmap.cc
  splash/SplashClip.cc
  splash/SplashFTFont.cc
  splash/SplashFTFontEngine.cc
  splash/SplashFTFontFile.cc
  splash/SplashFont.cc
  splash/SplashFontEngine.cc
  splash/SplashFontFile.cc
  splash/SplashFontFileID.cc
  splash/SplashPath.cc
  splash/SplashPattern.cc
  splash/SplashScreen.cc
  splash/SplashState.cc
  splash/SplashXPath.cc
  splash/SplashXPathScanner.cc
)
set(poppler_LIBS Freetype::Freetype ZLIB::ZLIB)
if(FONTCONFIG_FOUND)
  set(poppler_LIBS ${poppler_LIBS} Fontconfig::Fontconfig)
endif()

if(Androidlib) 
  set(poppler_LIBS ${poppler_LIBS} ${Androidlib})
endif()

if(JPEG_FOUND)
  set(poppler_SRCS ${poppler_SRCS}
    poppler/DCTStream.cc
  )
  set(poppler_LIBS ${poppler_LIBS} JPEG::JPEG)
endif()
if(ENABLE_ZLIB_UNCOMPRESS)
  set(poppler_SRCS ${poppler_SRCS}
    poppler/FlateStream.cc
  )
endif()
if(ENABLE_LIBCURL)
  set(poppler_SRCS ${poppler_SRCS}
    poppler/CurlCachedFile.cc
    poppler/CurlPDFDocBuilder.cc
  )
  set(poppler_LIBS ${poppler_LIBS} CURL::libcurl)
endif()
if (ENABLE_NSS3)
  set(poppler_SRCS ${poppler_SRCS}
    poppler/NSSCryptoSignBackend.cc
  )
  set(poppler_LIBS ${poppler_LIBS} PkgConfig::NSS3)
endif()
if (ENABLE_GPGME)
  set(poppler_SRCS ${poppler_SRCS}
    poppler/GPGMECryptoSignBackend.cc
  )
  set(poppler_LIBS ${poppler_LIBS} Gpgmepp)
endif()
if (OpenJPEG_FOUND)
  set(poppler_SRCS ${poppler_SRCS}
    poppler/JPEG2000Stream.cc
  )
  set(poppler_LIBS ${poppler_LIBS} openjp2)
else ()
  set(poppler_SRCS ${poppler_SRCS}
    poppler/JPXStream.cc
  )
endif()
if(USE_CMS)
  set(poppler_LIBS ${poppler_LIBS} ${LCMS2_LIBRARIES})
endif()
if(WIN32)
  # use clean APIs
  add_definitions(-DWIN32_LEAN_AND_MEAN)
  # gdi32 is needed under win32
  set(poppler_LIBS ${poppler_LIBS} gdi32)
endif()
if(PNG_FOUND)
  set(poppler_LIBS ${poppler_LIBS} PNG::PNG)
endif()
if(ENABLE_LIBTIFF)
  set(poppler_LIBS ${poppler_LIBS} TIFF::TIFF)
endif()
if(Boost_FOUND)
  set(poppler_LIBS ${poppler_LIBS} Boost::boost)
endif()

if (GPERF AND SED AND RUN_GPERF_IF_PRESENT)
    macro(ADD_GPERF_FILE input)
        add_custom_command(OUTPUT poppler/${input}.c
                       COMMAND ${GPERF} poppler/${input}.gperf > ${CMAKE_CURRENT_BINARY_DIR}/poppler/${input}.c
                       COMMAND ${GPERF} poppler/${input}.gperf > ${CMAKE_CURRENT_SOURCE_DIR}/poppler/${input}.pregenerated.c
                       COMMAND ${SED} -i -e "s#${GPERF}#gperf#" ${CMAKE_CURRENT_SOURCE_DIR}/poppler/${input}.pregenerated.c
                       COMMAND clang-format -i ${CMAKE_CURRENT_SOURCE_DIR}/poppler/${input}.pregenerated.c || true
                       DEPENDS poppler/${input}.gperf
                       WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

        set(poppler_SRCS ${poppler_SRCS}
            poppler/${input}.c
        )
    endmacro()
else()
    macro(ADD_GPERF_FILE input)
        set(poppler_SRCS ${poppler_SRCS}
            poppler/${input}.pregenerated.c
        )
    endmacro()
endif()

ADD_GPERF_FILE(CourierWidths)
ADD_GPERF_FILE(CourierBoldWidths)
ADD_GPERF_FILE(CourierBoldObliqueWidths)
ADD_GPERF_FILE(CourierObliqueWidths)
ADD_GPERF_FILE(HelveticaWidths)
ADD_GPERF_FILE(HelveticaBoldWidths)
ADD_GPERF_FILE(HelveticaBoldObliqueWidths)
ADD_GPERF_FILE(HelveticaObliqueWidths)
ADD_GPERF_FILE(SymbolWidths)
ADD_GPERF_FILE(TimesBoldWidths)
ADD_GPERF_FILE(TimesBoldItalicWidths)
ADD_GPERF_FILE(TimesItalicWidths)
ADD_GPERF_FILE(TimesRomanWidths)
ADD_GPERF_FILE(ZapfDingbatsWidths)

set(POPPLER_SOVERSION_NUMBER "141")

set(LINKER_SCRIPT "${CMAKE_BINARY_DIR}/libpoppler.map")
configure_file(
    "${CMAKE_SOURCE_DIR}/poppler/libpoppler.map.in"
    ${LINKER_SCRIPT})

if(MSVC)
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
add_library(poppler ${poppler_SRCS} ${LINKER_SCRIPT})
if (OpenJPEG_FOUND)
  # check if we can remove this when we depend on newer openjpeg versions, 2.5 seems fixed
  # target openjp2 may lack interface include directories
  target_include_directories(poppler SYSTEM PRIVATE ${OPENJPEG_INCLUDE_DIRS})
endif()
if(USE_CMS)
  target_include_directories(poppler SYSTEM PRIVATE ${LCMS2_INCLUDE_DIR})
endif()
generate_export_header(poppler BASE_NAME poppler-private EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/poppler_private_export.h")
set_target_properties(poppler PROPERTIES
                              VERSION ${POPPLER_SOVERSION_NUMBER}.0.0
                              SOVERSION ${POPPLER_SOVERSION_NUMBER})

if(UNIX AND (NOT APPLE))
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/linkerscript_test.map "VERS_1 {\nglobal:\n *;};\n")

    # once we require cmake 3.18,
    # the next set of lines can be changed
    # to the check_linker_flags function instead
    include(CheckCXXSourceCompiles)
    set(OLD_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
    set(CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS} -Wl,--version-script=${CMAKE_CURRENT_BINARY_DIR}/linkerscript_test.map)
    check_cxx_source_compiles("
      int main(int, char**) {
        return 0;
      }
    " SUPPORT_VERSION_SCRIPT)
    set(CMAKE_REQUIRED_FLAGS ${OLD_CMAKE_REQUIRED_FLAGS})

    file(REMOVE ${CMAKE_CURRENT_BINARY_DIR}/linkerscript_test.map)
    if (SUPPORT_VERSION_SCRIPT)
      set_target_properties(poppler PROPERTIES LINK_OPTIONS LINKER:--version-script=${LINKER_SCRIPT})
    endif()
endif()
if(MINGW AND BUILD_SHARED_LIBS)
    get_target_property(POPPLER_SOVERSION poppler SOVERSION)
    set_target_properties(poppler PROPERTIES SUFFIX "-${POPPLER_SOVERSION}${CMAKE_SHARED_LIBRARY_SUFFIX}")
endif()
target_link_libraries(poppler LINK_PRIVATE ${poppler_LIBS})
install(TARGETS poppler RUNTIME DESTINATION bin LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(ENABLE_UNSTABLE_API_ABI_HEADERS)
  install(FILES
    poppler/Annot.h
    poppler/AnnotStampImageHelper.h
    poppler/Array.h
    poppler/CachedFile.h
    poppler/Catalog.h
    poppler/CharCodeToUnicode.h
    poppler/CMap.h
    poppler/DateInfo.h
    poppler/Decrypt.h
    poppler/Dict.h
    poppler/Error.h
    poppler/FDPDFDocBuilder.h
    poppler/FILECacheLoader.h
    poppler/FileSpec.h
    poppler/FontEncodingTables.h
    poppler/FontInfo.h
    poppler/Form.h
    poppler/Function.h
    poppler/Gfx.h
    poppler/GfxFont.h
    poppler/GfxState.h
    poppler/GfxState_helpers.h
    poppler/GlobalParams.h
    poppler/Hints.h
    poppler/HashAlgorithm.h
    poppler/JArithmeticDecoder.h
    poppler/JBIG2Stream.h
    poppler/JSInfo.h
    poppler/Lexer.h
    poppler/Link.h
    poppler/Linearization.h
    poppler/LocalPDFDocBuilder.h
    poppler/MarkedContentOutputDev.h
    poppler/Movie.h
    poppler/NameToCharCode.h
    poppler/Object.h
    poppler/OptionalContent.h
    poppler/Outline.h
    poppler/OutputDev.h
    poppler/Page.h
    poppler/PageTransition.h
    poppler/Parser.h
    poppler/PDFDoc.h
    poppler/PDFDocBuilder.h
    poppler/PDFDocEncoding.h
    poppler/PDFDocFactory.h
    poppler/PopplerCache.h
    poppler/ProfileData.h
    poppler/PreScanOutputDev.h
    poppler/PSTokenizer.h
    poppler/Rendition.h
    poppler/CertificateInfo.h
    poppler/SignatureInfo.h
    poppler/Stream-CCITT.h
    poppler/Stream.h
    poppler/StructElement.h
    poppler/StructTreeRoot.h
    poppler/UnicodeMap.h
    poppler/UnicodeMapFuncs.h
    poppler/UnicodeMapTables.h
    poppler/UnicodeTypeTable.h
    poppler/UnicodeCClassTables.h
    poppler/UnicodeCompTables.h
    poppler/UnicodeDecompTables.h
    poppler/ViewerPreferences.h
    poppler/XRef.h
    poppler/CharTypes.h
    poppler/ErrorCodes.h
    poppler/NameToUnicodeTable.h
    poppler/PSOutputDev.h
    poppler/TextOutputDev.h
    poppler/SecurityHandler.h
    poppler/BBoxOutputDev.h
    poppler/UTF.h
    poppler/Sound.h
    ${CMAKE_CURRENT_BINARY_DIR}/poppler_private_export.h
    ${CMAKE_CURRENT_BINARY_DIR}/poppler/poppler-config.h
    poppler/SplashOutputDev.h
    DESTINATION include/poppler)
  install(FILES
    goo/GooTimer.h
    goo/GooString.h
    goo/gmem.h
    goo/gdir.h
    goo/gfile.h
    goo/ImgWriter.h
    goo/GooCheckedOps.h
    goo/GooLikely.h
    goo/gstrtod.h
    goo/grandom.h
    DESTINATION include/poppler/goo)
  if(PNG_FOUND)
    install(FILES
      goo/PNGWriter.h
      DESTINATION include/poppler/goo)
  endif()
  if(ENABLE_LIBTIFF)
    install(FILES
      goo/TiffWriter.h
      DESTINATION include/poppler/goo)
  endif()
  if(JPEG_FOUND)
    install(FILES
      goo/JpegWriter.h
      DESTINATION include/poppler/goo)
  endif()
  install(FILES
    fofi/FoFiBase.h
    fofi/FoFiEncodings.h
    fofi/FoFiTrueType.h
    fofi/FoFiType1.h
    fofi/FoFiType1C.h
    fofi/FoFiIdentifier.h
    DESTINATION include/poppler/fofi)
  if(ENABLE_LIBCURL)
    install(FILES
      poppler/CurlCachedFile.h
      poppler/CurlPDFDocBuilder.h
      DESTINATION include/poppler)
  endif()
  if(OpenJPEG_FOUND)
    install(FILES
      poppler/JPEG2000Stream.h
      DESTINATION include/poppler)
  else()
    install(FILES
      poppler/JPXStream.h
      DESTINATION include/poppler)
  endif()
  install(FILES
    splash/Splash.h
    splash/SplashBitmap.h
    splash/SplashClip.h
    splash/SplashErrorCodes.h
    splash/SplashFTFont.h
    splash/SplashFTFontEngine.h
    splash/SplashFTFontFile.h
    splash/SplashFont.h
    splash/SplashFontEngine.h
    splash/SplashFontFile.h
    splash/SplashFontFileID.h
    splash/SplashGlyphBitmap.h
    splash/SplashMath.h
    splash/SplashPath.h
    splash/SplashPattern.h
    splash/SplashScreen.h
    splash/SplashState.h
    splash/SplashTypes.h
    splash/SplashXPath.h
    splash/SplashXPathScanner.h
    DESTINATION include/poppler/splash)
  if(CAIRO_FOUND)
    install(FILES
      poppler/CairoFontEngine.h
      poppler/CairoOutputDev.h
      poppler/CairoRescaleBox.h
      DESTINATION include/poppler)
  endif()
endif()


if(ENABLE_UTILS)
  add_subdirectory(utils)
endif()
if(ENABLE_GLIB)
  add_subdirectory(glib)
endif()
if (BUILD_MANUAL_TESTS)
  add_subdirectory(test)
endif()
if(ENABLE_QT5)
  add_subdirectory(qt5)
endif()
if(ENABLE_QT6)
  add_subdirectory(qt6)
endif()
if(ENABLE_CPP)
  add_subdirectory(cpp)
endif()

# Configure "Requires" field & install .pc files for packagers
set(PC_REQUIRES "")
set(PC_REQUIRES_PRIVATE "Requires.private: poppler = ${POPPLER_VERSION}")

if(PKG_CONFIG_EXECUTABLE)
  poppler_create_install_pkgconfig(poppler.pc ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
  if(ENABLE_QT5)
    poppler_create_install_pkgconfig(poppler-qt5.pc ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
  endif()
  if(ENABLE_QT6)
    poppler_create_install_pkgconfig(poppler-qt6.pc ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
  endif()
  if(ENABLE_GLIB)
    poppler_create_install_pkgconfig(poppler-glib.pc ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
  endif()
  if(ENABLE_CPP)
    poppler_create_install_pkgconfig(poppler-cpp.pc ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
  endif()
else()
  MESSAGE(STATUS ".pc files will not be installed becasue of missing 'pkg-config'!")
endif()

# Summarize build options & display warnings for user
message("Building Poppler with support for:")
show_end_message("font configuration" ${font_configuration})
show_end_message_yesno("use boost (Splash)" ENABLE_BOOST)
show_end_message_yesno("cairo output" CAIRO_FOUND)
show_end_message_yesno("qt5 wrapper" ENABLE_QT5)
show_end_message_yesno("qt6 wrapper" ENABLE_QT6)
show_end_message_yesno("glib wrapper" ENABLE_GLIB)
show_end_message_yesno("  introspection" INTROSPECTION_FOUND)
show_end_message_yesno("  gtk-doc" ENABLE_GTK_DOC)
show_end_message_yesno("cpp wrapper" ENABLE_CPP)
show_end_message_yesno("use libjpeg" ENABLE_LIBJPEG)
show_end_message_yesno("use libpng" ENABLE_LIBPNG)
show_end_message_yesno("use libtiff" ENABLE_LIBTIFF)
show_end_message_yesno("use zlib uncompress" ENABLE_ZLIB_UNCOMPRESS)
show_end_message_yesno("use nss3" ENABLE_NSS3)
show_end_message_yesno("use gpg" ENABLE_GPGME)
show_end_message("  default signature backend" ${DEFAULT_SIGNATURE_BACKEND})
show_end_message_yesno("use curl" ENABLE_LIBCURL)
show_end_message_yesno("use libopenjpeg2" WITH_OPENJPEG)
show_end_message_yesno("use lcms2" USE_CMS)
show_end_message_yesno("command line utils" ENABLE_UTILS)
show_end_message_yesno("fuzz target" ENABLE_FUZZER)
show_end_message("test data dir" ${TESTDATADIR})

if(NOT ENABLE_LIBJPEG AND HAVE_DCT_DECODER)
  message("Warning: Using libjpeg is recommended. The internal DCT decoder is unmaintained.")
endif()

if(NOT HAVE_DCT_DECODER)
  message("Warning: You're not compiling any DCT decoder. Some files will fail to display properly.")
endif()

if(ENABLE_ZLIB_UNCOMPRESS)
  message("Warning: Using zlib is not totally safe")
endif()

if(NOT WITH_OPENJPEG AND HAVE_JPX_DECODER)
  message("Warning: Using libopenjpeg2 is recommended. The internal JPX decoder is unmaintained.")
endif()

if(NOT HAVE_JPX_DECODER)
  message("Warning: You're not compiling any JPX decoder. Some files will fail to display properly.")
endif()

if(NOT ENABLE_BOOST)
  message("Warning: Use of boost is recommended for better performance.")
endif()

set(ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${POPPLER_VERSION})
add_custom_target(dist
    COMMAND
        COMMAND git log --stat | fmt --split-only > ${CMAKE_BINARY_DIR}/ChangeLog
        COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar
        COMMAND tar -C ${CMAKE_BINARY_DIR} -rf ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar ChangeLog --transform='s,,${ARCHIVE_NAME}/,' --owner root:0 --group root:0
        COMMAND tar -C ${CMAKE_BINARY_DIR} -rf ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar glib/reference/html --transform='s,,${ARCHIVE_NAME}/,' --owner root:0 --group root:0
        COMMAND xz -9 ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
