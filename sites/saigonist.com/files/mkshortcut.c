/* mkshortcut.c -- create a Windows shortcut
 *
 * Copyright (c) 2002 Joshua Daniel Franklin
 * Copyright (c) 2010 Andy Koppe
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * See the COPYING file for license information.
 *
 * Exit values
 *   1: user error (syntax error)
 *   2: system error (out of memory, etc.)
 *   3: windows error (interface failed)
 *
 * Compile with: gcc -o prog.exe mkshortcut.c -lpopt -lole32 -luuid
 *  (You'd need to uncomment the moved to common.h lines.)
 *
 */

#define UNICODE

#if HAVE_CONFIG_H
# include "config.h"
#endif
#include "common.h"

#define NOCOMATTRIBUTE

#include <sys/cygwin.h>
#include <locale.h>
#include <wchar.h>
#include <shlobj.h>
#include <olectlid.h>
/* moved to common.h */
#include <stdio.h>
#include <popt.h>

static const char versionID[] = PACKAGE_VERSION;
static const char revID[] =
  "$Id: mkshortcut.c,v 1.12 2010/08/15 21:02:27 cwilson Exp $";
static const char copyrightID[] =
  "Copyright (c) 2002 Joshua Daniel Franklin.\n"
  "Copyright (c) 2010 Andy Koppe.\n"
  "All rights reserved.\n"
  "Licensed under GPL v2.0\n";

typedef struct optvals_s
{
  int icon_flag;
  int unix_flag;
  int windows_flag;
  int allusers_flag;
  int desktop_flag;
  int smprograms_flag;
  int show_flag;
  int offset;
  char *name_arg;
  char *desc_arg;
  char *dir_name_arg;
  char *argument_arg;
  char *target_arg;
  char *icon_name_arg;
} optvals;

static int mkshortcut (optvals opts);
static void printTopDescription (FILE * f, char *name);
static void printBottomDescription (FILE * f, char *name);
static const char *getVersion ();
static void usage (FILE * f, char *name);
static void help (FILE * f, char *name);
static void version (FILE * f, char *name);
static void license (FILE * f, char *name);
static void error (char *);
static void *nullcheck (void *);

static char *program_name;
static poptContext optCon;

