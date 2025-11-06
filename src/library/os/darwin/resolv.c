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

 #define _GNU_SOURCE
 #include <config.h>

 #include <private/mainloop.h>
 #include <private/session.h>
 #include <dispatch/dispatch.h>
 #include <private/intl.h>

 #include <CoreFoundation/CoreFoundation.h>
 #include <CFNetwork/CFHost.h>
 #include <sys/socket.h>
 #include <netdb.h>
 #include <arpa/inet.h>

 typedef struct {
	LIB3270_NET_CONTEXT parent;
	H3270 *hSession;
	CFStringRef cfhostname;
	CFHostRef hostRef;

	void *timer;
	char *hostname;
	char *service;

 } Context;

 /// @brief Called with the user asks for disconnection/cancel.
 /// @param hSession The tn3270 session.
 /// @param context The current context
 /// @return 0 if ok
 static int cancel(H3270 *hSession, Context *context) {	

	return ENOTSUP;
 }

 static int finalize(H3270 *hSession, Context *context) {

	debug("%s: Releasing resolver context %p",__FUNCTION__,context);

	if(context->timer) {
		hSession->timer.remove(hSession,context->timer);
		context->timer = NULL;
	}

	if(context->hostname) {
	    CFRelease(context->hostname);
	}

	if(context->hostRef) {
		CFHostSetClient(context->hostRef, NULL, NULL);
	    CFRelease(context->hostRef);
	}

	lib3270_free(context);
	return 0;

 }

 static void failed(H3270 *hSession, Context *context) {
	connection_close(hSession,-1);
 }

 static int net_timeout(H3270 *hSession, Context *context) {

	trace_network(
		hSession,
		"Hostname resolution to %s:%s has timed out, cancelling\n",
		context->hostname,
		context->service
	);

	context->timer = NULL;
	connection_close(hSession,ETIMEDOUT);

	LIB3270_POPUP popup = {
		.name		= "connection-error",
		.type		= LIB3270_NOTIFY_CONNECTION_ERROR,
		.title		= _("Connection error"),
		.summary	= _("Unable to connect to host"),
		.body		= strerror(ETIMEDOUT),
		.label		= _("OK")
	};

	lib3270_popup(hSession, &popup, 0);

	return 0;

 }

