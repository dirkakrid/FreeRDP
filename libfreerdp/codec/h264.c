/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * H.264 Bitmap Compression
 *
 * Copyright 2014 Mike McDonald <Mike.McDonald@software.dell.com>
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

#include <winpr/crt.h>
#include <winpr/print.h>
#include <winpr/intrin.h>
#include <winpr/sysinfo.h>
#include <winpr/stream.h>
#include <winpr/bitstream.h>

#include <freerdp/primitives.h>
#include <freerdp/codec/h264.h>
#include <freerdp/log.h>

#define TAG FREERDP_TAG("codec")

/**
 * Dummy subsystem
 */

static int dummy_decompress(H264_CONTEXT* h264, BYTE* pSrcData, UINT32 SrcSize)
{
	return -1;
}

static void dummy_uninit(H264_CONTEXT* h264)
{

}

static BOOL dummy_init(H264_CONTEXT* h264)
{
	return TRUE;
}

static H264_CONTEXT_SUBSYSTEM g_Subsystem_dummy =
{
	"dummy",
	dummy_init,
	dummy_uninit,
	dummy_decompress
};

/**
 * Media Foundation subsystem
 */

#if defined(_WIN32) && defined(WITH_MEDIA_FOUNDATION)

#include <ks.h>
#include <codecapi.h>

#include <mfapi.h>
#include <mferror.h>
#include <wmcodecdsp.h>
#include <mftransform.h>

#undef DEFINE_GUID
#define INITGUID
#include <initguid.h>

DEFINE_GUID(CLSID_CMSH264DecoderMFT,0x62CE7E72,0x4C71,0x4d20,0xB1,0x5D,0x45,0x28,0x31,0xA8,0x7D,0x9D);
DEFINE_GUID(CLSID_VideoProcessorMFT,0x88753b26,0x5b24,0x49bd,0xb2,0xe7,0x0c,0x44,0x5c,0x78,0xc9,0x82);
DEFINE_GUID(IID_IMFTransform,0xbf94c121,0x5b05,0x4e6f,0x80,0x00,0xba,0x59,0x89,0x61,0x41,0x4d);
DEFINE_GUID(MF_MT_MAJOR_TYPE,0x48eba18e,0xf8c9,0x4687,0xbf,0x11,0x0a,0x74,0xc9,0xf9,0x6a,0x8f);
DEFINE_GUID(MF_MT_FRAME_SIZE,0x1652c33d,0xd6b2,0x4012,0xb8,0x34,0x72,0x03,0x08,0x49,0xa3,0x7d);
DEFINE_GUID(MF_MT_DEFAULT_STRIDE,0x644b4e48,0x1e02,0x4516,0xb0,0xeb,0xc0,0x1c,0xa9,0xd4,0x9a,0xc6);
DEFINE_GUID(MF_MT_SUBTYPE,0xf7e34c9a,0x42e8,0x4714,0xb7,0x4b,0xcb,0x29,0xd7,0x2c,0x35,0xe5);
DEFINE_GUID(MF_XVP_DISABLE_FRC,0x2c0afa19,0x7a97,0x4d5a,0x9e,0xe8,0x16,0xd4,0xfc,0x51,0x8d,0x8c);
DEFINE_GUID(MFMediaType_Video,0x73646976,0x0000,0x0010,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71);
DEFINE_GUID(MFVideoFormat_RGB32,22,0x0000,0x0010,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71);
DEFINE_GUID(MFVideoFormat_ARGB32,21,0x0000,0x0010,0x80,0x00,0x00,0xAA,0x00,0x38,0x9B,0x71);
DEFINE_GUID(MFVideoFormat_H264,0x34363248,0x0000,0x0010,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71);
DEFINE_GUID(MFVideoFormat_IYUV,0x56555949,0x0000,0x0010,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71);
DEFINE_GUID(IID_ICodecAPI,0x901db4c7,0x31ce,0x41a2,0x85,0xdc,0x8f,0xa0,0xbf,0x41,0xb8,0xda);
DEFINE_GUID(CODECAPI_AVLowLatencyMode,0x9c27891a,0xed7a,0x40e1,0x88,0xe8,0xb2,0x27,0x27,0xa0,0x24,0xee);
DEFINE_GUID(CODECAPI_AVDecVideoMaxCodedWidth,0x5ae557b8,0x77af,0x41f5,0x9f,0xa6,0x4d,0xb2,0xfe,0x1d,0x4b,0xca);
DEFINE_GUID(CODECAPI_AVDecVideoMaxCodedHeight,0x7262a16a,0xd2dc,0x4e75,0x9b,0xa8,0x65,0xc0,0xc6,0xd3,0x2b,0x13);

#ifndef __IMFDXGIDeviceManager_FWD_DEFINED__
#define __IMFDXGIDeviceManager_FWD_DEFINED__
typedef interface IMFDXGIDeviceManager IMFDXGIDeviceManager;
#endif /* __IMFDXGIDeviceManager_FWD_DEFINED__ */

#ifndef __IMFDXGIDeviceManager_INTERFACE_DEFINED__
#define __IMFDXGIDeviceManager_INTERFACE_DEFINED__

typedef struct IMFDXGIDeviceManagerVtbl
{        
	HRESULT (STDMETHODCALLTYPE * QueryInterface)(IMFDXGIDeviceManager* This, REFIID riid, void** ppvObject);
	ULONG   (STDMETHODCALLTYPE * AddRef)(IMFDXGIDeviceManager* This);
        ULONG   (STDMETHODCALLTYPE * Release)(IMFDXGIDeviceManager* This);
        HRESULT (STDMETHODCALLTYPE * CloseDeviceHandle)(IMFDXGIDeviceManager* This, HANDLE hDevice);
        HRESULT (STDMETHODCALLTYPE * GetVideoService)(IMFDXGIDeviceManager* This, HANDLE hDevice, REFIID riid, void** ppService);
        HRESULT (STDMETHODCALLTYPE * LockDevice)(IMFDXGIDeviceManager* This, HANDLE hDevice, REFIID riid, void** ppUnkDevice, BOOL fBlock);
        HRESULT (STDMETHODCALLTYPE * OpenDeviceHandle)(IMFDXGIDeviceManager* This, HANDLE* phDevice);
        HRESULT (STDMETHODCALLTYPE * ResetDevice)(IMFDXGIDeviceManager* This, IUnknown* pUnkDevice, UINT resetToken);
        HRESULT (STDMETHODCALLTYPE * TestDevice)(IMFDXGIDeviceManager* This, HANDLE hDevice);
        HRESULT (STDMETHODCALLTYPE * UnlockDevice)(IMFDXGIDeviceManager* This, HANDLE hDevice, BOOL fSaveState);
}
IMFDXGIDeviceManagerVtbl;

interface IMFDXGIDeviceManager
{
	CONST_VTBL struct IMFDXGIDeviceManagerVtbl* lpVtbl;
};

#endif /* __IMFDXGIDeviceManager_INTERFACE_DEFINED__ */

typedef HRESULT (__stdcall * pfnMFStartup)(ULONG Version, DWORD dwFlags);
typedef HRESULT (__stdcall * pfnMFShutdown)(void);
typedef HRESULT (__stdcall * pfnMFCreateSample)(IMFSample** ppIMFSample);
typedef HRESULT (__stdcall * pfnMFCreateMemoryBuffer)(DWORD cbMaxLength, IMFMediaBuffer** ppBuffer);
typedef HRESULT (__stdcall * pfnMFCreateMediaType)(IMFMediaType** ppMFType);
typedef HRESULT (__stdcall * pfnMFCreateDXGIDeviceManager)(UINT* pResetToken, IMFDXGIDeviceManager** ppDXVAManager);

struct _H264_CONTEXT_MF
{
	ICodecAPI* codecApi;
	IMFTransform* transform;
	IMFMediaType* inputType;
	IMFMediaType* outputType;
	IMFSample* sample;
	UINT32 frameWidth;
	UINT32 frameHeight;
	IMFSample* outputSample;
	IMFMediaBuffer* outputBuffer;
	HMODULE mfplat;
	pfnMFStartup MFStartup;
	pfnMFShutdown MFShutdown;
	pfnMFCreateSample MFCreateSample;
	pfnMFCreateMemoryBuffer MFCreateMemoryBuffer;
	pfnMFCreateMediaType MFCreateMediaType;
	pfnMFCreateDXGIDeviceManager MFCreateDXGIDeviceManager;
};
typedef struct _H264_CONTEXT_MF H264_CONTEXT_MF;

static HRESULT mf_find_output_type(H264_CONTEXT_MF* sys, const GUID* guid, IMFMediaType** ppMediaType)
{
	DWORD idx = 0;
	GUID mediaGuid;
	HRESULT hr = S_OK;
	IMFMediaType* pMediaType = NULL;

	while (1)
	{
		hr = sys->transform->lpVtbl->GetOutputAvailableType(sys->transform, 0, idx, &pMediaType);

		if (FAILED(hr))
			break;

		pMediaType->lpVtbl->GetGUID(pMediaType, &MF_MT_SUBTYPE, &mediaGuid);

		if (IsEqualGUID(&mediaGuid, guid))
		{
			*ppMediaType = pMediaType;
			return S_OK;
		}

		pMediaType->lpVtbl->Release(pMediaType);

		idx++;
	}

	return hr;
}

