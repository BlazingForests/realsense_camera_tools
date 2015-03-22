/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012-2014 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include <Windows.h>
#include <WindowsX.h>
#include <commctrl.h>
#include <string>
#include <map>
#include "resource1.h"
#include "pxcsensemanager.h"
#include "pxcmetadata.h"
#include "service/pxcsessionservice.h"
#include "main.h"

static const int NPROFILES_MAX=100;
static const int NDEVICES_MAX=100;

static const int IDC_STATUS=10000;
static const int ID_DEVICEX=21000;
static const int ID_STREAM1X=22000;
static const int IDXM_DEVICE=0;
static const int IDXM_MODE=1;
static const int IDXM_SYNC=2;

/* SDK instances */
PXCSession *g_session=0;
PXCCapture *g_capture=0;
PXCCapture::Device *g_device=0;
PXCCaptureManager *g_captureManager=0;

/* Menu */
static std::map<int /*ctrl*/,pxcUID /*iuid*/> g_devices;
static std::map<int /*ctrl*/,HMENU> g_streams;
static HMENU g_mode_menu;
static HMENU g_sync_menu;

/* Panel Bitmap */
static HBITMAP g_bitmaps[2]={0,0};

/* Threading control */
static volatile bool g_running=false;
volatile bool g_stop=true;

/* Open/Save File Name */
pxcCHAR g_file[1024]={0};

/* Controls and Layout */
static const int g_stream_buttons[]={
	IDC_STREAM1, IDC_STREAM2, IDC_STREAM3, IDC_STREAM4, 
	IDC_STREAM5, IDC_STREAM6, IDC_STREAM7, IDC_STREAM8,
};

static const int g_radio_buttons[]={
	IDC_STREAM1, IDC_STREAM2, IDC_STREAM3, IDC_STREAM4, 
	IDC_STREAM5, IDC_STREAM6, IDC_STREAM7, IDC_STREAM8,
	IDC_UVMAP, IDC_INVUVMAP, IDC_PROJECTION 
};

static const int g_controls[]={
	IDC_STREAM1, IDC_STREAM2,  IDC_STREAM3, IDC_STREAM4, 
	IDC_STREAM5, IDC_STREAM6, IDC_STREAM7, IDC_STREAM8, 
	IDC_UVMAP, IDC_INVUVMAP, IDC_PROJECTION, IDC_SCALE, IDC_MIRROR, 
	IDC_DOTS, IDC_PIP, ID_START, ID_STOP 
};

static RECT g_layout[3+sizeof(g_controls)/sizeof(g_controls[0])];

static void SaveLayout(HWND hwndDlg) {
	GetClientRect(hwndDlg,&g_layout[0]);
	ClientToScreen(hwndDlg,(LPPOINT)&g_layout[0].left);
	ClientToScreen(hwndDlg,(LPPOINT)&g_layout[0].right);
	GetWindowRect(GetDlgItem(hwndDlg,IDC_MPANEL),&g_layout[1]);
	GetWindowRect(GetDlgItem(hwndDlg,IDC_STATUS),&g_layout[2]);
	for (int i=0;i<sizeof(g_controls)/sizeof(g_controls[0]);i++)
		GetWindowRect(GetDlgItem(hwndDlg,g_controls[i]),&g_layout[3+i]);
}

static void RedoLayout(HWND hwndDlg) {
	RECT rect;
	GetClientRect(hwndDlg,&rect);

	/* Status */
	SetWindowPos(GetDlgItem(hwndDlg,IDC_STATUS),hwndDlg,
		0,
		rect.bottom-(g_layout[2].bottom-g_layout[2].top),
		rect.right-rect.left,
		(g_layout[2].bottom-g_layout[2].top),SWP_NOZORDER);

	/* Main & PIP Panels */
	int mx=(g_layout[1].left-g_layout[0].left);
	int my=(g_layout[1].top-g_layout[0].top);
	int mw=rect.right-(g_layout[1].left-g_layout[0].left)-(g_layout[0].right-g_layout[1].right);
	int mh=rect.bottom-(g_layout[1].top-g_layout[0].top)-(g_layout[0].bottom-g_layout[1].bottom);
	SetWindowPos(GetDlgItem(hwndDlg,IDC_MPANEL),hwndDlg,mx,my,mw,mh,SWP_NOZORDER);

	/* Buttons & CheckBoxes */
	int offset=0;
	for (int i=0;i<sizeof(g_controls)/sizeof(g_controls[0]);i++) {
		SetWindowPos(GetDlgItem(hwndDlg,g_controls[i]),hwndDlg,
			rect.right-(g_layout[0].right-g_layout[3+i].left),
			(g_layout[3+i].top-g_layout[0].top-offset),
			(g_layout[3+i].right-g_layout[3+i].left),
			(g_layout[3+i].bottom-g_layout[3+i].top),
			SWP_NOZORDER);

		/* If not visible, off the size of the button for the rest controls */
		pxcCHAR line[256];
		GetDlgItemText(hwndDlg, g_controls[i], line, sizeof(line)/sizeof(line[0]));
		if (!wcscmp(line,L"-")) offset+=g_layout[3+i].bottom-g_layout[3+i].top;
	}
}