// Callback function executed when the resolution is complete
void HostResolutionCallback(CFHostRef host, CFHostInfoType infoType, const CFStreamError *error, void *info) {

	Context *context = (Context *)info;

	/*
    if (error && error->error != 0) {
        fprintf(stderr, "DNS resolution failed for %s. Error: %d\n", CFStringGetCStringPtr(context->hostname, kCFStringEncodingUTF8), error->error);
    } else if (infoType == kCFHostAddresses) {
        Boolean hasBeenResolved;
        CFArrayRef addresses = CFHostGetAddressing(host, &hasBeenResolved);

        if (hasBeenResolved && addresses != NULL) {
            CFIndex count = CFArrayGetCount(addresses);
            printf("Resolved addresses for %s:\n", CFStringGetCStringPtr(context->hostname, kCFStringEncodingUTF8));

            for (CFIndex i = 0; i < count; i++) {
                CFDataRef addressData = (CFDataRef)CFArrayGetValueAtIndex(addresses, i);
                struct sockaddr *sockaddrPtr = (struct sockaddr *)CFDataGetBytePtr(addressData);
                char addressString[INET6_ADDRSTRLEN];

                if (sockaddrPtr->sa_family == AF_INET) {
                    struct sockaddr_in *ipv4 = (struct sockaddr_in *)sockaddrPtr;
                    inet_ntop(AF_INET, &(ipv4->sin_addr), addressString, INET6_ADDRSTRLEN);
                    printf("  IPv4: %s\n", addressString);
                } else if (sockaddrPtr->sa_family == AF_INET6) {
                    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)sockaddrPtr;
                    inet_ntop(AF_INET6, &(ipv6->sin6_addr), addressString, INET6_ADDRSTRLEN);
                    printf("  IPv6: %s\n", addressString);
                }
            }
        } else {
            printf("No addresses found for %s\n", CFStringGetCStringPtr(context->hostname, kCFStringEncodingUTF8));
        }
    }
	*/

    // Stop the run loop after completion
    CFRunLoopStop(CFRunLoopGetCurrent());

}

 ///
 /// @brief Asynchronously resolves the hostname to an IP address.
 ///
 /// This function takes a hostname as input and performs an asynchronous DNS
 /// resolution to obtain the corresponding IP address. It uses non-blocking
 /// operations to ensure that the main thread is not blocked while waiting for
 /// the DNS resolution to complete.
 ///
 /// @param hostname The hostname to be resolved.
 /// @param service The service/port name.
 /// @param timeout The connection timeout in seconds.
 /// @return The resolver network context, NULL if failed.
 ///
 LIB3270_INTERNAL LIB3270_NET_CONTEXT * resolv_hostname(H3270 *hSession, const char *hostname, const char *service, time_t timeout) {

	debug("Connection %s:%s",hostname,service);

	// Allocate and setup context
	Context *context = lib3270_malloc(sizeof(Context) + strlen(hostname) + strlen(service) + 3);
	memset(context,0,sizeof(Context));
	context->hSession = hSession;

	context->hostname = (char *) (context + 1);
	strcpy(context->hostname,hostname);
	context->service = context->hostname + strlen(hostname) + 1;
	strcpy(context->service,service);	

	// Set disconnect handler
	context->parent.disconnect = (void *) cancel;
	context->parent.finalize = (void *) finalize;

	// Setup timer.
	context->timer = hSession->timer.add(hSession,timeout*1000,(void *) net_timeout,context);

	// Setup CFHost resolution here...
	context->cfhostname = CFStringCreateWithCString(NULL, context->hostname, kCFStringEncodingUTF8);
	context->hostRef = CFHostCreateWithName(NULL, context->cfhostname);

	if (context->hostRef == NULL) {

		LIB3270_POPUP popup = {
			.name		= "resolver-error",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("DNS Resolver error"),
			.summary	= _("Failed to create resolver context"),
			.body		= _("Failed to create a context to perform the host name resolution. Please check your system configuration."),
			.label		= _("OK")
		};

		connection_close(hSession,-1);
		lib3270_popup_async(hSession, &popup);

		return NULL;
	}

	CFHostClientContext hostContext = {
		.version = 0,
		.info = &context,
		.retain = NULL,
		.release = NULL,
		.copyDescription = NULL
    };

	// Set the client context and callback
    if (!CFHostSetClient(context->hostRef, HostResolutionCallback, &hostContext)) {

		LIB3270_POPUP popup = {
			.name		= "resolver-error",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("DNS Resolver error"),
			.summary	= _("Failed to setup resolver client"),
			.body		= _("Failed to perform the host name resolution. Please check your system configuration."),
			.label		= _("OK")
		};

		connection_close(hSession,-1);
		lib3270_popup_async(hSession, &popup);

		return NULL;
    }

	// Schedule the host resolution on the current run loop (main loop in this case)
    CFHostScheduleWithRunLoop(context->hostRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    // Start the asynchronous resolution for addresses
    CFStreamError error;
    if (!CFHostStartInfoResolution(context->hostRef, kCFHostAddresses, &error)) {

		CFHostUnscheduleFromRunLoop(context->hostRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

		LIB3270_POPUP popup = {
			.name		= "resolver-error",
			.type		= LIB3270_NOTIFY_ERROR,
			.title		= _("DNS Resolver error"),
			.summary	= _("Failed to start CFHost info resolution."),
			.body		= _("Failed to perform the host name resolution. Please check your system configuration."),
			.label		= _("OK")
		};

		connection_close(hSession,-1);
		lib3270_popup_async(hSession, &popup);

		// Start the run loop and wait for the callback to call CFRunLoopStop()
    	CFRunLoopRun();

		return NULL;

	}	
	return (LIB3270_NET_CONTEXT *) context;
 }
