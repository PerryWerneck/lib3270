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

	LIB3270_NET_CONTEXT parent;
	H3270 *hSession;
	
	HCERTSTORE hMyCertStore;
	SCHANNEL_CRED SchannelCred;
	CredHandle hClientCreds;
	CtxtHandle hContext;
	
	
	void (*complete)(H3270 *hSession);

 } Context;

 static int disconnect(H3270 *hSession, Context *context) {

	return 0;
 }

 static int finalize(H3270 *hSession, Context *context) {


	if(context->hMyCertStore) {
		CertCloseStore(context->hMyCertStore,0);
		context->hMyCertStore = NULL;
	}

	lib3270_free(context);
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
	
	context->hMyCertStore = CertOpenSystemStore(0, "MY");
	if(!context->hMyCertStore) {
		trace_ssl(context->hSession,"Failed to open certificate store\n");
		return GetLastError();
	}

	// https://learn.microsoft.com/en-us/windows/win32/api/schannel/ns-schannel-schannel_cred
	memset( &context->SchannelCred, 0, sizeof(context->SchannelCred));	
	// context->SchannelCred.grbitEnabledProtocols = SP_PROT_TLS1;
	context->SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;

	context->SchannelCred.dwFlags = 
		SCH_CRED_NO_DEFAULT_CREDS|SCH_CRED_REVOCATION_CHECK_CHAIN;



	return 0;
 }

 LIB3270_INTERNAL int start_tls(H3270 *hSession, void (*complete)(H3270 *hSession)) {

	int rc;

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
	context->hSession = hSession;
	context->complete = complete;
	context->parent.disconnect = (void *) disconnect;
	context->parent.finalize = (void *) finalize;

	set_network_context(hSession,(LIB3270_NET_CONTEXT *) context);

	rc = create_credentials(context);
	if(rc) {
		
		lib3270_autoptr(char) name = lib3270_strdup_printf("security-credentials-%d",rc);
		lib3270_autoptr(char) body = lib3270_strdup_printf(
			_("Windows error %d creating security credentials. Please check your system configuration."),
			rc
		);

		LIB3270_POPUP popup = {
			.name		= name,
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("Security credentials error"),
			.summary	= _("Failed to create security credentials"),
			.body		= body,
			.label		= _("OK")
		};

		connection_close(hSession,errno);
		lib3270_popup_async(hSession, &popup);

		return -1;

	}



	return 0;
 }

