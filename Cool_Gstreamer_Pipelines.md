# Super Cool Gstreamer Pipelines

## G729 Codec and RTP over UDP

### Sender

Make sure to set $RECEIVER_IP environment variable to the receiving device's IP address

For internal data transfer: 
```shell
export RECEIVER_IP=127.0.0.1
```

*Send Audio with Anti-aliasing (I believe handled in audioresample)*

`gst-launch-1.0 -v autoaudiosrc ! audio/x-raw,rate=44100,format=F32LE,channels=1 ! audioconvert ! audio/x-raw,format=S16LE,channels=1 ! audioresample ! audio/x-raw,rate=8000 ! g729enc ! rtpg729pay ! udpsink host=$RECEIVER_IP port=5000`

### Reciever

Make sure to set $STREAM_CAPS to proper capabilities with:

```shell
export STREAM_CAPS="application/x-rtp,media=(string)audio,clock-rate=(int)8000,encoding-name=(string)G729"
```

*Receive Audio and Display FFT*

`gst-launch-1.0 udpsrc port=5000 caps=$STREAM_CAPS ! rtpg729depay ! g729dec ! tee name=t ! queue ! audioconvert ! autoaudiosink t. ! queue ! audioconvert ! audio/x-raw,format=S16LE,channels=2,rate=8000 ! spectrascope ! videoconvert ! video/x-raw,height=400,width=900 ! autovideosink`

### Modifying SSRC and Frame Count

If you're unfamiliar with RTP, take a look at the (RTP RFC)[https://tools.ietf.org/html/rfc3550]

G.729 frames are conventionally 10 ms windows. With 8 KHz 16 bit signed pcm raw audio and a compression ratio of 16, this leads to 10 Bytes of compressed audio per frame. The RTP header with no CSRCs (contributing sources), should be 12 Bytes long.

To change the frame count per packet, and thereby packetsize, the *mtu* and *min-ptime* properties of the rtpg729pay must be changed. The resulting packet size will be the **minimum of the two**. The properties are in units of ns, so 3 frames is 30 ms which is (3 * 10,000,000 ns). The min-ptime should be set to the intended packet size. mtu should be set to its maximum, which is 4294967295. min-ptime cannot exceed mtu, so the maximum frames per packet is 429. To be safe, *max-ptime* should also be set to the same value as min-ptime to ensure the packet does not go above the intended size.

On rare occasions, the pipeline may not provide enough data to fill the packet to the intended size. To prevent this, a *queue* can be put between the g729enc and rtpg729pay with the *min-threshold-bytes* property set to (frames * 10) Bytes.

SSRC is normally random, but can be set to an arbitrary number from 0 to 4294967294. 4294967295 is the default and sets the ssrc to random. To set the ssrc, just set the *ssrc* property of the rtpg729pay.

*Example*

Create an environment variable for the intended frames per packet:

`export FRAMES=7`

Create an environment variable for the SSRC:

`export SSRC=4`

Run the modified pipeline:

`gst-launch-1.0 -v autoaudiosrc ! audio/x-raw,rate=44100,format=F32LE,channels=1 ! audioconvert ! audio/x-raw,format=S16LE,channels=1 ! audioresample ! audio/x-raw,rate=8000 ! g729enc ! queue min-threshold-bytes=$(($FRAMES * 10)) ! rtpg729pay mtu=4294967295 ssrc=$SSRC min-ptime=$(($FRAMES * 10000000)) ! udpsink host=$RECEIVER_IP port=5000`

Confirm with wireshark. The resulting packet should be 82 Bytes: 70 for the 7 frames and 12 for the RTP header.


## Opus Codec and RTP over UDP
### Sender
Make sure to set $RECEIVER_IP environment variable to the receiving device's IP address

For internal data transfer: 
```shell
export RECEIVER_IP=127.0.0.1
```

*Send Audio*

`gst-launch-1.0 -v autoaudiosrc ! audio/x-raw,format=S16LE,channels=1,rate=8000 ! opusenc bitrate=8000 ! rtpopuspay ! udpsink host=$RECEIVER_IP port=5000`

### Receiver

Make sure to set $STREAM_CAPS to proper capabilities with:

```shell
export STREAM_CAPS="application/x-rtp,media=(string)audio,payload=(int)96,clock-rate=(int)48000,encoding-params=(int)2,encoding-name=(string)OPUS"
```

*Play Audio*

`gst-launch-1.0 udpsrc port=5000 caps=$STREAM_CAPS ! rtpopusdepay ! opusdec ! audioconvert ! autoaudiosink`

*Show Waveform*

`gst-launch-1.0 udpsrc port=5000 caps=$STREAM_CAPS ! rtpopusdepay ! opusdec ! queue ! wavescope ! videoconvert ! video/x-raw,width=800,height=400 ! autovideosink`

*Play Audio and Waveform simultaneously*

`gst-launch-1.0 udpsrc port=5000 caps=$STREAM_CAPS ! rtpopusdepay ! opusdec ! tee name=t ! queue ! audioconvert ! autoaudiosink t. ! queue ! wavescope ! videoconvert ! autovideosink`

## G726 Codec and RTP over UDP

### Sender

`gst-launch-1.0 -v autoaudiosrc ! audio/x-raw,format=S16LE,channels=1,rate=8000 ! avenc_g726 bitrate=8000 ! rtpg726pay ! udpsink host=$RECEIVER_IP port=5000`

### Receiver

Make sure to set $STREAM_CAPS to proper capabilities with:

```shell
export STREAM_CAPS="application/x-rtp,media=(string)audio,payload=(int)96,clock-rate=(int)8000,encoding-name=(string)G726-16"
```

*Play Audio*

`gst-launch-1.0 udpsrc port=5000 caps=$STREAM_CAPS ! rtpg726depay ! avdec_g726 ! audioconvert ! autoaudiosink`

## G726 Codec Via File

Make sure to set $RAW\_FILE, $ENCODED\_FILE, and $DECODED\_FILE environment variables to the proper files.

For example:
```shell
export RAW_FILE = test.raw
export ENCODED_FILE = test_enc_g726.gdp
export DECODED_FILE = test_dec_g726.raw
```

To record a test file, you can use your computers microphone via the following command:

`gst-launch-1.0 autoaudiosrc ! audio/x-raw,format=S16LE,channels=1,rate=8000 ! filesink location=RAW_FILE`

### Encoding and Recording Raw File

`gst-launch-1.0 filesrc location=$RAW_FILE ! audio/x-raw,format=S16LE,channels=1,rate=8000 ! avenc_g726 bitrate=8000 ! gdppay ! filesink location=$ENCODED_FILE`

### Decoding and Recording Encoded File

`gst-launch-1.0 filesrc location=$ENCODED_FILE ! gdpdepay ! avdec_g726 ! filesink location=$DECODED_FILE`

## Dad to Me Hearing Display (also live FFT diagram!)

`gst-launch-1.0 autoaudiosrc ! tee name=t ! queue ! audioconvert ! audiowsinclimit cutoff=500 length=80 ! audioconvert ! spectrascope ! videoconvert ! video/x-raw,width=900,height=400 ! autovideosink t. ! queue ! audioconvert ! spectrascope ! videoconvert ! video/x-raw,width=900,height=400 ! autovideosink`

