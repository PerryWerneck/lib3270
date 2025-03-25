/* SPDX-License-Identifier: LGPL-3.0-or-later */

/*
 * Copyright (C) 2025 Banco do Brasil S.A.
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

/*
 * Contatos:
 *
 * perry.werneck@gmail.com      (Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com     (Erico Mascarenhas Mendonça)
 *
 */

 // References:
 //
 // https://docs.microsoft.com/en-us/windows/win32/secauthn/creating-a-secure-connection-using-schannel
 // https://gist.github.com/odzhan/1b5836c4c8b02d4d9cb9ec574432432c

 #include <config.h>

 #include <winsock2.h>
 #include <windows.h>
 #include <sspi.h>
 #include <schannel.h>
 
 #include <lib3270/defs.h>
 #include <lib3270/memory.h>
 #include <private/defs.h>
 #include <private/network.h>
 #include <private/session.h>
 #include <private/win32_schannel.h>
 #include <private/trace.h>
 #include <private/intl.h>

 /// @brief Connection context for WinSC connections.
 typedef struct {

	int instances;

	LIB3270_NET_CONTEXT parent;
	H3270 *hSession;
	
	SCHANNEL_CRED SchannelCred;
	CredHandle hClientCreds;
	HANDLE thread;
	
	void (*complete)(H3270 *hSession);

 } Context;

 static int disconnect(H3270 *hSession, Context *context) {

	if(hSession->connection.sock != INVALID_SOCKET) {
		closesocket(hSession->connection.sock);
		hSession->connection.sock = INVALID_SOCKET;
	}

	return 0;
 }

 static int finalize(H3270 *hSession, Context *context) {

	if(--context->instances) {
		return 0;
	}

	HANDLE thread = context->thread;
	context->thread = 0;

	//if(context->hMyCertStore) {
	//	CertCloseStore(context->hMyCertStore,0);
	//	context->hMyCertStore = NULL;
	//}

	lib3270_free(context);


	// Close thread.
	if(thread) {
		CloseHandle(thread);
	}

	debug("WinSC context %p finalized\n",context);
	
	return 0;
 }

 LIB3270_INTERNAL PSecurityFunctionTable security_context(void) {
	static PSecurityFunctionTable securityFunctionTable = NULL;
	if(!securityFunctionTable) {
		securityFunctionTable = InitSecurityInterface();
	}
	return securityFunctionTable;
 }

 static int create_credentials(Context *context) {
	
	//context->hMyCertStore = CertOpenSystemStore(0, "MY");
	//if(!context->hMyCertStore) {
	//	trace_ssl(context->hSession,"Failed to open certificate store\n");
	//	return GetLastError();
	//}

	// https://learn.microsoft.com/en-us/windows/win32/api/schannel/ns-schannel-schannel_cred
	memset( &context->SchannelCred, 0, sizeof(context->SchannelCred));	
	// context->SchannelCred.grbitEnabledProtocols = SP_PROT_TLS1;
	context->SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;

	context->SchannelCred.dwFlags = 
		SCH_CRED_NO_DEFAULT_CREDS|SCH_CRED_REVOCATION_CHECK_CHAIN;

	// The SCH_CRED_MANUAL_CRED_VALIDATION flag is specified because
    // we do the server certificate verification manually.
    // SchannelCred.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;

	// Create an SSPI credential.
	TimeStamp tsExpiry;
	SECURITY_STATUS Status = security_context()->AcquireCredentialsHandle( 
		NULL,                 		// Name of principal    
		UNISP_NAME_A,         		// Name of package
		SECPKG_CRED_OUTBOUND, 		// Flags indicating use
		NULL,                 		// Pointer to logon ID
		&context->SchannelCred,		// Package specific data
		NULL,                 		// Pointer to GetKey() func
		NULL,                 		// Value to pass to GetKey()
		&context->hClientCreds,     // (out) Cred Handle
		&tsExpiry          			// (out) Lifetime (optional)
	);

	if(Status != SEC_E_OK) {
		trace_ssl(context->hSession, "Failed to acquire credentials handle. Error code: 0x%x\n", (int) Status);
		return Status;
	}

	return 0;

 }

 static DWORD __stdcall PerformClientHandshake(LPVOID lpParam) {
	Context *context = (Context *) lpParam;
	trace_ssl(context->hSession, "Running TLS/SSL handshake\n");
	

	return 0;
 }

 LIB3270_INTERNAL int start_tls(H3270 *hSession, void (*complete)(H3270 *hSession)) {

	debug("------------------[ %s ]------------------\n",__FUNCTION__);

	memset(&hSession->ssl.message,0,sizeof(hSession->ssl.message));
	set_ssl_state(hSession,LIB3270_SSL_NEGOTIATING);

	PSecurityFunctionTable sspi = security_context();

	if(!sspi) {
		LIB3270_POPUP popup = {
			.name		= "security-interface-error",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("Security interface error"),
			.summary	= _("Failed to initialize security interface"),
			.body		= _("Failed to initialize the Windows security interface. Please check your system configuration."),
			.label		= _("OK")
		};

		connection_close(hSession,-1);
		lib3270_popup_async(hSession, &popup);

		return -1;

	}

	Context *context = lib3270_new(Context);
	memset(context,0,sizeof(Context));
	context->instances = 1;
	context->hSession = hSession;
	context->complete = complete;
	context->parent.disconnect = (void *) disconnect;
	context->parent.finalize = (void *) finalize;

	set_network_context(hSession,(LIB3270_NET_CONTEXT *) context);

	// Create credentials
	{
		int rc = create_credentials(context);
		if(rc) {
			
			lib3270_autoptr(char) name = lib3270_strdup_printf("security-credentials-%d",rc);

			lib3270_autoptr(char) summary = lib3270_strdup_printf(
				_("Failed to establish a secure TLS/SSL connection with the server at URL: %s."),
				hSession->connection.url
			);

			lib3270_autoptr(char) body = lib3270_strdup_printf(
				_("Windows error 0x%x creating security credentials. Please check your system configuration."),
				rc
			);

			LIB3270_POPUP popup = {
				.name		= name,
				.type		= LIB3270_NOTIFY_ERROR,
				.title		= _("TLS/SSL error"),
				.summary	= summary,
				.body		= body,
				.label		= _("OK")
			};

			connection_close(hSession,errno);
			lib3270_popup_async(hSession, &popup);

			return -1;

		}
	}

	context->instances++;
	context->thread = CreateThread(NULL,0,PerformClientHandshake,context,0,NULL);
	if(!context->thread) {

		context->instances--;
		trace_ssl(hSession,"Failed to create handshake thread\n");

		LIB3270_POPUP popup = {
			.name		= "security-handshake-error",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("TLS/SSL error"),
			.summary	= _("Failed to create handshake thread"),
			.body		= _("Failed to create a thread to perform the TLS/SSL handshake. Please check your system configuration."),
			.label		= _("OK")
		};

		connection_close(hSession,-1);
		lib3270_popup_async(hSession, &popup);

		return -1;
	}

	finalize(hSession,context); // Release current instance, the thread still have one.
	return 0;

 }

