/**
 * FreeRDP: A Remote Desktop Protocol Client
 * Kerberos Auth Protocol
 *
 * Copyright 2015 ANSSI, Author Thomas Calderon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <winpr/crt.h>
#include <winpr/sspi.h>
#include <winpr/print.h>
#include <winpr/sysinfo.h>
#include <winpr/registry.h>

#include "kerberos.h"

#include "../sspi.h"
#include "../../log.h"
#define TAG WINPR_TAG("sspi.Kerberos")

char* KRB_PACKAGE_NAME = "Kerberos";

static sspi_gss_OID_desc g_SSPI_GSS_C_SPNEGO_KRB5 = { 9, (void *) "\x2a\x86\x48\x86\xf7\x12\x01\x02\x02" };
sspi_gss_OID SSPI_GSS_C_SPNEGO_KRB5 = &g_SSPI_GSS_C_SPNEGO_KRB5;

KRB_CONTEXT* kerberos_ContextNew()
{
	KRB_CONTEXT* context;

	context = (KRB_CONTEXT*) calloc(1, sizeof(KRB_CONTEXT));

	if (!context)
		return NULL;

	context->minor_status = 0;
	context->major_status = 0;

	context->gss_ctx = SSPI_GSS_C_NO_CONTEXT;
	context->cred = SSPI_GSS_C_NO_CREDENTIAL;

	return context;
}

void kerberos_ContextFree(KRB_CONTEXT* context)
{
	if (!context)
		return;

	/* FIXME: should probably free some GSSAPI stuff */

	free(context);
}

SECURITY_STATUS SEC_ENTRY kerberos_AcquireCredentialsHandleW(SEC_WCHAR* pszPrincipal, SEC_WCHAR* pszPackage,
				   ULONG fCredentialUse, void* pvLogonID, void* pAuthData,
				   SEC_GET_KEY_FN pGetKeyFn, void* pvGetKeyArgument,
				   PCredHandle phCredential, PTimeStamp ptsExpiry)
{
	return SEC_E_OK;
}

SECURITY_STATUS SEC_ENTRY kerberos_AcquireCredentialsHandleA(SEC_CHAR* pszPrincipal, SEC_CHAR* pszPackage,
				   ULONG fCredentialUse, void* pvLogonID, void* pAuthData,
				   SEC_GET_KEY_FN pGetKeyFn, void* pvGetKeyArgument,
				   PCredHandle phCredential, PTimeStamp ptsExpiry)
{
	return SEC_E_OK;
}

SECURITY_STATUS SEC_ENTRY kerberos_FreeCredentialsHandle(PCredHandle phCredential)
{
	SSPI_CREDENTIALS* credentials;

	if (!phCredential)
		return SEC_E_INVALID_HANDLE;

	credentials = (SSPI_CREDENTIALS*) sspi_SecureHandleGetLowerPointer(phCredential);

	if (!credentials)
		return SEC_E_INVALID_HANDLE;

	sspi_CredentialsFree(credentials);

	return SEC_E_OK;
}

SECURITY_STATUS SEC_ENTRY kerberos_QueryCredentialsAttributesW(PCredHandle phCredential, ULONG ulAttribute, void* pBuffer)
{
	if (ulAttribute == SECPKG_CRED_ATTR_NAMES)
	{
		return SEC_E_OK;
	}

	return SEC_E_UNSUPPORTED_FUNCTION;
}

SECURITY_STATUS SEC_ENTRY kerberos_QueryCredentialsAttributesA(PCredHandle phCredential, ULONG ulAttribute, void* pBuffer)
{
	return kerberos_QueryCredentialsAttributesW(phCredential, ulAttribute, pBuffer);
}

SECURITY_STATUS SEC_ENTRY kerberos_InitializeSecurityContextW(PCredHandle phCredential, PCtxtHandle phContext,
				    SEC_WCHAR* pszTargetName, ULONG fContextReq, ULONG Reserved1,
				    ULONG TargetDataRep, PSecBufferDesc pInput, ULONG Reserved2,
				    PCtxtHandle phNewContext, PSecBufferDesc pOutput,
				    ULONG* pfContextAttr, PTimeStamp ptsExpiry)
{
	return SEC_E_UNSUPPORTED_FUNCTION;
}

