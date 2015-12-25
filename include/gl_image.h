
#define __USE_PNG__
#define __USE_JPG__

#ifdef __CYGWIN__
// prevent gethostname redefinition error caused by inclusion of <png.h>
#include <winsock.h>
#endif

#ifdef __USE_PNG__
#include "png.h"
#endif
#ifdef __USE_JPG__
#include "jpeglib.h"
#endif

byte *LoadPCX (FILE *f, char *name);

byte *LoadTGA (FILE *fin, char *name);

#ifdef __USE_JPG__
byte *LoadJPG (FILE *fin, char *name);
#endif
#ifdef __USE_PNG__
byte *LoadPNG (FILE *fin, char *name);
#endif

byte *LoadDDS(FILE *f, char *name);