static std::wstring Profile2String(PXCCapture::Device::StreamProfile *pinfo) {
    pxcCHAR line[256]=L"";
    if (pinfo->frameRate.min && pinfo->frameRate.max && pinfo->frameRate.min != pinfo->frameRate.max) {
        swprintf_s<sizeof(line)/sizeof(pxcCHAR)>(line,L"%s %dx%dx%d-%d",
            PXCImage::PixelFormatToString(pinfo->imageInfo.format),pinfo->imageInfo.width,pinfo->imageInfo.height,
            (int)pinfo->frameRate.min, (int)pinfo->frameRate.max);
    } else {
        pxcF32 frameRate=pinfo->frameRate.min?pinfo->frameRate.min:pinfo->frameRate.max;
        swprintf_s<sizeof(line)/sizeof(pxcCHAR)>(line,L"%s %dx%dx%d",
            PXCImage::PixelFormatToString(pinfo->imageInfo.format),pinfo->imageInfo.width,pinfo->imageInfo.height,
            (int)frameRate);
    }
	return std::wstring(line);
}

static void ReleaseDevice(void) {
	if (g_device) g_device->Release();
	if (g_capture && !g_captureManager) g_capture->Release();
	g_device=0;
	g_capture=0;
}

static void ReleaseDeviceAndCaptureManager(void) {
	ReleaseDevice();
	if (g_captureManager) 
    {
        g_captureManager->CloseStreams();
        g_captureManager->Release();
    }
	g_captureManager=0;
}

static void SetDevice(HWND hwndDlg, int ctrl) {
	if (!g_captureManager) {
		ReleaseDevice();
		if (g_session->CreateImpl<PXCCapture>(g_devices[ctrl], &g_capture)<PXC_STATUS_NO_ERROR) return;
	}
	g_device=g_capture->CreateDevice((ctrl-ID_DEVICEX)%NDEVICES_MAX);
	if (!g_device) ReleaseDevice();
}

static void PopulateDevices(HWND hwndDlg) {
    SetStatus(hwndDlg,L"Scanning...");
	ReleaseDeviceAndCaptureManager();
    g_devices.clear();

	PXCSession::ImplDesc mdesc={};
	mdesc.group=PXCSession::IMPL_GROUP_SENSOR;
	mdesc.subgroup=PXCSession::IMPL_SUBGROUP_VIDEO_CAPTURE;

	HMENU menu1=CreatePopupMenu();
	for (int m=0,iitem=0;;m++) {
		PXCSession::ImplDesc desc1;
		if (g_session->QueryImpl(&mdesc,m,&desc1)<PXC_STATUS_NO_ERROR) break;

		PXCCapture* capture=0;
		if (g_session->CreateImpl<PXCCapture>(&desc1,&capture)<PXC_STATUS_NO_ERROR) continue;

		for (int j=0;;j++) {
			PXCCapture::DeviceInfo dinfo;
            if (capture->QueryDeviceInfo(j,&dinfo)<PXC_STATUS_NO_ERROR) break;

			int id=ID_DEVICEX+(iitem++)*NDEVICES_MAX+j;
            g_devices[id]=desc1.iuid;
			AppendMenu(menu1,MF_STRING,id,dinfo.name);
		}
		capture->Release();
	}
	SetStatus(hwndDlg, L"OK");

    HMENU menu=GetMenu(hwndDlg);
	DeleteMenu(menu,IDXM_DEVICE,MF_BYPOSITION);
	CheckMenuRadioItem(menu1,0,GetMenuItemCount(menu1),0,MF_BYPOSITION);
	InsertMenu(menu,IDXM_DEVICE,MF_BYPOSITION|MF_POPUP,(UINT_PTR)menu1,L"Device");
	SetDevice(hwndDlg, ID_DEVICEX);
}

static bool PopulateDeviceFromFile(HWND hwndDlg) {
	if (!g_file[0]) return false;
    SetStatus(hwndDlg,L"Scanning...");

	ReleaseDeviceAndCaptureManager();
    g_devices.clear();

	g_captureManager=g_session->CreateCaptureManager();
	if (!g_captureManager) {
		SetStatus(hwndDlg,L"Failed to create CaptureManager");
		return false;
	}

	if (g_captureManager->SetFileName(g_file,false)<PXC_STATUS_NO_ERROR) {
		SetStatus(hwndDlg,L"Failed to open file");
		ReleaseDeviceAndCaptureManager();
		return false;
	}

	SetStatus(hwndDlg, g_file);
	g_capture=g_captureManager->QueryCapture();
	HMENU menu1=CreatePopupMenu();
	for (int d=0,iitem=0;;d++) {
		PXCCapture::DeviceInfo dinfo;
		if (g_capture->QueryDeviceInfo(d,&dinfo)<PXC_STATUS_NO_ERROR) break;
		int id=ID_DEVICEX+(iitem++)*NDEVICES_MAX+d;
		g_devices[id]=0;
		AppendMenu(menu1,MF_STRING,id,dinfo.name);
	}

	HMENU menu=GetMenu(hwndDlg);
	DeleteMenu(menu,IDXM_DEVICE,MF_BYPOSITION);
	CheckMenuRadioItem(menu1,0,GetMenuItemCount(menu1),0,MF_BYPOSITION);
	InsertMenu(menu,IDXM_DEVICE,MF_BYPOSITION|MF_POPUP,(UINT_PTR)menu1,L"Device");
	SetDevice(hwndDlg, ID_DEVICEX);
	return true;
}