int kerberos_SetContextServicePrincipalNameA(KRB_CONTEXT* context, SEC_CHAR* ServicePrincipalName)
{
	char* p;
	UINT32 major_status;
	UINT32 minor_status;
	char* gss_name = NULL;
	sspi_gss_buffer_desc name_buffer;

	if (!ServicePrincipalName)
	{
		context->target_name = NULL;
		return 1;
	}

	/* GSSAPI expects a SPN of type <service>@FQDN, let's construct it */

	gss_name = _strdup(ServicePrincipalName);

	if (!gss_name)
		return -1;

	p = strchr(gss_name, '/');

	if (p)
		*p = '@';

	name_buffer.value = gss_name;
	name_buffer.length = strlen(gss_name) + 1;

	major_status = sspi_gss_import_name(&minor_status, &name_buffer,
			SSPI_GSS_C_NT_HOSTBASED_SERVICE, &(context->target_name));

	free(gss_name);

	if (SSPI_GSS_ERROR(major_status))
	{
		WLog_ERR(TAG, "error: gss_import_name failed");
		return -1;
	}

	return 1;
}

SECURITY_STATUS SEC_ENTRY kerberos_InitializeSecurityContextA(PCredHandle phCredential, PCtxtHandle phContext,
				    SEC_CHAR* pszTargetName, ULONG fContextReq, ULONG Reserved1,
				    ULONG TargetDataRep, PSecBufferDesc pInput, ULONG Reserved2,
				    PCtxtHandle phNewContext, PSecBufferDesc pOutput,
				    ULONG* pfContextAttr, PTimeStamp ptsExpiry)
{
	KRB_CONTEXT* context;
	SSPI_CREDENTIALS* credentials;
	PSecBuffer input_buffer = NULL;
	PSecBuffer output_buffer = NULL;
	sspi_gss_buffer_desc input_tok;
	sspi_gss_buffer_desc output_tok;
	sspi_gss_OID actual_mech;
	sspi_gss_OID desired_mech;
	UINT32 actual_services;

	input_tok.length = 0;
	output_tok.length = 0;
	desired_mech = SSPI_GSS_C_SPNEGO_KRB5;

	context = (KRB_CONTEXT*) sspi_SecureHandleGetLowerPointer(phContext);

	if (!context)
	{
		context = kerberos_ContextNew();

		if (!context)
			return SEC_E_INSUFFICIENT_MEMORY;

		credentials = (SSPI_CREDENTIALS*) sspi_SecureHandleGetLowerPointer(phCredential);
		context->credentials = credentials;

		if (kerberos_SetContextServicePrincipalNameA(context, pszTargetName) < 0)
			return SEC_E_INTERNAL_ERROR;

		sspi_SecureHandleSetLowerPointer(phNewContext, context);
		sspi_SecureHandleSetUpperPointer(phNewContext, (void*) KRB_PACKAGE_NAME);
	}

	if (!pInput)
	{
		context->major_status = sspi_gss_init_sec_context(&(context->minor_status),
				context->cred, &(context->gss_ctx), context->target_name,
				desired_mech, SSPI_GSS_C_MUTUAL_FLAG | SSPI_GSS_C_DELEG_FLAG,
				SSPI_GSS_C_INDEFINITE, SSPI_GSS_C_NO_CHANNEL_BINDINGS,
				&input_tok, &actual_mech, &output_tok, &actual_services, &(context->actual_time));

		if (SSPI_GSS_ERROR(context->major_status))
		{
			WLog_ERR(TAG, "Kerberos: Initialize failed, do you have correct kerberos tgt initialized?");
			WLog_ERR(TAG, "Kerberos: gss_init_sec_context failed with %d", SSPI_GSS_C_GSS_CODE);
			return SEC_E_INTERNAL_ERROR;
		}

		if (context->major_status & SSPI_GSS_S_CONTINUE_NEEDED)
		{
			if (output_tok.length != 0)
			{
				if (!pOutput)
					return SEC_E_INVALID_TOKEN;

				if (pOutput->cBuffers < 1)
					return SEC_E_INVALID_TOKEN;

				output_buffer = sspi_FindSecBuffer(pOutput, SECBUFFER_TOKEN);

				if (!output_buffer)
					return SEC_E_INVALID_TOKEN;

				if (output_buffer->cbBuffer < 1)
					return SEC_E_INVALID_TOKEN;

				CopyMemory(output_buffer->pvBuffer, output_tok.value, output_tok.length);
				output_buffer->cbBuffer = output_tok.length;

				sspi_gss_release_buffer(&(context->minor_status), &output_tok);
				return SEC_I_CONTINUE_NEEDED;
			}
		}
	}
	else
	{
		input_buffer = sspi_FindSecBuffer(pInput, SECBUFFER_TOKEN);

		if (!input_buffer)
			return SEC_E_INVALID_TOKEN;

		if (input_buffer->cbBuffer < 1)
			return SEC_E_INVALID_TOKEN;

		input_tok.value = input_buffer->pvBuffer;
		input_tok.length = input_buffer->cbBuffer;

		context->major_status = sspi_gss_init_sec_context(&(context->minor_status),
				context->cred, &(context->gss_ctx), context->target_name,
				desired_mech, SSPI_GSS_C_MUTUAL_FLAG | SSPI_GSS_C_DELEG_FLAG,
				SSPI_GSS_C_INDEFINITE, SSPI_GSS_C_NO_CHANNEL_BINDINGS,
				&input_tok, &actual_mech, &output_tok, &actual_services, &(context->actual_time));

		if (SSPI_GSS_ERROR(context->major_status))
		{
			WLog_ERR(TAG, "Kerberos: Initialize failed, do you have correct kerberos tgt initialized?");
			WLog_ERR(TAG, "Kerberos: gss_init_sec_context failed with %lu\n", SSPI_GSS_C_GSS_CODE);
			return SEC_E_INTERNAL_ERROR;
		}

		if (output_tok.length == 0)
		{
			/* Free output_buffer to detect second call in NLA */
			output_buffer = sspi_FindSecBuffer(pOutput, SECBUFFER_TOKEN);
			sspi_SecBufferFree(output_buffer);
			return SEC_E_OK;
		}
		else
		{
			return SEC_E_INTERNAL_ERROR;
		}
	}

	return SEC_E_INTERNAL_ERROR;
}