static HRESULT mf_create_output_sample(H264_CONTEXT_MF* sys)
{
	HRESULT hr = S_OK;
	MFT_OUTPUT_STREAM_INFO streamInfo;

	if (sys->outputSample)
	{
		sys->outputSample->lpVtbl->Release(sys->outputSample);
		sys->outputSample = NULL;
	}

	hr = sys->MFCreateSample(&sys->outputSample);

	if (FAILED(hr))
	{
		WLog_ERR(TAG, "MFCreateSample failure: 0x%04X", hr);
		goto error;
	}

	hr = sys->transform->lpVtbl->GetOutputStreamInfo(sys->transform, 0, &streamInfo);

	if (FAILED(hr))
	{
		WLog_ERR(TAG, "GetOutputStreamInfo failure: 0x%04X", hr);
		goto error;
	}

	hr = sys->MFCreateMemoryBuffer(streamInfo.cbSize, &sys->outputBuffer);

	if (FAILED(hr))
	{
		WLog_ERR(TAG, "MFCreateMemoryBuffer failure: 0x%04X", hr);
		goto error;
	}

	sys->outputSample->lpVtbl->AddBuffer(sys->outputSample, sys->outputBuffer);

	if (FAILED(hr))
	{
		WLog_ERR(TAG, "AddBuffer failure: 0x%04X", hr);
		goto error;
	}

	sys->outputBuffer->lpVtbl->Release(sys->outputBuffer);

error:
	return hr;
}

static int mf_decompress(H264_CONTEXT* h264, BYTE* pSrcData, UINT32 SrcSize)
{
	HRESULT hr;
	BYTE* pbBuffer = NULL;
	DWORD cbMaxLength = 0;
	DWORD cbCurrentLength = 0;
	DWORD outputStatus = 0;
	IMFSample* inputSample = NULL;
	IMFMediaBuffer* inputBuffer = NULL;
	IMFMediaBuffer* outputBuffer = NULL;
	MFT_OUTPUT_DATA_BUFFER outputDataBuffer;
	H264_CONTEXT_MF* sys = (H264_CONTEXT_MF*) h264->pSystemData;

	hr = sys->MFCreateMemoryBuffer(SrcSize, &inputBuffer);

	if (FAILED(hr))
	{
		WLog_ERR(TAG, "MFCreateMemoryBuffer failure: 0x%04X", hr);
		goto error;
	}

	hr = inputBuffer->lpVtbl->Lock(inputBuffer, &pbBuffer, &cbMaxLength, &cbCurrentLength);

	if (FAILED(hr))
	{
		WLog_ERR(TAG, "Lock failure: 0x%04X", hr);
		goto error;
	}

	CopyMemory(pbBuffer, pSrcData, SrcSize);

	hr = inputBuffer->lpVtbl->SetCurrentLength(inputBuffer, SrcSize);

	if (FAILED(hr))
	{
		WLog_ERR(TAG, "SetCurrentLength failure: 0x%04X", hr);
		goto error;
	}

	hr = inputBuffer->lpVtbl->Unlock(inputBuffer);

	if (FAILED(hr))
	{
		WLog_ERR(TAG, "Unlock failure: 0x%04X", hr);
		goto error;
	}

	hr = sys->MFCreateSample(&inputSample);

	if (FAILED(hr))
	{
		WLog_ERR(TAG, "MFCreateSample failure: 0x%04X", hr);
		goto error;
	}

	inputSample->lpVtbl->AddBuffer(inputSample, inputBuffer);

	if (FAILED(hr))
	{
		WLog_ERR(TAG, "AddBuffer failure: 0x%04X", hr);
		goto error;
	}

	inputBuffer->lpVtbl->Release(inputBuffer);

	hr = sys->transform->lpVtbl->ProcessInput(sys->transform, 0, inputSample, 0);

	if (FAILED(hr))
	{
		WLog_ERR(TAG, "ProcessInput failure: 0x%04X", hr);
		goto error;
	}

	hr = mf_create_output_sample(sys);

	if (FAILED(hr))
	{
		WLog_ERR(TAG, "mf_create_output_sample failure: 0x%04X", hr);
		goto error;
	}

	outputDataBuffer.dwStreamID = 0;
	outputDataBuffer.dwStatus = 0;
	outputDataBuffer.pEvents = NULL;
	outputDataBuffer.pSample = sys->outputSample;

	hr = sys->transform->lpVtbl->ProcessOutput(sys->transform, 0, 1, &outputDataBuffer, &outputStatus);

	if (hr == MF_E_TRANSFORM_STREAM_CHANGE)
	{
		BYTE* pYUVData;
		int offset = 0;
		UINT32 stride = 0;
		UINT64 frameSize = 0;
		IMFAttributes* attributes = NULL;

		if (sys->outputType)
		{
			sys->outputType->lpVtbl->Release(sys->outputType);
			sys->outputType = NULL;
		}

		hr = mf_find_output_type(sys, &MFVideoFormat_IYUV, &sys->outputType);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "mf_find_output_type failure: 0x%04X", hr);
			goto error;
		}

		hr = sys->transform->lpVtbl->SetOutputType(sys->transform, 0, sys->outputType, 0);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "SetOutputType failure: 0x%04X", hr);
			goto error;
		}

		hr = mf_create_output_sample(sys);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "mf_create_output_sample failure: 0x%04X", hr);
			goto error;
		}

		hr = sys->outputType->lpVtbl->GetUINT64(sys->outputType, &MF_MT_FRAME_SIZE, &frameSize);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "GetUINT64(MF_MT_FRAME_SIZE) failure: 0x%04X", hr);
			goto error;
		}

		sys->frameWidth = (UINT32) (frameSize >> 32);
		sys->frameHeight = (UINT32) frameSize;

		hr = sys->outputType->lpVtbl->GetUINT32(sys->outputType, &MF_MT_DEFAULT_STRIDE, &stride);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "GetUINT32(MF_MT_DEFAULT_STRIDE) failure: 0x%04X", hr);
			goto error;
		}

		h264->iStride[0] = stride;
		h264->iStride[1] = stride / 2;
		h264->iStride[2] = stride / 2;

		pYUVData = (BYTE*) calloc(1, 2 * stride * sys->frameHeight);

		h264->pYUVData[0] = &pYUVData[offset];
		pYUVData += h264->iStride[0] * sys->frameHeight;

		h264->pYUVData[1] = &pYUVData[offset];
		pYUVData += h264->iStride[1] * (sys->frameHeight / 2);

		h264->pYUVData[2] = &pYUVData[offset];
		pYUVData += h264->iStride[2] * (sys->frameHeight / 2);

		h264->width = sys->frameWidth;
		h264->height = sys->frameHeight;
	}
	else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT)
	{
		
	}
	else if (FAILED(hr))
	{
		WLog_ERR(TAG, "ProcessOutput failure: 0x%04X", hr);
		goto error;
	}
	else
	{
		int offset = 0;
		BYTE* buffer = NULL;
		DWORD bufferCount = 0;
		DWORD cbMaxLength = 0;
		DWORD cbCurrentLength = 0;

		hr = sys->outputSample->lpVtbl->GetBufferCount(sys->outputSample, &bufferCount);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "GetBufferCount failure: 0x%04X", hr);
			goto error;
		}

		hr = sys->outputSample->lpVtbl->GetBufferByIndex(sys->outputSample, 0, &outputBuffer);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "GetBufferByIndex failure: 0x%04X", hr);
			goto error;
		}

		hr = outputBuffer->lpVtbl->Lock(outputBuffer, &buffer, &cbMaxLength, &cbCurrentLength);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "Lock failure: 0x%04X", hr);
			goto error;
		}

		CopyMemory(h264->pYUVData[0], &buffer[offset], h264->iStride[0] * sys->frameHeight);
		offset += h264->iStride[0] * sys->frameHeight;

		CopyMemory(h264->pYUVData[1], &buffer[offset], h264->iStride[1] * (sys->frameHeight / 2));
		offset += h264->iStride[1] * (sys->frameHeight / 2);

		CopyMemory(h264->pYUVData[2], &buffer[offset], h264->iStride[2] * (sys->frameHeight / 2));
		offset += h264->iStride[2] * (sys->frameHeight / 2);

		hr = outputBuffer->lpVtbl->Unlock(outputBuffer);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "Unlock failure: 0x%04X", hr);
			goto error;
		}

		outputBuffer->lpVtbl->Release(outputBuffer);
	}

	inputSample->lpVtbl->Release(inputSample);

	return 1;

error:
	fprintf(stderr, "mf_decompress error\n");
	return -1;
}

static int mf_compress(H264_CONTEXT* h264, BYTE** ppDstData, UINT32* pDstSize)
{
	H264_CONTEXT_MF* sys = (H264_CONTEXT_MF*) h264->pSystemData;

	return 1;
}