/* Set radio buttons: Uncheck all others */
static void ButtonCheck(HWND hwndDlg, int ctrl) {
	for (int i=0;i<(int)sizeof(g_radio_buttons)/sizeof(g_radio_buttons[0]);i++)
		CheckDlgButton(hwndDlg,g_radio_buttons[i],g_radio_buttons[i]==ctrl?BST_CHECKED:BST_UNCHECKED);
}

static bool ButtonEnable(HWND hwndDlg, int ctrl, bool enable) {
	return Button_Enable(GetDlgItem(hwndDlg,ctrl), enable?TRUE:FALSE)!=0;
}

bool ButtonIsEnabled(HWND hwndDlg, int ctrl) {
	return IsWindowEnabled(GetDlgItem(hwndDlg,ctrl))!=0;
}

bool ButtonIsChecked(HWND hwndDlg, int ctrl) {
	return Button_GetCheck(GetDlgItem(hwndDlg,ctrl))==BST_CHECKED;
}

bool MenuIsChecked(HMENU menu, int mctl) {
	return (GetMenuState(menu,mctl,MF_BYCOMMAND)&MF_CHECKED)!=0;
}

bool MenuIsChecked(HWND hwndDlg, int midx, int mctl) {
	return MenuIsChecked(GetSubMenu(GetMenu(hwndDlg),midx),mctl);
}

bool MenuIsEnabled(HWND hwndDlg, int midx, int mctl) {
	return !(GetMenuState(GetSubMenu(GetMenu(hwndDlg),midx),mctl,MF_BYCOMMAND)&MF_DISABLED);
}

static void MenuRadioCheck(HMENU menu, int iitem) {
	CheckMenuRadioItem(menu,0,GetMenuItemCount(menu)-1,iitem,MF_BYPOSITION);
}

static void MenuRadioCheck(HWND hwndDlg, int midx, int iitem) {
	MenuRadioCheck(GetSubMenu(GetMenu(hwndDlg), midx),iitem);
}

static void MenuEnable(HWND hwndDlg, int midx, int mctl, bool enable) {
	EnableMenuItem(GetSubMenu(GetMenu(hwndDlg),midx),mctl,MF_BYCOMMAND|(enable?MF_ENABLED:MF_DISABLED));
}

bool IsMirrorEnabled(HWND hwndDlg)
{
    return (Button_GetCheck(GetDlgItem(hwndDlg,IDC_MIRROR))==BST_CHECKED);
}

PXCCapture::Device::StreamProfileSet GetProfileSet(HWND hwndDlg) {
	PXCCapture::Device::StreamProfileSet profiles={};
	if (!g_device) return profiles;

	PXCCapture::DeviceInfo dinfo;
	g_device->QueryDeviceInfo(&dinfo);
	for (int s=0, mi=IDXM_DEVICE+1;s<PXCCapture::STREAM_LIMIT;s++) {
		PXCCapture::StreamType st=PXCCapture::StreamTypeFromIndex(s);
		if (!(dinfo.streams&st)) continue;
		
		int id=ID_STREAM1X+s*NPROFILES_MAX;
		int nprofiles=g_device->QueryStreamProfileSetNum(st);
		for (int p=0;p<nprofiles;p++) {
			if (!MenuIsChecked(hwndDlg, mi, id+p)) continue;
			PXCCapture::Device::StreamProfileSet profiles1={};
			g_device->QueryStreamProfileSet(st, p, &profiles1);
			profiles[st]=profiles1[st];
		}
		mi++;
	}
	return profiles;
}

