// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>   // Include OpenCV API

int main(int argc, char * argv[]) try
{
    rs2::pipeline pipe;

    //Create a configuration for configuring the pipeline with a non default profile
    rs2::config cfg;

    //image dimensions 320x180; 320x240; 424x240; 640x360; 640x480; 848x480; 960x540; 1280x720; 9120x1080;
    const size_t width = 1280;
    const size_t height = 720;
    //Frame rate (FPS) = 6, 15, 30, 60;
    const size_t FPS = 30;

    //Add desired streams to configuration
    cfg.enable_stream(RS2_STREAM_COLOR, width, height, RS2_FORMAT_BGR8, FPS);

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
    namedWindow(window_name, cv::WINDOW_AUTOSIZE);

    while (cv::waitKey(1) < 0 && getWindowProperty(window_name, cv::WND_PROP_AUTOSIZE) >= 0)
    {
        //Get each frame
        frames = pipe.wait_for_frames();

        // Creating OpenCV Matrix from a color image
        cv::Mat color(cv::Size(width, height), CV_8UC3, (void*)frames.get_color_frame().get_data(), cv::Mat::AUTO_STEP);
        // Update the window with new data
        imshow(window_name, color);
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
