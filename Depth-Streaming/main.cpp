#include <stdio.h>
#include <iostream>
#include <opencv/cv.hpp>        //you may need to
#include <opencv2/highgui/highgui.hpp>  //adjust import locations
#include <opencv2/core/core.hpp>     //depending on your machine setup
#include <opencv2/imgproc.hpp>

// include OpenCV header file
#include <opencv2/opencv.hpp>

// include the librealsense C++ header file
#include <librealsense2/rs.hpp>


using namespace cv;           //make available OpenCV namespace

int main(int argc, char * argv[]) try
{
    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

    //Contruct a pipeline which abstracts the device
    rs2::pipeline pipe;

    //Create a configuration for configuring the pipeline with a non default profile
    rs2::config cfg;

    //image dimensions 424x240; 480x270; 640x360; 640x480; 848x480; 1280x720; Not valid for depth 1920x1080;
    const size_t width = 1280;
    const size_t height = 720;

    //Frame rate (FPS) = 6, 15, 25, 30, 60, 90, 100. However the most common 6, 15, 30.
    const size_t FPS = 30;

    //Add desired streams to configuration
    //cfg.enable_stream(RS2_STREAM_INFRARED, width, height, RS2_FORMAT_Y8, FPS);
    cfg.enable_stream(RS2_STREAM_DEPTH, width, height, RS2_FORMAT_Z16, FPS);

    //Instruct pipeline to start streaming with the requested configuration
    pipe.start(cfg);

    // Camera warmup - dropping several first frames to let auto-exposure stabilize
    rs2::frameset frames;
    for(int i = 0; i < 30; i++)
    {
        //Wait for all configured streams to produce a frame
        frames = pipe.wait_for_frames();
    }

    const auto window_name = "Display Image";
    namedWindow(window_name, WINDOW_AUTOSIZE);

    while (waitKey(1) < 0 && getWindowProperty(window_name, WND_PROP_AUTOSIZE) >= 0)
    {
        // Wait for next set of frames from the camera
        rs2::frameset data = pipe.wait_for_frames();
        
        // apply_filter(color_map) - colorization of depth image
        rs2::frame depth = data.get_depth_frame().apply_filter(color_map);

        // Create OpenCV matrix of size (w,h) from the colorized depth data
        Mat image(Size(width, height), CV_8UC3, (void*)depth.get_data(), Mat::AUTO_STEP);

        // Update the window with new data
        imshow(window_name, image);
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}