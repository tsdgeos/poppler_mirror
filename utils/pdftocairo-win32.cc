#include <cairo-win32.h>

static void win32GetFitToPageTransform(cairo_matrix_t *m)
{
  if (!print)
    return;

  HDC hdc = cairo_win32_surface_get_dc(surface);
  int logx = GetDeviceCaps(hdc, LOGPIXELSX);
  int logy = GetDeviceCaps(hdc, LOGPIXELSY);
  cairo_matrix_scale (m, logx / 72.0, logy / 72.0);
}

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

static void parseSource(DEVMODEA *devmode, GooString *source)
{
  int src;
  const Win32Option *option = win32PaperSource;
  while (option->name) {
    if (source->cmp(option->name) == 0) {
      src = option->value;
      break;
    }
  }
  if (!option->name) {
    if (isInt(source->getCString())) {
      src = atoi(source->getCString());
    } else {
      fprintf(stderr, "Warning: Unknown paper source \"%s\"\n", source->getCString());
      return;
    }
  }

  devmode->dmDefaultSource = src;
  devmode->dmFields |= DM_DEFAULTSOURCE;
}

static const Win32Option win32DuplexMode[] =
{
  {"simplex", DMDUP_SIMPLEX},
  {"horizontal", DMDUP_HORIZONTAL},
  {"vertical", DMDUP_VERTICAL},
  {NULL, 0}
};

static void parseDuplex(DEVMODEA *devmode, GooString *mode)
{
  if (duplex)
    fprintf(stderr, "Warning: duplex mode is specified both as standalone and printer options\n");

  int win32Duplex;
  const Win32Option *option = win32DuplexMode;
  while (option->name) {
    if (mode->cmp(option->name) == 0) {
      win32Duplex = option->value;
      break;
    }
  }
  if (!option->name) {
    fprintf(stderr, "Warning: Unknown duplex mode \"%s\"\n", mode->getCString());
    return;
  }

  devmode->dmDuplex = win32Duplex;
  devmode->dmFields |= DM_DUPLEX;
}

static void fillCommonPrinterOptions(DEVMODEA *devmode, double w, double h)
{
  devmode->dmPaperWidth = w * 254.0 / 72.0;
  devmode->dmPaperLength = h * 254.0 / 72.0;
  printf("PAPER %d, %d\n", devmode->dmPaperWidth, devmode->dmPaperLength);
  devmode->dmFields |= DM_PAPERWIDTH | DM_PAPERLENGTH;
  if (duplex) {
    devmode->dmDuplex = DMDUP_HORIZONTAL;
    devmode->dmFields |= DM_DUPLEX;
  }
}

static void fillPrinterOptions(DEVMODEA *devmode)
{
  //printOpt format is: <opt1>=<val1>,<opt2>=<val2>,...
  const char *nextOpt = printOpt.getCString();
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
      parseSource(devmode, &value);
    } else if (opt.cmp("duplex") == 0) {
      parseDuplex(devmode, &value);
    } else {
      fprintf(stderr, "Warning: unknown printer option \"%s\"\n", opt.getCString());
    }
  }
}

static void win32BeginDocument(GooString *outputFileName, double w, double h)
{
  if (!print)
    return;

  if (printer.getCString()[0] == 0)
  {
    DWORD szName = 0;
    GetDefaultPrinterA(NULL, &szName);
    char *devname = (char*)gmalloc(szName);
    GetDefaultPrinterA(devname, &szName);
    printer.Set(devname);
    gfree(devname);
  }
  char *cPrinter = printer.getCString();

  //Query the size of the DEVMODE struct
  LONG szProp = DocumentPropertiesA(NULL, NULL, cPrinter, NULL, NULL, 0);
  if (szProp < 0)
  {
    fprintf(stderr, "Error: Printer \"%s\" not found", cPrinter);
    exit(99);
  }
  DEVMODEA *devmode = (DEVMODEA*)gmalloc(szProp);
  memset(devmode, 0, szProp);
  devmode->dmSize = sizeof(DEVMODEA);
  devmode->dmSpecVersion = DM_SPECVERSION;
  //Load the current default configuration for the printer into devmode
  if (DocumentPropertiesA(NULL, NULL, cPrinter, devmode, NULL, DM_OUT_BUFFER) < 0)
  {
    fprintf(stderr, "Error: Printer \"%s\" not found", cPrinter);
    exit(99);
  }
  fillCommonPrinterOptions(devmode, w, h);
  fillPrinterOptions(devmode);
  HDC hdc = CreateDCA(NULL, cPrinter, NULL, devmode);
  gfree(devmode);
  if (!hdc)
  {
    fprintf(stderr, "Error: Printer \"%s\" not found", cPrinter);
    exit(99);
  }

  DOCINFOA docinfo;
  memset(&docinfo, 0, sizeof(docinfo));
  docinfo.cbSize = sizeof(docinfo);
  if (outputFileName->cmp("fd://0") == 0)
    docinfo.lpszDocName = "pdftocairo <stdin>";
  else
    docinfo.lpszDocName = outputFileName->getCString();
  if (StartDocA(hdc, &docinfo) <=0) {
    fprintf(stderr, "Error: StartDoc failed");
    exit(99);
  }
    
  surface = cairo_win32_printing_surface_create(hdc);
}

static void win32BeginPage(double w, double h)
{
  if (!print)
    return;
  StartPage(cairo_win32_surface_get_dc(surface));
}

static void win32EndPage(GooString *imageFileName)
{
  if (!print)
    return;
  EndPage(cairo_win32_surface_get_dc(surface));
}

static void win32EndDocument()
{
  if (!print)
    return;

  HDC hdc = cairo_win32_surface_get_dc(surface);
  EndDoc(hdc);
  DeleteDC(hdc);
}
