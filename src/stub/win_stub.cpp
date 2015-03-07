
#include <mozilla/Char16.h>
#include <stdio.h>

int internal_main(int argc, char **argv);

#include <common.h>


//#define XPCOM_GLUE 1

#include <windows.h>
#include <tchar.h>
#include <io.h>

#include <nsXPCOMGlue.h>
#include <nsCOMPtr.h>
#include <nsStringAPI.h>
#include <nsIFile.h>
// #include <xulapp/nsXULAppAPI.h>

#if 1 //def MB_DEBUG
# define Outputf fprintf
#else
# define Outputf (1) ? (void)0 : (void)::fprintf
#endif

FILE *pFileLog;

struct nsXREAppData
{
  /**
   * This should be set to sizeof(nsXREAppData). This structure may be
   * extended in future releases, and this ensures that binary compatibility
   * is maintained.
   */
  PRUint32 size;

  /**
   * The directory of the application to be run. May be null if the
   * xulrunner and the app are installed into the same directory.
   */
  nsIFile* directory;

  /**
   * The name of the application vendor. This must be ASCII, and is normally
   * mixed-case, e.g. "Mozilla". Optional (may be null), but highly
   * recommended. Must not be the empty string.
   */
  const char *vendor;

  /**
   * The name of the application. This must be ASCII, and is normally
   * mixed-case, e.g. "Firefox". Required (must not be null or an empty
   * string).
   */
  const char *name;

  /**
   * The major version, e.g. "0.8.0+". Optional (may be null), but
   * required for advanced application features such as the extension
   * manager and update service. Must not be the empty string.
   */
  const char *version;

  /**
   * The application's build identifier, e.g. "2004051604"
   */
  const char *buildID;

  /**
   * The application's UUID. Used by the extension manager to determine
   * compatible extensions. Optional, but required for advanced application
   * features such as the extension manager and update service.
   *
   * This has traditionally been in the form
   * "{AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE}" but for new applications
   * a more readable form is encouraged: "appname@vendor.tld". Only
   * the following characters are allowed: a-z A-Z 0-9 - . @ _ { } *
   */
  const char *ID;

  /**
   * The copyright information to print for the -h commandline flag,
   * e.g. "Copyright (c) 2003 mozilla.org".
   */
  const char *copyright;

  /**
   * Combination of NS_XRE_ prefixed flags (defined below).
   */
  PRUint32 flags;

  /**
   * The location of the XRE. XRE_main may not be able to figure this out
   * programatically.
   */
  nsIFile* xreDirectory;

  /**
   * The minimum/maximum compatible XRE version.
   */
  const char *minVersion;
  const char *maxVersion;

  /**
   * The server URL to send crash reports to.
   */
  const char *crashReporterURL;

  /**
   * The profile directory that will be used. Optional (may be null). Must not
   * be the empty string, must be ASCII. The path is split into components
   * along the path separator characters '/' and '\'.
   *
   * The application data directory ("UAppData", see below) is normally
   * composed as follows, where $HOME is platform-specific:
   *
   *   UAppData = $HOME[/$vendor]/$name
   *
   * If present, the 'profile' string will be used instead of the combination of
   * vendor and name as follows:
   *
   *   UAppData = $HOME/$profile
   */
  const char *profile;
};


int (* XRE_main)(int,char * * const,struct nsXREAppData const *);
void (* XRE_FreeAppData)(struct nsXREAppData *);
nsresult (* XRE_CreateAppData)(class nsIFile *,struct nsXREAppData * *);

#define USE_WMAIN
#ifdef USE_WMAIN

static char *AllocConvertUTF16toUTF8(const WCHAR *pwcsbuf)
{
  int nmblen;

  nmblen = ::WideCharToMultiByte(CP_UTF8,0, pwcsbuf,-1, NULL,0, NULL,NULL);
  if (nmblen<=0)
    return NULL;

  char *pmbsbuf = new char[nmblen];
  if (!pmbsbuf)
    return NULL;

  nmblen = ::WideCharToMultiByte(CP_UTF8,0, pwcsbuf,-1, pmbsbuf,nmblen, NULL,NULL);
  if (nmblen<=0) {
    delete [] pmbsbuf;
    return NULL;
  }

  return pmbsbuf;
}

static void FreeAllocStrings(int argc, char **argv)
{
  while (argc) {
    --argc;
    delete [] argv[argc];
  }

  delete [] argv;
}

