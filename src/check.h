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
 * Return whether it is time to halt the machine.
 * 
 * @param   seconds  The time, in seconds, that it is required that
 *                   the machine has been unused, before the machine
 *                   halts. If 0 is returned, it will be updated to
 *                   name the number of seconds in which it is
 *                   appropriate to check again.
 * @return           1 if it is time, 0 if it is not time, -1 on error.
 */
int is_time_for_halt(unsigned long long int* seconds);


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
void halt(int argc, char* argv[]);

