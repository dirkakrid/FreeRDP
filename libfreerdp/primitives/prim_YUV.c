/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 *
 * Copyright 2014 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#include <freerdp/types.h>
#include <freerdp/primitives.h>
#include <freerdp/codec/color.h>

#include "prim_YUV.h"

/**
 * @brief general_YUV420CombineToYUV444
 *	U444(2x,2y) = Um(2x,2y) * 4 - Ua(2x+1,2y) - Ua(2x,2y+1) - Ua(2x+1,2y+1)
 *	V444(2x,2y) = Vm(2x,2y) * 4 - Va(2x+1,2y) - Va(2x,2y+1) - Va(2x+1,2y+1)
 * @param pSrc    Pointer to auxilary YUV420 data
 * @param srcStep Step width in auxilary YUV420 data
 * @param pDst    Pointer to main YUV420 data
 * @param dstStep Step width in main YUV420 data
 * @param roi     Area information for YUV420 data
 *
 * @return PRIMITIVES_SUCCESS on success, an error code otherwise.
 */
static pstatus_t general_YUV420CombineToYUV444(
		const BYTE* pMainSrc[3], const UINT32 srcMainStep[3],
		const BYTE* pAuxSrc[3], const UINT32 srcAuxStep[3],
		BYTE* pDst[3], const UINT32 dstStep[3],
		const prim_size_t* roi)
{
	UINT32 nWidth, nHeight;
	UINT32 x, y;
	UINT32 halfWidth, halfHeight;
	const BYTE *Ua, *Va, *Ya;
	const BYTE *Um, *Vm;
	BYTE* pU;
	BYTE* pV;

	nWidth = roi->width & ~0x01;
	nHeight = roi->height & ~0x01;
	halfWidth = nWidth / 2;
	halfHeight = nHeight / 2;

	if (pMainSrc)
	{
		/* Y data is already here... */
		/* B1 */
		size_t size = dstStep[0] * nHeight;
		memcpy(pDst[0], pMainSrc[0], size);

		/* The first half of U, V are already here part of this frame. */
		/* B2 and B3 */
		for (y=0; y<halfHeight; y++)
		{
			Um = pMainSrc[1] + srcMainStep[1] * y;
			Vm = pMainSrc[2] + srcMainStep[2] * y;
			pU = pDst[1] + dstStep[1] * y * 2;
			pV = pDst[1] + dstStep[2] * y * 2;
			for (x=0; x<halfWidth; x++)
			{
				pU[2*x] = Um[x];
				pV[2*x] = Vm[x];
			}
		}
		if (!pAuxSrc)
			return PRIMITIVES_SUCCESS;
	} else if (!pAuxSrc)
		return -1;

	/* The second half of U and V is a bit more tricky... */
	/* B4 */
	for (y=0; y<halfHeight; y++)
	{
		Ya = pAuxSrc[0] + srcAuxStep[0] * y;
		pU = pDst[1] + dstStep[1] * y * 2 + 1;

		for (x=0; x<nWidth; x++)
			pU[x] = Ya[x];
	}

	/* B5 */
	for (y=halfHeight; y<nHeight; y++)
	{
		Ya = pAuxSrc[0] + srcAuxStep[0] * y;
		pV = pDst[1] + dstStep[2] * (y - halfHeight) * 2 + 1;

		for (x=0; x<nWidth; x++)
			pV[x] = Ya[x];
	}

	/* B6 and B7 */
	for (y=0; y<halfHeight; y++)
	{
		Ua = pAuxSrc[1] + srcAuxStep[1] * y;
		Va = pAuxSrc[2] + srcAuxStep[2] * y;
		pU = pDst[1] + dstStep[1] * y * 2;
		pV = pDst[2] + dstStep[2] * y * 2;

		for (x=0; x<halfWidth; x++)
		{
			pU[2*x+1] = Ua[x];
			pV[2*x+1] = Va[x];
		}
	}

	/* B7 */
	for (y=0; y<halfHeight; y++)
	{
		Um = pAuxSrc[1] + srcAuxStep[1] * y;
		pU = pDst[1] + dstStep[1] * y * 2;
		for (x=0; x<halfWidth; x++)
			pU[2*x+1] = Um[x];
	}

	return PRIMITIVES_SUCCESS;
}

