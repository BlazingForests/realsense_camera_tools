/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2011-2014 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#include <windows.h>
#include "pxcsensemanager.h"
#include "pxcmetadata.h"
#include "util_cmdline.h"
#include "util_render.h"
#include <conio.h>

int wmain(int argc, WCHAR* argv[]) {
    /* Creates an instance of the PXCSenseManager */
    PXCSenseManager *pp = PXCSenseManager::CreateInstance();
    if (!pp) {
        wprintf_s(L"Unable to create the SenseManager\n");
        return 3;
    }

    // Optional steps to send feedback to Intel Corporation to understand how often each SDK sample is used.
    PXCMetadata * md = pp->QuerySession()->QueryInstance<PXCMetadata>();
    if(md)
    {
        pxcCHAR sample_name[] = L"Camera Viewer";
        md->AttachBuffer(PXCSessionService::FEEDBACK_SAMPLE_INFO, (pxcBYTE*)sample_name, sizeof(sample_name));
    }

    /* Collects command line arguments */
    UtilCmdLine cmdl(pp->QuerySession());
    if (!cmdl.Parse(L"-listio-nframes-sdname-csize-dsize-isize-file-record-noRender-mirror",argc,argv)) return 3;

    /* Sets file recording or playback */
    PXCCaptureManager *cm=pp->QueryCaptureManager();
    cm->SetFileName(cmdl.m_recordedFile, cmdl.m_bRecord);
    if (cmdl.m_sdname) cm->FilterByDeviceInfo(cmdl.m_sdname,0,0);

    // Create stream renders
    UtilRender renderc(L"Color"), renderd(L"Depth"), renderi(L"IR");
    pxcStatus sts;
    do {
        /* Apply command line arguments */
        if (cmdl.m_csize.size()>0) {
            pp->EnableStream(PXCCapture::STREAM_TYPE_COLOR, cmdl.m_csize.front().first.width, cmdl.m_csize.front().first.height, (pxcF32)cmdl.m_csize.front().second);
        }
        if (cmdl.m_dsize.size()>0) {
            pp->EnableStream(PXCCapture::STREAM_TYPE_DEPTH, cmdl.m_dsize.front().first.width, cmdl.m_dsize.front().first.height, (pxcF32)cmdl.m_dsize.front().second);
        }
        if (cmdl.m_isize.size()>0) {
            pp->EnableStream(PXCCapture::STREAM_TYPE_IR,    cmdl.m_isize.front().first.width, cmdl.m_isize.front().first.height, (pxcF32)cmdl.m_isize.front().second);
        }
        if (cmdl.m_csize.size()==0 && cmdl.m_dsize.size()==0 && cmdl.m_isize.size()==0) {
            PXCVideoModule::DataDesc desc={};
            if (cm->QueryCapture()) {
                cm->QueryCapture()->QueryDeviceInfo(0, &desc.deviceInfo);
            } else {
				desc.deviceInfo.streams = PXCCapture::STREAM_TYPE_COLOR | PXCCapture::STREAM_TYPE_DEPTH | PXCCapture::STREAM_TYPE_IR;
            }
            pp->EnableStreams(&desc);
        }

        /* Initializes the pipeline */
        sts = pp->Init();

        if (sts<PXC_STATUS_NO_ERROR) {
            wprintf_s(L"Failed to locate any video stream(s)\n");
            break;
        }
        if ( cmdl.m_bMirror ) {
            pp->QueryCaptureManager()->QueryDevice()->SetMirrorMode( PXCCapture::Device::MirrorMode::MIRROR_MODE_HORIZONTAL );
        } else {
            pp->QueryCaptureManager()->QueryDevice()->SetMirrorMode( PXCCapture::Device::MirrorMode::MIRROR_MODE_DISABLED );
        }


		//get all info about realsense f200
		PXCCapture::DeviceInfo dev_info;
		pp->QueryCaptureManager()->QueryCapture()->QueryDeviceInfo(0, &dev_info);

		PXCCapture::Device *dev = NULL;
		dev = pp->QueryCaptureManager()->QueryDevice();

		if(!dev)
		{
			wprintf_s(L"Device NULL !!!!!\n");
		}

		if(dev)
		{
			wprintf_s(L"===============================\nColor Stream Properties");

			wprintf_s(L"\nThe color stream exposure, in log base 2 seconds.\n");
			pxcI32 color_exposure = dev->QueryColorExposure();
			wprintf_s(L"%d\n", color_exposure);

			//wprintf_s(L"\nThe color stream exposure property information\n");
			//PXCCapture::Device::PropertyInfo color_exposure_info = dev->QueryColorExposureInfo();

			wprintf_s(L"\nThe color stream brightness from  -10,000 (pure black) to 10,000 (pure white).\n");
			pxcI32 color_brightness = dev->QueryColorBrightness();
			wprintf_s(L"%d\n", color_brightness);
		
			//wprintf_s(L"\nThe color stream brightness property information\n");
			//PXCCapture::Device::PropertyInfo color_brightness_info = dev->QueryColorBrightnessInfo();

			wprintf_s(L"\nThe color stream contrast, from 0 to 10,000.\n");
			pxcI32 color_contrast = dev->QueryColorContrast();
			wprintf_s(L"%d\n", color_contrast);

			//wprintf_s(L"\nThe color stream contrast property information\n");
			//PXCCapture::Device::PropertyInfo color_contrast_info = dev->QueryColorContrastInfo();

			wprintf_s(L"\nThe color stream saturation, from 0 to 10,000.\n");
			pxcI32 color_saturation = dev->QueryColorSaturation();
			wprintf_s(L"%d\n", color_saturation);

			//wprintf_s(L"\nThe color stream saturation property information\n");
			//PXCCapture::Device::PropertyInfo color_saturation_info = dev->QueryColorSaturationInfo();


			wprintf_s(L"\nThe color stream hue, from -180,000 to 180,000 (representing -180 to 180 degrees.)\n");
			pxcI32 color_hue = dev->QueryColorHue();
			wprintf_s(L"%d\n", color_hue);

			//wprintf_s(L"\nThe color stream Hue property information\n");
			//PXCCapture::Device::PropertyInfo color_hue_info = dev->QueryColorHueInfo();


			wprintf_s(L"\nThe color stream gamma, from 1 to 500.\n");
			pxcI32 color_gamma = dev->QueryColorGamma();
			wprintf_s(L"%d\n", color_gamma);

			//wprintf_s(L"\nThe color stream gamma property information\n");
			//PXCCapture::Device::PropertyInfo color_gamma_info = dev->QueryColorGammaInfo();


			wprintf_s(L"\nThe color stream balance, as a color temperature in degrees Kelvin.\n");
			pxcI32 color_white_balance = dev->QueryColorWhiteBalance();
			wprintf_s(L"%d\n", color_white_balance);

			//The color stream  white balance property information
			//PropertyInfo    QueryColorWhiteBalanceInfo()


			wprintf_s(L"\nThe color stream sharpness, from 0 to 100.\n");
			pxcI32 color_sharpness = dev->QueryColorSharpness();
			wprintf_s(L"%d\n", color_sharpness);

			//The color stream  Sharpness property information
			//PropertyInfo    QueryColorSharpnessInfo()


			wprintf_s(L"\nThe color stream back light compensation status from 0 to 4.\n");
			pxcI32 color_backlight_compensation = dev->QueryColorBackLightCompensation();
			wprintf_s(L"%d\n", color_backlight_compensation);

			//The color stream  back light compensation property information
			//PropertyInfo    QueryColorBackLightCompensationInfo()

			wprintf_s(L"\nThe color stream gain adjustment, with negative values darker, positive values brighter, and zero as normal.\n");
			pxcI32 color_gain = dev->QueryColorGain();
			wprintf_s(L"%d\n", color_gain);

			//The color stream  gain property information
			//PropertyInfo    QueryColorGainInfo()


			wprintf_s(L"\nThe power line frequency in Hz.\n");
			PXCCapture::Device::PowerLineFrequency color_powerline_frequency = dev->QueryColorPowerLineFrequency();
			if(color_powerline_frequency == PXCCapture::Device::PowerLineFrequency::POWER_LINE_FREQUENCY_DISABLED)
			{
				wprintf_s(L"POWER_LINE_FREQUENCY_DISABLED\n");
			}
			else if(color_powerline_frequency == PXCCapture::Device::PowerLineFrequency::POWER_LINE_FREQUENCY_50HZ)
			{
				wprintf_s(L"POWER_LINE_FREQUENCY_50HZ\n");
			}
			else if(color_powerline_frequency == PXCCapture::Device::PowerLineFrequency::POWER_LINE_FREQUENCY_60HZ)
			{
				wprintf_s(L"POWER_LINE_FREQUENCY_60HZ\n");
			}

		
     
			//wprintf_s(L"\nThe power line frequency default value.\n");
			//PXCCapture::Device::PowerLineFrequency color_powerline_frequency_default = dev->QueryColorPowerLineFrequencyDefaultValue();
			//wprintf_s(L"%d\n", color_powerline_frequency);


			wprintf_s(L"\nThe color-sensor horizontal and vertical field of view parameters, in degrees. \n");
			PXCPointF32 color_field_of_view = dev->QueryColorFieldOfView();
			wprintf_s(L"x, y = %f, %f\n", color_field_of_view.x, color_field_of_view.y);


			wprintf_s(L"\nThe color-sensor focal length in pixels. The parameters vary with the resolution setting.\n");
			PXCPointF32 color_focal_lenght = dev->QueryColorFocalLength();
			wprintf_s(L"x, y = %f, %f\n", color_focal_lenght.x, color_focal_lenght.y);


			wprintf_s(L"\nThe color-sensor focal length in mm.\n");
			pxcF32 color_focal_lenght_mm = dev->QueryColorFocalLengthMM();
			wprintf_s(L"%f\n", color_focal_lenght_mm);


			wprintf_s(L"\nThe color-sensor principal point in pixels. The parameters vary with the resolution setting.\n");
			PXCPointF32 color_principal_point = dev->QueryColorPrincipalPoint();
			wprintf_s(L"x, y = %f, %f\n", color_principal_point.x, color_principal_point.y);

			

			wprintf_s(L"\n=====================================\nDepth Stream Properties\n");


			wprintf_s(L"\nThe special depth map value to indicate that the corresponding depth map pixel is of low-confidence.\n");
			pxcU16 depth_low_confidence_value = dev->QueryDepthLowConfidenceValue();
			wprintf_s(L"%d\n", depth_low_confidence_value);

			wprintf_s(L"\nThe confidence threshold that is used to floor the depth map values. The range is from 0 to 15.\n");
			pxcI16 depth_confidence_threshold = dev->QueryDepthConfidenceThreshold();
			wprintf_s(L"%d\n", depth_confidence_threshold);

			//The property information for the confidence threshold that is used to floor the depth map values. The range is from 0 to 15.
			//PropertyInfo QueryDepthConfidenceThresholdInfo()

			wprintf_s(L"\nThe unit of depth values in micrometer if PIXEL_FORMAT_DEPTH_RAW\n");
			pxcF32 depth_unit = dev->QueryDepthUnit();
			wprintf_s(L"%f\n", depth_unit);
	
			wprintf_s(L"\nThe depth-sensor horizontal and vertical field of view parameters, in degrees. \n");
			PXCPointF32 depth_field_of_view = dev->QueryDepthFieldOfView();
			wprintf_s(L"x, y = %f, %f\n", depth_field_of_view.x, depth_field_of_view.y);

			wprintf_s(L"\nThe depth-sensor, sensing distance parameters, in millimeters.\n");
			PXCRangeF32 depth_sensor_range = dev->QueryDepthSensorRange();
			wprintf_s(L"min, max = %f, %f\n", depth_sensor_range.min, depth_sensor_range.max);


			wprintf_s(L"\nThe depth-sensor focal length in pixels. The parameters vary with the resolution setting.\n");
			PXCPointF32 depth_focal_lenght = dev->QueryDepthFocalLength();
			wprintf_s(L"x, y = %f, %f\n", depth_focal_lenght.x, depth_focal_lenght.y);

			wprintf_s(L"\nThe depth-sensor focal length in mm.\n");
			pxcF32 depth_focal_lenght_mm = dev->QueryDepthFocalLengthMM();
			wprintf_s(L"%f\n", depth_focal_lenght_mm);


			wprintf_s(L"\nThe depth-sensor principal point in pixels. The parameters vary with the resolution setting.\n");
			PXCPointF32 depth_principal_point = dev->QueryDepthPrincipalPoint();
			wprintf_s(L"x, y = %f, %f\n", depth_principal_point.x, depth_principal_point.y);




			wprintf_s(L"\n==============================================\nDevice Properties\n");


			wprintf_s(L"\nIf true, allow resolution change and throw PXC_STATUS_STREAM_CONFIG_CHANGED.\n");
			pxcBool device_allow_profile_change = dev->QueryDeviceAllowProfileChange();
			wprintf_s(L"%d\n", device_allow_profile_change);

			wprintf_s(L"\nThe mirror mode\n");
			PXCCapture::Device::MirrorMode mirror_mode = dev->QueryMirrorMode();
			if(mirror_mode == PXCCapture::Device::MirrorMode::MIRROR_MODE_DISABLED)
			{
				wprintf_s(L"MIRROR_MODE_DISABLED\n");
			}
			else if(mirror_mode == PXCCapture::Device::MirrorMode::MIRROR_MODE_HORIZONTAL)
			{
				wprintf_s(L"MIRROR_MODE_HORIZONTAL\n");
			}


			wprintf_s(L"\n===============================================\nMisc. Properties\n");
			//PROPERTY_PROJECTION_SERIALIZABLE    =   3003,        /* pxcU32         R    The meta data identifier of the Projection instance serialization data. */


			wprintf_s(L"\n===============================================\nDevice Specific Properties\n");

			wprintf_s(L"\nThe laser power value from 0 (minimum) to 16 (maximum).\n");
			pxcI32 ivcam_laser_power = dev->QueryIVCAMLaserPower();
			wprintf_s(L"%d\n", ivcam_laser_power);

			//The laser power proeprty information. 
			//PropertyInfo QueryIVCAMLaserPowerInfo()
        
			wprintf_s(L"\nThe IVCAM accuracy value\n");
			PXCCapture::Device::IVCAMAccuracy ivcam_accuracy = dev->QueryIVCAMAccuracy();
			if(ivcam_accuracy == PXCCapture::Device::IVCAMAccuracy::IVCAM_ACCURACY_FINEST)
			{
				wprintf_s(L"IVCAM_ACCURACY_FINEST\n");
			}
			else if(ivcam_accuracy == PXCCapture::Device::IVCAMAccuracy::IVCAM_ACCURACY_MEDIAN)
			{
				wprintf_s(L"IVCAM_ACCURACY_MEDIAN\n");
			}
			else if(ivcam_accuracy == PXCCapture::Device::IVCAMAccuracy::IVCAM_ACCURACY_COARSE)
			{
				wprintf_s(L"IVCAM_ACCURACY_COARSE\n");
			}
		

			//The accuracy value property Information
			//IVCAMAccuracy QueryIVCAMAccuracyDefaultValue()

			wprintf_s(L"\nThe filter option (smoothing aggressiveness) ranged from 0 (close range) to 7 (far range).\n");
			pxcI32 ivcam_filter_option = dev->QueryIVCAMFilterOption();
			wprintf_s(L"%d\n", ivcam_filter_option);

			//The IVCAM Filter Option property information. 
			//PropertyInfo QueryIVCAMFilterOptionInfo()

			wprintf_s(L"\nThis option specifies the motion and range trade off. The value ranged from 0 (short exposure, range, and better motion) to 100 (long exposure, range).\n");
			pxcI32 ivcam_motion_range_trade_off = dev->QueryIVCAMMotionRangeTradeOff();
			wprintf_s(L"%d\n", ivcam_motion_range_trade_off);

		
			//The IVCAM Filter Option property information. 
			//PropertyInfo QueryIVCAMMotionRangeTradeOffInfo()
		}


        /* Stream Data */
        for (int nframes=0;nframes<cmdl.m_nframes;nframes++) {
            /* Waits until new frame is available and locks it for application processing */
            sts=pp->AcquireFrame(false);

            if (sts<PXC_STATUS_NO_ERROR) {
                if (sts==PXC_STATUS_STREAM_CONFIG_CHANGED) {
                    wprintf_s(L"Stream configuration was changed, re-initilizing\n");
                    pp->Close();
                }
                break;
            }

            /* Render streams, unless -noRender is selected */
            if (cmdl.m_bNoRender == false) {
                const PXCCapture::Sample *sample = pp->QuerySample();
                if (sample) {
                    if (sample->depth && !renderd.RenderFrame(sample->depth)) break;
                    if (sample->color && !renderc.RenderFrame(sample->color)) break;
                    if (sample->ir    && !renderi.RenderFrame(sample->ir))    break;
                }
            }
            /* Releases lock so pipeline can process next frame */
            pp->ReleaseFrame();

            if( _kbhit() ) { // Break loop
                int c = _getch() & 255;
                if( c == 27 || c == 'q' || c == 'Q') break; // ESC|q|Q for Exit
            }
        }
    } while (sts == PXC_STATUS_STREAM_CONFIG_CHANGED);

    wprintf_s(L"Exiting\n");

    // Clean Up
    pp->Release();
    return 0;
}
