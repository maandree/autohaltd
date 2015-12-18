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
#include "common.h"

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>



/**
 * Update process image?
 */
static volatile sig_atomic_t received_update = 0;



/**
 * Invoked when killed by `SIGHUP`.
 * 
 * @param  signo  Will be `SIGHUP`.
 */
static void signal_update(int signo)
{
  received_update = 1;
  signal(signo, signal_update);
}


/**
 * Use by autohaltd to sleep for an extended time.
 * When the sleep is done, the process exec:s into
 * another images that handels the potential halt.
 * If the machine is not to halt, the process will
 * exec back into this process image for another
 * sleep. The goal with this setup is to reduce
 * memory footprint, since the process is mostly
 * ideal.
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
  unsigned partial_seconds;
  
  /* Get sleep interval, and validate `argc`. */
  {
    char* seconds_ = getenv("AUTOHALTD_INTERVAL");
    if (!seconds_ || !argc)
      return 1;
    seconds = (unsigned long long int)atoll(seconds_);
    /* (No memory usage difference between using `genenv` and doing it manually.
     *  The binary is however reduces with (only) 8 bytes on amd64.) */
  }
  if (seconds == 0)
    seconds = (unsigned long long int)(AUTOHALTD_DEFAULT_INTERVAL);
  
  /* Set up signal hander for online updating. */
  signal(SIGHUP, signal_update);
  
  /* Sleep. */
  while (seconds > 0)
    {
      if (seconds > 65535ULL)
	partial_seconds = 65535;
      else
	partial_seconds = (unsigned)seconds;
      seconds -= partial_seconds - sleep(partial_seconds);
      if (received_update)
	{
	  execv(AUTOHALTD_SLEEP_PATHNAME, argv);
	  perror(*argv);
	  received_update = 0;
	}
    }
  
  /* Perhaps shutdown. */
  execv(AUTOHALTD_CHECK_PATHNAME, argv);
  perror(*argv);
  return 1;
}