static void CheckSelection(HWND hwndDlg) {
	if (!g_device) return;
	PXCCapture::Device::StreamProfileSet given=GetProfileSet(hwndDlg);
	PXCCapture::DeviceInfo dinfo;
	g_device->QueryDeviceInfo(&dinfo);
	bool recheck=false;
	for (int s=0, mi=IDXM_DEVICE+1;s<PXCCapture::STREAM_LIMIT;s++) {
		PXCCapture::StreamType st=PXCCapture::StreamTypeFromIndex(s);
		if (!(dinfo.streams&st)) continue;

		int id=ID_STREAM1X+s*NPROFILES_MAX;
		int nprofiles=g_device->QueryStreamProfileSetNum(st);
		PXCCapture::Device::StreamProfileSet test=given;
		for (int p=0;p<nprofiles;p++) {
			PXCCapture::Device::StreamProfileSet profiles1={};
			g_device->QueryStreamProfileSet(st, p, &profiles1);
			test[st]=profiles1[st];
			MenuEnable(hwndDlg, mi, id+p, g_device->IsStreamProfileSetValid(&test)!=0);
		}

		bool enable=given[st].imageInfo.format!=0;
		ButtonEnable(hwndDlg, g_stream_buttons[s], enable);
		if (ButtonIsChecked(hwndDlg, g_stream_buttons[s]) && !enable)
			recheck=true; // if a checked radio button is disabled, display another active stream.
		mi++;
	}

	if (recheck) {
		for (int s=0, mi=IDXM_DEVICE+1;s<PXCCapture::STREAM_LIMIT;s++) {
			if (!ButtonIsEnabled(hwndDlg, g_stream_buttons[s])) continue;
			ButtonCheck(hwndDlg, g_stream_buttons[s]);
			break;
		}
	}

	bool uvmapping=given.color.imageInfo.format && given.depth.imageInfo.format && MenuIsChecked(g_sync_menu,ID_SYNC_SYNC);
	ButtonEnable(hwndDlg, IDC_UVMAP, uvmapping);
	ButtonEnable(hwndDlg, IDC_INVUVMAP, uvmapping);
	ButtonEnable(hwndDlg, IDC_PROJECTION, uvmapping);
	ButtonEnable(hwndDlg, IDC_DOTS, uvmapping);
    if ( (ButtonIsChecked(hwndDlg, IDC_UVMAP) || ButtonIsChecked(hwndDlg, IDC_INVUVMAP) || ButtonIsChecked(hwndDlg, IDC_PROJECTION)) && !uvmapping)
    {
        for (int s=0, mi=IDXM_DEVICE+1;s<PXCCapture::STREAM_LIMIT;s++) {
            if (!ButtonIsEnabled(hwndDlg, g_stream_buttons[s])) continue;
            ButtonCheck(hwndDlg, g_stream_buttons[s]);
            break;
        }
    }
}

PXCCapture::DeviceInfo GetCheckedDevice(HWND hwndDlg) {
	PXCCapture::DeviceInfo dinfo={};
	if (g_device) g_device->QueryDeviceInfo(&dinfo);
	return dinfo;
}

void SetStatus(HWND hwndDlg, pxcCHAR *line) {
	HWND hwndStatus=GetDlgItem(hwndDlg,IDC_STATUS);
    if (hwndStatus) SetWindowText(hwndStatus,line);
}

static void PopulateStreams(HWND hwndDlg) {
    HMENU menu=GetMenu(hwndDlg);

	/* Remove all stream menus */
	for (;;) {
		pxcCHAR line[256];
		GetMenuString(menu, IDXM_DEVICE+1, line, sizeof(line)/sizeof(line[0]), MF_BYPOSITION);
		if (!wcscmp(line,L"Mode")) break;
		DeleteMenu(menu,IDXM_DEVICE+1,MF_BYPOSITION);
	}

	/* Hide all stream buttons */
	for (int i=0;i<PXCCapture::STREAM_LIMIT;i++) {
		ShowWindow(GetDlgItem(hwndDlg, g_stream_buttons[i]), SW_HIDE);
		SetWindowText(GetDlgItem(hwndDlg, g_stream_buttons[i]), L"-");
	}

	/* Clean up */
	g_streams.clear();

	if (g_device) {
		PXCCapture::DeviceInfo dinfo;
		g_device->QueryDeviceInfo(&dinfo);

		int last=0;
		for (int s=PXCCapture::STREAM_LIMIT-1;s>=0;s--) {
			PXCCapture::StreamType st=PXCCapture::StreamTypeFromIndex(s);
			if (!(dinfo.streams&st)) continue;
			const pxcCHAR *name=PXCCapture::StreamTypeToString(st);

			/* Create a profile menu */
			HMENU smenu=CreatePopupMenu();
			int id=ID_STREAM1X+s*NPROFILES_MAX;
			int nprofiles=g_device->QueryStreamProfileSetNum(st);
			for (int p=0;p<nprofiles;p++) {
				PXCCapture::Device::StreamProfileSet profiles={};
				if (g_device->QueryStreamProfileSet(st, p, &profiles) < PXC_STATUS_NO_ERROR) break;

				PXCCapture::Device::StreamProfile &profile=profiles[st];
                AppendMenu(smenu,MF_STRING,id+p,Profile2String(&profile).c_str());
				g_streams[id+p]=smenu;
				last=id+p;
			}

			/* Add a NONE choice and Insert the submenu */
		    AppendMenu(smenu,MF_STRING,id+nprofiles,L"None");
			g_streams[id+nprofiles]=smenu;
			CheckMenuRadioItem(smenu, id, id+nprofiles, id+nprofiles, MF_CHECKED);
			InsertMenu(menu, IDXM_DEVICE+1, MF_BYPOSITION|MF_POPUP, (UINT_PTR)smenu,name);

			/* Set/Unset the stream buttons */
			HWND hButton=GetDlgItem(hwndDlg, g_stream_buttons[s]);
			ShowWindow(hButton, SW_SHOW);
			SetWindowText(hButton, name);
			CheckDlgButton(hwndDlg, g_stream_buttons[s], BST_UNCHECKED);
		}
		/* Pre-select the first stream. */
		if (last) {
			/* The first profile of the last enumerated stream: Note we enumerated in the reverse order. */
			int id=ID_STREAM1X+((last-ID_STREAM1X)/NPROFILES_MAX)*NPROFILES_MAX; 
			PostMessage(hwndDlg, WM_COMMAND, id, 0);
		}
    }

	/* Redraw menu and buttons */
	DrawMenuBar(hwndDlg);
	RedoLayout(hwndDlg);
	InvalidateRect(hwndDlg, 0, TRUE);
}

