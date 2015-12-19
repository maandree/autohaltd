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
#include "common.h"
#include "check.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>



/**
 * Used by autohaltd to check if its time to shut down,
 * and if so, do so using shutdown(8). If it is not time
 * it exec:s autohaltd-sleep with an adjusted sleep length.
 * 
 * @param   argc  The number of arguments in `argv`. Must be atleast 1.
 * @param   argv  Command line arguments, the name of the process,
 *                followed by arguments to pass to shutdown(8), in
 *                addition to the standard arguments.
 * @return        Always 1. The process will only exit on error.
 */
int main(int argc, char* argv[])
{
  unsigned long long int seconds;
  char envval[3 * sizeof(seconds) + 1];
  int r;
  sigset_t set;
  char* seconds_;
  
  /* Block signals. This process image is ephemeral. */
  signal(SIGHUP, SIG_IGN);
  siginterrupt(SIGHUP, 0);
  sigemptyset(&set);
  sigaddset(&set, SIGHUP);
  sigprocmask(SIG_BLOCK, &set, NULL);
  
  /* Get sleep interval, and validate `argc`. */
  seconds_ = getenv("AUTOHALTD_INTERVAL_PROPER");
  if (!seconds_ || !argc)
    return 1;
  seconds = (unsigned long long int)atoll(seconds_);
  if (seconds == 0)
    seconds = (unsigned long long int)(AUTOHALTD_DEFAULT_INTERVAL);
  
  /* How long ago was it that anyone logout? */
  r = is_time_for_halt(&seconds);
  if (r < 0)
    goto fail;
  if (r == 0)
    goto resleep;
  
  /* Halt. */
  halt(argc, argv);
  goto fail;
  
  /* Sleep. */
 resleep:
  sprintf(envval, "%llu", seconds);
  if (setenv("AUTOHALTD_INTERVAL", envval, 1))
    goto fail;
  siginterrupt(SIGHUP, 1);
  sigprocmask(SIG_UNBLOCK, &set, NULL);
  execv(AUTOHALTD_SLEEP_PATHNAME, argv);
 fail:
  perror(*argv);
  return 1;
}

