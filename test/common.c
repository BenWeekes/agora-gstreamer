#include <gst/gst.h> 

char* get_state_name(GstState state){
    if(state==0){
       	return "pending";
    }
	else if(state==1){
      	return "NULL";
    }
    else if(state==2){
      	return "READY";
   	}
	else if(state==3){
      	return "PAUSED";
    }
	else if(state==4){
      	return "PLAYING";
    }
}
void do_print_element_state(GstElement* element){
	if (GST_IS_ELEMENT (element) ) {
		GstState state, pending;
		gst_element_get_state (element,&state,&pending,1000*10);
		g_print("%s \t %s\n", GST_ELEMENT_NAME(element), get_state_name(state));                      
	}
}
void print_element_states(GstElement *pipeline){

	g_print("************************************\n");
	GstIterator *it;
	gboolean done = FALSE;
	GstElement *element=NULL;

	GValue item = G_VALUE_INIT;
	it = gst_bin_iterate_elements (GST_BIN (pipeline) );

	while (!done) {

		switch (gst_iterator_next (it, &item) ) {
		      	case GST_ITERATOR_OK: 
			    	element = GST_ELEMENT (g_value_get_object (&item));     
				 	do_print_element_state(element);
					g_value_reset (&item);
                 	break;
	       	 	case GST_ITERATOR_RESYNC:
		           	gst_iterator_resync (it);
			        break;
		      	case GST_ITERATOR_ERROR:
		      	case GST_ITERATOR_DONE:
			        done = TRUE;
			        break;
	  }
	}
}

void set_element_state(GstElement *pipeline, const char* elementName,  GstState state){

	GstElement *element = gst_bin_get_by_name(GST_BIN(pipeline), elementName);
	if(element!=NULL){
		gst_element_set_state (element, state);
	}
	else{
		g_print("cannot set the state of %s\n", elementName);
	}
}

