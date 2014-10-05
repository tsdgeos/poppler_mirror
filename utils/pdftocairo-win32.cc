#include <cairo.h>
#ifdef CAIRO_HAS_WIN32_SURFACE

#include <cairo-win32.h>

#include "parseargs.h"
#include "pdftocairo-win32.h"

static HDC hdc;
static DEVMODEA *devmode;
static char *printerName;

struct Win32Option
{
  const char *name;
  int value;
};

static const Win32Option win32PaperSource[] =
{
  {"upper", DMBIN_UPPER},
  {"onlyone", DMBIN_ONLYONE},
  {"lower", DMBIN_LOWER},
  {"middle", DMBIN_MIDDLE},
  {"manual", DMBIN_MANUAL},
  {"envelope", DMBIN_ENVELOPE},
  {"envmanual", DMBIN_ENVMANUAL},
  {"auto", DMBIN_AUTO},
  {"tractor", DMBIN_TRACTOR},
  {"smallfmt", DMBIN_SMALLFMT},
  {"largefmt", DMBIN_LARGEFMT},
  {"largecapacity", DMBIN_LARGECAPACITY},
  {"formsource", DMBIN_FORMSOURCE},
  {NULL, 0}
};

static void parseSource(GooString *source)
{
  const Win32Option *option = win32PaperSource;
  while (option->name) {
    if (source->cmp(option->name) == 0) {
      devmode->dmDefaultSource = option->value;
      devmode->dmFields |= DM_DEFAULTSOURCE;
      return;
    }
    option++;
  }
  fprintf(stderr, "Warning: Unknown paper source \"%s\"\n", source->getCString());
}

static const Win32Option win32DuplexMode[] =
{
  {"off", DMDUP_SIMPLEX},
  {"short", DMDUP_HORIZONTAL},
  {"long", DMDUP_VERTICAL},
  {NULL, 0}
};

static void parseDuplex(GooString *mode)
{
  const Win32Option *option = win32DuplexMode;
  while (option->name) {
    if (mode->cmp(option->name) == 0) {
      devmode->dmDuplex = option->value;
      devmode->dmFields |= DM_DUPLEX;
      return;
    }
    option++;
  }
  fprintf(stderr, "Warning: Unknown duplex mode \"%s\"\n", mode->getCString());
}

static void fillCommonPrinterOptions(double w, double h, GBool duplex)
{
  if (duplex) {
    devmode->dmDuplex = DMDUP_HORIZONTAL;
    devmode->dmFields |= DM_DUPLEX;
  }
}

static void fillPagePrinterOptions(double w, double h)
{
  w *= 254.0 / 72.0; // units are 0.1mm
  h *= 254.0 / 72.0;
  if (w > h) {
    devmode->dmOrientation = DMORIENT_LANDSCAPE;
    devmode->dmPaperWidth = h;
    devmode->dmPaperLength = w;
  } else {
    devmode->dmOrientation = DMORIENT_PORTRAIT;
    devmode->dmPaperWidth = w;
    devmode->dmPaperLength = h;
  }
  devmode->dmPaperSize = 0;
  devmode->dmFields |= DM_ORIENTATION | DM_PAPERWIDTH | DM_PAPERLENGTH;
}


static void fillPrinterOptions(GBool duplex, GooString *printOpt)
{
  //printOpt format is: <opt1>=<val1>,<opt2>=<val2>,...
  const char *nextOpt = printOpt->getCString();
  while (nextOpt && *nextOpt)
  {
    const char *comma = strchr(nextOpt, ',');
    GooString opt;
    if (comma) {
      opt.Set(nextOpt, comma - nextOpt);
      nextOpt = comma + 1;
    } else {
      opt.Set(nextOpt);
      nextOpt = NULL;
    }
    //here opt is "<optN>=<valN> "
    const char *equal = strchr(opt.getCString(), '=');
    if (!equal) {
      fprintf(stderr, "Warning: unknown printer option \"%s\"\n", opt.getCString());
      continue;
    }
    int iequal = equal - opt.getCString();
    GooString value(&opt, iequal + 1, opt.getLength() - iequal - 1);
    opt.del(iequal, opt.getLength() - iequal);
    //here opt is "<optN>" and value is "<valN>"

    if (opt.cmp("source") == 0) {
      parseSource(&value);
    } else if (opt.cmp("duplex") == 0) {
      if (duplex)
	fprintf(stderr, "Warning: duplex mode is specified both as standalone and printer options\n");
      else
	parseDuplex( &value);
    } else {
      fprintf(stderr, "Warning: unknown printer option \"%s\"\n", opt.getCString());
    }
  }
}

