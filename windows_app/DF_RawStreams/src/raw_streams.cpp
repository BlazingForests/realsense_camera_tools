/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2013-2014 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include <windows.h>
#include "resource1.h"
#include "pxcsensemanager.h"
#include "projection.h"
#include "timer.h"
#include "main.h"



extern int save_depth_value;


/* Checking if sensor device connect or not */
class MyHandler: public PXCSenseManager::Handler {
public:
	virtual pxcStatus PXCAPI OnConnect(PXCCapture::Device *device, pxcBool connected) {
		SetStatus(hwndDlg, connected?L"Device Connected": L"Device Disconnected");
        if (connected) {
            device->SetDeviceAllowProfileChange(adaptive);
            if (sm && profiles) {
                /* Enable all selected streams */
                PXCCapture::Device::StreamProfileSet _profiles={};
                device->QueryStreamProfileSet(PXCCapture::STREAM_TYPE_COLOR|PXCCapture::STREAM_TYPE_DEPTH|PXCCapture::STREAM_TYPE_IR, 0, &_profiles);
	            for (PXCCapture::StreamType st=PXCCapture::STREAM_TYPE_COLOR;st!=PXCCapture::STREAM_TYPE_ANY;st++) {
		            PXCCapture::Device::StreamProfile &profile=(*profiles)[st];
		            if (!profile.imageInfo.format) continue;
                    if (adaptive && device->QueryStreamProfileSetNum(st)==1) profile = _profiles[st];
		            sm->EnableStream(st,profile.imageInfo.width, profile.imageInfo.height, profile.frameRate.max);
	            }
	            sm->QueryCaptureManager()->FilterByStreamProfiles(profiles);
            }
        }
		return PXC_STATUS_NO_ERROR;
	}
	MyHandler(HWND hwndDlg1,bool adaptive,PXCSenseManager* sm,PXCCapture::Device::StreamProfileSet* profiles):hwndDlg(hwndDlg1) {this->adaptive=adaptive;this->sm=sm;this->profiles=profiles;}
protected:
	HWND hwndDlg;
    bool adaptive;
    PXCSenseManager* sm;
    PXCCapture::Device::StreamProfileSet* profiles;
};

void StreamSamples(HWND hwndDlg, PXCCapture::DeviceInfo *dinfo, PXCCapture::Device::StreamProfileSet *profiles, bool isAdaptive, bool synced, bool isRecord, bool isPlayback, pxcCHAR *file) {
    bool sts = true;
	UserSEL ss={};
	GetUserSEL(hwndDlg, ss);

    /* Create a PXCSenseManager instance */
    PXCSenseManager *pp=g_session->CreateSenseManager();
	if (!pp) {
		SetStatus(hwndDlg,L"Init Failed");
		return;
	}

	/* Set file name for playback or recording */
    if (isRecord && file[0]) 
        pp->QueryCaptureManager()->SetFileName(file,true);
    if (isPlayback && file[0]) 
        pp->QueryCaptureManager()->SetFileName(file,false);

    /* Set device source */
    if (!isPlayback) pp->QueryCaptureManager()->FilterByDeviceInfo(dinfo);

    /* Enable all selected streams in handler */
	MyHandler handler(hwndDlg,isAdaptive,pp,profiles);

    /* Initialization */
    FPSTimer timer;
    while (true) {
        SetStatus(hwndDlg, L"Init Started");
        if (pp->Init(&handler)<PXC_STATUS_NO_ERROR) {\
            SetStatus(hwndDlg, L"Init Failed");
            sts = false;
            break;
        }

        /* Set mirror mode */
        PXCCapture::Device::MirrorMode mirror = IsMirrorEnabled(hwndDlg) ? PXCCapture::Device::MirrorMode::MIRROR_MODE_HORIZONTAL : PXCCapture::Device::MirrorMode::MIRROR_MODE_DISABLED;
        pp->QueryCaptureManager()->QueryDevice()->SetMirrorMode(mirror);
    
        /* For UV Mapping & Projection only: Save certain properties */
        Projection projection(pp->QuerySession(), pp->QueryCaptureManager()->QueryDevice(), &profiles->depth.imageInfo, &profiles->color.imageInfo);

        SetStatus(hwndDlg, L"Streaming...");

		static bool processing = false;

        pxcStatus sts2=PXC_STATUS_NO_ERROR;
        for (int nframes=0;!g_stop;nframes++) {
            /* Wait until a frame is ready, synchronized or asynchronously */
            sts2=pp->AcquireFrame(synced);
            if (sts2<PXC_STATUS_NO_ERROR && sts2!=PXC_STATUS_DEVICE_LOST) break;

			/* If device is not lost */
			if (sts) {
				/* Retrieve the sample */
				PXCCapture::Sample *sample = (PXCCapture::Sample*)pp->QuerySample();

				/* Read Menu/Buttons every 15 frames */
				if (nframes%15==0) GetUserSEL(hwndDlg, ss);

				/* Perform projection */
				if ((ss.isUVMap || ss.isProjection) && sample->color && sample->depth) {
					ss.first=PXCCapture::STREAM_TYPE_COLOR;
					ss.second=PXCCapture::STREAM_TYPE_DEPTH;
					if (ss.isUVMap) projection.DepthToColorByUVMAP(sample->color, sample->depth, ss.dots);
					if (ss.isProjection) projection.DepthToColorByFunction(sample->color, sample->depth, ss.dots);

					processing = true;

				}
				if (ss.isInvUVMap && sample->color && sample->depth) {
					ss.first=PXCCapture::STREAM_TYPE_COLOR;
					ss.second=PXCCapture::STREAM_TYPE_DEPTH;
					projection.ColorToDepthByInvUVMAP(sample->color, sample->depth, ss.dots);
				}

				/* Display */
				PXCImage::ImageInfo info={0,0};
				if (ss.first && (*sample)[ss.first])  {
					QueueBitmap((*sample)[ss.first], 0);
					info=(*sample)[ss.first]->QueryInfo();
				}
				if (ss.second && (*sample)[ss.second]) {
					QueueBitmap((*sample)[ss.second], 1);
				}

				QueueUpdate();
				if (info.width>0) {
					timer.Tick(hwndDlg,&info);
				}
			}

            PXCCapture::Device::MirrorMode mirror = IsMirrorEnabled(hwndDlg) ? PXCCapture::Device::MirrorMode::MIRROR_MODE_HORIZONTAL : PXCCapture::Device::MirrorMode::MIRROR_MODE_DISABLED;
            if (pp->QueryCaptureManager()->QueryDevice()->QueryMirrorMode() != mirror)
            {
                pp->QueryCaptureManager()->QueryDevice()->SetMirrorMode(mirror);
            }

			/* Resume next frame processing */
            pp->ReleaseFrame();
        }

		if(processing)
		{
			save_depth_value = DEPTH_BEGIN;
			processing = false;
		}


        if (sts2==PXC_STATUS_STREAM_CONFIG_CHANGED) {
            /* profile was changed for adaptive stream */
            SetStatus(hwndDlg, L"Adaptive");
            /* restart SenseManager */
            pp->Close();
            continue;
        }
        break;
    }

    pp->Close();
    if (sts) SetStatus(hwndDlg, L"Stopped");
	pp->Release();
}

