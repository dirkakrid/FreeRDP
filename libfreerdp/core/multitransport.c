/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * MULTITRANSPORT PDUs
 *
 * Copyright 2014 Dell Software <Mike.McDonald@software.dell.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define WITH_DEBUG_MULTITRANSPORT

#include "multitransport.h"
#include "rdpudp.h"

static BOOL multitransport_send_initiate_error(
	rdpMultitransport* multitransport,
	UINT32 requestId,
	UINT32 hrResponse
)
{
	rdpRdp* rdp;
	wStream* s;

	rdp = multitransport->rdp;

	/* Send the response PDU to the server */
	s = rdp_message_channel_pdu_init(rdp);
	if (s == NULL) return FALSE;

	DEBUG_AUTODETECT("sending initiate error PDU");

	Stream_Write_UINT32(s, requestId); /* requestId (4 bytes) */
	Stream_Write_UINT32(s, hrResponse); /* hrResponse (4 bytes) */

	return rdp_send_message_channel_pdu(rdp, s, SEC_TRANSPORT_RSP);
}

static BOOL multitransport_recv_initiate_request(
	rdpMultitransport* multitransport,
	UINT32 requestId,
	UINT16 requestedProtocol,
	BYTE* securityCookie
)
{
	rdpUdp* rdpudp;

	DEBUG_MULTITRANSPORT("requestId=%x, requestedProtocol=%x", requestId, requestedProtocol);

	rdpudp = rdpudp_new(multitransport->rdp);
	if (rdpudp == NULL) return FALSE;

	return rdpudp_init(rdpudp, requestId, requestedProtocol, securityCookie);
}

int rdp_recv_multitransport_packet(rdpRdp* rdp, wStream* s)
{
	UINT32 requestId;
	UINT16 requestedProtocol;
	UINT16 reserved;
	BYTE securityCookie[16];

	if (Stream_GetRemainingLength(s) < 24)
		return -1;

	Stream_Read_UINT32(s, requestId); /* requestId (4 bytes) */
	Stream_Read_UINT16(s, requestedProtocol); /* requestedProtocol (2 bytes) */
	Stream_Read_UINT16(s, reserved); /* reserved (2 bytes) */
	Stream_Read(s, securityCookie, 16); /* securityCookie (16 bytes) */

	multitransport_recv_initiate_request(rdp->multitransport, requestId, requestedProtocol, securityCookie);

	return 0;
}

rdpMultitransport* multitransport_new(rdpRdp* rdp)
{
	rdpMultitransport* multitransport = (rdpMultitransport*)malloc(sizeof(rdpMultitransport));
	if (multitransport)
	{
		memset(multitransport, 0, sizeof(rdpMultitransport));

		multitransport->rdp = rdp;
	}
	
	return multitransport;
}

void multitransport_free(rdpMultitransport* multitransport)
{
	if (multitransport == NULL) return;

	free(multitransport);
}
