#ifndef AGORA_H264_PARSER_H
#define AGORA_H264_PARSER_H

#include <gst/gst.h>

typedef struct {

    int type;
    int start_position;
    int end_position;

} H264Nal;

typedef struct {

    int is_key_frame;
    int start_position;
    int end_position;

} H264Frame;

/*find the first nal start and end position.
 It returns the length of the nal, or 0 if did not find start of
 nal, or -1 if did not find end of nal*/
int find_nal_unit(u_int8_t *buf, int size, H264Nal* nal);

/*extract the boundires and frame type from a given buffer*/
int get_frame(u_int8_t *buffer, int size, H264Frame* frame);

#endif