/**
 * | R |    ( | 256     0    403 | |    Y    | )
 * | G | = (  | 256   -48   -120 | | U - 128 |  ) >> 8
 * | B |    ( | 256   475      0 | | V - 128 | )
 *
 * | Y |    ( |  54   183     18 | | R | )         |  0  |
 * | U | = (  | -29   -99    128 | | G |  ) >> 8 + | 128 |
 * | V |    ( | 128  -116    -12 | | B | )         | 128 |
 */

static pstatus_t general_YUV444ToRGB_8u_P3AC4R(
		const BYTE* pSrc[3], const UINT32 srcStep[3],
		BYTE* pDst, UINT32 dstStep, const prim_size_t* roi)
{
	UINT32 x, y;
	BYTE Y, U, V;
	UINT32 R, G, B;
	UINT32 Yp, Up, Vp;
	UINT32 Up48, Up475;
	UINT32 Vp403, Vp120;
	UINT32 nWidth, nHeight;

	nWidth = (roi->width) & ~0x0001;
	nHeight = (roi->height) & ~0x0001;

	for (y = 0; y < nHeight; y++)
	{
		const BYTE* pY = pSrc[0] + y * srcStep[0];
		const BYTE* pU = pSrc[1] + y * srcStep[1];
		const BYTE* pV = pSrc[2] + y * srcStep[2];
		BYTE* pRGB = pDst + y * dstStep * 4;

		for (x = 0; x < nWidth; x++)
		{
			Y = pY[x];
			U = pU[x];
			V = pV[x];

			Up = U - 128;
			Vp = V - 128;

			Up48 = 48 * Up;
			Up475 = 475 * Up;

			Vp403 = Vp * 403;
			Vp120 = Vp * 120;

			/* 3rd pixel */
			Yp = Y << 8;

			R = (Yp + Vp403) >> 8;
			G = (Yp - Up48 - Vp120) >> 8;
			B = (Yp + Up475) >> 8;

			if (R < 0)
				R = 0;
			else if (R > 255)
				R = 255;

			if (G < 0)
				G = 0;
			else if (G > 255)
				G = 255;

			if (B < 0)
				B = 0;
			else if (B > 255)
				B = 255;

			pRGB[0] = (BYTE) B;
			pRGB[1] = (BYTE) G;
			pRGB[2] = (BYTE) R;
			pRGB[3] = 0xFF;
		}
	}

	return PRIMITIVES_SUCCESS;
}

/**
 * | R |    ( | 256     0    403 | |    Y    | )
 * | G | = (  | 256   -48   -120 | | U - 128 |  ) >> 8
 * | B |    ( | 256   475      0 | | V - 128 | )
 *
 * | Y |    ( |  54   183     18 | | R | )         |  0  |
 * | U | = (  | -29   -99    128 | | G |  ) >> 8 + | 128 |
 * | V |    ( | 128  -116    -12 | | B | )         | 128 |
 */

