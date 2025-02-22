/*
 * Copyright 1995, 1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006 by Paul
 *    Mattes.
 *  Permission to use, copy, modify, and distribute this software and its
 *  documentation for any purpose and without fee is hereby granted,
 *  provided that the above copyright notice appear in all copies and that
 *  both that copyright notice and this permission notice appear in
 *  supporting documentation.
 *
 * x3270, c3270, s3270 and tcl3270 are distributed in the hope that they will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the file LICENSE
 * for more details.
 */

/***
 * @brief Global declarations for util.c.
 */

LIB3270_INTERNAL char 	* ctl_see(int c);

LIB3270_INTERNAL char 	* xs_buffer(const char *fmt, ...) LIB3270_GNUC_FORMAT(1, 2);
LIB3270_INTERNAL void	  xs_error(const char *fmt, ...) LIB3270_GNUC_FORMAT(1, 2);
LIB3270_INTERNAL void	  xs_warning(const char *fmt, ...) LIB3270_GNUC_FORMAT(1, 2);

#include <private/mainloop.h>

/**
 * @brief "unescape" text (Replaces %value for corresponding character).
 *
 * @param text	Text to convert.
 *
 * @return Converted string (release it with g_free).
 *
 */
LIB3270_INTERNAL char * unescape(const char *text);

/**
 * @brief Compare strings ignoring non alphanumeric chars.
 *
 * @param s1	First string.
 * @param s2	Second string.
 *
 * @return 0 if equal, non zero if not.
 *
 */
LIB3270_INTERNAL int compare_alnum(const char *s1, const char *s2);

