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
#define _GNU_SOURCE
#include "check.h"
#include "common.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <alloca.h>
#include <string.h>
#include <limits.h>
#include <utmpx.h>
#include <utmp.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <sys/stat.h>



/**
 * Check whether a NORMAL_PROCESS record represents a login.
 * 
 * @param   u       The login record.
 * @param   active  Will be set to 1 if active, 0 if inactive.
 * @return          1 if it is a login, 0 otherwise.
 */
static int is_login(struct utmpx* u, int* active)
{
  static int line_initalised = 0;
  static char line[sizeof(DEVDIR "/") / sizeof(char) + UT_LINESIZE];
  char* const line_ = line + sizeof(DEVDIR) / sizeof(char);
  char fdbuf[sizeof(PROCDIR "//fd/") / sizeof(char) + 3 * sizeof(pid_t) + 3 * sizeof(int)];
  struct stat ttyattr, attr;
  int i;
  
  if (line_initalised == 0)
    {
      line_initalised = 1;
      memcpy(line, DEVDIR "/", sizeof(DEVDIR "/"));
    }
  
  memcpy(line_, u->ut_line, UT_LINESIZE * sizeof(char));
  line_[UT_LINESIZE] = '\0';
  
  if (stat(line, &ttyattr))
    return 0;
  if (S_ISCHR(ttyattr.st_mode) == 0)
    return 0;
  
  for (i = 0; i <= 2; i++)
    {
      sprintf(fdbuf, "%s/%ji/fd/%i", PROCDIR, (intmax_t)(u->ut_pid), i);
      if (stat(fdbuf, &attr))
	break;
      if (memcmp(&ttyattr, &attr, sizeof(attr)))
	break;
    }
  *active = (i > 2);
  return 1;
}


/**
 * Get the number of active logins, and the time of
 * since the last logout.
 * 
 * @param   duration  Output parameter for the time since the last logout.
 * @return            The number of active logins, truncated to `INT_MAX`
 *                    in the impossible event that there are more logins.
 *                    -1 on error.
 */