static void mf_uninit(H264_CONTEXT* h264)
{
	H264_CONTEXT_MF* sys = (H264_CONTEXT_MF*) h264->pSystemData;

	if (sys)
	{
		if (sys->transform)
		{
			sys->transform->lpVtbl->Release(sys->transform);
			sys->transform = NULL;
		}

		if (sys->codecApi)
		{
			sys->codecApi->lpVtbl->Release(sys->codecApi);
			sys->codecApi = NULL;
		}

		if (sys->inputType)
		{
			sys->inputType->lpVtbl->Release(sys->inputType);
			sys->inputType = NULL;
		}

		if (sys->outputType)
		{
			sys->outputType->lpVtbl->Release(sys->outputType);
			sys->outputType = NULL;
		}

		if (sys->outputSample)
		{
			sys->outputSample->lpVtbl->Release(sys->outputSample);
			sys->outputSample = NULL;
		}

		if (sys->mfplat)
		{
			FreeLibrary(sys->mfplat);
			sys->mfplat = NULL;
		}

		free(h264->pYUVData[0]);
		h264->pYUVData[0] = h264->pYUVData[1] = h264->pYUVData[2] = NULL;
		h264->iStride[0] = h264->iStride[1] = h264->iStride[2] = 0;

		sys->MFShutdown();
		
		CoUninitialize();

		free(sys);
		h264->pSystemData = NULL;
	}
}

static BOOL mf_init(H264_CONTEXT* h264)
{
	HRESULT hr;
	H264_CONTEXT_MF* sys;

	sys = (H264_CONTEXT_MF*) calloc(1, sizeof(H264_CONTEXT_MF));

	if (!sys)
		goto error;

	h264->pSystemData = (void*) sys;

	/* http://decklink-sdk-delphi.googlecode.com/svn/trunk/Blackmagic%20DeckLink%20SDK%209.7/Win/Samples/Streaming/StreamingPreview/DecoderMF.cpp */

	sys->mfplat = LoadLibraryA("mfplat.dll");

	if (!sys->mfplat)
		goto error;

	sys->MFStartup = (pfnMFStartup) GetProcAddress(sys->mfplat, "MFStartup");
	sys->MFShutdown = (pfnMFShutdown) GetProcAddress(sys->mfplat, "MFShutdown");
	sys->MFCreateSample = (pfnMFCreateSample) GetProcAddress(sys->mfplat, "MFCreateSample");
	sys->MFCreateMemoryBuffer = (pfnMFCreateMemoryBuffer) GetProcAddress(sys->mfplat, "MFCreateMemoryBuffer");
	sys->MFCreateMediaType = (pfnMFCreateMediaType) GetProcAddress(sys->mfplat, "MFCreateMediaType");
	sys->MFCreateDXGIDeviceManager = (pfnMFCreateDXGIDeviceManager) GetProcAddress(sys->mfplat, "MFCreateDXGIDeviceManager");

	if (!sys->MFStartup || !sys->MFShutdown || !sys->MFCreateSample || !sys->MFCreateMemoryBuffer ||
		!sys->MFCreateMediaType || !sys->MFCreateDXGIDeviceManager)
		goto error;

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	if (h264->Compressor)
	{

	}
	else
	{
		VARIANT var = { 0 };

		hr = sys->MFStartup(MF_VERSION, 0);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "MFStartup failure: 0x%04X", hr);
			goto error;
		}

		hr = CoCreateInstance(&CLSID_CMSH264DecoderMFT, NULL, CLSCTX_INPROC_SERVER, &IID_IMFTransform, (void**) &sys->transform);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "CoCreateInstance(CLSID_CMSH264DecoderMFT) failure: 0x%04X", hr);
			goto error;
		}

		hr = sys->transform->lpVtbl->QueryInterface(sys->transform, &IID_ICodecAPI, (void**) &sys->codecApi);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "QueryInterface(IID_ICodecAPI) failure: 0x%04X", hr);
			goto error;
		}

		var.vt = VT_UI4;
		var.ulVal = 1;
		
		hr = sys->codecApi->lpVtbl->SetValue(sys->codecApi, &CODECAPI_AVLowLatencyMode, &var);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "SetValue(CODECAPI_AVLowLatencyMode) failure: 0x%04X", hr);
			goto error;
		}

		hr = sys->MFCreateMediaType(&sys->inputType);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "MFCreateMediaType failure: 0x%04X", hr);
			goto error;
		}

		hr = sys->inputType->lpVtbl->SetGUID(sys->inputType, &MF_MT_MAJOR_TYPE, &MFMediaType_Video);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "SetGUID(MF_MT_MAJOR_TYPE) failure: 0x%04X", hr);
			goto error;
		}

		hr = sys->inputType->lpVtbl->SetGUID(sys->inputType, &MF_MT_SUBTYPE, &MFVideoFormat_H264);
		
		if (FAILED(hr))
		{
			WLog_ERR(TAG, "SetGUID(MF_MT_SUBTYPE) failure: 0x%04X", hr);
			goto error;
		}

		hr = sys->transform->lpVtbl->SetInputType(sys->transform, 0, sys->inputType, 0);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "SetInputType failure: 0x%04X", hr);
			goto error;
		}

		hr = mf_find_output_type(sys, &MFVideoFormat_IYUV, &sys->outputType);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "mf_find_output_type failure: 0x%04X", hr);
			goto error;
		}

		hr = sys->transform->lpVtbl->SetOutputType(sys->transform, 0, sys->outputType, 0);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "SetOutputType failure: 0x%04X", hr);
			goto error;
		}

		hr = mf_create_output_sample(sys);

		if (FAILED(hr))
		{
			WLog_ERR(TAG, "mf_create_output_sample failure: 0x%04X", hr);
			goto error;
		}
	}
	return TRUE;

error:
	WLog_ERR(TAG, "mf_init failure");
	mf_uninit(h264);
	return FALSE;
}

static H264_CONTEXT_SUBSYSTEM g_Subsystem_MF =
{
	"MediaFoundation",
	mf_init,
	mf_uninit,
	mf_decompress,
	mf_compress
};

#endif

/**
 * x264 subsystem
 */

#ifdef WITH_X264

#define NAL_UNKNOWN	X264_NAL_UNKNOWN
#define NAL_SLICE	X264_NAL_SLICE
#define NAL_SLICE_DPA	X264_NAL_SLICE_DPA
#define NAL_SLICE_DPB	X264_NAL_SLICE_DPB
#define NAL_SLICE_DPC	X264_NAL_SLICE_DPC
#define NAL_SLICE_IDR	X264_NAL_SLICE_IDR
#define NAL_SEI		X264_NAL_SEI
#define NAL_SPS		X264_NAL_SPS
#define NAL_PPS		X264_NAL_PPS
#define NAL_AUD		X264_NAL_AUD
#define NAL_FILLER	X264_NAL_FILLER

#define NAL_PRIORITY_DISPOSABLE	X264_NAL_PRIORITY_DISPOSABLE
#define NAL_PRIORITY_LOW	X264_NAL_PRIORITY_LOW
#define NAL_PRIORITY_HIGH	X264_NAL_PRIORITY_HIGH
#define NAL_PRIORITY_HIGHEST	X264_NAL_PRIORITY_HIGHEST

#include <stdint.h>
#include <x264.h>

struct _H264_CONTEXT_X264
{
	void* dummy;
};
typedef struct _H264_CONTEXT_X264 H264_CONTEXT_X264;

static int x264_decompress(H264_CONTEXT* h264, BYTE* pSrcData, UINT32 SrcSize)
{
	//H264_CONTEXT_X264* sys = (H264_CONTEXT_X264*) h264->pSystemData;

	return 1;
}

static int x264_compress(H264_CONTEXT* h264, BYTE** ppDstData, UINT32* pDstSize)
{
	//H264_CONTEXT_X264* sys = (H264_CONTEXT_X264*) h264->pSystemData;

	return 1;
}

static void x264_uninit(H264_CONTEXT* h264)
{
	H264_CONTEXT_X264* sys = (H264_CONTEXT_X264*) h264->pSystemData;

	if (sys)
	{
		free(sys);
		h264->pSystemData = NULL;
	}
}

static BOOL x264_init(H264_CONTEXT* h264)
{
	H264_CONTEXT_X264* sys;

	sys = (H264_CONTEXT_X264*) calloc(1, sizeof(H264_CONTEXT_X264));

	if (!sys)
	{
		goto EXCEPTION;
	}

	h264->pSystemData = (void*) sys;

	if (h264->Compressor)
	{

	}
	else
	{

	}

	return TRUE;

EXCEPTION:
	x264_uninit(h264);

	return FALSE;
}

static H264_CONTEXT_SUBSYSTEM g_Subsystem_x264 =
{
	"x264",
	x264_init,
	x264_uninit,
	x264_decompress,
	x264_compress
};

#undef NAL_UNKNOWN
#undef NAL_SLICE
#undef NAL_SLICE_DPA
#undef NAL_SLICE_DPB
#undef NAL_SLICE_DPC
#undef NAL_SLICE_IDR
#undef NAL_SEI
#undef NAL_SPS
#undef NAL_PPS
#undef NAL_AUD
#undef NAL_FILLER

#undef NAL_PRIORITY_DISPOSABLE
#undef NAL_PRIORITY_LOW
#undef NAL_PRIORITY_HIGH
#undef NAL_PRIORITY_HIGHEST

#endif

/**
 * OpenH264 subsystem
 */

#ifdef WITH_OPENH264

#include "wels/codec_def.h"
#include "wels/codec_api.h"

struct _H264_CONTEXT_OPENH264
{
	ISVCDecoder* pDecoder;
	ISVCEncoder* pEncoder;
	SEncParamExt EncParamExt;
};
typedef struct _H264_CONTEXT_OPENH264 H264_CONTEXT_OPENH264;

static BOOL g_openh264_trace_enabled = FALSE;

