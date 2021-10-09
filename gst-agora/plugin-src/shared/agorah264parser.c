
#include "agorah264parser.h"
#include <stdio.h>

int find_nal_unit(u_int8_t *buf, int size, H264Nal* nal){

   if (size < 4) {
		return 0;
	}
	int i = 0;
	// find start
	nal->start_position = 0;
	nal->end_position = 0;

	while (buf[i] != 0 || buf[i + 1] != 0 ||
		   !(buf[i + 2] == 1 || (buf[i + 2] == 0 && buf[i + 3] == 1))) {
		i++;
		if (size < i + 4) {
			return 0;
		} // did not find nal start
	}

	nal->start_position = i;

	if (buf[i + 2] == 1) {
		nal->type = buf[i + 3] & 0x1f;
		i += 4;
	} else if (size > i + 4) {
		nal->type = buf[i + 4] & 0x1f;
		i += 5;
	} else {
		return 0;
	}

	if (size < i + 4) {
		nal->end_position = size - 1;
		return -1;
	}

	while (buf[i] != 0 || buf[i + 1] != 0 ||
		   !(buf[i + 2] == 1 || (buf[i + 2] == 0 && buf[i + 3] == 1))) {
		i++;
		if (size < i + 4) {
			nal->end_position = size - 1;
			return -1;
		} // did not find nal end, stream ended first
	}

	nal->end_position = i - 1;
	return (nal->end_position - nal->start_position);
}

#define BIT(num, bit) (((num) & (1 << (7 - bit))) > 0)
int exp_golomb_decode(u_int8_t *buffer, int size, int* bitOffset)
{
	int totalBits = size << 3;
	int leadingZeroBits = 0;
	for (int i = *bitOffset; i < totalBits && !BIT(buffer[i / 8], i % 8); i++) {
		leadingZeroBits++;
	}
	int offset = 0;
	int bitPos = *bitOffset + leadingZeroBits + 1;
	for (int i = 0; i < leadingZeroBits; i++) {
		offset = (offset << 1) + BIT(buffer[bitPos / 8], bitPos % 8);
		bitPos++;
	}
	*bitOffset += leadingZeroBits + 1 + leadingZeroBits;
	return (1 << leadingZeroBits) - 1 + offset;
}

int get_frame(u_int8_t *buffer, int size, H264Frame* frame){

    H264Nal current_nal;

	int frame_start = 0;
	int frame_end = 0;
	int ret;

    int data_offset_=0;

	// get first nalu for frame_start
	ret = find_nal_unit(&buffer[data_offset_], size - data_offset_, &current_nal);
	if (ret == 0) {
		printf("end of buffer without extracting a frame\n");
        return 0;
	}
	frame_start = data_offset_ + current_nal.start_position;

	// get first I slice or P slice for frame_type
	while (current_nal.type != 1 && current_nal.type != 5) {
		data_offset_ += current_nal.end_position + 1;

		ret = find_nal_unit(&buffer[data_offset_], size - data_offset_, &current_nal);
		if (ret == 0) {
			printf("end of buffer without extracting a frame\n");
            return 0;
		}
	}
	int offset = data_offset_ + current_nal.start_position;
	offset += buffer[offset + 2] ? 3 : 4 + 1;

	int bitOffset = 0;
	int first_mb_in_slice =
			exp_golomb_decode(&buffer[offset], size - offset, &bitOffset);
	int slice_type = exp_golomb_decode(&buffer[offset], size - offset, &bitOffset);

	if (current_nal.type == 5) { // IDR
		frame->is_key_frame = 1;
	} else {
		slice_type %= 5;
		if (slice_type == 2 || slice_type == 4) { // I SLICE
			frame->is_key_frame = 1;
		} else { // P SLICE
			frame->is_key_frame = 0;
		}
	}
	int prev_first_mb_in_slice = first_mb_in_slice;
	int prev_nal_type = current_nal.type;

	// judge the slice is the last slice in a frame or not
	while (1) {
		data_offset_ += current_nal.end_position + 1;
		ret = find_nal_unit(&buffer[data_offset_], size - data_offset_, &current_nal);

		if(prev_nal_type != current_nal.type) break;
		offset = data_offset_ + current_nal.start_position;
		offset += buffer[offset + 2] ? 3 : 4 + 1;
		bitOffset = 0;
		int Last_FNIS = first_mb_in_slice;
		first_mb_in_slice =
				exp_golomb_decode(&buffer[offset], size - offset, &bitOffset);
		if ((prev_first_mb_in_slice > first_mb_in_slice) ||
			(prev_first_mb_in_slice == first_mb_in_slice && prev_first_mb_in_slice == 0)) {
			break;
		}
		if (ret == 0) {
			printf("*end of buffer without extracting a frame\n");
            return 0;
		}
	}

	frame_end = data_offset_ - 1;
	
    frame->start_position=frame_start;
    frame->end_position=frame_end;

	return 1;
}
