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


/**
 * The pathname of this daemon.
 */
#ifndef AUTOHALTD_PATHNAME
# define AUTOHALTD_PATHNAME  SBINDIR "autohaltd"
#endif

/**
 * The pathname of the process image used for just sleeping.
 */
#ifndef AUTOHALTD_SLEEP_PATHNAME
# define AUTOHALTD_SLEEP_PATHNAME  LIBEXECDIR "/" PACKAGE "/autohaltd-sleep"
#endif

/**
 * The pathname of the process image used for shuting down
 * if the machine is inactive.
 */
#ifndef AUTOHALTD_CHECK_PATHNAME
# define AUTOHALTD_CHECK_PATHNAME  LIBEXECDIR "/" PACKAGE "/autohaltd-check"
#endif

/**
 * The filename of the shutdown program.
 */
#ifndef SHUTDOWN_FILENAME
# define SHUTDOWN_FILENAME  "shutdown"
#endif

/**
 * The default interval.
 */
#ifndef AUTOHALTD_DEFAULT_INTERVAL
# define AUTOHALTD_DEFAULT_INTERVAL  (1 * 60 * 60)  /* 1 hour */
#endif

