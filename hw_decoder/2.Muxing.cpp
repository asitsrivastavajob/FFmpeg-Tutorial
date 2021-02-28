#include "stdafx.h"
#include <stdio.h>
#if 0
#include "iostream"
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


int main(int argc, char *argv[])
{
	av_register_all();
	bool fragmented_mp4_options = false;
	AVFormatContext* avformat_input_Context = NULL;
	AVFormatContext *avformat_output_Context = NULL;
	
	int ret = 0;
	/* open the input file */
	ret = avformat_open_input(&avformat_input_Context, argv[1], NULL, NULL);

	if ( ret < 0) {
		fprintf(stderr, "Cannot open input file '%d'\n", argv[1]);
		return -1;
	}

	ret = avformat_find_stream_info(avformat_input_Context,NULL);
	if (ret < 0)
	{
		cout << "avformat_find_stream_info Failed" << endl;
		return ret;
	}
	
	avformat_alloc_output_context2(&avformat_output_Context, NULL, NULL, "output.ts");
	if (avformat_output_Context == NULL)
	{
		cout << "avformat_alloc_output_context2 Failed" << endl;
		return -1;
	}

	int num_of_streams = avformat_input_Context->nb_streams;
	int* streams_list = NULL;
	streams_list =(int*)av_mallocz_array(num_of_streams, sizeof(*streams_list));
	if (!streams_list) {
		ret = AVERROR(ENOMEM);
		return -1;
	}

	int stream_index = 0;
	for (int i = 0; i < avformat_input_Context->nb_streams; i++)
	{
		AVCodecParameters* codec_parameters = avformat_input_Context->streams[i]->codecpar;
		AVStream* out_stream;
		AVStream* in_stream = avformat_input_Context->streams[i];
		// specific for video and audio
		if (codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO || codec_parameters->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			streams_list[i] = stream_index++;
			out_stream = avformat_new_stream(avformat_output_Context, NULL);
			ret = avcodec_parameters_copy(out_stream->codecpar, avformat_input_Context->streams[i]->codecpar);
			if (ret < 0)
			{
				cout << "avcodec_parameters_copy Failed" << endl;
				return ret;
			}
		}
	}


	if (!(avformat_output_Context->oformat->flags & AVFMT_NOFILE))
	{
		ret = avio_open(&avformat_output_Context->pb,"output.ts",AVIO_FLAG_WRITE);
		if (ret < 0)
		{
			cout << "avio_open Failed" << endl;
			return ret;
		}
	}

	AVDictionary* opts = NULL;

	if (fragmented_mp4_options) {
		av_dict_set(&opts, "movflags", "frag_keyframe+empty_moov+default_base_moof", 0);
	}

	ret = avformat_write_header(avformat_output_Context, &opts);
	if (ret < 0)
	{
		cout << "avformat_write_header Failed" << endl;
		return ret;
	}

	AVPacket pPacket;

	while (av_read_frame(avformat_input_Context, &pPacket) >= 0) {
			  
		AVStream* in_stream, * out_stream;
		in_stream = avformat_input_Context->streams[pPacket.stream_index];
		int out_stream_index = streams_list[pPacket.stream_index];
		out_stream = avformat_output_Context->streams[out_stream_index];

		/* copy packet */
		pPacket.pts = av_rescale_q_rnd(pPacket.pts, in_stream->time_base, out_stream->time_base, /*AV_ROUND_NEAR_INF |*/AV_ROUND_PASS_MINMAX);
		pPacket.dts = av_rescale_q_rnd(pPacket.dts, in_stream->time_base, out_stream->time_base, /*AV_ROUND_NEAR_INF |*/AV_ROUND_PASS_MINMAX);
		pPacket.duration = av_rescale_q(pPacket.duration, in_stream->time_base, out_stream->time_base);
		pPacket.pos = -1;

		ret = av_interleaved_write_frame(avformat_output_Context, &pPacket);
		if (ret < 0) {
			fprintf(stderr, "Error muxing packet\n");
			break;
		}
		else
		{
			cout << "successfully writing frame" << endl;
		}

		av_packet_unref(&pPacket);
	}
	av_write_trailer(avformat_output_Context);

	avformat_close_input(&avformat_input_Context);
	/* close output */
	if (avformat_output_Context && !(avformat_output_Context->oformat->flags & AVFMT_NOFILE))
		avio_closep(&avformat_output_Context->pb);
	avformat_free_context(avformat_output_Context);
	av_freep(&streams_list);
	if (ret < 0 && ret != AVERROR_EOF) {
		cout << "error occured" << endl;
		return 1;
	}
	return 0;
}

#endif




