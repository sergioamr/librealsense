//---------------------------------------------------------------------
// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering

#include <algorithm>            // std::min, std::max

//---------------------------------------------------------------------

#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>

// Helper functions
void register_glfw_callbacks(window& app, glfw_state& app_state);

char file_record[512];

#define NOT_IR

#define BAG_0 "capture_151415"
#define BAG_1 "capture_101803"
#define BAG_2 "down_capture_100858"
#define BAG_3 "horizontal_capture_100246_small"
#define BAG_4 "horizontal_capture_101434_tall"

#define FILE_PATH "C:\\Users\\smartinez\\Desktop\\"

bool color_depth = false;
bool color_ir = false;

bool save_frame = false;
int bag = 0;

inline bool exists(const std::string& name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

// Registers the state variable and callbacks to allow mouse control of the pointcloud
void er_register_glfw_callbacks(window& app, glfw_state& app_state)
{
	app.on_left_mouse = [&](bool pressed)
	{
		app_state.ml = pressed;
	};

	app.on_mouse_scroll = [&](double xoffset, double yoffset)
	{
		app_state.offset_x -= static_cast<float>(xoffset);
		app_state.offset_y -= static_cast<float>(yoffset);
	};

	app.on_mouse_move = [&](double x, double y)
	{
		if (app_state.ml)
		{
			app_state.yaw -= (x - app_state.last_x);
			app_state.yaw = std::max(app_state.yaw, -120.0);
			app_state.yaw = std::min(app_state.yaw, +120.0);
			app_state.pitch += (y - app_state.last_y);
			app_state.pitch = std::max(app_state.pitch, -80.0);
			app_state.pitch = std::min(app_state.pitch, +80.0);
		}
		app_state.last_x = x;
		app_state.last_y = y;
	};

	app.on_key_release = [&](int key)
	{
		switch (key) {
		case 32:
			app_state.yaw = app_state.pitch = 0; app_state.offset_x = app_state.offset_y = 0.0;
			break;
		case 49:
			color_depth = !color_depth;
			break;
		case 50:
			color_ir = !color_ir;
			break;
		case 51:
			bag++;
			break;
		case 83:
			save_frame = true;
			break;

		}
	};
}

bool get_texcolor(rs2::video_frame *frame, float u, float v, uint8_t &r, uint8_t &g, uint8_t &b)
{
	auto ptr = frame;
	if (ptr == nullptr) {
		return false;
	}

	const int w = ptr->get_width(), h = ptr->get_height();
	int x = std::min(std::max(int(u*w + .5f), 0), w - 1);
	int y = std::min(std::max(int(v*h + .5f), 0), h - 1);
	int idx = x * ptr->get_bits_per_pixel() / 8 + y * ptr->get_stride_in_bytes();
	const auto texture_data = reinterpret_cast<const uint8_t*>(ptr->get_data());

	r = texture_data[idx++];
	g = texture_data[idx++];
	b = texture_data[idx];

	return true;
}

int main(int argc, char * argv[]) try
{
	if (argc > 1) {
		strcpy(file_record, argv[1]);
	}
	else {
		sprintf(file_record, "%s%s.bag", FILE_PATH, BAG_1);
	}

    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "Earth Rover - Field Capture");
    // Construct an object to manage view state
    glfw_state app_state;
    // register callbacks to allow manipulation of the pointcloud
	er_register_glfw_callbacks(app, app_state);

    // Declare pointcloud object, for calculating pointclouds and texture mappings
    rs2::pointcloud pc;
    // We want the points object to be persistent so we can display the last cloud when a frame drops
    rs2::points points;

    // Declare RealSense pipeline, encapsulating the actual device and sensors

	rs2::config cfg;

#ifndef NOT_IR
	cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 6);
	cfg.enable_stream(RS2_STREAM_INFRARED, 640, 480, RS2_FORMAT_Y8, 6);
	cfg.enable_stream(RS2_STREAM_COLOR, 1280, 720, RS2_FORMAT_RGBA8, 6);
#endif

	cfg.enable_device_from_file(file_record);

	// Create a shared pointer to a pipeline
	auto pipe = std::make_shared<rs2::pipeline>();

    // Start streaming with default recommended configuration
    pipe->start(cfg);

	rs2::colorizer color_map;

	int current_bag = 0;
	int frame_count = 0;

	pcl::PointCloud<pcl::PointXYZRGB> cloud;

	// Fill in the cloud data
	cloud.width = 1280;
	cloud.height = 720;
	cloud.is_dense = false;
	cloud.points.resize(1280*720);

    while (app) // Application still alive?
    {
		if (current_bag != bag) {
			pipe->stop();
			pipe = std::make_shared<rs2::pipeline>();

			switch (bag % 4) {
				case 0:
					sprintf(file_record, "%s%s.bag", FILE_PATH, BAG_1);
					break;
				case 1:
					sprintf(file_record, "%s%s.bag", FILE_PATH, BAG_2);
					break;
				case 2:
					sprintf(file_record, "%s%s.bag", FILE_PATH, BAG_3);
					break;
				case 3:
					sprintf(file_record, "%s%s.bag", FILE_PATH, BAG_4);
					break;
			}

			current_bag = bag;

			rs2::config cfg;
			cfg.enable_device_from_file(file_record);
			pipe->start(cfg);
		}

        // Wait for the next set of frames from the camera
        auto frames = pipe->wait_for_frames();

        auto depth = frames.get_depth_frame();

        // Generate the pointcloud and texture mappings
        points = pc.calculate(depth);

		rs2::video_frame color = frames.get_color_frame();

		// Find and colorize the depth data
		rs2::video_frame color_depth_map = color_map(frames.get_depth_frame());

		// For cameras that don't have RGB sensor, we'll map the pointcloud to infrared instead of color
		if (color_ir)
			color = frames.get_infrared_frame();

		if (color_depth) {
			color = color_depth_map;
		}

        // Tell pointcloud object to map to this color frame
        pc.map_to(color);

        // Upload the color frame to OpenGL
        app_state.tex.upload(color);

        // Draw the pointcloud
        draw_pointcloud(app.width(), app.height(), app_state, points);

		if (save_frame || true) {

			char frame_name[512];

			if (!exists(frame_name)) {

				//rs2_error *error;
				//sprintf(frame_name, "data\\%s_%d.ply", BAG_1, frame_count);
//				rs2_export_to_ply(points.get(), frame_name, color.get(), &error);

				//std::cerr << "Saving " << cloud.points.size() << " data points " << frame_name << std::endl;

				/*
				sprintf(frame_name, "data/%s_%d.pcd", BAG_1, frame_count);
				auto vertices = points.get_vertices();              // get vertices
				auto tex_coords = points.get_texture_coordinates(); // and texture coordinates

				for (int i = 0; i < points.size(); i++)
				{
					auto p = &cloud.points[i];
					p->x = vertices[i].x;
					p->y = vertices[i].y;
					p->z = vertices[i].z;

					if (color) {
						//if (tex_coords[i].u < 0 || tex_coords[i].u > 1 || tex_coords[i].v < 0 || tex_coords[i].v > 1) {
						//	get_texcolor(&color_depth_map, tex_coords[i].u, tex_coords[i].v, p->r, p->g, p->b);
						//} else {
							get_texcolor(&color, tex_coords[i].u, tex_coords[i].v, p->r, p->g, p->b);
						//}
					}
				}

				//pcl::io::savePCDFileASCII(frame_name, cloud);
				//std::cerr << "Saved " << std::endl;
				*/
			}

			save_frame = false;
		}


		frame_count++;
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