static void DrawBitmap(HWND hwndDlg, PXCImage *image, int index) {
    if (g_bitmaps[index]) {
        DeleteObject(g_bitmaps[index]);
		g_bitmaps[index]=0;
    }
    PXCImage::ImageInfo info=image->QueryInfo();
    PXCImage::ImageData data;
    if (image->AcquireAccess(PXCImage::ACCESS_READ,PXCImage::PIXEL_FORMAT_RGB32, &data)>=PXC_STATUS_NO_ERROR) {
        HWND hwndPanel=GetDlgItem(hwndDlg,IDC_MPANEL);
        HDC dc=GetDC(hwndPanel);
		BITMAPINFO binfo;
		memset(&binfo,0,sizeof(binfo));
		binfo.bmiHeader.biWidth= data.pitches[0]/4;
		binfo.bmiHeader.biHeight= - (int)info.height;
		binfo.bmiHeader.biBitCount=32;
		binfo.bmiHeader.biPlanes=1;
		binfo.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
		binfo.bmiHeader.biCompression=BI_RGB;
        g_bitmaps[index]=CreateDIBitmap(dc, &binfo.bmiHeader, CBM_INIT, data.planes[0], &binfo, DIB_RGB_COLORS);
        ReleaseDC(hwndPanel, dc);
		image->ReleaseAccess(&data);
    }
    else
    {
        g_stop = true;
    }
}

static RECT GetResizeRect(RECT rc, BITMAP bm) { /* Keep the aspect ratio */
	RECT rc1;
	float sx=(float)rc.right/(float)bm.bmWidth;
	float sy=(float)rc.bottom/(float)bm.bmHeight;
	float sxy=sx<sy?sx:sy;
	rc1.right=(int)(bm.bmWidth*sxy);
	rc1.left=(rc.right-rc1.right)/2+rc.left;
	rc1.bottom=(int)(bm.bmHeight*sxy);
	rc1.top=(rc.bottom-rc1.bottom)/2+rc.top;
	return rc1;
}

