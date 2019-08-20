#include <stdio.h>
#include <iostream>
#include <thread>
#include <atomic>


#include <opencv/cv.hpp>        //you may need to
#include <opencv2/highgui/highgui.hpp>  //adjust import locations
#include <opencv2/core/core.hpp>     //depending on your machine setup
#include <opencv2/imgproc.hpp>


// include OpenCV header file
#include <opencv2/opencv.hpp>

// include the librealsense C++ header file
#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.h>

#define sub_sampling_magnitude 2


///make available OpenCV namespace
using namespace cv;

// 3rd party header for writing png files
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main(int argc, char * argv[]) try
{

    /// Spatially align all streams to depth viewport
    /// We do this because:
    ///   a. Usually depth has wider FOV, and we only really need depth for this demo
    ///   b. We don't want to introduce new holes
    rs2::align align_to(RS2_STREAM_DEPTH);


    /// Decimation filter reduces the amount of data (while preserving best samples)
    rs2::decimation_filter dec;
    /// If the demo is too slow, make sure you run in Release (-DCMAKE_BUILD_TYPE=Release)
    /// but you can also increase the following parameter to decimate depth more (reducing quality)
    dec.set_option(RS2_OPTION_FILTER_MAGNITUDE, sub_sampling_magnitude);


    /// Threshhold filter will eliminate pixels of the depth image,
    /// that are closer than min distanse, further than max distanse.
    /// this will also speed-up the algorith.
    rs2::threshold_filter depth_threshhold;
    depth_threshhold.set_option(RS2_OPTION_MIN_DISTANCE, 0.5); // setting min distance
    depth_threshhold.set_option(RS2_OPTION_MAX_DISTANCE, 1.4); //setting max distance


    /// Define transformations from and to Disparity domain
    rs2::disparity_transform depth2disparity;
    rs2::disparity_transform disparity2depth(false);


    /// Define spatial filter (edge-preserving)
    rs2::spatial_filter spat;
    spat.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.6); // setting delta parameter for spatal filter
    spat.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 8); // setting alpha parameter for spatal filter
    /// Enable hole-filling
    /// Hole filling is an agressive heuristic and it gets the depth wrong many times
    /// However, this demo is not built to handle holes
    /// (the shortest-path will always prefer to "cut" through the holes since they have zero 3D distance)
    spat.set_option(RS2_OPTION_HOLES_FILL, 2); // 2 = fill all the zero pixels

    /// Define temporal filter
    rs2::temporal_filter temp;
    temp.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.5); // setting delta parameter for spatal filter
    temp.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 20); // setting alpha parameter for spatal filter

    /// Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
    /// Change colorization scheme from jet to grayscale
    // color_map.set_option(RS2_OPTION_COLOR_SCHEME, 2.f);

    ///Contruct a pipeline which abstracts the device
    rs2::pipeline pipe;

    rs2::config cfg;

    ///image dimensions 424x240; 480x270; 640x360; 640x480; 848x480; 1280x720; Not valid for depth 1920x1080;
    const size_t width = 1280;
    const size_t height = 720;

    ///Frame rate (FPS) = 6, 15, 30, 60;
    const size_t FPS = 6;

    ///Add desired streams to configuration
    ///cfg.enable_stream(RS2_STREAM_INFRARED, width, height, RS2_FORMAT_Y8, FPS);
    cfg.enable_stream(RS2_STREAM_DEPTH, width, height, RS2_FORMAT_Z16, FPS);
    cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_RGB8, FPS);

    ///Instruct pipeline to start streaming with the requested configuration
    auto profile = pipe.start(cfg);

    auto sensor = profile.get_device().first<rs2::depth_sensor>();

    /// Set the device to High Accuracy preset of the D400 stereoscopic cameras
    if (sensor && sensor.is<rs2::depth_stereo_sensor>())
    {
        sensor.set_option(RS2_OPTION_VISUAL_PRESET, RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY);
    }

    /// Camera warmup - dropping several first frames to let auto-exposure stabilize
    rs2::frameset frames;
    for(int i = 0; i < 30; i++)
    {
        /// Wait for all configured streams to produce a frame
        frames = pipe.wait_for_frames();
    }

    auto stream = profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();

    /// After initial post-processing, frames will flow into this queue:
    rs2::frame_queue postprocessed_depth_frames;
    rs2::frame_queue postprocessed_color_frames;

    /// Alive boolean will signal the worker threads to finish-up
    std::atomic_bool alive{ true };

    /// Video-processing thread will fetch frames from the camera,
    /// apply post-processing and send the result to the main thread for rendering
    /// It recieves synchronized (but not spatially aligned) pairs
    /// and outputs synchronized and aligned pairs
    std::thread video_processing_thread([&]() {
        while (alive)
        {
            /// Fetch frames from the pipeline and send them for processing
            rs2::frameset data;
            if (pipe.poll_for_frames(&data))
            {
                rs2::frame depth = data.get_depth_frame();
                rs2::frame colorframe = data.get_color_frame();

                /// First make the frames spatially aligned
                depth = depth.apply_filter(align_to);

                /// Decimation will reduce the resultion of the depth image,
                /// closing small holes and speeding-up the algorithm
                depth = depth.apply_filter(dec);

                /// Threshhold filter will eliminate pixels of the depth image,
                /// that are closer than min distanse, further than max distanse.
                /// this will also speed-up the algorith.
                depth = depth.apply_filter(depth_threshhold);

                /// To make sure far-away objects are filtered proportionally
                /// we try to switch to disparity domain
                depth = depth.apply_filter(depth2disparity);

                /// Apply spatial filtering
                depth = depth.apply_filter(spat);

                /// Apply temporal filtering
                depth = depth.apply_filter(temp);

                /// If we are in disparity domain, switch back to depth
                depth = depth.apply_filter(disparity2depth);

                /// Apply color map for visualization of depth
                depth = depth.apply_filter(color_map);

                // Send resulting frames for visualization in the main thread
                postprocessed_depth_frames.enqueue(depth);
                postprocessed_color_frames.enqueue(colorframe);
            }
        }
    });

    const auto depth_window_name = "Display Depth Image";
    namedWindow(depth_window_name, WINDOW_AUTOSIZE);
    const auto color_window_name = "Display Color Image";
    namedWindow(color_window_name, WINDOW_AUTOSIZE);

    rs2::frame current_depth_frame;
    rs2::frame current_color_frame;

    long i = 1; //simple counter for naming images

    while (waitKey(1) < 0 && getWindowProperty(depth_window_name, WND_PROP_AUTOSIZE) >= 0 && getWindowProperty(color_window_name, WND_PROP_AUTOSIZE) >= 0)
    {
        //creating new name for file;
        std::string name = std::to_string(i);

        // Fetch the latest available post-processed frameset
        postprocessed_depth_frames.poll_for_frame(&current_depth_frame);

        if (current_depth_frame)
        {
            auto vf = current_depth_frame.as<rs2::video_frame>();
            // Create OpenCV matrix of size (w,h) from the colorized depth data
            Mat image(Size(vf.get_width(), vf.get_height()), CV_8UC3, (void *) current_depth_frame.get_data(), Mat::AUTO_STEP);

            // Write images to disk
            std::stringstream png_file;
            png_file << vf.get_profile().stream_name() << name << ".png";
            stbi_write_png(png_file.str().c_str(), vf.get_width(), vf.get_height(), vf.get_bytes_per_pixel(), vf.get_data(), vf.get_stride_in_bytes());
            std::cout << "Saved " << png_file.str() << std::endl;

            // Update the window with new data
            imshow(depth_window_name, image);


        }

        // Fetch the latest available post-processed frameset
        postprocessed_color_frames.poll_for_frame(&current_color_frame);

        if (current_color_frame)
        {
            auto vf = current_color_frame.as<rs2::video_frame>();
            // Create OpenCV matrix of size (w,h) from the colorized depth data
            Mat image(Size(vf.get_width(), vf.get_height()), CV_8UC3, (void *) current_color_frame.get_data(), Mat::AUTO_STEP);

            // Write images to disk
            std::stringstream png_file;
            png_file << vf.get_profile().stream_name() << name << ".png";
            stbi_write_png(png_file.str().c_str(), vf.get_width(), vf.get_height(), vf.get_bytes_per_pixel(), vf.get_data(), vf.get_stride_in_bytes());
            std::cout << "Saved " << png_file.str() << std::endl;

            // Update the window with new data
            //imshow(color_window_name, image);
        }


        i++;
    }

    // Signal threads to finish and wait until they do
    alive = false;
    video_processing_thread.join();

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