int wmain(int argc, WCHAR **argv)
{
  /*if (AllocConsole()) {
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);
  }*/
  
  // pFileLog = fopen("log.txt", "w");
  pFileLog = stderr;
  
  char **argvConverted = new char*[argc + 1];
  if (!argvConverted)
    return 127;

  for (int i = 0; i < argc; ++i) {
    argvConverted[i] = AllocConvertUTF16toUTF8(argv[i]);
    if (!argvConverted[i]) {
      return 127;
    }
  }
  argvConverted[argc] = NULL;

  // need to save argvConverted copy for later deletion.
  char **deleteUs = new char*[argc+1];
  if (!deleteUs) {
    FreeAllocStrings(argc, argvConverted);
    return 127;
  }
  for (int i=0; i<argc; i++)
    deleteUs[i] = argvConverted[i];

  Outputf(pFileLog, "WMAIN; Enter internal_main\n");
  int result = internal_main(argc, argvConverted);
  Outputf(pFileLog, "WMAIN; Leave internal_main\n");

  delete[] argvConverted;
  FreeAllocStrings(argc, deleteUs);

  return result;
}
#else
int main(int argc, char **argv)
{
  //pFileLog = fopen("log.txt", "w");
  pFileLog = stderr;
  
  Outputf(pFileLog, "Enter internal_main\n");
  return internal_main(argc, argv);
  Outputf(pFileLog, "Leave internal_main\n");
}
#endif

void usage(int argc, char **argv)
{
  Outputf(pFileLog, "Usage: %s [-greapp <Path to GRE> <CueMol2 topdir>\n", argv[0]);
}

void sanitize_dirpath(char *sbuf)
{
  // remove the trailing backslash char of directory pathname
  int nlen = ::strlen(sbuf);
  if (sbuf[nlen-1]=='\\') {
    sbuf[nlen-1] ='\0';
    Outputf(pFileLog, "modified: %s\n", sbuf);
  }
}

void dumpClientFunction(void *userPortion, size_t blockSize)
{
  Outputf(pFileLog, "########## memory leak %p (%d)\n", userPortion, blockSize);
}


