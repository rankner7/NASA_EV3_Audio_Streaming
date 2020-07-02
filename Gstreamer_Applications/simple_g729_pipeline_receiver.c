#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data){

  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

int main (int argc, char *argv[]) {
	
	char *port;

	if (argc != 2){
		printf("Insufficient number of arguements!\n");
		g_printerr ("Usage: %s <PORT> \n", argv[0]);
    return -1;
	}
	else{
		port = argv[1];
		printf("Port:           |%s|\n", port);
	}
	
  GMainLoop *loop;

  GstElement *pipeline, *udpsrc, *decoder, *rtp_depay, *converter, *audiosink;
  GstBus *bus;
  guint bus_watch_id;

  gst_init (NULL, NULL);

  loop = g_main_loop_new (NULL, FALSE);

	//Create Pipeline Elements
  pipeline = gst_pipeline_new ("g729_receiver");
  udpsrc     = gst_element_factory_make ("udpsrc",        "stream_src");
  decoder    = gst_element_factory_make ("g729dec",       "dencoder");
  rtp_depay  = gst_element_factory_make ("rtpg729depay",  "depayloader");
	converter  = gst_element_factory_make ("audioconvert",  "converter");
  audiosink  = gst_element_factory_make ("autoaudiosink", "output");

	//Check proper creation
  if (!pipeline || !udpsrc || !decoder || !rtp_depay || !converter || !audiosink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

	printf("PIPELINE ELEMENTS CREATED PROPERLY\n");

	//Set Properties of Objects
	GstCaps *rtp_caps;
	rtp_caps = gst_caps_new_simple ("application/x-rtp",
          "media", G_TYPE_STRING, "audio",
          "clock-rate", G_TYPE_INT, 8000,
          "encoding-name", G_TYPE_STRING, "G729",
          NULL);

  g_object_set (G_OBJECT (udpsrc), 
								"port", atoi(port),
								"caps", rtp_caps,
								 NULL);
	
	
	//Attach Bus
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

	//Add Elements to Bin
  gst_bin_add_many (GST_BIN (pipeline),
                    udpsrc, rtp_depay, decoder, converter, audiosink, NULL);

	printf("ELEMENTS ADDED TO BIN\n");

	//Link Proper Capabilities
	gst_element_link_many(udpsrc, rtp_depay, decoder, converter, audiosink, NULL);

	printf("ELEMENTS LINKED PROPERLY\n");

  g_print ("Now playing from port %s\n", port);
  gst_element_set_state (pipeline, GST_STATE_PLAYING);


  g_print ("Running...\n");
  g_main_loop_run (loop);


  g_print ("Returned, stopping playback\n");
  gst_element_set_state (pipeline, GST_STATE_NULL);

  g_print ("Deleting pipeline\n");
  gst_object_unref (GST_OBJECT (pipeline));
  g_source_remove (bus_watch_id);
  g_main_loop_unref (loop);

  return 0;
}
