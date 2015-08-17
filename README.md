# realsense_camera_tools
win tools for use realsense camera in ROS

Those tools use for get realsense camera properties and RGB to depth UVmap data

## Dependencies

[realsense camera SDK](https://software.intel.com/en-us/intel-realsense-sdk/download)

This was tested with with F200 Depth Camera Manager (DCM) v1.2 and Realsense SDK v4.0.0, although newer versions may also work.

### Installation instructions

1. Download [F200 Depth Camera Manager (DCM) v1.2](http://registrationcenter-download.intel.com/akdlm/irc_nas/5105/intel_rs_dcm_f200_1.2.14.24922.exe). Note that newer versions may also work.
2. For the SDK, you'll need to register [here](https://registrationcenter.intel.com/en/forms/?productid=2383). After this you can install either the newest version of the SDK or v4.0.0 for which this was tested.
3. You'll also need to compile the code. Easiest is to install Microsoft Visual Studio 2010 or similar. You can then open the VC++ Project files.

## Run

### DF_CameraViewer
use to get camera properties

### DF_RawStreams
use to get RGB to depth UVmap data


## Q&A

maybe screenshot can help you.

### RGB to depth misalignment

First, I suggest that you to update realsense's firmware.

Then, do the following steps!


![uvmap_step1](https://github.com/BlazingForests/realsense_camera_tools/raw/master/screenshot/uvmap_step1.png)

![uvmap_step2](https://github.com/BlazingForests/realsense_camera_tools/raw/master/screenshot/uvmap_step2.png)

![uvmap_step3](https://github.com/BlazingForests/realsense_camera_tools/raw/master/screenshot/uvmap_step3.png)

![uvmap_step4](https://github.com/BlazingForests/realsense_camera_tools/raw/master/screenshot/uvmap_step4.png)