static pstatus_t general_YUV420ToRGB_8u_P3AC4R(
		const BYTE* pSrc[3], const UINT32 srcStep[3],
		BYTE* pDst, UINT32 dstStep, const prim_size_t* roi)
{
	UINT32 x, y;
	UINT32 dstPad;
	UINT32 srcPad[3];
	BYTE Y, U, V;
	UINT32 halfWidth;
	UINT32 halfHeight;
	const BYTE* pY;
	const BYTE* pU;
	const BYTE* pV;
	UINT32 R, G, B;
	UINT32 Yp, Up, Vp;
	UINT32 Up48, Up475;
	UINT32 Vp403, Vp120;
	BYTE* pRGB = pDst;
	UINT32 nWidth, nHeight;
	UINT32 lastRow, lastCol;

	pY = pSrc[0];
	pU = pSrc[1];
	pV = pSrc[2];

	lastCol = roi->width & 0x01;
	lastRow = roi->height & 0x01;

	nWidth = (roi->width + 1) & ~0x0001;
	nHeight = (roi->height + 1) & ~0x0001;

	halfWidth = nWidth / 2;
	halfHeight = nHeight / 2;

	srcPad[0] = (srcStep[0] - nWidth);
	srcPad[1] = (srcStep[1] - halfWidth);
	srcPad[2] = (srcStep[2] - halfWidth);

	dstPad = (dstStep - (nWidth * 4));

	for (y = 0; y < halfHeight; )
	{
		if (++y == halfHeight)
			lastRow <<= 1;

		for (x = 0; x < halfWidth; )
		{
			if (++x == halfWidth)
				lastCol <<= 1;

			U = *pU++;
			V = *pV++;

			Up = U - 128;
			Vp = V - 128;

			Up48 = 48 * Up;
			Up475 = 475 * Up;

			Vp403 = Vp * 403;
			Vp120 = Vp * 120;

			/* 1st pixel */

			Y = *pY++;
			Yp = Y << 8;

			R = (Yp + Vp403) >> 8;
			G = (Yp - Up48 - Vp120) >> 8;
			B = (Yp + Up475) >> 8;

			if (R < 0)
				R = 0;
			else if (R > 255)
				R = 255;

			if (G < 0)
				G = 0;
			else if (G > 255)
				G = 255;

			if (B < 0)
				B = 0;
			else if (B > 255)
				B = 255;

			*pRGB++ = (BYTE) B;
			*pRGB++ = (BYTE) G;
			*pRGB++ = (BYTE) R;
			*pRGB++ = 0xFF;

			/* 2nd pixel */

			if (!(lastCol & 0x02))
			{
				Y = *pY++;
				Yp = Y << 8;

				R = (Yp + Vp403) >> 8;
				G = (Yp - Up48 - Vp120) >> 8;
				B = (Yp + Up475) >> 8;

				if (R < 0)
					R = 0;
				else if (R > 255)
					R = 255;

				if (G < 0)
					G = 0;
				else if (G > 255)
					G = 255;

				if (B < 0)
					B = 0;
				else if (B > 255)
					B = 255;

				*pRGB++ = (BYTE) B;
				*pRGB++ = (BYTE) G;
				*pRGB++ = (BYTE) R;
				*pRGB++ = 0xFF;
			}
			else
			{
				pY++;
				pRGB += 4;
				lastCol >>= 1;
			}
		}

		pY += srcPad[0];
		pU -= halfWidth;
		pV -= halfWidth;
		pRGB += dstPad;

		for (x = 0; x < halfWidth; )
		{
			if (++x == halfWidth)
				lastCol <<= 1;

			U = *pU++;
			V = *pV++;

			Up = U - 128;
			Vp = V - 128;

			Up48 = 48 * Up;
			Up475 = 475 * Up;

			Vp403 = Vp * 403;
			Vp120 = Vp * 120;

			/* 3rd pixel */

			Y = *pY++;
			Yp = Y << 8;

			R = (Yp + Vp403) >> 8;
			G = (Yp - Up48 - Vp120) >> 8;
			B = (Yp + Up475) >> 8;

			if (R < 0)
				R = 0;
			else if (R > 255)
				R = 255;

			if (G < 0)
				G = 0;
			else if (G > 255)
				G = 255;

			if (B < 0)
				B = 0;
			else if (B > 255)
				B = 255;

			*pRGB++ = (BYTE) B;
			*pRGB++ = (BYTE) G;
			*pRGB++ = (BYTE) R;
			*pRGB++ = 0xFF;

			/* 4th pixel */

			if (!(lastCol & 0x02))
			{
				Y = *pY++;
				Yp = Y << 8;

				R = (Yp + Vp403) >> 8;
				G = (Yp - Up48 - Vp120) >> 8;
				B = (Yp + Up475) >> 8;

				if (R < 0)
					R = 0;
				else if (R > 255)
					R = 255;

				if (G < 0)
					G = 0;
				else if (G > 255)
					G = 255;

				if (B < 0)
					B = 0;
				else if (B > 255)
					B = 255;

				*pRGB++ = (BYTE) B;
				*pRGB++ = (BYTE) G;
				*pRGB++ = (BYTE) R;
				*pRGB++ = 0xFF;
			}
			else
			{
				pY++;
				pRGB += 4;
				lastCol >>= 1;
			}
		}

		pY += srcPad[0];
		pU += srcPad[1];
		pV += srcPad[2];
		pRGB += dstPad;
	}

	return PRIMITIVES_SUCCESS;
}