static void openh264_trace_callback(H264_CONTEXT* h264, int level, const char* message)
{
	WLog_INFO(TAG, "%d - %s", level, message);
}

static int openh264_decompress(H264_CONTEXT* h264, BYTE* pSrcData, UINT32 SrcSize)
{
	DECODING_STATE state;
	SBufferInfo sBufferInfo;
	SSysMEMBuffer* pSystemBuffer;
	H264_CONTEXT_OPENH264* sys = (H264_CONTEXT_OPENH264*) h264->pSystemData;

	if (!sys->pDecoder)
		return -2001;

	/*
	 * Decompress the image.  The RDP host only seems to send I420 format.
	 */

	h264->pYUVData[0] = NULL;
	h264->pYUVData[1] = NULL;
	h264->pYUVData[2] = NULL;

	ZeroMemory(&sBufferInfo, sizeof(sBufferInfo));

	state = (*sys->pDecoder)->DecodeFrame2(sys->pDecoder, pSrcData, SrcSize, h264->pYUVData, &sBufferInfo);

	if (sBufferInfo.iBufferStatus != 1)
	{
		if (state == dsNoParamSets)
		{
			/* this happens on the first frame due to missing parameter sets */
			state = (*sys->pDecoder)->DecodeFrame2(sys->pDecoder, NULL, 0, h264->pYUVData, &sBufferInfo);
		}
		else if (state == dsErrorFree)
		{
			/* call DecodeFrame2 again to decode without delay */
			state = (*sys->pDecoder)->DecodeFrame2(sys->pDecoder, NULL, 0, h264->pYUVData, &sBufferInfo);
		}
		else
		{
			WLog_WARN(TAG, "DecodeFrame2 state: 0x%02X iBufferStatus: %d", state, sBufferInfo.iBufferStatus);
			return -2002;
		}
	}

	if (sBufferInfo.iBufferStatus != 1)
	{
		WLog_WARN(TAG, "DecodeFrame2 iBufferStatus: %d", sBufferInfo.iBufferStatus);
		return 0;
	}

	if (state != dsErrorFree)
	{
		WLog_WARN(TAG, "DecodeFrame2 state: 0x%02X", state);
		return -2003;
	}

	pSystemBuffer = &sBufferInfo.UsrData.sSystemBuffer;

#if 0
	WLog_INFO(TAG, "h264_decompress: state=%u, pYUVData=[%p,%p,%p], bufferStatus=%d, width=%d, height=%d, format=%d, stride=[%d,%d]",
		state, h264->pYUVData[0], h264->pYUVData[1], h264->pYUVData[2], sBufferInfo.iBufferStatus,
		pSystemBuffer->iWidth, pSystemBuffer->iHeight, pSystemBuffer->iFormat,
		pSystemBuffer->iStride[0], pSystemBuffer->iStride[1]);
#endif

	if (pSystemBuffer->iFormat != videoFormatI420)
		return -2004;

	if (!h264->pYUVData[0] || !h264->pYUVData[1] || !h264->pYUVData[2])
		return -2005;

	h264->iStride[0] = pSystemBuffer->iStride[0];
	h264->iStride[1] = pSystemBuffer->iStride[1];
	h264->iStride[2] = pSystemBuffer->iStride[1];

	h264->width = pSystemBuffer->iWidth;
	h264->height = pSystemBuffer->iHeight;

	return 1;
}

static int openh264_compress(H264_CONTEXT* h264, BYTE** ppDstData, UINT32* pDstSize)
{
	int i, j;
	int status;
	SFrameBSInfo info;
	SSourcePicture pic;
	SBitrateInfo bitrate;
	H264_CONTEXT_OPENH264* sys = (H264_CONTEXT_OPENH264*) h264->pSystemData;

	if (!sys->pEncoder)
		return -1;

	if (!h264->pYUVData[0] || !h264->pYUVData[1] || !h264->pYUVData[2])
		return -1;

	if ((sys->EncParamExt.iPicWidth != h264->width) || (sys->EncParamExt.iPicHeight != h264->height))
	{
		status = (*sys->pEncoder)->GetDefaultParams(sys->pEncoder, &sys->EncParamExt);

		if (status < 0)
		{
			WLog_ERR(TAG, "Failed to get OpenH264 default parameters (status=%ld)", status);
			return status;
		}

		sys->EncParamExt.iUsageType = SCREEN_CONTENT_REAL_TIME;
		sys->EncParamExt.iPicWidth = h264->width;
		sys->EncParamExt.iPicHeight = h264->height;
		sys->EncParamExt.fMaxFrameRate = h264->FrameRate;
		sys->EncParamExt.iMaxBitrate = UNSPECIFIED_BIT_RATE;
		sys->EncParamExt.bEnableDenoise = 0;
		sys->EncParamExt.bEnableLongTermReference = 0;
		sys->EncParamExt.bEnableFrameSkip = 0;
		sys->EncParamExt.iSpatialLayerNum = 1;
		sys->EncParamExt.iMultipleThreadIdc = h264->NumberOfThreads;
		sys->EncParamExt.sSpatialLayers[0].fFrameRate = h264->FrameRate;
		sys->EncParamExt.sSpatialLayers[0].iVideoWidth = sys->EncParamExt.iPicWidth;
		sys->EncParamExt.sSpatialLayers[0].iVideoHeight = sys->EncParamExt.iPicHeight;
		sys->EncParamExt.sSpatialLayers[0].iMaxSpatialBitrate = sys->EncParamExt.iMaxBitrate;

		switch (h264->RateControlMode)
		{
			case H264_RATECONTROL_VBR:
				sys->EncParamExt.iRCMode = RC_BITRATE_MODE;
				sys->EncParamExt.iTargetBitrate = h264->BitRate;
				sys->EncParamExt.sSpatialLayers[0].iSpatialBitrate = sys->EncParamExt.iTargetBitrate;
				break;

			case H264_RATECONTROL_CQP:
				sys->EncParamExt.iRCMode = RC_OFF_MODE;
				sys->EncParamExt.sSpatialLayers[0].iDLayerQp = h264->QP;
				break;
		}

		if (sys->EncParamExt.iMultipleThreadIdc > 1)
		{
			sys->EncParamExt.sSpatialLayers[0].sSliceCfg.uiSliceMode = SM_AUTO_SLICE;
		}

		status = (*sys->pEncoder)->InitializeExt(sys->pEncoder, &sys->EncParamExt);

		if (status < 0)
		{
			WLog_ERR(TAG, "Failed to initialize OpenH264 encoder (status=%ld)", status);
			return status;
		}

		status = (*sys->pEncoder)->GetOption(sys->pEncoder, ENCODER_OPTION_SVC_ENCODE_PARAM_EXT,
			&sys->EncParamExt);

		if (status < 0)
		{
			WLog_ERR(TAG, "Failed to get initial OpenH264 encoder parameters (status=%ld)", status);
			return status;
		}
	}
	else
	{
		switch (h264->RateControlMode)
		{
			case H264_RATECONTROL_VBR:
				if (sys->EncParamExt.iTargetBitrate != h264->BitRate)
				{
					sys->EncParamExt.iTargetBitrate = h264->BitRate;
					bitrate.iLayer = SPATIAL_LAYER_ALL;
					bitrate.iBitrate = h264->BitRate;

					status = (*sys->pEncoder)->SetOption(sys->pEncoder, ENCODER_OPTION_BITRATE,
						&bitrate);

					if (status < 0)
					{
						WLog_ERR(TAG, "Failed to set encoder bitrate (status=%ld)", status);
						return status;
					}
				}
				if (sys->EncParamExt.fMaxFrameRate != h264->FrameRate)
				{
					sys->EncParamExt.fMaxFrameRate = h264->FrameRate;

					status = (*sys->pEncoder)->SetOption(sys->pEncoder, ENCODER_OPTION_FRAME_RATE,
						&sys->EncParamExt.fMaxFrameRate);

					if (status < 0)
					{
						WLog_ERR(TAG, "Failed to set encoder framerate (status=%ld)", status);
						return status;
					}
				}
				break;

			case H264_RATECONTROL_CQP:
				if (sys->EncParamExt.sSpatialLayers[0].iDLayerQp != h264->QP)
				{
					sys->EncParamExt.sSpatialLayers[0].iDLayerQp = h264->QP;

					status = (*sys->pEncoder)->SetOption(sys->pEncoder, ENCODER_OPTION_SVC_ENCODE_PARAM_EXT,
						&sys->EncParamExt);

					if (status < 0)
					{
						WLog_ERR(TAG, "Failed to set encoder parameters (status=%ld)", status);
						return status;
					}
				}
				break;
		}
	}

	memset(&info, 0, sizeof(SFrameBSInfo));
	memset(&pic, 0, sizeof(SSourcePicture));
	pic.iPicWidth = h264->width;
	pic.iPicHeight = h264->height;
	pic.iColorFormat = videoFormatI420;
	pic.iStride[0] = h264->iStride[0];
	pic.iStride[1] = h264->iStride[1];
	pic.iStride[2] = h264->iStride[2];
	pic.pData[0] = h264->pYUVData[0];
	pic.pData[1] = h264->pYUVData[1];
	pic.pData[2] = h264->pYUVData[2];

	status = (*sys->pEncoder)->EncodeFrame(sys->pEncoder, &pic, &info);

	if (status < 0)
	{
		WLog_ERR(TAG, "Failed to encode frame (status=%ld)", status);
		return status;
	}

	*ppDstData = info.sLayerInfo[0].pBsBuf;
	*pDstSize = 0;

	for (i = 0; i < info.iLayerNum; i++)
	{
		for (j = 0; j < info.sLayerInfo[i].iNalCount; j++)
		{
			*pDstSize += info.sLayerInfo[i].pNalLengthInByte[j];
		}
	}

	return 1;
}