static void UpdatePanel(HWND hwndDlg) {
	if (!g_bitmaps[0]) return;

	HWND panel=GetDlgItem(hwndDlg,IDC_MPANEL);
	RECT rc;
	GetClientRect(panel,&rc);
    //if (rc.right - rc.left < 40) return;

	HDC dc=GetDC(panel);
	HBITMAP bitmap=CreateCompatibleBitmap(dc,rc.right,rc.bottom);
	HDC dc2=CreateCompatibleDC(dc);
	SelectObject(dc2,bitmap);
	FillRect(dc2,&rc,(HBRUSH)GetStockObject(GRAY_BRUSH));
	SetStretchBltMode(dc2, HALFTONE);

	/* Draw the main window */
	HDC dc3=CreateCompatibleDC(dc);
	SelectObject(dc3,g_bitmaps[0]);
	BITMAP bm;
	GetObject(g_bitmaps[0],sizeof(BITMAP),&bm);

    bool scale=ButtonIsChecked(hwndDlg,IDC_SCALE);
    if (scale) {
        RECT rc1=GetResizeRect(rc,bm);
        StretchBlt(dc2,rc1.left,rc1.top,rc1.right,rc1.bottom,dc3,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
    } else {
        BitBlt(dc2,0,0,rc.right,rc.bottom,dc3,0,0,SRCCOPY);
    }

	/* Draw the PIP window */
	while (g_bitmaps[1]) {
		LRESULT dots=Button_GetState(GetDlgItem(hwndDlg,IDC_PIP));
		RECT rc2=rc;
		if (dots&BST_CHECKED) {
			rc2.right=rc.right/2; rc2.bottom=rc.bottom/2;
		} else if (dots&BST_INDETERMINATE) {
			rc2.right=rc.right/4; rc2.bottom=rc.bottom/4;
		} else break; /* no need to draw PIP */

		SelectObject(dc3,g_bitmaps[1]);
		GetObject(g_bitmaps[1],sizeof(BITMAP),&bm);

		RECT rc1=GetResizeRect(rc2,bm);
		StretchBlt(dc2,rc.right-rc1.right,rc.bottom-rc1.bottom,rc1.right,rc1.bottom,dc3,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);
		break;
	}
	DeleteObject(dc3);
	DeleteObject(dc2);
	ReleaseDC(hwndDlg,dc);

	HBITMAP bitmap2=(HBITMAP)SendMessage(panel,STM_GETIMAGE,0,0);
	if (bitmap2) DeleteObject(bitmap2);
	SendMessage(panel,STM_SETIMAGE,IMAGE_BITMAP,(LPARAM)bitmap);
	InvalidateRect(panel,0,TRUE);
}

struct DisplayQueue { 
    PXCImage *image; 
    int index; 
};

const int           g_display_queue_size=4;
CRITICAL_SECTION    g_display_cs;
HANDLE              g_display_update;
DisplayQueue        g_display_queue[g_display_queue_size];

void QueueBitmap(PXCImage *image, int index) {
    image->AddRef();

    EnterCriticalSection(&g_display_cs);
    if (g_display_queue[0].image) g_display_queue[0].image->Release();
    for (pxcI32 i=1;i<g_display_queue_size;i++)
        g_display_queue[i-1]=g_display_queue[i];
    g_display_queue[g_display_queue_size-1].image=image;
    g_display_queue[g_display_queue_size-1].index=index;
    LeaveCriticalSection(&g_display_cs);
}

void QueueUpdate(void) {
    SetEvent(g_display_update);
}

static DWORD WINAPI DisplayProc(LPVOID arg) {
    while (!g_stop) {
        DisplayQueue display_queue[g_display_queue_size];

        WaitForSingleObject(g_display_update,INFINITE);
        EnterCriticalSection(&g_display_cs);
        for (int i=0;i<g_display_queue_size;i++) {
            display_queue[i]=g_display_queue[i];
            g_display_queue[i].image=NULL;
        }
        LeaveCriticalSection(&g_display_cs);

        for (int i=0;i<g_display_queue_size;i++) {
            if (!display_queue[i].image) continue;
            DrawBitmap((HWND)arg,display_queue[i].image, display_queue[i].index);
            display_queue[i].image->Release();
        }
        UpdatePanel((HWND)arg);
    }
    return 0;
}

static DWORD WINAPI ThreadProc(LPVOID arg) {
    InitializeCriticalSection(&g_display_cs);
    g_display_update=CreateEvent(0,FALSE,FALSE,0);
    for (int i=0;i<g_display_queue_size;i++)
        g_display_queue[i].image=0;
    HANDLE thread=CreateThread(0,0,DisplayProc,arg,0,0);

	HWND hwndDlg=(HWND)arg;
	PXCCapture::DeviceInfo dinfo=GetCheckedDevice(hwndDlg);
	PXCCapture::Device::StreamProfileSet profiles=GetProfileSet(hwndDlg);
	StreamSamples((HWND)arg, 
		&dinfo,
		&profiles,
		MenuIsChecked(g_mode_menu,ID_MODE_ADAPT),
		MenuIsChecked(g_sync_menu,ID_SYNC_SYNC),
		MenuIsChecked(g_mode_menu,ID_MODE_RECORD),
		MenuIsChecked(g_mode_menu,ID_MODE_PLAYBACK),
		g_file
		);

    g_stop=true;
    QueueUpdate();
    WaitForSingleObject(thread,INFINITE);
    for (int i=0;i<g_display_queue_size;i++) 
        if (g_display_queue[i].image) g_display_queue[i].image->Release();
    CloseHandle(g_display_update);
    DeleteCriticalSection(&g_display_cs);

	PostMessage((HWND)arg,WM_COMMAND,ID_STOP,0);
	g_running=false;
	return 0;
}

static void GetPlaybackFile(void) {
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.lpstrFilter=L"RSSDK clip (*.rssdk)\0*.rssdk\0Old format clip (*.pcsdk)\0*.pcsdk\0All Files (*.*)\0*.*\0\0";
	ofn.lpstrFile=g_file; g_file[0]=0;
	ofn.nMaxFile=sizeof(g_file)/sizeof(pxcCHAR);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
	if (!GetOpenFileName(&ofn)) g_file[0]=0;
}

static void GetRecordFile(void) {
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.lpstrFilter=L"RSSDK clip (*.rssdk)\0*.rssdk\0All Files (*.*)\0*.*\0\0";
	ofn.lpstrFile=g_file; g_file[0]=0;
	ofn.nMaxFile=sizeof(g_file)/sizeof(pxcCHAR);
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_EXPLORER;
	if (GetSaveFileName(&ofn)) {
        if (ofn.nFilterIndex==1 && ofn.nFileExtension==0) {
            int len = wcslen(g_file);
            if (len>1 && len<sizeof(g_file)/sizeof(pxcCHAR)-7) {
                wcscpy_s(&g_file[len], rsize_t(7), L".rssdk\0");
            }
        }
    } else g_file[0] = 0;
}

static void DisableAllMenus(HWND hwndDlg) {
	HMENU menu=GetMenu(hwndDlg);
	for (int i=0;i<GetMenuItemCount(menu);i++)
		EnableMenuItem(menu,i,MF_BYPOSITION|MF_GRAYED);
	DrawMenuBar(hwndDlg);
}

static void EnableAllMenus(HWND hwndDlg) {
	HMENU menu=GetMenu(hwndDlg);
	for (int i=0;i<GetMenuItemCount(menu);i++)
		EnableMenuItem(menu,i,MF_BYPOSITION|MF_ENABLED);
	DrawMenuBar(hwndDlg);
}

void GetUserSEL(HWND hwndDlg, UserSEL &ss) {
	ss.isUVMap=ButtonIsChecked(hwndDlg,IDC_UVMAP);
	ss.isProjection=ButtonIsChecked(hwndDlg,IDC_PROJECTION);
	ss.isInvUVMap=ButtonIsChecked(hwndDlg,IDC_INVUVMAP);
	ss.dots=1;
	LRESULT dots=Button_GetState(GetDlgItem(hwndDlg,IDC_DOTS));
	if (dots&BST_CHECKED) ss.dots=9;
	if (dots&BST_INDETERMINATE) ss.dots=5;
	for (int s=0;s<sizeof(g_stream_buttons)/sizeof(g_stream_buttons[0]);s++) {
		if (!ButtonIsChecked(hwndDlg, g_stream_buttons[s])) continue; 
		PXCCapture::StreamType stream=PXCCapture::StreamTypeFromIndex(s);
		if (ss.first!=stream)
        {
		    ss.second=ss.first;
		    ss.first=stream;
        }
        /* If PIP is selected and second stream was not initialized, use first enabled stream. */
        if(ss.second == PXCCapture::STREAM_TYPE_ANY && ButtonIsChecked(hwndDlg, IDC_PIP))
        {
            for (int s1=0;s<sizeof(g_stream_buttons)/sizeof(g_stream_buttons[0]);s1++) {
                if(!ButtonIsChecked(hwndDlg, g_stream_buttons[s1]) && ButtonIsEnabled(hwndDlg, g_stream_buttons[s1]))
                {
                    ss.second = PXCCapture::StreamTypeFromIndex(s1);
                    break;
                }
            }
        }
		break;
	}
}

static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM) { 
	HMENU menu=GetMenu(hwndDlg);
	HMENU menu1;
	DWORD wParam2;

    switch (message) { 
    case WM_INITDIALOG:
		SaveLayout(hwndDlg);
		menu1=GetMenu(hwndDlg);
		g_mode_menu=GetSubMenu(menu1,IDXM_MODE);
		g_sync_menu=GetSubMenu(menu1,IDXM_SYNC);
        MenuRadioCheck(g_sync_menu,0);
		CheckDlgButton(hwndDlg,IDC_MIRROR,BST_CHECKED);
		CheckDlgButton(hwndDlg,IDC_SCALE,BST_CHECKED);
		PopulateDevices(hwndDlg);
        PopulateStreams(hwndDlg);
        CheckSelection(hwndDlg);
        return TRUE; 
    case WM_COMMAND: 
		/* Check if any of the device menu is pressed */
		menu1=GetSubMenu(menu,IDXM_DEVICE);
		wParam2=LOWORD(wParam);
		if (wParam2>=ID_DEVICEX && g_devices.find(wParam2)!=g_devices.end()) {
			MenuRadioCheck(hwndDlg,IDXM_DEVICE, (wParam2-ID_DEVICEX)/NDEVICES_MAX);
			SetDevice(hwndDlg, wParam2);
            PopulateStreams(hwndDlg);
            CheckSelection(hwndDlg);
			return TRUE;
		}

		/* Check if any of the stream profile menu is pressed */
		if (wParam2>=ID_STREAM1X && g_streams.find(wParam2)!=g_streams.end()) {
		    MenuRadioCheck(g_streams[wParam2],(wParam2-ID_STREAM1X)%NPROFILES_MAX);
			ButtonCheck(hwndDlg, g_stream_buttons[(wParam2-ID_STREAM1X)/NPROFILES_MAX]);
            CheckSelection(hwndDlg);
			return TRUE;
		}

		/* Additional selections */
        switch (wParam2) {
        case IDCANCEL:
			g_stop=true;
			if (g_running) {
				PostMessage(hwndDlg,WM_COMMAND,IDCANCEL,0);
			} else {
				DestroyWindow(hwndDlg); 
				PostQuitMessage(0);
			}
            return TRUE;
		case ID_SYNC_SYNC:
			MenuRadioCheck(g_sync_menu,0);
			CheckSelection(hwndDlg);
			break;
		case ID_SYNC_NOSYNC:
			MenuRadioCheck(g_sync_menu,1);
			CheckSelection(hwndDlg);
			break;
		case ID_START:
			Button_Enable(GetDlgItem(hwndDlg,ID_START),false);
			Button_Enable(GetDlgItem(hwndDlg,ID_STOP),true);
            CheckDlgButton(hwndDlg,IDC_PIP,MF_UNCHECKED);
			DisableAllMenus(hwndDlg);
			g_stop=false;
			g_running=true;
			CreateThread(0,0,ThreadProc,hwndDlg,0,0);
			Sleep(0);
			return TRUE;
		case ID_STOP:
			g_stop=true;
			if (g_running) {
				PostMessage(hwndDlg,WM_COMMAND,ID_STOP,0);
			} else {
				EnableAllMenus(hwndDlg);
				Button_Enable(GetDlgItem(hwndDlg,ID_START),true);
				Button_Enable(GetDlgItem(hwndDlg,ID_STOP),false);
			}
			return TRUE;
		case ID_MODE_LIVE:
			MenuRadioCheck(g_mode_menu,0);
	        PopulateDevices(hwndDlg);
            PopulateStreams(hwndDlg);
            g_file[0]=L'';
			CheckSelection(hwndDlg);
			return TRUE;
		case ID_MODE_ADAPT:
			MenuRadioCheck(g_mode_menu,1);
            PopulateDevices(hwndDlg);
            if (g_device) g_device->SetDeviceAllowProfileChange(TRUE);
            PopulateStreams(hwndDlg);
            g_file[0]=L'';
			CheckSelection(hwndDlg);
			return TRUE;
		case ID_MODE_PLAYBACK:
			GetPlaybackFile();
            if (PopulateDeviceFromFile(hwndDlg)) {
			    MenuRadioCheck(g_mode_menu,2);
			} else {
			    MenuRadioCheck(g_mode_menu,0);
		        PopulateDevices(hwndDlg);
            }
            PopulateStreams(hwndDlg);
			CheckSelection(hwndDlg);
			return TRUE;
		case ID_MODE_RECORD:
			GetRecordFile();
            if (g_file[0]) {
			    MenuRadioCheck(g_mode_menu,3);
            } else {
			    MenuRadioCheck(g_mode_menu,0);
            }
            PopulateDevices(hwndDlg);
            PopulateStreams(hwndDlg);
			CheckSelection(hwndDlg);
			return TRUE;
		}

		/* If radio button is pressed, check it */
		for (int i=0;i<sizeof(g_radio_buttons)/sizeof(g_radio_buttons[0]);i++)
			if (wParam2==g_radio_buttons[i]) ButtonCheck(hwndDlg, g_radio_buttons[i]);
		
		switch (HIWORD(wParam)) {
		case BN_CLICKED:
			if (wParam2==IDC_PIP) RedoLayout(hwndDlg);
			break;
		}
		break;
	case WM_SIZE:
		RedoLayout(hwndDlg);
		return TRUE;
    } 
    return FALSE; 
} 

