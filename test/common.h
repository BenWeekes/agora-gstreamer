#ifndef _COMMON_H_
#define _COMMON_H_

#include <gst/gst.h> 

void print_element_states(GstElement *pipeline);

void set_element_state(GstElement *pipeline, const char* elementName,  GstState state);

#endif