static void openh264_uninit(H264_CONTEXT* h264)
{
	H264_CONTEXT_OPENH264* sys = (H264_CONTEXT_OPENH264*) h264->pSystemData;

	if (sys)
	{
		if (sys->pDecoder)
		{
			(*sys->pDecoder)->Uninitialize(sys->pDecoder);
			WelsDestroyDecoder(sys->pDecoder);
			sys->pDecoder = NULL;
		}

		if (sys->pEncoder)
		{
			(*sys->pEncoder)->Uninitialize(sys->pEncoder);
			WelsDestroySVCEncoder(sys->pEncoder);
			sys->pEncoder = NULL;
		}

		free(sys);
		h264->pSystemData = NULL;
	}
}

static BOOL openh264_init(H264_CONTEXT* h264)
{
	long status;
	SDecodingParam sDecParam;
	H264_CONTEXT_OPENH264* sys;
	static int traceLevel = WELS_LOG_DEBUG;
	static EVideoFormatType videoFormat = videoFormatI420;
	static WelsTraceCallback traceCallback = (WelsTraceCallback) openh264_trace_callback;

	sys = (H264_CONTEXT_OPENH264*) calloc(1, sizeof(H264_CONTEXT_OPENH264));

	if (!sys)
	{
		goto EXCEPTION;
	}

	h264->pSystemData = (void*) sys;

	if (h264->Compressor)
	{
		WelsCreateSVCEncoder(&sys->pEncoder);

		if (!sys->pEncoder)
		{
			WLog_ERR(TAG, "Failed to create OpenH264 encoder");
			goto EXCEPTION;
		}
	}
	else
	{
		WelsCreateDecoder(&sys->pDecoder);

		if (!sys->pDecoder)
		{
			WLog_ERR(TAG, "Failed to create OpenH264 decoder");
			goto EXCEPTION;
		}

		ZeroMemory(&sDecParam, sizeof(sDecParam));
		sDecParam.eOutputColorFormat  = videoFormatI420;
		sDecParam.eEcActiveIdc = ERROR_CON_FRAME_COPY;
		sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;

		status = (*sys->pDecoder)->Initialize(sys->pDecoder, &sDecParam);

		if (status != 0)
		{
			WLog_ERR(TAG, "Failed to initialize OpenH264 decoder (status=%ld)", status);
			goto EXCEPTION;
		}

		status = (*sys->pDecoder)->SetOption(sys->pDecoder, DECODER_OPTION_DATAFORMAT, &videoFormat);

		if (status != 0)
		{
			WLog_ERR(TAG, "Failed to set data format option on OpenH264 decoder (status=%ld)", status);
		}

		if (g_openh264_trace_enabled)
		{
			status = (*sys->pDecoder)->SetOption(sys->pDecoder, DECODER_OPTION_TRACE_LEVEL, &traceLevel);

			if (status != 0)
			{
				WLog_ERR(TAG, "Failed to set trace level option on OpenH264 decoder (status=%ld)", status);
			}

			status = (*sys->pDecoder)->SetOption(sys->pDecoder, DECODER_OPTION_TRACE_CALLBACK, &traceCallback);

			if (status != 0)
			{
				WLog_ERR(TAG, "Failed to set trace callback option on OpenH264 decoder (status=%ld)", status);
			}

			status = (*sys->pDecoder)->SetOption(sys->pDecoder, DECODER_OPTION_TRACE_CALLBACK_CONTEXT, &h264);

			if (status != 0)
			{
				WLog_ERR(TAG, "Failed to set trace callback context option on OpenH264 decoder (status=%ld)", status);
			}
		}
	}

	return TRUE;

EXCEPTION:
	openh264_uninit(h264);

	return FALSE;
}

static H264_CONTEXT_SUBSYSTEM g_Subsystem_OpenH264 =
{
	"OpenH264",
	openh264_init,
	openh264_uninit,
	openh264_decompress,
	openh264_compress
};

#endif

/**
 * libavcodec subsystem
 */

#ifdef WITH_LIBAVCODEC

#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>

struct _H264_CONTEXT_LIBAVCODEC
{
	AVCodec* codec;
	AVCodecContext* codecContext;
	AVCodecParserContext* codecParser;
	AVFrame* videoFrame;
};
typedef struct _H264_CONTEXT_LIBAVCODEC H264_CONTEXT_LIBAVCODEC;

static int libavcodec_decompress(H264_CONTEXT* h264, BYTE* pSrcData, UINT32 SrcSize)
{
	int status;
	int gotFrame = 0;
	AVPacket packet;
	H264_CONTEXT_LIBAVCODEC* sys = (H264_CONTEXT_LIBAVCODEC*) h264->pSystemData;

	av_init_packet(&packet);

	packet.data = pSrcData;
	packet.size = SrcSize;

	status = avcodec_decode_video2(sys->codecContext, sys->videoFrame, &gotFrame, &packet);

	if (status < 0)
	{
		WLog_ERR(TAG, "Failed to decode video frame (status=%d)", status);
		return -1;
	}

#if 0
	WLog_INFO(TAG, "libavcodec_decompress: frame decoded (status=%d, gotFrame=%d, width=%d, height=%d, Y=[%p,%d], U=[%p,%d], V=[%p,%d])",
		status, gotFrame, sys->videoFrame->width, sys->videoFrame->height,
		sys->videoFrame->data[0], sys->videoFrame->linesize[0],
		sys->videoFrame->data[1], sys->videoFrame->linesize[1],
		sys->videoFrame->data[2], sys->videoFrame->linesize[2]);
#endif

	if (gotFrame)
	{
		h264->pYUVData[0] = sys->videoFrame->data[0];
		h264->pYUVData[1] = sys->videoFrame->data[1];
		h264->pYUVData[2] = sys->videoFrame->data[2];

		h264->iStride[0] = sys->videoFrame->linesize[0];
		h264->iStride[1] = sys->videoFrame->linesize[1];
		h264->iStride[2] = sys->videoFrame->linesize[2];

		h264->width = sys->videoFrame->width;
		h264->height = sys->videoFrame->height;
	}
	else
		return -2;

	return 1;
}

static void libavcodec_uninit(H264_CONTEXT* h264)
{
	H264_CONTEXT_LIBAVCODEC* sys = (H264_CONTEXT_LIBAVCODEC*) h264->pSystemData;

	if (!sys)
		return;

	if (sys->videoFrame)
	{
		av_free(sys->videoFrame);
	}

	if (sys->codecParser)
	{
		av_parser_close(sys->codecParser);
	}

	if (sys->codecContext)
	{
		avcodec_close(sys->codecContext);
		av_free(sys->codecContext);
	}

	free(sys);
	h264->pSystemData = NULL;
}

static BOOL libavcodec_init(H264_CONTEXT* h264)
{
	H264_CONTEXT_LIBAVCODEC* sys;

	sys = (H264_CONTEXT_LIBAVCODEC*) calloc(1, sizeof(H264_CONTEXT_LIBAVCODEC));

	if (!sys)
	{
		goto EXCEPTION;
	}

	h264->pSystemData = (void*) sys;

	avcodec_register_all();

	sys->codec = avcodec_find_decoder(CODEC_ID_H264);

	if (!sys->codec)
	{
		WLog_ERR(TAG, "Failed to find libav H.264 codec");
		goto EXCEPTION;
	}

	sys->codecContext = avcodec_alloc_context3(sys->codec);

	if (!sys->codecContext)
	{
		WLog_ERR(TAG, "Failed to allocate libav codec context");
		goto EXCEPTION;
	}

	if (sys->codec->capabilities & CODEC_CAP_TRUNCATED)
	{
		sys->codecContext->flags |= CODEC_FLAG_TRUNCATED;
	}

	if (avcodec_open2(sys->codecContext, sys->codec, NULL) < 0)
	{
		WLog_ERR(TAG, "Failed to open libav codec");
		goto EXCEPTION;
	}

	sys->codecParser = av_parser_init(CODEC_ID_H264);

	if (!sys->codecParser)
	{
		WLog_ERR(TAG, "Failed to initialize libav parser");
		goto EXCEPTION;
	}

	sys->videoFrame = avcodec_alloc_frame();

	if (!sys->videoFrame)
	{
		WLog_ERR(TAG, "Failed to allocate libav frame");
		goto EXCEPTION;
	}

	return TRUE;

EXCEPTION:
	libavcodec_uninit(h264);

	return FALSE;
}

static H264_CONTEXT_SUBSYSTEM g_Subsystem_libavcodec =
{
	"libavcodec",
	libavcodec_init,
	libavcodec_uninit,
	libavcodec_decompress
};

#endif