int
main (int argc, const char **argv)
{
  const char **rest;
  int rc;
  int ec = 0;
  optvals opts;

  const char *tmp_str;
  int icon_offset_flag;
  char icon_name[MAX_PATH];
  const char *arg;

  struct poptOption helpOptionsTable[] = {
    {"help", 'h', POPT_ARG_NONE, NULL, '?',
     "Show this help message", NULL},
    {"usage", '\0', POPT_ARG_NONE, NULL, 'u',
     "Display brief usage message", NULL},
    {"version", 'v', POPT_ARG_NONE, NULL, 'v',
     "Display version information", NULL},
    {"license", '\0', POPT_ARG_NONE, NULL, 'l',
     "Display licensing information", NULL},
    {NULL, '\0', 0, NULL, 0, NULL, NULL}
  };

  struct poptOption generalOptionsTable[] = {
    {"arguments", 'a', POPT_ARG_STRING, NULL, 'a',
     "Use arguments ARGS", "ARGS"},
    {"desc", 'd', POPT_ARG_STRING, NULL, 'd',
     "Text for description/tooltip (defaults to POSIX path of TARGET)",
     "DESC"},
    {"icon", 'i', POPT_ARG_STRING, NULL, 'i',
     "Icon file for link to use", "ICONFILE"},
    {"iconoffset", 'j', POPT_ARG_INT, &(opts.offset), 'j',
     "Offset of icon in icon file (default is 0)", NULL},
    {"name", 'n', POPT_ARG_STRING, NULL, 'n',
     "Name for link (defaults to TARGET)", "NAME"},
    {"show", 's', POPT_ARG_STRING, NULL, 's',
     "Window to show: normal, minimized, maximized", "norm|min|max"},
    {"workingdir", 'w', POPT_ARG_STRING, NULL, 'w',
     "Set working directory (defaults to directory path of TARGET)", "PATH"},
    {"allusers", 'A', POPT_ARG_VAL, &(opts.allusers_flag), 1,
     "Use 'All Users' instead of current user for -D,-P", NULL},
    {"desktop", 'D', POPT_ARG_VAL, &(opts.desktop_flag), 1,
     "Create link relative to 'Desktop' directory", NULL},
    {"smprograms", 'P', POPT_ARG_VAL, &(opts.smprograms_flag), 1,
     "Create link relative to Start Menu 'Programs' directory", NULL},
    {NULL, '\0', 0, NULL, 0, NULL, NULL}
  };

  struct poptOption opt[] = {
    {NULL, '\0', POPT_ARG_INCLUDE_TABLE, generalOptionsTable, 0,
     "General options", NULL},
    {NULL, '\0', POPT_ARG_INCLUDE_TABLE, helpOptionsTable, 0,
     "Help options", NULL},
    {NULL, '\0', 0, NULL, 0, NULL, NULL}
  };
  
  setlocale(LC_CTYPE, "");

  tmp_str = strrchr (argv[0], '/');
  if (tmp_str == NULL)
    {
      tmp_str = strrchr (argv[0], '\\');
    }
  if (tmp_str == NULL)
    {
      tmp_str = argv[0];
    }
  else
    {
      tmp_str++;
    }
  nullcheck (program_name = strdup (tmp_str));

  icon_offset_flag = 0;

  opts.offset = 0;
  opts.icon_flag = 0;
  opts.unix_flag = 0;
  opts.windows_flag = 0;
  opts.allusers_flag = 0;
  opts.desktop_flag = 0;
  opts.smprograms_flag = 0;
  opts.show_flag = SW_SHOWNORMAL;
  opts.target_arg = NULL;
  opts.argument_arg = NULL;
  opts.name_arg = NULL;
  opts.desc_arg = NULL;
  opts.dir_name_arg = NULL;
  opts.icon_name_arg = NULL;

  /* Parse options */
  optCon = poptGetContext (NULL, argc, argv, opt, 0);
  poptSetOtherOptionHelp (optCon, "[OPTION]* TARGET");
  while ((rc = poptGetNextOpt (optCon)) > 0)
    {
      switch (rc)
        {
        case '?':
          help (stdout, program_name);
          goto exit;
        case 'u':
          usage (stdout, program_name);
          goto exit;
        case 'v':
          version (stdout, program_name);
          goto exit;
        case 'l':
          license (stdout, program_name);
          goto exit;
        case 'd':
          if (arg = poptGetOptArg (optCon))
            nullcheck (opts.desc_arg = strdup (arg));
          break;
        case 'i':
          opts.icon_flag = 1;
          if (arg = poptGetOptArg (optCon))
            nullcheck (opts.desc_arg = strdup (arg));
          break;
        case 'j':
          icon_offset_flag = 1;
          break;
        case 'n':
            nullcheck (opts.desc_arg = strdup (arg));
          break;
        case 's':
          if (arg = poptGetOptArg (optCon))
            {
              if (strcmp (arg, "min") == 0)
                {
                  opts.show_flag = SW_SHOWMINNOACTIVE;
                }
              else if (strcmp (arg, "max") == 0)
                {
                  opts.show_flag = SW_SHOWMAXIMIZED;
                }
              else if (strcmp (arg, "norm") == 0)
                {
                  opts.show_flag = SW_SHOWNORMAL;
                }
              else
                {
                  fprintf (stderr, "%s: %s not valid for show window\n",
                           program_name, arg);
                  return 2;
                }
            }
          break;
        case 'w':
          if (arg = poptGetOptArg (optCon))
            nullcheck (opts.dir_name_arg = strdup (arg));
          break;
        case 'a':
          if (arg = poptGetOptArg (optCon))
            nullcheck (opts.argument_arg = strdup (arg));
          break;
          // case 'A' 
          // case 'D'
          // case 'P' all handled by popt itself
        }
    }

  if (icon_offset_flag & !opts.icon_flag)
    {
      fprintf (stderr,
               "%s: --iconoffset|-j only valid in conjuction with --icon|-i\n",
               program_name);
      usage (stderr, program_name);
      ec = 1;
      goto exit;
    }

  if (opts.smprograms_flag && opts.desktop_flag)
    {
      fprintf (stderr,
               "%s: --smprograms|-P not valid in conjuction with --desktop|-D\n",
               program_name);
      usage (stderr, program_name);
      ec = 1;
      goto exit;
    }

  if (rc < -1)
    {
      fprintf (stderr, "%s: bad argument %s: %s\n",
               program_name, poptBadOption (optCon, POPT_BADOPTION_NOALIAS),
               poptStrerror (rc));
      ec = 1;
      goto exit;
    }

  rest = poptGetArgs (optCon);

  if (rest && *rest)
    {
      if ((opts.target_arg = strdup (*rest)) == NULL)
        {
          fprintf (stderr, "%s: memory allocation error\n", program_name);
          ec = 2;
          goto exit;
        }
      rest++;
      if (rest && *rest)
        {
          fprintf (stderr, "%s: Too many arguments: ", program_name);
          while (*rest)
            fprintf (stderr, "%s ", *rest++);
          fprintf (stderr, "\n");
          usage (stderr, program_name);
          ec = 1;
        }
      else
        {
          // THE MEAT GOES HERE
          ec = mkshortcut (opts);
        }
    }
  else
    {
      fprintf (stderr, "%s: TARGET not specified\n", program_name);
      usage (stderr, program_name);
      ec = 1;
    }

exit:
  poptFreeContext (optCon);
  if (opts.target_arg)
    {
      free (opts.target_arg);
    }
  if (opts.name_arg)
    {
      free (opts.name_arg);
    }
  if (opts.desc_arg)
    {
      free (opts.desc_arg);
    }
  if (opts.dir_name_arg)
    {
      free (opts.dir_name_arg);
    }
  if (opts.argument_arg)
    {
      free (opts.argument_arg);
    }
  if (opts.icon_name_arg)
    {
      free (opts.icon_name_arg);
    }
  free (program_name);
  return (ec);
}