SECURITY_STATUS SEC_ENTRY kerberos_QueryContextAttributesW(PCtxtHandle phContext, ULONG ulAttribute, void* pBuffer)
{
	return SEC_E_OK;
}

SECURITY_STATUS SEC_ENTRY kerberos_QueryContextAttributesA(PCtxtHandle phContext, ULONG ulAttribute, void* pBuffer)
{
	if (!phContext)
		return SEC_E_INVALID_HANDLE;

	if (!pBuffer)
		return SEC_E_INSUFFICIENT_MEMORY;

	if (ulAttribute == SECPKG_ATTR_SIZES)
	{
		SecPkgContext_Sizes* ContextSizes = (SecPkgContext_Sizes*) pBuffer;

		/* FIXME: don't use hardcoded values */
		ContextSizes->cbMaxToken = 2010;
		ContextSizes->cbMaxSignature = 0;
		ContextSizes->cbBlockSize = 60;
		ContextSizes->cbSecurityTrailer = 0;

		return SEC_E_OK;
	}

	return SEC_E_UNSUPPORTED_FUNCTION;
}

SECURITY_STATUS SEC_ENTRY kerberos_EncryptMessage(PCtxtHandle phContext, ULONG fQOP,
		PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
	int index;
	int conf_state;
	UINT32 major_status;
	UINT32 minor_status;
	KRB_CONTEXT* context;
	sspi_gss_buffer_desc input;
	sspi_gss_buffer_desc output;
	PSecBuffer data_buffer = NULL;
	PSecBuffer signature_buffer = NULL;

	context = (KRB_CONTEXT*) sspi_SecureHandleGetLowerPointer(phContext);

	if (!context)
		return SEC_E_INVALID_HANDLE;

	for (index = 0; index < (int) pMessage->cBuffers; index++)
	{
		if (pMessage->pBuffers[index].BufferType == SECBUFFER_DATA)
			data_buffer = &pMessage->pBuffers[index];
		else if (pMessage->pBuffers[index].BufferType == SECBUFFER_TOKEN)
			signature_buffer = &pMessage->pBuffers[index];
	}

	if (!data_buffer)
		return SEC_E_INVALID_TOKEN;

	if (!signature_buffer)
		return SEC_E_INVALID_TOKEN;

	input.value = data_buffer->pvBuffer;
	input.length = data_buffer->cbBuffer;

	major_status = sspi_gss_wrap(&minor_status, context->gss_ctx, TRUE,
			SSPI_GSS_C_QOP_DEFAULT, &input, &conf_state, &output);

	if (SSPI_GSS_ERROR(major_status))
	{
		WLog_ERR(TAG, "error: gss_wrap failed");
		return SEC_E_INTERNAL_ERROR;
	}

	if (conf_state == 0)
	{
		WLog_ERR(TAG, "error: gss_wrap confidentiality was not applied");
		sspi_gss_release_buffer(&minor_status, &output);
		return SEC_E_INTERNAL_ERROR;
	}

	CopyMemory(signature_buffer->pvBuffer, output.value, output.length);
	sspi_gss_release_buffer(&minor_status, &output);

	return SEC_E_OK;
}