#pragma warning(disable:4706) /* assignment within conditional */
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR, int) {
	InitCommonControls();

	g_session=PXCSession::CreateInstance();
	if (!g_session) {
        MessageBoxW(0,L"Failed to create an SDK session",L"Raw Streams",MB_ICONEXCLAMATION|MB_OK);
        return 1;
    }

    /* Optional steps to send feedback to Intel Corporation to understand how often each SDK sample is used. */
    PXCMetadata * md = g_session->QueryInstance<PXCMetadata>();
    if(md)
    {
        pxcCHAR sample_name[] = L"Raw Streams";
        md->AttachBuffer(PXCSessionService::FEEDBACK_SAMPLE_INFO, (pxcBYTE*)sample_name, sizeof(sample_name));
    }

    HWND hWnd=CreateDialogW(hInstance,MAKEINTRESOURCE(IDD_MAINFRAME),0,DialogProc);
    if (!hWnd)  {
        MessageBoxW(0,L"Failed to create a window",L"Raw Streams",MB_ICONEXCLAMATION|MB_OK);
        return 1;
    }

	HWND hWnd2=CreateStatusWindow(WS_CHILD|WS_VISIBLE,L"OK",hWnd,IDC_STATUS);
	if (!hWnd2) {
        MessageBoxW(0,L"Failed to create a status bar",L"Raw Streams",MB_ICONEXCLAMATION|MB_OK);
        return 1;
	}

	UpdateWindow(hWnd);

    MSG msg;
	for (int sts;(sts=GetMessageW(&msg,NULL,0,0));) {
        if (sts == -1) return sts;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

	g_stop=true;
	while (g_running) Sleep(5);
	ReleaseDeviceAndCaptureManager();
	g_session->Release();
    return (int)msg.wParam;
}


