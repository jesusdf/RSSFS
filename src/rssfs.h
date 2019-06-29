/*
 * RSSFS - RSS in userspace.
 * http://www.jardinpresente.com.ar/wiki/index.php/RSSFS
 *
 * Copyright (C) 2007 Marc E. <santusmarc@users.sourceforge.net>
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
 *
 * FUSE global variables and includes
 * $Id$
 */

#define VERSION "0.1"

// Extension to use in the file names (this appends to the rss <title> tag, remove it if you dont want one.
#define RSSEXT ".torrent"

// Whether to use multithreading or not
#define MULTITHREADS

// Download files to ram directly when parsing the RSS url
// Toggable in Makefile
#ifndef NO_PREFETCH
    #define PREFETCH
#endif

// Don't query the server for file attributes when parsing the RSS file
// Toggable in Makefile
//#define DELAY_METADATA_LOADING

// Whether to print LOG_DEBUG messages to syslog
#define DEBUG

#ifdef DEBUG
#include <syslog.h>
#endif