SECURITY_STATUS SEC_ENTRY kerberos_DecryptMessage(PCtxtHandle phContext,
		PSecBufferDesc pMessage, ULONG MessageSeqNo, ULONG* pfQOP)
{
	int index;
	int conf_state;
	UINT32 major_status;
	UINT32 minor_status;
	KRB_CONTEXT* context;
	sspi_gss_buffer_desc input;
	sspi_gss_buffer_desc output;
	PSecBuffer data_buffer = NULL;
	PSecBuffer signature_buffer = NULL;

	context = (KRB_CONTEXT*) sspi_SecureHandleGetLowerPointer(phContext);

	if (!context)
		return SEC_E_INVALID_HANDLE;

	for (index = 0; index < (int) pMessage->cBuffers; index++)
	{
		if (pMessage->pBuffers[index].BufferType == SECBUFFER_DATA)
			data_buffer = &pMessage->pBuffers[index];
		else if (pMessage->pBuffers[index].BufferType == SECBUFFER_TOKEN)
			signature_buffer = &pMessage->pBuffers[index];
	}

	if (!data_buffer)
		return SEC_E_INVALID_TOKEN;

	if (!signature_buffer)
		return SEC_E_INVALID_TOKEN;

	input.value = data_buffer->pvBuffer;
	input.length = data_buffer->cbBuffer;

	major_status = sspi_gss_unwrap(&minor_status, context->gss_ctx, &input, &output, &conf_state, NULL);

	if (SSPI_GSS_ERROR(major_status))
	{
		WLog_ERR(TAG, "error: gss_unwrap failed");
		return SEC_E_INTERNAL_ERROR;
	}

	if (conf_state == 0)
	{
		WLog_ERR(TAG, "error: gss_unwrap confidentiality was not applied");
		sspi_gss_release_buffer(&minor_status, &output);
		return SEC_E_INTERNAL_ERROR;
	}

	CopyMemory(signature_buffer->pvBuffer, output.value, output.length);
	sspi_gss_release_buffer(&minor_status, &output);

	return SEC_E_OK;
}

SECURITY_STATUS SEC_ENTRY kerberos_MakeSignature(PCtxtHandle phContext,
		ULONG fQOP, PSecBufferDesc pMessage, ULONG MessageSeqNo)
{
	return SEC_E_OK;
}

SECURITY_STATUS SEC_ENTRY kerberos_VerifySignature(PCtxtHandle phContext,
		PSecBufferDesc pMessage, ULONG MessageSeqNo, ULONG* pfQOP)
{
	return SEC_E_OK;
}

const SecPkgInfoA KERBEROS_SecPkgInfoA =
{
	0x000F3BBF,		/* fCapabilities */
	1,			/* wVersion */
	0x0010,			/* wRPCID */
	0x00002EE0,		/* cbMaxToken */
	"Kerberos",		/* Name */
	"Microsoft Kerberos V1.0"	/* Comment */
};