static int get_number_of_logins_and_last_logout(struct timespec* duration)
{
#define ADJUST_NSEC(ts)						\
  do								\
    {								\
      if ((ts)->tv_nsec > 1000000000L)				\
	(ts)->tv_nsec -= 1000000000L, (ts)->tv_sec += 1;	\
      else if ((ts)->tv_nsec < 0L)				\
	(ts)->tv_nsec += 1000000000L, (ts)->tv_sec -= 1;	\
    }								\
  while (0)
#ifdef _HAVE_UT_TV
# define SET_TIMESPEC(ts, u)					\
  ((ts)->tv_sec = (time_t)((u)->ut_tv.tv_sec),			\
   (ts)->tv_nsec = (long)((u)->ut_tv.tv_usec) * 1000L)
#else
# define SET_TIMESPEC(ts, u)					\
  ((ts)->tv_sec = (time_t)((u)->ut_time),			\
   (ts)->tv_nsec = 0)
#endif
  
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wpadded"
#endif
  struct utmpx* u;
  int rc = 0, saved_errno, active;
  pid_t* logins = NULL;
  size_t logins_ptr = 0;
  size_t logins_size = 0;
  struct utmpx* obsolete = NULL;
  size_t obsolete_ptr = 0;
  size_t obsolete_size = 0;
  size_t i;
  void* new;
  struct timespec delta;
  struct timespec now;
  struct timespec oldtime;
  struct timespec newtime;
  int have_oldtime = 0;
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif
  
  if (clock_gettime(CLOCK_REALTIME, &now))
    return -1;
  *duration = now;
  memset(&delta, 0, sizeof(delta));
  
  setutxent();
  
  while ((u = getutxent()))
    switch (u->ut_type)
      {
	/* Strings are not necessarily terminated! */
	
      /* Not LOGIN_PROCESS, the login process changes LOGIN_PROCESS
       * to USER_PROCESS. LOGIN_PROCESS indicates getty, or a login
       * that has been be completed. */
      case USER_PROCESS:
	if (!is_login(u, &active))
	  continue;
	if (!active)
	  goto inactive_login;
	if (logins_ptr == logins_size)
	  {
	    logins_size = logins_size ? (logins_size << 1) : 16;
	    new = realloc(logins, logins_size * sizeof(*logins));
	    if (new == NULL)
	      goto fail;
	    logins = new;
	  }
	logins[logins_ptr++] = u->ut_pid;
	if (rc < INT_MAX)
	  rc++;
	break;
      inactive_login:
	if (obsolete_ptr == obsolete_size)
	  {
	    obsolete_size = obsolete_size ? (obsolete_size << 1) : 16;
	    new = realloc(obsolete, obsolete_size * sizeof(*obsolete));
	    if (new == NULL)
	      goto fail;
	    obsolete = new;
	  }
	obsolete[obsolete_ptr++] = *u;
	break;
	
      case DEAD_PROCESS:
      case LOGIN_PROCESS: /* See above. */
      case INIT_PROCESS: /* Spawned by init, potentially a getty. */
	for (i = 0; i < logins_ptr; i++)
	  if (logins[i] == u->ut_pid)
	    break;
	if (i == logins_ptr)
	  continue;
	memmove(logins + i, logins + i + 1, (--logins_ptr - i) * sizeof(*logins));
	if ((0 < rc) && (rc < INT_MAX))
	  rc--;
	SET_TIMESPEC(duration, u);
	break;
	
      case BOOT_TIME:
	have_oldtime = 0;
	memset(&delta, 0, sizeof(delta));
	SET_TIMESPEC(duration, u);
	break;
	
      case OLD_TIME:
	have_oldtime = 1;
	SET_TIMESPEC(&oldtime, u);
	break;
	
      case NEW_TIME:
	if (have_oldtime == 0)
	  continue;
	have_oldtime = 0;
	SET_TIMESPEC(&newtime, u);
	newtime.tv_sec -= oldtime.tv_sec;
	newtime.tv_nsec -= oldtime.tv_nsec;
	ADJUST_NSEC(&newtime);
	delta.tv_sec += newtime.tv_sec;
	delta.tv_nsec += newtime.tv_nsec;
	ADJUST_NSEC(&delta);
	break;
      default:
	continue;
      }
  if ((errno != ESRCH) && (errno != ENOENT)) /* sic! */
    goto fail;
  
  duration->tv_sec -= delta.tv_sec;
  duration->tv_nsec -= delta.tv_nsec;
  ADJUST_NSEC(duration);
  
  duration->tv_sec = now.tv_sec - duration->tv_sec;
  duration->tv_nsec = now.tv_nsec - duration->tv_nsec;
  ADJUST_NSEC(duration);
  
  /* Update obsolete records. */
  for (i = 0; i < obsolete_ptr; i++)
    {
      obsolete[i].ut_type = DEAD_PROCESS;
#ifdef _HAVE_UT_TV
      obsolete[i].ut_tv.tv_sec = (int32_t)(now.tv_sec);
      obsolete[i].ut_tv.tv_usec = (int32_t)(now.tv_nsec / 1000L);
#else
      obsolete[i].ut_time = now.tv_sec;
#endif
      (void) pututxline(obsolete + i);
    }
  
 done:
  saved_errno = errno;
  endutxent();
  free(logins);
  free(obsolete);
  errno = saved_errno;
  return rc;
  
 fail:
  rc = -1;
  goto done;
}


/**
 * Return whether it is time to halt the machine.
 * 
 * @param   seconds  The time, in seconds, that it is required that
 *                   the machine has been unused, before the machine
 *                   halts. If 0 is returned, it will be updated to
 *                   name the number of seconds in which it is
 *                   appropriate to check again.
 * @return           1 if it is time, 0 if it is not time, -1 on error.
 */
int is_time_for_halt(unsigned long long int* seconds)
{
  struct timespec duration;
  int r;
  
  /* How long ago was it that anyone logout? */
  r = get_number_of_logins_and_last_logout(&duration);
  if (r < 0)
    return -1;
  if ((unsigned long long int)(duration.tv_sec) < *seconds)
    {
      *seconds -= (unsigned long long int)(duration.tv_sec);
      return 0;
    }
  /* Has everyone logged out? */
  if (r > 0)
    return 0;
  
  return 1;
}


/**
 * Halt the machine.
 * 
 * This function only returns on failure.
 * 
 * @param  argc  The number of elements in `argv`. Must be at least 1.
 * @param  argv  The zeroth element is ignored. The following elements
 *               shall be the arguments to pass to shutdown(8), in
 *               addition to '-h' and 'now' as the last to arguments.
 */
void halt(int argc, char* argv[])
{
  char** args;
  
  args = alloca((size_t)(argc + 3) * sizeof(char*));
  memcpy(args, argv, (size_t)argc * sizeof(char*));
#ifdef __GNUC__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wcast-qual"
#endif
  args[0] = (char*)(SHUTDOWN_FILENAME);
  args[argc + 0] = (char*)"-h";
  args[argc + 1] = (char*)"now";
  args[argc + 2] = NULL;
#ifdef __GNUC__
# pragma GCC diagnostic pop
#endif
  
  execv(SHUTDOWN_FILENAME, args);
}

