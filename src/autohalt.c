/**
 * Copyright © 2015  Mattias Andrée (maandree@member.fsf.org)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define _GNU_SOURCE /* For getopt_long. */
#include "common.h"
#include "check.h"
#include "info.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/resource.h>
#ifdef USE_GETTEXT
# include <locale.h>
# include <libintl.h>
# define _(MSG)  (gettext(MSG))
#else
# define _(MSG)  (MSG)
#endif



/**
 * `argv[0]` from `main`.
 */
static const char* execname;


/**
 * Print usage information.
 * 
 * @return  Zero on success, -1 on error.
 */
static int print_help(void)
{
  return printf(_("SYNOPSIS\n"
		  "\t%s [OPTION]... [INTERVAL]... [-- [SHUTDOWN_ARGUMENT]...]\n"
		  "\n"
		  "DESCRIPTION\n"
		  "\tautohalt shall shut down the machine onlyif the sum of\n"
		  "\tINTERVAL has elapsed since the last user logout. Each\n"
		  "\tINTERVAL must by a non-negative integer, optionally\n"
		  "\twith a unit:\n"
		  "\t\n"
		  "\ts	Seconds.\n"
		  "\tm	Minutes.\n"
		  "\th	Hours.\n"
		  "\t\n"
		  "\tAny argument added after '--' is passed to shutdown(8).\n"
		  "\n"
		  "OPTIONS\n"
		  "\t-h, --help         Print usage information.\n"
		  "\t-v, --version      Print program name and version.\n"
		  "\t-c, --copyright    Print copyright information.\n"
		  "\n"),
		execname) < 0 ? -1 : 0;
}


/**
 * Shut down the machine if it has been inactive for an extended time.
 * 
 * @param   argc  The number of elements in `argv`.
 * @param   argv  Command line arguments, run with `--help` for more information.
 * @return        0 on success, 1 on error, 2 on usage error.
 */
int main(int argc, char* argv[])
{
#define EXIT_USAGE(MSG)  \
  return fprintf(stderr, _("%s: %s. Type '%s --help' for help.\n"), execname, MSG, execname), 2
#define USAGE_ASSERT(ASSERTION, MSG)  \
  do { if (!(ASSERTION))  EXIT_USAGE(MSG); } while (0)
  
  int r, have_internal = 0;
  unsigned long long int seconds = 0;
  struct option long_options[] =
    {
      {"help",       no_argument, NULL, 'h'},
      {"version",    no_argument, NULL, 'v'},
      {"copyright",  no_argument, NULL, 'c'},
      {NULL,         0,           NULL,  0 }
    };
  
  /* Set up for internationalisation. */
#if defined(USE_GETTEXT) && defined(PACKAGE) && defined(LOCALEDIR)
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
#endif
  
  /* Parse command line. */
  execname = argc ? *argv : "autohalt";
  for (;;)
    {
      r = getopt_long(argc, argv, "-hvc", long_options, NULL);
      if      (r == -1)   break;
      else if (r == 'h')  return -(print_help());
      else if (r == 'v')  return -(print_version("autohalt"));
      else if (r == 'c')  return -(print_copyright());
      else if (r ==  1 )  /* `'-'` would have be some much better than `1`. */
	{
	  /* Parse interval parameter. */
	  long long int temp;
	  char* p;
	  have_internal = 1;
	  USAGE_ASSERT(isdigit(*optarg), "Interval arguments must be non-negative integers");
	  temp = strtoll(optarg, &p, 10);
	  USAGE_ASSERT(strlen(p) < 2, "Invalid interval units are 's', 'm', and 'h'");
	  switch (*p)
	    {
	    case 'h':  temp *= 60;  /* fall through */
	    case 0:
	    case 'm':  temp *= 60;  /* fall through */
	    case 's':  break;
	    default:
	      EXIT_USAGE("Invalid interval units are 's', 'm', and 'h'");
	    }
	  seconds += (unsigned long long int)temp;
	}
      else if (r == '?')
	EXIT_USAGE(_("Invalid input"));
      else
	abort();
    }
  USAGE_ASSERT (argc, "Command line must at least include the zeroth argument");
  memmove(argv + 1, argv + optind, (size_t)(argc - optind) * sizeof(char*));
  
  /* Validate interval, and possible fall back to default. */
  USAGE_ASSERT(!have_internal || seconds, "The interval cannot be zero");
  if (seconds == 0)
    seconds = (unsigned long long int)(AUTOHALTD_DEFAULT_INTERVAL);
  
  /* Check privileges. */
  USAGE_ASSERT(!getuid(), "This daemon must be run as root");
  
  /* How long ago was it that anyone logout? */
  r = is_time_for_halt(&seconds);
  if (r < 0)
    goto fail;
  if (r == 0)
    return 0;
  
  /* Halt. */
  halt(argc, argv);
  
 fail:
  if (errno)
    perror(*argv);
  return 1;
}