static wchar_t *
conv_arg (char *arg, char *arg_name)
{
  wchar_t *ret;
  int len = mbstowcs(0, arg, 0);
  if (len < 0)
    {
      fprintf (stderr, "%s: invalid bytes in %s\n",
               program_name, arg_name);
      exit (1);
    }
  ret = nullcheck (malloc (++len * sizeof (wchar_t)));
  mbstowcs(ret, arg, len);
  return ret;
}

static wchar_t *
conv_path (char *path, cygwin_conv_path_t flags)
{
  wchar_t *wpath;

  if (strchr (path, ':') != NULL)
    error ("all paths must be in POSIX format");

  wpath = cygwin_create_path (CCP_POSIX_TO_WIN_W | flags, path);
  if (!wpath)
     {
      fprintf (stderr,
               "%s: could not convert path: $s\n",
               program_name, path);
      exit (1);
     }

  if (wcsncmp(wpath, L"\\\\?\\UNC\\", 8) == 0)
    wpath += 6, *wpath = '\\'; /* Replace '\\?\UNC\' with \\ */
  else if (wcsncmp(wpath, L"\\\\?\\", 4) == 0)
    wpath += 4; /* Drop '\\?\' */
  
  return wpath;
 }


int
mkshortcut (optvals opts)
{
  wchar_t *link_name, *link_base;
  wchar_t *exe_name;
  wchar_t *dir_name;
  wchar_t *icon_name;
  wchar_t *desc;
  wchar_t *args;
  int len;

  /* For OLE interface */
  HRESULT hres;
  IShellLink *shell_link;
  IPersistFile *persist_file;

  /*  If there's a colon in the TARGET, it should be a URL */
  if (strchr (opts.target_arg, ':') != NULL)
    {
      /*  Nope, somebody's trying a W32 path  */
      if (opts.target_arg[1] == ':')
        error ("all paths must be in POSIX format");
       
      exe_name = conv_arg (opts.target_arg, "target URL");
      dir_name = L"";       /* No working dir for URL */    }
  /* Convert TARGET to win32 path */
  else
    {
      exe_name = conv_path (opts.target_arg, 0);

      /*  Get a working dir from 'w' option */
      if (opts.dir_name_arg != NULL)
        dir_name = conv_path (opts.dir_name_arg, 0);
      /*  Get a working dir from the exepath */
      else
        {
          wchar_t *last_slash = wcsrchr (exe_name, '\\');
          if (last_slash) {
            len = last_slash - exe_name;
            dir_name = nullcheck (malloc ((len + 1) * sizeof(wchar_t)));
            wmemcpy (dir_name, exe_name, len);
            dir_name[len] = 0;
          }
          else
            dir_name = L"";
        }
    }

  /*  Generate a name for the link if not given */
  if (opts.name_arg == NULL)
    {
      wchar_t *last_slash = wcsrchr (exe_name, '\\');
      if (!last_slash)
        last_slash = wcsrchr (exe_name, '/');
      if (!last_slash)
        error("need link name");
      link_base = last_slash + 1;
    }
  /*  User specified a name, so check it and convert  */
  else
    {
      if (opts.desktop_flag || opts.smprograms_flag)
        {
          /*  Cannot have absolute path relative to Desktop/SM Programs */
          if (opts.name_arg[0] == '/')
            error ("absolute pathnames not allowed with -D/-P");
        }
      link_base = conv_path (opts.name_arg, CCP_RELATIVE);
    }

  /* Make space for the full link path */
  len = wcslen (link_base);
  link_name = nullcheck (malloc((MAX_PATH + len + 8) * sizeof (wchar_t)));

  /* Get special folder location if required */
  if (opts.desktop_flag || opts.smprograms_flag)
    {
      int dir;
      LPITEMIDLIST id;
      if (opts.desktop_flag)
        dir = opts.allusers_flag ? CSIDL_COMMON_DESKTOPDIRECTORY
                                 : CSIDL_DESKTOPDIRECTORY;
    else
        dir = opts.allusers_flag ? CSIDL_COMMON_PROGRAMS
                                 : CSIDL_PROGRAMS;      
      SHGetSpecialFolderLocation (NULL, dir, &id);
      SHGetPathFromIDList (id, link_name);
      wcscat (link_name, L"\\");
    }

  else
    *link_name = 0;
  
  wcscat(link_name, link_base);

  /* Append ".lnk" if necessary. */
  if (len <= 4 || wcscmp (link_base + len - 4, L".lnk") != 0)
    wcscat(link_name, L".lnk");
  
  /* Convert icon name */
  if (opts.icon_name_arg != NULL)
    icon_name = conv_path (opts.icon_name_arg, 0);
  else
    icon_name = NULL;
  
  /* Convert shortcut argument */
  if (opts.argument_arg != NULL)
    args = conv_arg (opts.argument_arg, "argument");
  else
    args = NULL;
  
  /* Convert description */
  if (opts.desc_arg != NULL)
    desc = conv_arg (opts.desc_arg, "description");
  else
    {
      /* Put the POSIX path in the "Description", just to be nice */
      desc = conv_arg(cygwin_create_path(CCP_WIN_W_TO_POSIX, exe_name),
                      "executable name");
    }

  /*  Beginning of Windows interface */
  hres = OleInitialize (NULL);
  if (hres != S_FALSE && hres != S_OK)
    {
      fprintf (stderr, "%s: Could not initialize OLE interface\n",
               program_name);
      return (3);
    }

  hres =
    CoCreateInstance (&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                      &IID_IShellLink, (void **) &shell_link);
  if (SUCCEEDED (hres))
    {
      hres =
        shell_link->lpVtbl->QueryInterface (shell_link, &IID_IPersistFile,
                                            (void **) &persist_file);
      if (SUCCEEDED (hres))
        {
          shell_link->lpVtbl->SetPath (shell_link, exe_name);
          shell_link->lpVtbl->SetDescription (shell_link, desc);
          shell_link->lpVtbl->SetWorkingDirectory (shell_link, dir_name);
          if (opts.argument_arg)
            shell_link->lpVtbl->SetArguments (shell_link, args);
          if (opts.icon_flag)
            shell_link->lpVtbl->SetIconLocation (shell_link,
                                                 icon_name,
                                                 opts.offset);
          if (opts.show_flag != SW_SHOWNORMAL)
            shell_link->lpVtbl->SetShowCmd (shell_link, opts.show_flag);

          hres = persist_file->lpVtbl->Save (persist_file, link_name, TRUE);
          if (!SUCCEEDED (hres))
            {
              fprintf (stderr,
                       "%s: Saving \"%ls\" failed\n",
                       program_name, link_name);
              return (3);
            }
          persist_file->lpVtbl->Release (persist_file);
          shell_link->lpVtbl->Release (shell_link);
          return (0);
        }
      else
        {
          fprintf (stderr, "%s: QueryInterface failed\n", program_name);
          return (3);
        }
    }
  else
    {
      fprintf (stderr, "%s: CoCreateInstance failed\n", program_name);
      return (3);
    }
  
  return 0;
}

