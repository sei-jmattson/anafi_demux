/**
 * anafi_demux
 * demuxes Parrot Anafi rtsp stream into custom binary stream per frame
 * usage: anafi_demux url outfile [fps]
 *
 * jmattson@sei.cmu.edu
 * leveraging Parrot's groundsdk-tools video sink
**/
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

#define ULOG_TAG anafi_demux
#include <ulog.h>
ULOG_DECLARE_TAG(anafi_demux);

#include <media-buffers/mbuf_raw_video_frame.h>
#include <pdraw-vsink/pdraw_vsink.h>
#include <pdraw/pdraw_defs.h>
#include <video-defs/vdefs.h>
#include <video-metadata/vmeta.h>

// big-endian put int
void be_put(size_t v, FILE* fp);
void be_put(size_t v, FILE* fp)
{
	fputc(v>>24, fp);
	fputc(v>>16, fp);
	fputc(v>>8, fp);
	fputc(v, fp);
}

// big-endian put short
void be_put2(size_t v, FILE* fp);
void be_put2(size_t v, FILE* fp)
{
	fputc(v>>8, fp);
	fputc(v, fp);
}

int main(int argc, char **argv)
{
	struct pdraw_vsink *vsink = NULL;
	struct pdraw_media_info *media_info = NULL;
	struct pdraw_video_frame frame_info = {0};
	struct mbuf_raw_video_frame *frame = NULL;
	struct vmeta_frame *frame_meta = NULL;
	struct vmeta_buffer vb;
	const void *image_bf = NULL;
	int status = EXIT_SUCCESS, res;
	uint64_t frame_count = 0, subframe_count = 0;
	size_t frame_interval, image_bflen;
	float frame_rate, fps = 1.0;
	FILE *outfp;

	// create metadata buffer
	uint8_t *meta_bf = NULL;
	const size_t meta_bflen = 1024;
	meta_bf = malloc(meta_bflen);
	vmeta_buffer_set_data(&vb, meta_bf, meta_bflen, 0);

	if (argc < 3) {
		ULOGE("usage: %s <url> <output_path|'-'> [frame_interval]\n\tframe_interval defaults to 1 fps", argv[0]);
		exit(EXIT_FAILURE);
	}

	// open destination stream
	outfp = stdout;
	if (strcmp(argv[2], "-") != 0)
	{
		outfp = fopen(argv[2], "wb");
	}

	// get requested fps
	if (argc > 3) {
		fps = strtol(argv[3], NULL, 10);
		if (fps == 0) {
			fps = 1;
		}
	}

	// open input stream
	res = pdraw_vsink_start(argv[1], &media_info, &vsink);
	if (res < 0 || media_info == NULL) {
		ULOG_ERRNO("pdraw_vsink_start", -res);
		exit(EXIT_FAILURE);
	}

	ULOGI("media_info: name=%s, path=%s",
	      media_info->name,
	      media_info->path
	);

	ULOGI("media_info: duration=%.3fs, res=%ux%u, framerate=%u/%u",
	      media_info->duration / 1000000.0,
	      media_info->video.raw.info.resolution.width,
	      media_info->video.raw.info.resolution.height,
	      media_info->video.raw.info.framerate.num,
	      media_info->video.raw.info.framerate.den
	);

	// calculate frame interval
	frame_rate = ((float)media_info->video.raw.info.framerate.num) / media_info->video.raw.info.framerate.den + 0.1;
	if (fps > frame_rate) fps = frame_rate;
	frame_interval = frame_rate / fps;
	ULOGI("downsample interval: %ld", frame_interval);

	while (true) {

		/* Get a new frame */
		res = pdraw_vsink_get_frame(vsink, NULL, &frame_info, &frame);
		if (res < 0) {
			ULOG_ERRNO("pdraw_vsink_get_frame", -res);
			continue;
		}

		// ULOGI("meta type: %d", frame_meta->type);
		frame_count += 1;

		/* Get the video metadata */
		res = mbuf_raw_video_frame_get_metadata(frame, &frame_meta);
		if (res < 0 && res != -ENOENT) {
			ULOG_ERRNO("mbuf_raw_video_frame_get_metadata", -res);
			mbuf_raw_video_frame_unref(frame);
			continue;
		}

		if (frame_meta == NULL) {
			ULOGI("no frame meta");
			mbuf_raw_video_frame_unref(frame);
			continue;
		}

		// sub sample frames
		if (frame_count % frame_interval > 0){
			vmeta_frame_unref(frame_meta);
			mbuf_raw_video_frame_unref(frame);
			continue;
		}

		// process image
		res = mbuf_raw_video_frame_get_packed_buffer(frame, &image_bf, &image_bflen);
		if (res < 0){
			vmeta_frame_unref(frame_meta);
			mbuf_raw_video_frame_unref(frame);
			continue;
		}

		// pack protobuf
		vb.pos = 0;
		vmeta_frame_write(&vb, frame_meta);

		// output
		be_put(1346437120, outfp);
		be_put(vb.pos, outfp);
		fwrite(vb.data, vb.pos, 1, outfp);
		be_put2(frame_info.raw.info.resolution.width, outfp);
		be_put2(frame_info.raw.info.resolution.height, outfp);
		be_put(image_bflen, outfp);
		fwrite(image_bf, image_bflen, 1, outfp);

		// rate-limit logs
		subframe_count += 1;
		if (subframe_count % 10 == 0) {
			ULOGI("frame #%ld res: %dx%d",
				frame_count,
				frame_info.raw.info.resolution.width,
				frame_info.raw.info.resolution.height
			);
		}

		// release resources
		mbuf_raw_video_frame_release_packed_buffer(frame, image_bf);
		vmeta_frame_unref(frame_meta);
		mbuf_raw_video_frame_unref(frame);
	}

	// release resources
	fclose(outfp);
	free(meta_bf);
	res = pdraw_vsink_stop(vsink);
	if (res < 0)
		ULOG_ERRNO("pdraw_vsink_stop", -res);

	ULOGI("%s", (status == EXIT_SUCCESS) ? "success!" : "failed!");
	exit(status);
}
