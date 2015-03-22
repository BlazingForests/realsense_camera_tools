/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2013 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#pragma once
#include "pxcprojection.h"
#include "pxccapture.h"
#include "pxcsession.h"

class Projection {
public:

	Projection(PXCSession *session, PXCCapture::Device *device, PXCImage::ImageInfo *dinfo, PXCImage::ImageInfo *cinfo);
	~Projection(void);

    void DepthToColorByUVMAP(PXCImage *color, PXCImage *depth, int dots);
    void DepthToColorByFunction(PXCImage *color, PXCImage *depth, int dots);
	void ColorToDepthByInvUVMAP(PXCImage *color, PXCImage *depth, int dots);

protected:

    PXCProjection	*projection;
    pxcU16			invalid_value; /* invalid depth values */
	PXCPoint3DF32	*dcords;
	PXCPointF32		*ccords;
    PXCPointF32		*uvmap;
	PXCPointF32		*invuvmap;

	void PlotXY(pxcBYTE *cpixels, int xx, int yy, int cwidth, int cheight, int dots, int color);

    /* prohibit using copy & assignment constructors */
    Projection(Projection&) {}
    Projection& operator= (Projection&) { return *this; }
};
