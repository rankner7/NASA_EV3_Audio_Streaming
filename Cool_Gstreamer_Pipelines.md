# Super Cool Gstreamer Pipelines

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

