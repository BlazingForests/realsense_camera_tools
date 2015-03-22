/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2013 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#pragma once
#include <windows.h>
#include <string>
#include "pxcimage.h"
#include "pxccapture.h"
#include "pxcsession.h"


#define DEPTH_BEGIN 0
#define DEPTH_END 2047


struct UserSEL {
	PXCCapture::StreamType first;
	PXCCapture::StreamType second;
	bool	isUVMap;
	bool	isProjection;
	bool	isInvUVMap;
	int		dots;
};

extern volatile bool g_stop;
extern PXCSession *g_session;

void SetStatus(HWND hwndDlg, pxcCHAR *line);
void GetUserSEL(HWND hwndDlg, UserSEL &ss);
void QueueBitmap(PXCImage *image, int index);
void QueueUpdate(void);
bool IsMirrorEnabled(HWND hwndDlg);

void StreamSamples(HWND hwndDlg, PXCCapture::DeviceInfo *dinfo, 
					  PXCCapture::Device::StreamProfileSet *profiles, 
					  bool isAdaptive, bool synced, bool isRecord, bool isPlayback, 
					  pxcCHAR *file);
