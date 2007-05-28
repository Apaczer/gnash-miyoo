// 
//   Copyright (C) 2005, 2006, 2007 Free Software Foundation, Inc.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

// $Id: embedVideoDecoderGst.cpp,v 1.4 2007/05/28 15:41:01 ann Exp $

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef SOUND_GST

#include "embedVideoDecoderGst.h"

embedVideoDecoderGst::embedVideoDecoderGst() :
	pipeline(NULL),
	input(NULL),
	inputcaps(NULL),
	videocaps(NULL),
	output(NULL),
	decoder(NULL),
	colorspace(NULL),
	decodedFrame(NULL)

{
}

embedVideoDecoderGst::~embedVideoDecoderGst()
{
	if(decodedFrame) delete decodedFrame;

	if (pipeline) {
		gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (pipeline));
	}
}

void
embedVideoDecoderGst::createDecoder(int widthi, int heighti, int deblockingi, bool smoothingi, int formati, int outputFormati)
{
	// Save video attributes
	width = widthi;
	height = heighti;
	deblocking = deblockingi;
	smoothing = smoothingi;
	format = formati;
	outputFormat = outputFormati;

	// For now only H263/SVQ3, VP6 and screenvideo1 is supported
	if (format != CODEC_H263 && format != CODEC_VP6 && format != CODEC_SCREENVIDEO) return;

	// init GStreamer
	gst_init (NULL, NULL);

	// setup the pipeline
	pipeline = gst_pipeline_new (NULL);

	// Setup the pipeline elements

	// setup fake source
	input = gst_element_factory_make ("fakesrc", NULL);
	g_object_set (G_OBJECT (input),	"sizetype", 3, /*"can-activate-pull", FALSE,*/ "signal-handoffs", TRUE, NULL);
	// Setup the callback
	g_signal_connect (input, "handoff", G_CALLBACK (embedVideoDecoderGst::callback_handoff), this);

	// Setup the input capsfilter
	inputcaps = gst_element_factory_make ("capsfilter", NULL);
	GstCaps* caps;
	if (format == CODEC_H263) {
		caps = gst_caps_new_simple ("video/x-flash-video",
			"width", G_TYPE_INT, width,
			"height", G_TYPE_INT, height,
			"framerate", GST_TYPE_FRACTION, 25, 1,
			"flvversion", G_TYPE_INT, 1,
			NULL);
	} else if (format == CODEC_VP6) {
		caps = gst_caps_new_simple ("video/x-vp6-flash",
			"width", G_TYPE_INT, width,
			"height", G_TYPE_INT, height,
			"framerate", GST_TYPE_FRACTION, 25, 1,
			NULL);
	} else if (format == CODEC_SCREENVIDEO) {
		caps = gst_caps_new_simple ("video/x-flash-screen",
			"width", G_TYPE_INT, width,
			"height", G_TYPE_INT, height,
			"framerate", GST_TYPE_FRACTION, 25, 1,
			NULL);
	}
	g_object_set (G_OBJECT (inputcaps), "caps", caps, NULL);
	gst_caps_unref (caps);

	// Setup the capsfilter which demands either YUV or RGB videoframe format
	videocaps = gst_element_factory_make ("capsfilter", NULL);
	if (outputFormat == YUV) {
		caps = gst_caps_new_simple ("video/x-raw-yuv", NULL);
	} else {
		caps = gst_caps_new_simple ("video/x-raw-rgb", NULL);
	}
	g_object_set (G_OBJECT (videocaps), "caps", caps, NULL);
	gst_caps_unref (caps);

	// setup the videosink with callback
	output = gst_element_factory_make ("fakesink", NULL);
	g_object_set (G_OBJECT (output), "signal-handoffs", TRUE, NULL);
	g_signal_connect (output, "handoff", G_CALLBACK (embedVideoDecoderGst::callback_output), this);

	// setup the video colorspaceconverter converter
	colorspace = gst_element_factory_make ("ffmpegcolorspace", NULL);

	// Find the decoder and init the parser
	if (format == CODEC_H263) {
		decoder = gst_element_factory_make ("ffdec_flv", NULL);
	} else if (format == CODEC_VP6) {
		decoder = gst_element_factory_make ("ffdec_vp6f", NULL);
	} else if (format == CODEC_SCREENVIDEO) {
		decoder = gst_element_factory_make ("ffdec_flashsv", NULL);
	} else {
		return;
	}

	if (!pipeline || !input || !inputcaps || !videocaps || !output || !decoder || !colorspace) {
		return;
	}

	// Put the elemets in the pipeline and link them
	gst_bin_add_many (GST_BIN (pipeline), input, inputcaps, decoder, colorspace, videocaps, output, NULL);

	// link the elements
	gst_element_link_many(input, inputcaps, decoder, colorspace, videocaps, output, NULL);

	// This make callback_handoff wait for data
	input_lock = new boost::mutex::scoped_lock(input_mutex);

	// This make decodeFrame wait for data
	output_lock = new boost::mutex::scoped_lock(output_mutex);

	// Determine required buffer size and allocate buffer
	if (outputFormat == YUV) {
		decodedFrame = new image::yuv(width, height);
	} else if (outputFormat == RGB) {
		decodedFrame = new image::rgb(width, height);
	}

	// Start "playing"
	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);
}


// gnash calls this when it wants you to decode the given videoframe
image::image_base*
embedVideoDecoderGst::decodeFrame(uint8_t* data, int size)
{

	if (data == NULL) return decodedFrame;

	frame = data;
	frameSize = size;

	delete input_lock;

	output_lock = new boost::mutex::scoped_lock(output_mutex);

	return decodedFrame;
}

// The callback function which refills the buffer with data
void
embedVideoDecoderGst::callback_handoff (GstElement * /*c*/, GstBuffer *buffer, GstPad* /*pad*/, gpointer user_data)
{
	embedVideoDecoderGst* decoder = static_cast<embedVideoDecoderGst*>(user_data);

	decoder->input_lock = new boost::mutex::scoped_lock(decoder->input_mutex);

	GST_BUFFER_SIZE(buffer) = decoder->frameSize;

	GST_BUFFER_DATA(buffer) = decoder->frame;
}

// The callback function which passes the decoded video frame
void
embedVideoDecoderGst::callback_output (GstElement * /*c*/, GstBuffer *buffer, GstPad* /*pad*/, gpointer user_data)
{
	embedVideoDecoderGst* decoder = static_cast<embedVideoDecoderGst*>(user_data);

	if (decoder->decodedFrame) {

		if (decoder->outputFormat == YUV) {
			assert(0);

		/*	image::yuv* yuvframe = static_cast<image::yuv*>(decoder->decodedFrame);
			int copied = 0;
			uint8_t* ptr = GST_BUFFER_DATA(buffer);
			for (int i = 0; i < 3 ; i++)
			{
				int shift = (i == 0 ? 0 : 1);
				uint8_t* yuv_factor = m_Frame->data[i];
				int h = ns->videoheight >> shift;
				int w = ns->videowidth >> shift;
				for (int j = 0; j < h; j++)
				{
					copied += w;
					assert(copied <= yuvframe->size());
					memcpy(ptr, yuv_factor, w);
					yuv_factor += m_Frame->linesize[i];
					ptr += w;
				}
			}
			video->m_size = copied;*/
		} else {
			decoder->decodedFrame->update(GST_BUFFER_DATA(buffer));
		}
	}

	delete decoder->output_lock;

}

#endif // SOUND_GST
