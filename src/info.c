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
#include "info.h"

#include <stdio.h>
#ifdef USE_GETTEXT
# include <libintl.h>
# define _(MSG)  (gettext(MSG))
#else
# define _(MSG)  (MSG)
#endif



/**
 * Print program name and version.
 * 
 * @param   program  The name of the program.
 * @return           Zero on success, -1 on error.
 */
int print_version(const char* program)
{
  return printf(_("%s %s\n"
		  "Copyright (C) %s.\n"
		  "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>.\n"
		  "This is free software: you are free to change and redistribute it.\n"
		  "There is NO WARRANTY, to the extent permitted by law.\n"
		  "\n"
		  "Written by Mattias Andrée.\n"),
		program, "(autohaltd) " PROGRAM_VERSION,
		"2015 Mattias Andrée") < 0 ? -1 : 0;
}


/**
 * Print copyright information.
 * 
 * @return  Zero on success, -1 on error.
 */
int print_copyright(void)
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