int h264_decompress(H264_CONTEXT* h264, BYTE* pSrcData, UINT32 SrcSize,
		BYTE** ppDstData, DWORD DstFormat, int nDstStep, int nDstWidth,
		int nDstHeight, RDPGFX_RECT16* regionRects, int numRegionRects)
{
	int index;
	int status;
	int* iStride;
	BYTE* pDstData;
	BYTE* pDstPoint;
	prim_size_t roi;
	BYTE** pYUVData;
	int width, height;
	BYTE* pYUVPoint[3];
	RDPGFX_RECT16* rect;
	primitives_t* prims = primitives_get();

	if (!h264)
		return -1001;

#if 0
	WLog_INFO(TAG, "h264_decompress: pSrcData=%p, SrcSize=%u, pDstData=%p, nDstStep=%d, nDstHeight=%d, numRegionRects=%d",
		pSrcData, SrcSize, *ppDstData, nDstStep, nDstHeight, numRegionRects);
#endif

	if (!(pDstData = *ppDstData))
		return -1002;

	status = h264->subsystem->Decompress(h264, pSrcData, SrcSize);

	if (status == 0)
		return 1;

	if (status < 0)
		return status;

	pYUVData = h264->pYUVData;
	iStride = h264->iStride;

	for (index = 0; index < numRegionRects; index++)
	{
		rect = &(regionRects[index]);

		/* Check, if the output rectangle is valid in decoded h264 frame. */
		if ((rect->right > h264->width) || (rect->left > h264->width))
			return -1003;
		if ((rect->top > h264->height) || (rect->bottom > h264->height))
			return -1004;

		/* Check, if the output rectangle is valid in destination buffer. */
		if ((rect->right > nDstWidth) || (rect->left > nDstWidth))
			return -1005;
		if ((rect->bottom > nDstHeight) || (rect->top > nDstHeight))
			return -1006;

		width = rect->right - rect->left;
		height = rect->bottom - rect->top;
		
		pDstPoint = pDstData + rect->top * nDstStep + rect->left * 4;
		pYUVPoint[0] = pYUVData[0] + rect->top * iStride[0] + rect->left;

		pYUVPoint[1] = pYUVData[1] + rect->top/2 * iStride[1] + rect->left/2;
		pYUVPoint[2] = pYUVData[2] + rect->top/2 * iStride[2] + rect->left/2;

#if 0
		WLog_INFO(TAG, "regionRect: x: %d y: %d width: %d height: %d",
		       rect->left, rect->top, width, height);
#endif

		roi.width = width;
		roi.height = height;

		prims->YUV420ToRGB_8u_P3AC4R((const BYTE**) pYUVPoint, iStride, pDstPoint, nDstStep, &roi);
	}

	return 1;
}

int h264_compress(H264_CONTEXT* h264, BYTE* pSrcData, DWORD SrcFormat,
		int nSrcStep, int nSrcWidth, int nSrcHeight, BYTE** ppDstData, UINT32* pDstSize)
{
	int status = -1;
	prim_size_t roi;
	int nWidth, nHeight;
	primitives_t* prims = primitives_get();

	if (!h264)
		return -1;
	if (!h264->subsystem->Compress)
		return -1;

	nWidth = (nSrcWidth + 1) & ~1;
	nHeight = (nSrcHeight + 1) & ~1;

	if (!(h264->pYUVData[0] = (BYTE*) malloc(nWidth * nHeight)))
		return -1;
	h264->iStride[0] = nWidth;

	if (!(h264->pYUVData[1] = (BYTE*) malloc(nWidth * nHeight / 4)))
		goto error_1;
	h264->iStride[1] = nWidth / 2;

	if (!(h264->pYUVData[2] = (BYTE*) malloc(nWidth * nHeight / 4)))
		goto error_2;
	h264->iStride[2] = nWidth / 2;

	h264->width = nWidth;
	h264->height = nHeight;
	roi.width = nSrcWidth;
	roi.height = nSrcHeight;

	prims->RGBToYUV420_8u_P3AC4R(pSrcData, nSrcStep, h264->pYUVData, h264->iStride, &roi);

	status = h264->subsystem->Compress(h264, ppDstData, pDstSize);

	free(h264->pYUVData[2]);
	h264->pYUVData[2] = NULL;
error_2:
	free(h264->pYUVData[1]);
	h264->pYUVData[1] = NULL;
error_1:
	free(h264->pYUVData[0]);
	h264->pYUVData[0] = NULL;

	return status;
}

BOOL h264_context_init(H264_CONTEXT* h264)
{
#if defined(_WIN32) && defined(WITH_MEDIA_FOUNDATION)
	if (g_Subsystem_MF.Init(h264))
	{
		h264->subsystem = &g_Subsystem_MF;
		return TRUE;
	}
#endif

#ifdef WITH_LIBAVCODEC
	if (g_Subsystem_libavcodec.Init(h264))
	{
		h264->subsystem = &g_Subsystem_libavcodec;
		return TRUE;
	}
#endif

#ifdef WITH_OPENH264
	if (g_Subsystem_OpenH264.Init(h264))
	{
		h264->subsystem = &g_Subsystem_OpenH264;
		return TRUE;
	}
#endif

#ifdef WITH_X264
	if (g_Subsystem_x264.Init(h264))
	{
		h264->subsystem = &g_Subsystem_x264;
		return TRUE;
	}
#endif

	return FALSE;
}

int h264_context_reset(H264_CONTEXT* h264)
{
	return 1;
}

H264_CONTEXT* h264_context_new(BOOL Compressor)
{
	H264_CONTEXT* h264;

	h264 = (H264_CONTEXT*) calloc(1, sizeof(H264_CONTEXT));

	if (h264)
	{
		h264->Compressor = Compressor;

		h264->subsystem = &g_Subsystem_dummy;

		if (Compressor)
		{
			/* Default compressor settings, may be changed by caller */
			h264->BitRate = 1000000;
			h264->FrameRate = 30;
		}

		if (!h264_context_init(h264))
		{
			free(h264);
			return NULL;
		}
	}

	return h264;
}

void h264_context_free(H264_CONTEXT* h264)
{
	if (h264)
	{
		h264->subsystem->Uninit(h264);

		free(h264);
	}
}

/**
 * h.264 data packets to 'realtime' playback/preview using Apple's VideoToolbox
 * http://stackoverflow.com/a/25086313/497548
 */

/**
 * Network Abstraction Layer Units (NALs)
 *
 * 0		Unspecified                                                    non-VCL
 * 1		Coded slice of a non-IDR picture                               VCL
 * 2		Coded slice data partition A                                   VCL
 * 3		Coded slice data partition B                                   VCL
 * 4		Coded slice data partition C                                   VCL
 * 5		Coded slice of an IDR picture                                  VCL
 * 6		Supplemental enhancement information (SEI)                     non-VCL
 * 7		Sequence parameter set                                         non-VCL
 * 8		Picture parameter set                                          non-VCL
 * 9		Access unit delimiter                                          non-VCL
 * 10		End of sequence                                                non-VCL
 * 11		End of stream                                                  non-VCL
 * 12		Filler data                                                    non-VCL
 * 13		Sequence parameter set extension                               non-VCL
 * 14		Prefix NAL unit                                                non-VCL
 * 15		Subset sequence parameter set                                  non-VCL
 * 16		Depth parameter set                                            non-VCL
 * 17..18	Reserved                                                       non-VCL
 * 19		Coded slice of an auxiliary coded picture without partitioning non-VCL
 * 20		Coded slice extension                                          non-VCL
 * 21		Coded slice extension for depth view components                non-VCL
 * 22..23	Reserved                                                       non-VCL
 * 24..31	Unspecified                                                    non-VCL
 */

const char* h264_get_nal_unit_name(int nal_unit_type)
{
	switch (nal_unit_type)
	{
		case 0: return "Unspecified";
		case 1: return "Coded slice of a non-IDR picture";
		case 2: return "Coded slice data partition A";
		case 3: return "Coded slice data partition B";
		case 4: return "Coded slice data partition C";
		case 5: return "Coded slice of an IDR picture";
		case 6: return "Supplemental enhancement information (SEI)";
		case 7: return "Sequence parameter set";
		case 8: return "Picture parameter set";
		case 9: return "Access unit delimiter";
		case 10: return "End of sequence";
		case 11: return "End of stream";
		case 12: return "Filler data";
		case 13: return "Sequence parameter set extension";
		case 14: return "Prefix NAL unit";
		case 15: return "Subset sequence parameter set";
		case 16: return "Depth parameter set";
		case 17: return "Reserved17";
		case 18: return "Reserved18";
		case 19: return "Coded slice of an auxiliary coded picture without partitioning";
		case 20: return "Coded slice extension";
		case 21: return "Coded slice extension for depth view components";
		case 22: return "Reserved22";
		case 23: return "Reserved23";
		case 24: return "Unspecified24";
		case 25: return "Unspecified25";
		case 26: return "Unspecified26";
		case 27: return "Unspecified27";
		case 28: return "Unspecified28";
		case 29: return "Unspecified29";
		case 30: return "Unspecified30";
		case 31: return "Unspecified31";
	}

	return "Unknown";
}

static BOOL g_LZCNT = FALSE;

static INLINE UINT32 lzcnt_s(UINT32 x)
{
	if (!x)
		return 32;

	if (!g_LZCNT)
	{
		UINT32 y;
		int n = 32;
		y = x >> 16;  if (y != 0) { n = n - 16; x = y; }
		y = x >>  8;  if (y != 0) { n = n -  8; x = y; }
		y = x >>  4;  if (y != 0) { n = n -  4; x = y; }
		y = x >>  2;  if (y != 0) { n = n -  2; x = y; }
		y = x >>  1;  if (y != 0) return n - 2;
		return n - x;
	}

	return __lzcnt(x);
}

