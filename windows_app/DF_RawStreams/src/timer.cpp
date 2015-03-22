/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2013 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include <windows.h>
#include <string.h>
#include "timer.h"
#include "main.h"

FPSTimer::FPSTimer(void) {
	QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last);
    fps = 0;
}

void FPSTimer::Tick(HWND hwndDlg, PXCImage::ImageInfo *info) {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    fps++;
    if (now.QuadPart-last.QuadPart>freq.QuadPart) { // update every second
        last = now;

		pxcCHAR line[256];
		swprintf_s<sizeof(line)/sizeof(line[0])>(line,L"%s %dx%d FPS=%d", PXCImage::PixelFormatToString(info->format), info->width, info->height, fps);
		SetStatus(hwndDlg,line);
		fps=0;
    }
}