static pstatus_t general_RGBToYUV420_8u_P3AC4R(
		const BYTE* pSrc, UINT32 srcStep,
		BYTE* pDst[3], UINT32 dstStep[3], const prim_size_t* roi)
{
	UINT32 x, y;
	UINT32 dstPad[3];
	UINT32 halfWidth;
	UINT32 halfHeight;
	BYTE* pY;
	BYTE* pU;
	BYTE* pV;
	UINT32 Y, U, V;
	UINT32 R, G, B;
	UINT32 Ra, Ga, Ba;
	const BYTE* pRGB;
	UINT32 nWidth, nHeight;

	pU = pDst[1];
	pV = pDst[2];

	nWidth = (roi->width + 1) & ~0x0001;
	nHeight = (roi->height + 1) & ~0x0001;

	halfWidth = nWidth / 2;
	halfHeight = nHeight / 2;

	dstPad[0] = (dstStep[0] - nWidth);
	dstPad[1] = (dstStep[1] - halfWidth);
	dstPad[2] = (dstStep[2] - halfWidth);

	for (y = 0; y < halfHeight; y++)
	{
		for (x = 0; x < halfWidth; x++)
		{
			/* 1st pixel */
			pRGB = pSrc + y * 2 * srcStep + x * 2 * 4;
			pY = pDst[0] + y * 2 * dstStep[0] + x * 2;
			Ba = B = pRGB[0];
			Ga = G = pRGB[1];
			Ra = R = pRGB[2];
			Y = (54 * R + 183 * G + 18 * B) >> 8;
			pY[0] = (BYTE) Y;

			if (x * 2 + 1 < roi->width)
			{
				/* 2nd pixel */
				Ba += B = pRGB[4];
				Ga += G = pRGB[5];
				Ra += R = pRGB[6];
				Y = (54 * R + 183 * G + 18 * B) >> 8;
				pY[1] = (BYTE) Y;
			}

			if (y * 2 + 1 < roi->height)
			{
				/* 3rd pixel */
				pRGB += srcStep;
				pY += dstStep[0];
				Ba += B = pRGB[0];
				Ga += G = pRGB[1];
				Ra += R = pRGB[2];
				Y = (54 * R + 183 * G + 18 * B) >> 8;
				pY[0] = (BYTE) Y;

				if (x * 2 + 1 < roi->width)
				{
					/* 4th pixel */
					Ba += B = pRGB[4];
					Ga += G = pRGB[5];
					Ra += R = pRGB[6];
					Y = (54 * R + 183 * G + 18 * B) >> 8;
					pY[1] = (BYTE) Y;
				}
			}

			/* U */
			Ba >>= 2;
			Ga >>= 2;
			Ra >>= 2;
			U = ((-29 * Ra - 99 * Ga + 128 * Ba) >> 8) + 128;
			if (U < 0)
				U = 0;
			else if (U > 255)
				U = 255;
			*pU++ = (BYTE) U;

			/* V */
			V = ((128 * Ra - 116 * Ga - 12 * Ba) >> 8) + 128;
			if (V < 0)
				V = 0;
			else if (V > 255)
				V = 255;
			*pV++ = (BYTE) V;
		}

		pU += dstPad[1];
		pV += dstPad[2];
	}

	return PRIMITIVES_SUCCESS;
}

void primitives_init_YUV(primitives_t* prims)
{
	prims->YUV420ToRGB_8u_P3AC4R = general_YUV420ToRGB_8u_P3AC4R;
	prims->RGBToYUV420_8u_P3AC4R = general_RGBToYUV420_8u_P3AC4R;
	prims->YUV420CombineToYUV444 = general_YUV420CombineToYUV444;
	prims->YUV444ToRGB_8u_P3AC4R = general_YUV444ToRGB_8u_P3AC4R;

	primitives_init_YUV_opt(prims);
}

void primitives_deinit_YUV(primitives_t* prims)
{

}