int h264_parse_exp_golomb_code(wBitStream* bs)
{
	int vk;
	int cnt;
	int nbits;
	int code;

	cnt = lzcnt_s(bs->accumulator);

	nbits = BitStream_GetRemainingLength(bs);

	if (cnt > nbits)
		cnt = nbits;

	vk = cnt;

	while ((cnt == 32) && (BitStream_GetRemainingLength(bs) > 0))
	{
		BitStream_Shift32(bs);

		cnt = lzcnt_s(bs->accumulator);

		nbits = BitStream_GetRemainingLength(bs);

		if (cnt > nbits)
			cnt = nbits;

		vk += cnt;
	}

	if (vk > 0)
	{
		BitStream_Shift(bs, (vk % 32));
		BitStream_Shift(bs, 1);
		BitStream_Read_Bits(bs, code, vk);
		code += (1 << vk) - 1;
	}
	else
	{
		BitStream_Shift(bs, 1);
		code = 0;
	}

	return code;
}

struct h264_sequence_parameter_set
{
	int profile_idc;
	int constraint_set0_flag;
	int constraint_set1_flag;
	int constraint_set2_flag;
	int constraint_set3_flag;
	int constraint_set4_flag;
	int constraint_set5_flag;
	int reserved_zero_2bits;
	int level_idc;
	int seq_parameter_set_id;

	int chroma_format_idc;
	int separate_colour_plane_flag;
	int bit_depth_luma_minus8;
	int bit_depth_chroma_minus8;
	int qpprime_y_zero_transform_bypass_flag;
	int seq_scaling_matrix_present_flag;

	int log2_max_frame_num_minus4;
	int pic_order_cnt_type;
	int log2_max_pic_order_cnt_lsb_minus4;
	int delta_pic_order_always_zero_flag;
	int offset_for_non_ref_pic;
	int offset_for_top_to_bottom_field;
	int num_ref_frames_in_pic_order_cnt_cycle;

	int max_num_ref_frames;
	int gaps_in_frame_num_value_allowed_flag;
	int pic_width_in_mbs_minus1;
	int pic_height_in_map_units_minus1;
	int frame_mbs_only_flag;
	int mb_adaptive_frame_field_flag;
	int direct_8x8_inference_flag;

	int frame_cropping_flag;
	int frame_crop_left_offset;
	int frame_crop_right_offset;
	int frame_crop_top_offset;
	int frame_crop_bottom_offset;

	int vui_parameters_present_flag;
};

int h264_parse_sequence_parameter_set(struct h264_sequence_parameter_set* sps, BYTE* data, UINT32 size)
{
	int i;
	wBitStream s_bs;
	wBitStream* bs = &s_bs;

	ZeroMemory(sps, sizeof(struct h264_sequence_parameter_set));

	BitStream_Attach(bs, data, size);
	BitStream_Fetch(bs);

	BitStream_Read_Bits(bs, sps->profile_idc, 8);
	BitStream_Read_Bits(bs, sps->constraint_set0_flag, 1);
	BitStream_Read_Bits(bs, sps->constraint_set1_flag, 1);
	BitStream_Read_Bits(bs, sps->constraint_set2_flag, 1);
	BitStream_Read_Bits(bs, sps->constraint_set3_flag, 1);
	BitStream_Read_Bits(bs, sps->constraint_set4_flag, 1);
	BitStream_Read_Bits(bs, sps->constraint_set5_flag, 1);
	BitStream_Read_Bits(bs, sps->reserved_zero_2bits, 2);
	BitStream_Read_Bits(bs, sps->level_idc, 8);

	sps->seq_parameter_set_id = h264_parse_exp_golomb_code(bs);

	if (sps->profile_idc == 100 || sps->profile_idc == 110 || sps->profile_idc == 122 ||
			sps->profile_idc == 244 || sps->profile_idc == 44 || sps->profile_idc == 83 ||
			sps->profile_idc == 86 || sps->profile_idc == 118 || sps->profile_idc == 128)
	{
		sps->chroma_format_idc = h264_parse_exp_golomb_code(bs);

		if (sps->chroma_format_idc == 3)
			BitStream_Read_Bits(bs, sps->separate_colour_plane_flag, 1);

		sps->bit_depth_luma_minus8 = h264_parse_exp_golomb_code(bs);
		sps->bit_depth_chroma_minus8 = h264_parse_exp_golomb_code(bs);

		BitStream_Read_Bits(bs, sps->qpprime_y_zero_transform_bypass_flag, 1);
		BitStream_Read_Bits(bs, sps->seq_scaling_matrix_present_flag, 1);

		if (sps->seq_scaling_matrix_present_flag)
		{
			for (i = 0; i < ((sps->chroma_format_idc != 3) ? 8 : 12); i++)
			{
				/* seq_scaling_list_present_flag[i] */
				BitStream_Shift(bs, 1);
			}
		}
	}

	sps->log2_max_frame_num_minus4 = h264_parse_exp_golomb_code(bs);
	sps->pic_order_cnt_type = h264_parse_exp_golomb_code(bs);

	if (sps->pic_order_cnt_type == 0)
	{
		sps->log2_max_pic_order_cnt_lsb_minus4 = h264_parse_exp_golomb_code(bs);
	}
	else if (sps->pic_order_cnt_type == 1)
	{
		BitStream_Read_Bits(bs, sps->delta_pic_order_always_zero_flag, 1);
		sps->offset_for_non_ref_pic = h264_parse_exp_golomb_code(bs);
		sps->offset_for_top_to_bottom_field = h264_parse_exp_golomb_code(bs);
		sps->num_ref_frames_in_pic_order_cnt_cycle = h264_parse_exp_golomb_code(bs);

		for (i = 0; i < sps->num_ref_frames_in_pic_order_cnt_cycle; i++)
		{
			/* offset_for_ref_frame[i] */
			h264_parse_exp_golomb_code(bs);
		}
	}

	sps->max_num_ref_frames = h264_parse_exp_golomb_code(bs);
	BitStream_Read_Bits(bs, sps->gaps_in_frame_num_value_allowed_flag, 1);
	sps->pic_width_in_mbs_minus1 = h264_parse_exp_golomb_code(bs);
	sps->pic_height_in_map_units_minus1 = h264_parse_exp_golomb_code(bs);
	BitStream_Read_Bits(bs, sps->frame_mbs_only_flag, 1);

	if (!sps->frame_mbs_only_flag)
		BitStream_Read_Bits(bs, sps->mb_adaptive_frame_field_flag, 1);

	BitStream_Read_Bits(bs, sps->direct_8x8_inference_flag, 1);
	BitStream_Read_Bits(bs, sps->frame_cropping_flag, 1);

	if (sps->frame_cropping_flag)
	{
		sps->frame_crop_left_offset = h264_parse_exp_golomb_code(bs);
		sps->frame_crop_right_offset = h264_parse_exp_golomb_code(bs);
		sps->frame_crop_top_offset = h264_parse_exp_golomb_code(bs);
		sps->frame_crop_bottom_offset = h264_parse_exp_golomb_code(bs);
	}

	BitStream_Read_Bits(bs, sps->vui_parameters_present_flag, 1);

	if (sps->vui_parameters_present_flag)
	{

	}

	return 1;
}

struct h264_picture_parameter_set
{
	int pic_parameter_set_id;
	int seq_parameter_set_id;
	int entropy_coding_mode_flag;
	int bottom_field_pic_order_in_frame_present_flag;
	int num_slice_groups_minus1;
	int slice_group_map_type;
	int slice_group_change_direction_flag;
	int slice_group_change_rate_minus1;
	int pic_size_in_map_units_minus1;

	int num_ref_idx_l0_default_active_minus1;
	int num_ref_idx_l1_default_active_minus1;
	int weighted_pred_flag;
	int weighted_bipred_idc;
	int pic_init_qp_minus26;
	int pic_init_qs_minus26;
	int chroma_qp_index_offset;
	int deblocking_filter_control_present_flag;
	int constrained_intra_pred_flag;
	int redundant_pic_cnt_present_flag;

	int transform_8x8_mode_flag;
	int pic_scaling_matrix_present_flag;
	int second_chroma_qp_index_offset;
};

