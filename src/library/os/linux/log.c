/* SPDX-License-I static void console_finalize(H3270 *session, struct file_context *context) {
	lib3270_free(context);
 }
dentifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2008 Banco do Brasil S.A.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

 #include <config.h>
 #include <private/session.h>
 #include <private/trace.h>
 #include <lib3270/log.h>
 #include <lib3270/malloc.h>
 #include <stdio.h>
 #include <linux/limits.h>
 #include <sys/stat.h>
 #include <string.h>

 #ifdef HAVE_SYSLOG

 #include <syslog.h>
 
 static size_t syslog_instances = 0;

 
 static int syslog_write(const H3270 *session, struct syslog_context *context, const char *domain, const char *fmt, va_list args) {
	vsyslog(LOG_INFO, fmt, args);
	return 0;
 }

 static void syslog_finalize(const H3270 *session, struct syslog_context *context) {
	lib3270_free(context);

	if(--syslog_instances == 0) {
		closelog();
	}
 }

 LIB3270_EXPORT int lib3270_log_open_syslog(H3270 *hSession) {

	lib3270_log_close(hSession);

	if(syslog_instances++ == 0) {
		openlog(PACKAGE_NAME, LOG_CONS, LOG_USER);
	}

	LIB3270_LOG_CONTEXT *context = lib3270_new(LIB3270_LOG_CONTEXT);
	context->filename = "@syslog";
	context->write = (void *) syslog_write;
	context->finalize = (void *) syslog_finalize;
	hSession->log = (LIB3270_LOG_CONTEXT *) context;
	
	return 0;

 }

 #else 

 LIB3270_EXPORT int lib3270_log_open_syslog(H3270 *hSession) {
	return ENOTSUP;
 }

 #endif // HAVE_SYSLOG

 /// @brief Get log filename.
 LIB3270_EXPORT const char * lib3270_log_get_filename(const H3270 *hSession) {
	return hSession->log->filename;
 }

 static void get_timestamp(char timestamp[20]) {
	time_t ltime;
	time(&ltime);

#ifdef HAVE_LOCALTIME_R
	struct tm tm;
	strftime(timestamp, 20, "%x %X", localtime_r(&ltime,&tm));
#else
	strftime(timestamp, 20, "%x %X", localtime(&ltime));
#endif // HAVE_LOCALTIME_R
 }

 struct file_context {
	LIB3270_LOG_CONTEXT parent;
	FILE *fp;
 };

 static int file_write(H3270 *session, struct file_context *context, const char *domain, const char *fmt, va_list args) {

	char timestamp[20];

	get_timestamp(timestamp);
	
	fprintf(context->fp, "%s\t%-8s ", timestamp, domain);
	vfprintf(context->fp, fmt, args);
	fprintf(context->fp, "\n");

	fflush(context->fp);

	return 0;

 }

 static void file_finalize(H3270 *session, struct file_context *context) {
	fclose(context->fp);
	lib3270_free(context);
 }

 LIB3270_EXPORT int lib3270_log_open_file(H3270 *hSession, const char *template, time_t maxage) {

	lib3270_log_close(hSession);

	lib3270_autoptr(char) filename = trace_filename(hSession, template);
	if(!filename) {
		return ENOMEM;
	}

	struct stat st;
	if(!stat(filename,&st) && (time(0) - st.st_mtime) > maxage) {
		// file is old, remove it
		remove(filename);
	}

	FILE *fp = fopen(filename,"a");
	if(!fp) {
		return errno;
	}

	struct file_context *context = lib3270_malloc(sizeof(struct file_context)+strlen(filename)+1);

	context->fp = fp;

	context->parent.write =  (void *) file_write;
	context->parent.finalize = (void *) file_finalize;

	context->parent.filename = (char *) (context+1);
	strcpy((char *) context->parent.filename,filename);

	hSession->log = (LIB3270_LOG_CONTEXT *) context;

	return 0;
 }

 static void console_finalize(H3270 *session, struct file_context *context) {
	lib3270_free(context);
 }

 LIB3270_EXPORT int lib3270_log_open_console(H3270 *hSession, int option) {

	lib3270_log_close(hSession);

	struct file_context *context = lib3270_new(struct file_context);

	context->fp = option ? stderr : stdout;

	context->parent.write =  (void *) file_write;
	context->parent.finalize = (void *) console_finalize;

	hSession->log = (LIB3270_LOG_CONTEXT *) context;

	return 0;

 }
