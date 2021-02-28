#include "stdafx.h"
#include <stdio.h>
#include "iostream"
#if 1
using namespace std;
extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libavutil/pixdesc.h>
	#include <libavutil/hwcontext.h>
	#include <libavutil/opt.h>
	#include <libavutil/avassert.h>
	#include <libavutil/imgutils.h>
}


static void save_gray_frame(unsigned char* buf, int wrap, int xsize, int ysize, char* filename)
{
	FILE* f = NULL;
	int i;
	fopen_s(&f,filename, "w");
	
	//fprintf(f, "P5\n%d %d\n%d\n", xsize, ysize, 255);

	// writing line by line
	for (i = 0; i < ysize; i++)
		fwrite(buf + i * wrap, 1, xsize, f);

	fclose(f);
}

int main(int argc, char *argv[])
{
	av_register_all();

	AVFormatContext *avformatContext = NULL;
	avformatContext = avformat_alloc_context();

	/* open the input file */
	if (avformat_open_input(&avformatContext, argv[1], NULL, NULL) != 0) {
		fprintf(stderr, "Cannot open input file '%d'\n", argv[1]);
		return -1;
	}

	avformat_find_stream_info(avformatContext,NULL);
	cout << "Basic details of input file :- " << endl;
	cout<<"Duration : " << avformatContext->duration << endl;
	cout << "No of streams : " << avformatContext->nb_streams << endl;
	cout << "Bitrate :" << avformatContext->bit_rate << endl;
	cout << "Format :" << avformatContext->iformat->name << endl;
	int video_stream_index = 0;
	for (int i = 0; i < avformatContext->nb_streams; i++)
	{
		AVCodecParameters* codec_parameters = avformatContext->streams[i]->codecpar;  

		// specific for video and audio
		if (codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO) 
		{
			video_stream_index = i;
			printf("Video Codec: resolution %d x %d", codec_parameters->width, codec_parameters->height);

			AVCodec* pLocalCodec = avcodec_find_decoder(codec_parameters->codec_id);
			// general
			printf("\tCodec %d ID %d bit_rate %lld \n", pLocalCodec->long_name, pLocalCodec->id, codec_parameters->bit_rate);

			AVCodecContext* pCodecContext = avcodec_alloc_context3(pLocalCodec);
			avcodec_parameters_to_context(pCodecContext, codec_parameters);
			avcodec_open2(pCodecContext, pLocalCodec, NULL);

			AVPacket* pPacket = av_packet_alloc();
			AVFrame* pFrame = av_frame_alloc();
			int count = 0;
			while (av_read_frame(avformatContext, pPacket) >= 0) {
			  
				if (pPacket->stream_index == video_stream_index)
				{
					int response = avcodec_send_packet(pCodecContext, pPacket);
					if (response < 0) {
						printf("Error while sending a packet to the decoder \n");
						return response;
					}

					while (response >= 0)
					{
						response = avcodec_receive_frame(pCodecContext, pFrame);
						if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
							break;
						}
						else if (response < 0) {
							printf("Error while receiving a frame from the decoder \n");
							return response;
						}


						printf(
							"Frame %c (%d) pts %d dts %d key_frame %d [coded_picture_number %d, display_picture_number %d] \n",
							av_get_picture_type_char(pFrame->pict_type),
							pCodecContext->frame_number,
							pFrame->pts,
							pFrame->pkt_dts,
							pFrame->key_frame,
							pFrame->coded_picture_number,
							pFrame->display_picture_number
						);

						if (pFrame->format != AV_PIX_FMT_YUV420P)
						{
							printf("Warning: the generated file may not be a grayscale image, but could e.g. be just the R component if the video format is RGB");
						}

						
						char frame_filename[1024];
						snprintf(frame_filename, sizeof(frame_filename), "%d-%d.pgm", "frame", pCodecContext->frame_number);
						save_gray_frame(pFrame->data[0], pFrame->linesize[0], pFrame->width, pFrame->height, frame_filename);

						break;
					}
				}
				av_packet_unref(pPacket);
			}
		}
		else if (codec_parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
			printf("Audio Codec: %d channels, sample rate %d", codec_parameters->channels, codec_parameters->sample_rate);
		}
	}

	return 0;
}
#endif