int h264_parse_picture_parameter_set(struct h264_picture_parameter_set* pps, BYTE* data, UINT32 size)
{
	int i, iGroup;
	wBitStream s_bs;
	wBitStream* bs = &s_bs;

	ZeroMemory(pps, sizeof(struct h264_picture_parameter_set));

	BitStream_Attach(bs, data, size);
	BitStream_Fetch(bs);

	pps->pic_parameter_set_id = h264_parse_exp_golomb_code(bs);
	pps->seq_parameter_set_id = h264_parse_exp_golomb_code(bs);
	BitStream_Read_Bits(bs, pps->entropy_coding_mode_flag, 1);
	BitStream_Read_Bits(bs, pps->bottom_field_pic_order_in_frame_present_flag, 1);
	pps->num_slice_groups_minus1 = h264_parse_exp_golomb_code(bs);

	if (pps->num_slice_groups_minus1 > 0)
	{
		pps->slice_group_map_type = h264_parse_exp_golomb_code(bs);

		if (pps->slice_group_map_type == 0)
		{
			for (iGroup = 0; iGroup < pps->num_slice_groups_minus1; iGroup++)
			{
				h264_parse_exp_golomb_code(bs); /* run_length_minus1[iGroup] */
			}
		}
		else if (pps->slice_group_map_type == 2)
		{
			for (iGroup = 0; iGroup < pps->num_slice_groups_minus1; iGroup++)
			{
				h264_parse_exp_golomb_code(bs); /* top_left[iGroup] */
				h264_parse_exp_golomb_code(bs); /* bottom_right[iGroup] */
			}
		}
		else if ((pps->slice_group_map_type == 3) || (pps->slice_group_map_type == 4) ||
				(pps->slice_group_map_type == 5))
		{
			BitStream_Read_Bits(bs, pps->slice_group_change_direction_flag, 1);
			pps->slice_group_change_rate_minus1 = h264_parse_exp_golomb_code(bs);
		}
		else if (pps->slice_group_map_type == 6)
		{
			pps->pic_size_in_map_units_minus1 = h264_parse_exp_golomb_code(bs);

			for (i = 0; i < pps->pic_size_in_map_units_minus1; i++)
			{
				h264_parse_exp_golomb_code(bs); /* slice_group_id[i] */
			}
		}
	}

	pps->num_ref_idx_l0_default_active_minus1 = h264_parse_exp_golomb_code(bs);
	pps->num_ref_idx_l1_default_active_minus1 = h264_parse_exp_golomb_code(bs);
	BitStream_Read_Bits(bs, pps->weighted_pred_flag, 1);
	BitStream_Read_Bits(bs, pps->weighted_bipred_idc, 2);
	pps->pic_init_qp_minus26 = h264_parse_exp_golomb_code(bs);
	pps->pic_init_qs_minus26 = h264_parse_exp_golomb_code(bs);
	pps->chroma_qp_index_offset = h264_parse_exp_golomb_code(bs);
	BitStream_Read_Bits(bs, pps->deblocking_filter_control_present_flag, 1);
	BitStream_Read_Bits(bs, pps->constrained_intra_pred_flag, 1);
	BitStream_Read_Bits(bs, pps->redundant_pic_cnt_present_flag, 1);

	return 1;
}

/*
 * nal_unit_header_svc_extension() {
 * 	idr_flag			u(1)
 * 	priority_id			u(6)
 * 	no_inter_layer_pred_flag	u(1)
 * 	dependency_id			u(3)
 * 	quality_id			u(4)
 * 	temporal_id			u(3)
 * 	use_ref_base_pic_flag		u(1)
 * 	discardable_flag		u(1)
 * 	output_flag			u(1)
 * 	reserved_three_2bits		u(2)
 * }
 *
 * nal_unit_header_mvc_extension() {
 * 	non_idr_flag			u(1)
 * 	priority_id			u(6)
 * 	view_id				u(10)
 * 	temporal_id			u(3)
 * 	anchor_pic_flag			u(1)
 * 	inter_view_flag			u(1)
 * 	reserved_one_bit		u(1)
 * }
 *
 * nal_unit(NumBytesInNALunit) {
 * 	forbidden_zero_bit		f(1)
 * 	nal_ref_idc			u(2)
 * 	nal_unit_type			u(5)
 * 	NumBytesInRBSP = 0
 * 	nalUnitHeaderBytes = 1
 * 	if (nal_unit_type == 14 || nal_unit_type == 20) {
 * 		svc_extension_flag	u(1)
 * 		if (svc_extension_flag)
 * 			nal_unit_header_svc_extension()
 * 		else
 * 			nal_unit_header_mvc_extension()
 * 		nalUnitHeaderBytes += 3
 * 	}
 * 	for (i = nalUnitHeaderBytes; i < NumBytesInNALunit; i++) {
 * 		if (i + 2 < NumBytesInNALunit && next_bits(24) == 0x000003) {
 * 			rbsp_byte[NumBytesInRBSP++]	b(8)
 * 			rbsp_byte[NumBytesInRBSP++]	b(8)
 * 			i += 2
 * 			emulation_prevention_three_byte	f(8)
 * 		} else {
 * 			rbsp_byte[NumBytesInRBSP++]	b(8)
 * 		}
 * 	}
 * }
 *
 * byte_stream_nal_unit(NumBytesInNALunit) {
 * 	while (next_bits(24) != 0x000001 && next_bits(32) != 0x00000001)
 * 		leading_zero_8bits	f(8)
 * 	if (next_bits(24) != 0x000001)
 * 		zero_byte		f(8)
 * 	start_code_prefix_one_3bytes	f(24)
 * 	nal_unit(NumBytesInNALunit)
 * 	while (more_data_in_byte_stream() && next_bits(24) != 0x000001 && next_bits(32) != 0x00000001)
 * 		trailing_zero_8bits	f(8)
 * }
 *
 */

int h264_parse_byte_stream(BYTE* pSrcData, UINT32 SrcSize)
{
	BYTE* d;
	int nal_ref_idc;
	int nal_unit_type;
	int nal_unit_index;
	int svc_extension_flag;
	int nal_unit_header_bytes;
	int num_bytes_in_nal_unit;
	BYTE* p = pSrcData;
	BYTE* e = &p[SrcSize];

	g_LZCNT = IsProcessorFeaturePresentEx(PF_EX_LZCNT);

	if ((e - p) < 4)
		return -1;

	while (!((p[0] == 0) && (p[1] == 0) && (p[2] == 0) && (p[3] == 1)))
	{
		if (*p != 0)
			return -1;

		p++; /* leading_zero_bits, 0x00 */

		if ((e - p) < 4)
			return -1;
	}

	for (nal_unit_index = 0; nal_unit_index < 20; nal_unit_index++)
	{
		if ((e - p) < 3)
			return -1;

		if (!((p[0] == 0) && (p[1] == 0) && (p[2] == 1)))
		{
			p++; /* zero_byte, 0x00 */

			if (p >= e)
				return -1;
		}

		if ((e - p) < 3)
			return -1;

		p += 3; /* start_code_prefix_one_3bytes, 0x000001 */

		d = p;

		while ((!((d[0] == 0) && (d[1] == 0) && (d[2] == 1))) &&
			(!((d[0] == 0) && (d[1] == 0) && (d[2] == 0))))
		{
			if (d >= e)
				break;

			d++;
		}

		num_bytes_in_nal_unit = d - p;

		d = p;

		if (*p & 0x80)
			return -1; /* forbidden_zero_bit */

		nal_ref_idc = *p >> 5;
		nal_unit_type = *p & 0x1F;
		p++;

		nal_unit_header_bytes = 1;

		if ((nal_unit_type == 14) || (nal_unit_type == 20))
		{
			if ((e - p) < 3)
				return -1;

			svc_extension_flag = (*p >> 7) & 0x1;

			if (svc_extension_flag)
			{
				int idr_flag;
				int priority_id;
				int no_inter_layer_pred_flag;
				int dependency_id;
				int quality_id;
				int temporal_id;
				int use_ref_base_pic_flag;
				int discardable_flag;
				int output_flag;
				int reserved_three_2bits;

				idr_flag = (*p >> 6) & 0x1;
				priority_id = *p & 0x3F;
				p++;

				no_inter_layer_pred_flag = (*p >> 7) & 0x1;
				dependency_id = (*p >> 4) & 0x7;
				quality_id = *p & 0xF;
				p++;

				temporal_id = (*p >> 5) & 0x7;
				use_ref_base_pic_flag = (*p >> 4) & 0x1;
				discardable_flag = (*p >> 3) & 0x1;
				output_flag = (*p >> 2) & 0x1;
				reserved_three_2bits = *p & 0x3;
				p++;
			}
			else
			{
				int non_idr_flag;
				int priority_id;
				int view_id;
				int temporal_id;
				int anchor_pic_flag;
				int inter_view_flag;
				int reserved_one_bit;

				non_idr_flag = (*p >> 6) & 0x1;
				priority_id = *p & 0x3F;
				p++;

				view_id = (p[0] << 8) + (p[1] >> 6);
				p++;

				temporal_id = (*p >> 3) & 0x7;
				anchor_pic_flag = (*p >> 2) & 0x1;
				inter_view_flag = (*p >> 1) & 0x1;
				reserved_one_bit = *p & 0x1;
				p++;
			}

			nal_unit_header_bytes += 3;
		}

		if (nal_unit_type == 7) /* sequence parameter set */
		{
			struct h264_sequence_parameter_set sps;
			h264_parse_sequence_parameter_set(&sps, p, num_bytes_in_nal_unit);
		}
		else if (nal_unit_type == 8) /* picture parameter set */
		{
			struct h264_picture_parameter_set pps;
			h264_parse_picture_parameter_set(&pps, p, num_bytes_in_nal_unit);
		}

		p = d + num_bytes_in_nal_unit;

		while ((p < e) && (!(((e - p) >= 3) && (p[0] == 0) && (p[1] == 0) && (p[2] == 1))) &&
				(!(((e - p) >= 4) && (p[0] == 0) && (p[1] == 0) && (p[2] == 0) && (p[3] == 1))))
		{
			p++; /* trailing_zero_bits */
		}

		fprintf(stderr, "[%004d] type: %d ref_idc: %d size: %d name: %s\n",
				nal_unit_index, nal_unit_type, nal_ref_idc,
				num_bytes_in_nal_unit, h264_get_nal_unit_name(nal_unit_type));
	}

	return 1;
}
