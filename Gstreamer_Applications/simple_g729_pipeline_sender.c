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

static gboolean link_elements_with_filter (GstElement *element1, GstElement *element2, GstCaps *caps) {
  
  gboolean link_ok;

  link_ok = gst_element_link_filtered (element1, element2, caps);
  gst_caps_unref (caps);

  if (!link_ok) {
		gchar *name1, name2;

		g_object_get (G_OBJECT (element1), "name", &name1, NULL);
		g_object_get (G_OBJECT (element2), "name", &name2, NULL);
 		
    g_warning ("Failed to link '%s' and '%s'!\n", name1, name2);

		g_free (name1);
		g_free (name2);
  }

  return link_ok;
}

int main (int argc, char *argv[]) {
	
	char *port, *recv_ip;

	if (argc != 3){
		printf("Insufficient number of arguements!\n");
		g_printerr ("Usage: %s <RECEIVER_IP> <PORT> \n", argv[0]);
    return -1;
	}
	else{
		recv_ip = argv[1];
		port = argv[2];
		printf("\nReceiving IP: |%s|\n", recv_ip);
		printf("Port:           |%s|\n", port);
	}
	
  GMainLoop *loop;

  GstElement *pipeline, *audiosource, *encoder, *rtp_pay, *udpsink;
  GstBus *bus;
  guint bus_watch_id;

  gst_init (NULL, NULL);

  loop = g_main_loop_new (NULL, FALSE);

	//Create Pipeline Elements
  pipeline = gst_pipeline_new ("g729_sender");
  audiosource = gst_element_factory_make ("autoaudiosrc",  "mic_src");
  encoder     = gst_element_factory_make ("g729enc",       "encoder");
  rtp_pay     = gst_element_factory_make ("rtpg729pay",    "payloader");
  udpsink     = gst_element_factory_make ("udpsink",       "sink");

	//Check proper creation
  if (!pipeline || !audiosource || !encoder || !rtp_pay || !udpsink) {
    g_printerr ("One element could not be created. Exiting.\n");
    return -1;
  }

	//Set Properties of Objects
  g_object_set (G_OBJECT (udpsink), 
								"host", recv_ip,
								"port", atoi(port),
								 NULL);
	
  bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
  gst_object_unref (bus);

  gst_bin_add_many (GST_BIN (pipeline),
                    audiosource, encoder, rtp_pay, udpsink, NULL);

	//Link Proper Capabilities
	GstCaps *raw_caps;
	raw_caps = gst_caps_new_simple ("audio/x-raw",
          "format", G_TYPE_STRING, "S16LE",
          "channels", G_TYPE_INT, 1,
          "rate", G_TYPE_INT, 8000,
          NULL);
	
	if (!link_elements_with_filter(audiosource, encoder, raw_caps)){
		printf("Caps not negotiated! Exiting\n");
		return -1;
	}

	gst_element_link_many(encoder, rtp_pay, udpsink, NULL);

  g_print ("Now playing streaming from mic to %s:%s\n", recv_ip, port);
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
