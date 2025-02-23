/* SPDX-License-Identifier: LGPL-3.0-or-later */

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
 #include <winsock2.h>
 #include <windows.h>
 #include <private/session.h>
 #include <private/trace.h>
 #include <lib3270/log.h>
 #include <lib3270/memory.h>
 #include <stdio.h>
 #include <sys/stat.h>
 #include <string.h>

 struct syslog_context {
	LIB3270_LOG_CONTEXT parent;
	HANDLE handle;
 };

 static void syslog_finalize(H3270 *session, struct syslog_context *context) {
	DeregisterEventSource(context->handle);
	lib3270_free(context);
 }

 static int syslog_write(H3270 *session, struct syslog_context *context, const char *domain, const char *fmt, va_list args) {

	char	username[201];
	DWORD	szName = 200;

	memset(username,0,201);
	GetUserName(username, &szName);
	
	lib3270_autoptr(char) msg = lib3270_vsprintf(fmt,args);

	const char *outMsg[] = {
		username,
		domain,
		msg
	};

#ifdef DEBUG
	fprintf(stderr,"LOG(%s): %s\n",domain,msg);
	fflush(stderr);
#endif // DEBUG

	ReportEvent(
		context->handle,
		EVENTLOG_INFORMATION_TYPE,
		1,
		0,
		NULL,
		3,
		0,
		outMsg,
		NULL
	);

	return 0;

 }

 LIB3270_EXPORT int lib3270_log_open_syslog(H3270 *hSession) {

	struct syslog_context * context = lib3270_new(struct syslog_context);

	context->parent.finalize = (void *) syslog_finalize;
	context->parent.write = (void *) syslog_write;
	context->handle = RegisterEventSource(NULL, LIB3270_STRINGIZE_VALUE_OF(PRODUCT_NAME));

	return ENOTSUP;
 }

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
