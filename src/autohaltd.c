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
		  "\t-f, --foreground   Do not daemonise the process.\n"
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
 * Daemonise the process
 * 
 * @return  0 on success, -1 on error.
 */
static int daemonise(void)
{
  struct rlimit rlimit;
  int fd, signo, closeerr;
  int pipe_rw[2];
  pid_t pid;
  char* env;
  char b = 0;
  sigset_t set;
  
  if (getrlimit(RLIMIT_NOFILE, &rlimit))
    {
      perror(execname);
      rlimit.rlim_cur = 4 << 10;
    }
  for (fd = 4 /* we use 3 internally */; (rlim_t)fd < rlimit.rlim_cur; fd++)
    /* File descriptors with numbers above and including
     * `rlimit.rlim_cur` cannot be created. They cause EBADF. */
    close(fd);
  
  for (signo = 1; signo < _NSIG; signo++)
    signal(signo, SIG_DFL);
  
  sigfillset(&set);
  sigdelset(&set, SIGSTOP);
  sigdelset(&set, SIGCONT);
  sigdelset(&set, SIGTERM);
  sigdelset(&set, SIGHUP);
  sigprocmask(SIG_SETMASK, &set, NULL);
  
  /* Nothing in the environment can negatively impact us, and we may need it. */
  
  if (pipe(pipe_rw))
    return -1;
  if (dup2(pipe_rw[0], 10) == -1)
    return -1;
  close(pipe_rw[0]);
  pipe_rw[0] = 10;
  if (dup2(pipe_rw[1], 11) == -1)
    return -1;
  close(pipe_rw[1]);
  pipe_rw[1] = 11;
  
  pid = fork();
  if (pid == -1)
    return -1;
  
  if (pid <= 0)
    {
      close(pipe_rw[0]);
      
      if (setsid() == -1)
	perror(execname);
      
      pid = fork();
      if (pid == -1)
	perror(execname);
      
      if (pid > 0)
	exit(0);
      
      closeerr = (isatty(2) || (errno == EBADF));
      if ((env = getenv("DAEMONS_LOG_TO_STDERR")))
	if (!strcasecmp(env, "yes") || !strcasecmp(env, "y") || !strcmp(env, "1"))
	  closeerr = 0;
      fd = open(DEVDIR "/null", O_RDWR);
      if (fd == -1)
	perror(execname);
      else
	{
	  if (fd != 0)  close(0);
	  if (fd != 1)  close(1);
	  if (closeerr)
	    if (fd != 2)  close(2);
	  if (dup2(fd, 0) == -1)  perror(execname);
	  if (dup2(fd, 1) == -1)  perror(execname);
	  if (closeerr)
	    if (dup2(fd, 2) == -1)  perror(execname);
	  if (fd > 2)  close(fd);
	}
      
      umask(0);
      
      if (chdir("/"))
	perror(execname);
      
      fd = open(RUNDIR "/autohaltd.pid", O_WRONLY | O_CREAT | O_EXCL, 0644);
      if (fd == -1)
	{
	  if (errno == EEXIST)
	    {
	      fprintf(stderr, _("%s: PID file already exists: %s\n"),
		      execname, RUNDIR "/autohaltd.pid");
	      errno = 0;
	      return -1;
	    }
	  perror(execname);
	}
      else
	{
	  pid = getpid();
	  dprintf(fd, "%lli\n", (long long int)pid);
	  close(fd);
	}
      
      /* We cannot drop privileges. We need them! */
      
      if (write(pipe_rw[1], &b, (size_t)1) <= 0)
	return -1;
      if (close(pipe_rw[1]))
	return -1;
    }
  else
    {
      close(pipe_rw[1]);
      exit(read(pipe_rw[0], &b, (size_t)1) <= 0);
    }
  
  return 0;
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
  
  int r, have_internal = 0, foreground = 0;
  unsigned long long int seconds = 0;
  int pipe_rw[2];
  struct option long_options[] =
    {
      {"help",       no_argument, NULL, 'h'},
      {"version",    no_argument, NULL, 'v'},
      {"copyright",  no_argument, NULL, 'c'},
      {"foreground", no_argument, NULL, 'f'},
      {NULL,         0,           NULL,  0 }
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
      r = getopt_long(argc, argv, "-hvcf", long_options, NULL);
      if      (r == -1)   break;
      else if (r == 'h')  return -(print_help());
      else if (r == 'v')  return -(print_version());
      else if (r == 'c')  return -(print_copyright());
      else if (r == 'f')  foreground = 1;
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
  if (pipe_rw[0] != 3)
    {
      if (dup2(pipe_rw[0], 3) == -1)
	goto fail;
      close(pipe_rw[0]);
    }
  
  /* Daemonisation. */
  if (!foreground)
    if (daemonise())
      goto fail;
  
  /* TODO exec */
  
 fail:
  if (errno)
    perror(execname);
  return 1;
}

