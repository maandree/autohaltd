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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#ifdef USE_GETTEXT
# include <locale.h>
# include <libintl.h>
# define _(MSG)  (gettext(MSG))
#else
# define _(MSG)  (MSG)
#endif



/**
 * The default interval.
 */
#ifndef AUTOHALTD_DEFAULT_INTERVAL
# define AUTOHALTD_DEFAULT_INTERVAL  (1 * 60 * 60)  /* 1 hour */
#endif



/**
 * `argv[0]` from `main`.
 */
const char *execname;



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
		  "\tautohaltd shall shut down the machine when the sum of\n"
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
 * Print program name and version.
 * 
 * @return  Zero on success, -1 on error.
 */
static int print_version(void)
{
  return printf(_("%s\n"
		  "Copyright (C) %s.\n"
		  "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
		  "This is free software: you are free to change and redistribute it.\n"
		  "There is NO WARRANTY, to the extent permitted by law.\n"
		  "\n"
		  "Written by Mattias Andrée.\n"),
		"autohaltd " PROGRAM_VERSION,
		"2015 Mattias Andrée") < 0 ? -1 : 0;
}


/**
 * Print copyright information.
 * 
 * @return  Zero on success, -1 on error.
 */
static int print_copyright(void)
{
  return printf(_("autohaltd -- Bring the system down when it is inactive\n"
		  "Copyright (C) %s\n"
		  "\n"
		  "This program is free software: you can redistribute it and/or modify\n"
		  "it under the terms of the GNU General Public License as published by\n"
		  "the Free Software Foundation, either version 3 of the License, or\n"
		  "(at your option) any later version.\n"
		  "\n"
		  "This program is distributed in the hope that it will be useful,\n"
		  "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		  "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		  "GNU General Public License for more details.\n"
		  "\n"
		  "You should have received a copy of the GNU General Public License\n"
		  "along with this program.  If not, see <http://www.gnu.org/licenses/>.\n"),
		"2015  Mattias Andrée (maandree@member.fsf.org)"
		) < 0 ? -1 : 0;
}


/**
 * Shutdown the machine when it has been inactive for an extended time.
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
  int pipe_rw[2];
  struct option long_options[] =
    {
      {"help",      no_argument, NULL, 'h'},
      {"version",   no_argument, NULL, 'v'},
      {"copyright", no_argument, NULL, 'c'},
      {NULL,        0,           NULL,  0 }
    };
  
  /* Set up for internationalisation. */
#if defined(USE_GETTEXT) && defined(PACKAGE) && defined(LOCALEDIR)
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
#endif
  
  /* Parse command line. */
  execname = argc ? *argv : "autohaltd";
  for (;;)
    {
      r = getopt_long(argc, argv, "-hvc", long_options, NULL);
      if      (r == -1)   break;
      else if (r == 'h')  return -(print_help());
      else if (r == 'v')  return -(print_version());
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
	EXIT_USAGE (_("Invalid input"));
      else
	abort ();
    }
  memmove(argv + 1, argv + optind, (size_t)(argc - optind + 1) * sizeof(char*));
  /* “In any case, argv[argc] is a null pointer.” [The GNU C Reference Manual] */
  
  /* Validate interval, and possible fall back to default. */
  USAGE_ASSERT(!have_internal || seconds, "The interval cannot be zero");
  if (!have_internal)
    seconds = (unsigned long long int)(AUTOHALTD_DEFAULT_INTERVAL);
  
  /* The the next process image know the interval via a pipe. */
  if (pipe(pipe_rw))
    goto fail;
  if (write(pipe_rw[1], &seconds, sizeof(seconds)) != sizeof(seconds))
    goto fail;
  if (close(pipe_rw[1]))
    goto fail;
  if (pipe_rw[0] != 10)
    {
      if (dup2(pipe_rw[0], 10) == -1)
	goto fail;
      close(pipe_rw[0]);
    }
  
  /* TODO exec */
  
 fail:
  perror(execname);
  return 1;
}

