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
 * perry.werneck@gmail.com	(Alexandre Perry de Souza Werneck)
 * erico.mendonca@gmail.com	(Erico Mascarenhas Mendonça)
 *
 */

 #pragma once

 #include <winsock2.h>
 #include <windows.h>
 #include <sspi.h>
 #include <wincrypt.h>
 
 #include <lib3270/defs.h>
 #include <private/session.h>

 /// @brief Connection context for WinSC connections.
 typedef struct {

	LIB3270_NET_CONTEXT parent;
	H3270 *hSession;
	
	SCHANNEL_CRED SchannelCred;
	CredHandle hClientCreds;
	
	void (*complete)(H3270 *hSession);

 } Context;

 static inline void lib3270_autoptr_cleanup_CERT_CONTEXT(CERT_CONTEXT **ptr) {
	if(*ptr)
		CertFreeCertificateContext(*ptr);
	*ptr = NULL;
 }

 LIB3270_INTERNAL PSecurityFunctionTable security_context_new(void);
 