cairo_surface_t *win32BeginDocument(GooString *inputFileName, GooString *outputFileName,
				    double w, double h,
				    GooString *printer,
				    GooString *printOpt,
				    GBool setupdlg,
				    GBool duplex)
{
  if (printer->getCString()[0] == 0) {
    DWORD size = 0;
    GetDefaultPrinterA(NULL, &size);
    printerName = (char*)gmalloc(size);
    GetDefaultPrinterA(printerName, &size);
  } else {
    printerName = gstrndup(printer->getCString(), printer->getLength());
  }

  //Query the size of the DEVMODE struct
  LONG szProp = DocumentPropertiesA(NULL, NULL, printerName, NULL, NULL, 0);
  if (szProp < 0) {
    fprintf(stderr, "Error: Printer \"%s\" not found\n", printerName);
    exit(99);
  }
  devmode = (DEVMODEA*)gmalloc(szProp);
  memset(devmode, 0, szProp);
  devmode->dmSize = sizeof(DEVMODEA);
  devmode->dmSpecVersion = DM_SPECVERSION;
  //Load the current default configuration for the printer into devmode
  if (DocumentPropertiesA(NULL, NULL, printerName, devmode, devmode, DM_OUT_BUFFER) < 0) {
    fprintf(stderr, "Error: Printer \"%s\" not found\n", printerName);
    exit(99);
  }

  // Update devmode with selected print options
  fillCommonPrinterOptions(w, h, duplex);
  fillPrinterOptions(duplex, printOpt);

  // Call DocumentProperties again so the driver can update its private data
  // with the modified print options. This will also display the printer
  // properties dialog if setupdlg is true.
  int ret;
  DWORD mode = DM_IN_BUFFER | DM_OUT_BUFFER;
  if (setupdlg)
    mode |= DM_IN_PROMPT;
  ret = DocumentPropertiesA(NULL, NULL, printerName, devmode, devmode, mode);
  if (ret < 0) {
    fprintf(stderr, "Error: Printer \"%s\" not found\n", printerName);
    exit(99);
  }
  if (setupdlg && ret == IDCANCEL)
    exit(0);

  hdc = CreateDCA(NULL, printerName, NULL, devmode);
  if (!hdc) {
    fprintf(stderr, "Error: Printer \"%s\" not found\n", printerName);
    exit(99);
  }

  DOCINFOA docinfo;
  memset(&docinfo, 0, sizeof(docinfo));
  docinfo.cbSize = sizeof(docinfo);
  if (inputFileName->cmp("fd://0") == 0)
    docinfo.lpszDocName = "pdftocairo <stdin>";
  else
    docinfo.lpszDocName = inputFileName->getCString();
  if (outputFileName)
    docinfo.lpszOutput = outputFileName->getCString();
  if (StartDocA(hdc, &docinfo) <=0) {
    fprintf(stderr, "Error: StartDoc failed\n");
    exit(99);
  }

  return cairo_win32_printing_surface_create(hdc);
}

void win32BeginPage(double *w, double *h, GBool changePageSize, GBool useFullPage)
{
  if (changePageSize)
    fillPagePrinterOptions(*w, *h);
  if (DocumentPropertiesA(NULL, NULL, printerName, devmode, devmode, DM_IN_BUFFER | DM_OUT_BUFFER) < 0) {
    fprintf(stderr, "Error: Printer \"%s\" not found\n", printerName);
    exit(99);
  }
  ResetDCA(hdc, devmode);

  // Get actual paper size or if useFullPage is false the printable area.
  // Transform the hdc scale to points to be consistent with other cairo backends
  int x_dpi = GetDeviceCaps (hdc, LOGPIXELSX);
  int y_dpi = GetDeviceCaps (hdc, LOGPIXELSY);
  int x_off = GetDeviceCaps (hdc, PHYSICALOFFSETX);
  int y_off = GetDeviceCaps (hdc, PHYSICALOFFSETY);
  if (useFullPage) {
    *w = GetDeviceCaps (hdc, PHYSICALWIDTH)*72.0/x_dpi;
    *h = GetDeviceCaps (hdc, PHYSICALHEIGHT)*72.0/y_dpi;
  } else {
    *w = GetDeviceCaps (hdc, HORZRES)*72.0/x_dpi;
    *h = GetDeviceCaps (hdc, VERTRES)*72.0/y_dpi;
  }
  XFORM xform;
  xform.eM11 = x_dpi/72.0;
  xform.eM12 = 0;
  xform.eM21 = 0;
  xform.eM22 = y_dpi/72.0;
  if (useFullPage) {
    xform.eDx = -x_off;
    xform.eDy = -y_off;
  } else {
    xform.eDx = 0;
    xform.eDy = 0;
  }
  SetGraphicsMode (hdc, GM_ADVANCED);
  SetWorldTransform (hdc, &xform);

  StartPage(hdc);
}

void win32EndPage(GooString *imageFileName)
{
  EndPage(hdc);
}

void win32EndDocument()
{
  EndDoc(hdc);
  DeleteDC(hdc);
  gfree(devmode);
  gfree(printerName);
}

#endif // CAIRO_HAS_WIN32_SURFACE