WCHAR KERBEROS_SecPkgInfoW_Name[] = { 'K', 'e', 'r', 'b', 'e', 'r', 'o', 's', '\0' };

WCHAR KERBEROS_SecPkgInfoW_Comment[] =
{
	'K', 'e', 'r', 'b', 'e', 'r', 'o', 's', ' ',
	'S', 'e', 'c', 'u', 'r', 'i', 't', 'y', ' ',
	'P', 'a', 'c', 'k', 'a', 'g', 'e', '\0'
};

const SecPkgInfoW KERBEROS_SecPkgInfoW =
{
	0x000F3BBF,		/* fCapabilities */
	1,			/* wVersion */
	0x0010,			/* wRPCID */
	0x00002EE0,		/* cbMaxToken */
	KERBEROS_SecPkgInfoW_Name,	/* Name */
	KERBEROS_SecPkgInfoW_Comment	/* Comment */
};

const SecurityFunctionTableA KERBEROS_SecurityFunctionTableA =
{
	1,			/* dwVersion */
	NULL,			/* EnumerateSecurityPackages */
	kerberos_QueryCredentialsAttributesA,	/* QueryCredentialsAttributes */
	kerberos_AcquireCredentialsHandleA,	/* AcquireCredentialsHandle */
	kerberos_FreeCredentialsHandle,	/* FreeCredentialsHandle */
	NULL,			/* Reserved2 */
	kerberos_InitializeSecurityContextA,	/* InitializeSecurityContext */
	NULL,			/* AcceptSecurityContext */
	NULL,			/* CompleteAuthToken */
	NULL,			/* DeleteSecurityContext */
	NULL,			/* ApplyControlToken */
	kerberos_QueryContextAttributesA,	/* QueryContextAttributes */
	NULL,			/* ImpersonateSecurityContext */
	NULL,			/* RevertSecurityContext */
	kerberos_MakeSignature,	/* MakeSignature */
	kerberos_VerifySignature,	/* VerifySignature */
	NULL,			/* FreeContextBuffer */
	NULL,			/* QuerySecurityPackageInfo */
	NULL,			/* Reserved3 */
	NULL,			/* Reserved4 */
	NULL,			/* ExportSecurityContext */
	NULL,			/* ImportSecurityContext */
	NULL,			/* AddCredentials */
	NULL,			/* Reserved8 */
	NULL,			/* QuerySecurityContextToken */
	kerberos_EncryptMessage,	/* EncryptMessage */
	kerberos_DecryptMessage,	/* DecryptMessage */
	NULL,			/* SetContextAttributes */
};

const SecurityFunctionTableW KERBEROS_SecurityFunctionTableW =
{
	1,			/* dwVersion */
	NULL,			/* EnumerateSecurityPackages */
	kerberos_QueryCredentialsAttributesW,	/* QueryCredentialsAttributes */
	kerberos_AcquireCredentialsHandleW,	/* AcquireCredentialsHandle */
	kerberos_FreeCredentialsHandle,	/* FreeCredentialsHandle */
	NULL,			/* Reserved2 */
	kerberos_InitializeSecurityContextW,	/* InitializeSecurityContext */
	NULL,			/* AcceptSecurityContext */
	NULL,			/* CompleteAuthToken */
	NULL,			/* DeleteSecurityContext */
	NULL,			/* ApplyControlToken */
	kerberos_QueryContextAttributesW,	/* QueryContextAttributes */
	NULL,			/* ImpersonateSecurityContext */
	NULL,			/* RevertSecurityContext */
	kerberos_MakeSignature,	/* MakeSignature */
	kerberos_VerifySignature,	/* VerifySignature */
	NULL,			/* FreeContextBuffer */
	NULL,			/* QuerySecurityPackageInfo */
	NULL,			/* Reserved3 */
	NULL,			/* Reserved4 */
	NULL,			/* ExportSecurityContext */
	NULL,			/* ImportSecurityContext */
	NULL,			/* AddCredentials */
	NULL,			/* Reserved8 */
	NULL,			/* QuerySecurityContextToken */
	kerberos_EncryptMessage,	/* EncryptMessage */
	kerberos_DecryptMessage,	/* DecryptMessage */
	NULL,			/* SetContextAttributes */
};