int internal_main(int argc, char **argv)
{
  nsresult rv;
  char grePath[256];
  char appPath[256];
  PRBool greFound = PR_FALSE, appFound = PR_FALSE;

  int i;
  for (i=0; i<argc; ++i) {
    Outputf(pFileLog, "argv[%d] = <%s>\n", i, argv[i]);
  }

  if (argc>3) {
    if (strcmp(argv[argc-3], "-greapp")==0) {
      sanitize_dirpath(argv[argc-2]);
      char greDir[256];
      strncpy(greDir, argv[argc-2], sizeof greDir);
      _snprintf(grePath, sizeof(grePath), "%s\\xul.dll", greDir);

      sanitize_dirpath(argv[argc-1]);
      char appDir[256];
      strncpy(appDir, argv[argc-1], sizeof appDir);
      _snprintf(appPath, sizeof(appPath), "%s\\application.ini", appDir);

      if (_access(grePath, 0) == 0) {
        appFound = PR_TRUE;
      }
      else {
        Outputf(pFileLog, "Info: grePath <%s> not found\n", grePath);
      }

      if (_access(appPath, 0) == 0) {
        greFound = PR_TRUE;
      }
      else {
        Outputf(pFileLog, "Info: appPath <%s> not found\n", appPath);
      }

      argv[argc-3] = 0;
      argc-=3;
      // argv+=3;
    }
  }

  if (!greFound || !appFound) {
    char exeDir[256];
    if (!::GetModuleFileNameA(NULL, exeDir, sizeof exeDir)) {
      Outputf(pFileLog, "Fatal: GetModuleFileName failed\n");
      usage(argc,argv);
      return 1;
    }
    
    char *lastSlash = strrchr(exeDir, '\\');
    if (!lastSlash) {
      Outputf(pFileLog, "Fatal: invalid module name %s\n", exeDir);
      usage(argc,argv);
      return 1;
    }

    *(lastSlash) = '\0';

    if (!greFound) {
      _snprintf(grePath, sizeof(grePath), "%s\\xulrunner23\\xul.dll", exeDir);
      if (_access(grePath, 0) == 0) {
        greFound = PR_TRUE;
      }
      else {
        Outputf(pFileLog, "Info: grePath <%s> not found\n", grePath);
      }
    }

    if (!appFound) {
      _snprintf(appPath, sizeof(appPath), "%s\\application.ini", exeDir);
      if (_access(grePath, 0) == 0) {
        appFound = PR_TRUE;
      }
      else {
        Outputf(pFileLog, "Info: grePath <%s> not found\n", grePath);
      }
    }

  }
  
  if (!greFound) {
    Outputf(pFileLog, "Fatal: GRE not found!\n");
    usage(argc,argv);
    return 1;
  }

  if (!appFound) {
    Outputf(pFileLog, "Fatal: Application not found!\n");
    usage(argc,argv);
    return 1;
  }
  
  rv = XPCOMGlueStartup(grePath);
  Outputf(pFileLog, "grePath=<%s>\n", grePath);
  if (NS_FAILED(rv)) {
    Outputf(pFileLog, "FATAL: Cannot load XPCOM.\n");
    usage(argc,argv);
    return 1;
  }

  static const nsDynamicFunctionLoad kXULFuncs[] = {
    { "XRE_CreateAppData", (NSFuncPtr*) &XRE_CreateAppData },
    { "XRE_FreeAppData", (NSFuncPtr*) &XRE_FreeAppData },
    { "XRE_main", (NSFuncPtr*) &XRE_main },
    { nullptr, nullptr }
  };

  rv = XPCOMGlueLoadXULFunctions(kXULFuncs);
  if (NS_FAILED(rv)) {
    Outputf(pFileLog, "FATAL: Couldn't load XRE functions.\n");
    usage(argc,argv);
    return 1;
  }

  NS_LogInit();

  int retval;

  Outputf(pFileLog, "appPath=<%s>\n", appPath);

  nsXREAppData *pAppData = NULL;
  {
    //nsCOMPtr<nsIFile> iniFile;
    nsIFile *pIniFile;
    rv = NS_NewNativeLocalFile(nsDependentCString(appPath), PR_FALSE,
                               &pIniFile);
    //NS_ADDREF(pIniFile);
    if (NS_FAILED(rv)) {
      Outputf(pFileLog, "Couldn't find application.ini file.\n");
      usage(argc,argv);
      return 1;
    }

    rv = XRE_CreateAppData(pIniFile, &pAppData);
    if (NS_FAILED(rv)) {
      Outputf(pFileLog, "Error: couldn't parse application.ini.\n");
      usage(argc,argv);
      return 1;
    }
  }

  NS_ASSERTION(pAppData->directory, "Failed to get app directory.");

  if (!pAppData->xreDirectory) {
    // chop "libxul.so" off the GRE path
    char *lastSlash = strrchr(grePath, '\\');
    if (lastSlash) {
      *lastSlash = '\0';
    }
    NS_NewNativeLocalFile(nsDependentCString(grePath), PR_FALSE,
                          &pAppData->xreDirectory);
  }
  
#ifdef MB_DEBUG
  {
    char sbuf[256];
    _getcwd(sbuf, sizeof sbuf);
    Outputf(pFileLog, "CWD: %s\n", sbuf);
  }
#endif
  
  Outputf(pFileLog, "### ENTERING XRE_MAIN ###\n");
  //qlib::init();
  
  retval = XRE_main(argc, argv, pAppData);
  
  Outputf(pFileLog, "### LEAVING XRE_MAIN ###\n");
  XRE_FreeAppData(pAppData);
  //qlib::fini();
  
  Outputf(pFileLog, "### LEAVING APP 1 ###\n");
  NS_LogTerm();
  Outputf(pFileLog, "### LEAVING APP 2 ###\n");
  
  //  XPCOMGlueShutdown();
  Outputf(pFileLog, "### LEAVING APP 3 ###\n");
  
  _CrtSetDumpClient(dumpClientFunction);
  
  _CrtDumpMemoryLeaks() ;

  return 0;
}

#if 0
int WINAPI WinMain( HINSTANCE, HINSTANCE, LPSTR args, int )
{
  char** argv = static_cast<char**>(malloc(__argc * sizeof(char*)));
  for (int i = 0; i < __argc; i++) {
    argv[i] = strdup(__argv[i]);
  }

  // Do the real work.
  return main(__argc, argv);
}
#endif