static const char *
getVersion ()
{
  return versionID;
}

static void
printTopDescription (FILE * f, char *name)
{
  char s[20];
  fprintf (f, "%s is part of cygutils version %s\n", name, getVersion ());
  fprintf (f, "  create a Windows shortcut\n\n");
}
static void
printBottomDescription (FILE * f, char *name)
{
  fprintf (f,
           "\nNOTE: All filename arguments must be in unix (POSIX) format\n");
}

static
printLicense (FILE * f, char *name)
{
  fprintf (f,
           "This program is free software; you can redistribute it and/or\n");
  fprintf (f,
           "modify it under the terms of the GNU General Public License\n");
  fprintf (f,
           "as published by the Free Software Foundation; either version 2\n");
  fprintf (f, "of the License, or (at your option) any later version.\n");
  fprintf (f, "\n");
  fprintf (f,
           "This program is distributed in the hope that it will be useful,\n");
  fprintf (f,
           "but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
  fprintf (f,
           "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
  fprintf (f, "GNU General Public License for more details.\n");
  fprintf (f, "\n");
  fprintf (f,
           "You should have received a copy of the GNU General Public License\n");
  fprintf (f,
           "along with this program; if not, write to the Free Software\n");
  fprintf (f,
           "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.\n");
  fprintf (f, "\n");
  fprintf (f, "See the COPYING file for license information.\n");
}
static void
usage (FILE * f, char *name)
{
  poptPrintUsage (optCon, f, 0);
}

static void
help (FILE * f, char *name)
{
  printTopDescription (f, name);
  poptPrintHelp (optCon, f, 0);
  printBottomDescription (f, name);
}

static void
version (FILE * f, char *name)
{
  printTopDescription (f, name);
  fprintf (f, copyrightID);
}

static void
license (FILE * f, char *name)
{
  printTopDescription (f, name);
  printLicense (f, name);
}

static void
error (char *msg)
{
  fprintf (stderr, "%s: %s\n", program_name, msg);
  usage (stdout, program_name);
  exit (1);
}

static void *
nullcheck (void *p)
{
  if (p == NULL)
    {
      fprintf (stderr, "%s: memory allocation error\n", program_name);
      exit (2);
    }
  return p;
}
