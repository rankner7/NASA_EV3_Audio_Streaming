# G.729 and Gstreamer Tools for NASA JSC EV3

## Overview

The goal of this project and repository is to create a full G.729 audio hub for general use on computers and embedded systems, such as the raspberry pi. G.729 is a low bitrate, low loss, high intelligibility audio codec used for most communication between the International Space Station (ISS), ground, and visiting vehicles.

Due to licensising issues, G.729 was long shrouded in a proprietary cloud. However, in recent years, those restrictions have lifted and G.729 is open for general use. The International Telecommunication Union (ITU) G.729 source C code can be found at https://www.itu.int/rec/T-REC-G.729.

The solution presented in this repository attempts to provide a lightweight, open-source G.729 streaming solution of UDP using RTP. The project employs:
1. **Gstreamer** - dataflow, RTP, UDP
2. **Github Gladstone Project** - G.729 encoder and decoder plugin for Gstreamer
3. **ITU Source Code** - base C code that the G.729 plugin wraps around
4. Base C sockets and POSIX threads

Gstreamer forms the backbone of this project as it massively facilitates low-latency multimedia streaming. For a quick look at some useful command line audio pipelines using gstreamer, check out *Cool_Gstreamer_Pipelines*. This contains everything from G.729 send and recieve pipelines to real time FFT comparisons between a filtered and unfilter audio stream.

The full custom G.729 Hub solution is contained in **Gstreamer_Application** and is named *gstreamer_client.c*. This script allows any IP-bearing machine running this code to connect to another machine running the code and establish send and/or recieve G.729 encoded RTP/UDP audio pipelines. The script automatically handles connections from other clients with a socket server running in the background of the script. Connections with other clients is handled by a socket client running in the foreground waiting for the user to input the other client's IP and mode audio transmission. The mode can be 'listen', 'stream' or both 'listen and stream'. Dropped connections and pipeline cleanup is also handled. The script is highly threaded both with the socket server and client operations, and with Gstreamer.

## Installing

In order to install and run the script, copy the 'install\_G\_CLIENT.sh' into whatever .sh file you want and wherever you want. The install script will automatically put the code in a generated folder in your home directory, so location of the install script is unimportant. From the directory of the script, make the script executable with:

`chmod +x <script_name>.sh`

Then run the script with:

`./<script_name>.sh`

Follow the prompts on the screen. You may have to run the script more than once depending on your current Gstreamer configuration. Perinent environment variables will be added to your *.bashrc* file. On some operating systems, the environment variable setting within the install script does not apply to the user space. To avoid any uneccessary errors, **make sure to run the code in a fresh terminal**.

For transparency, the install script does the following steps:
1. Checks for OS Type
2. Checks for the following packages and installs if missing:
  * gcc, git, gstreamer-1.0, automake, autoconf, libtools
3. Clones the G.729 Github, downloads the ITU C code, and installed the custom plugin
4. Clones this github page, extracts *gstreamer_client.c*, compiles the script, and removes the rest
5. Sets the GST_PLUGIN path to include the custom G.729 plugin and adds the environment variable to the *.bashrc*

## Other Folders in this Repo

1. **Gstreamer_Testing** - Folder dedicated to various tests run on capabilities of gstreamer and the script. Contains overview readmes of testing procedures, pictures, and pertinent files
2. **ITU\_G729\_Source\_Code** - exactly what it says --> ITU c source code. Not really necessary to the project anymore, but is a useful reference
3. **Raspi** - Raspberry pi specific scripts
4. **Personal\_Tools** - miscellaneous software tools created to help with the project. Some are just generally useful
5. **Test\_Files** - raw and G.729 encoded audio test files
6. **Gstreamer\_Plugins** - empty currently but may be used in the future for custom plugins

## Future Work

1. GUI Frontend for easier connection management with other clients
2. SIP frontend to wrap around Gstreamer
3. Full testing of how many simultaenous streams can be handled at once and weave in *audiomixer* if needed
4. Make Windows capable --> batch script needs to be written and POSIX threads need to be replaced with windows-friendly threads


