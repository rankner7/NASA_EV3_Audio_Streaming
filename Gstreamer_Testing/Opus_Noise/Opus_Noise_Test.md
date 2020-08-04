# BACKGROUND NOISE IMPACT OPUS CODEC

## PREPARING FILES

### Download Party Sounds File

Go to [Free Sound Party Sounds](https://freesound.org/people/FreqMan/sounds/23153/) and download the file. You may need to register to download.

Move the file to whatever directory you want to work from and rename the file "party_sounds.wav".

### Set the Raw Format

Set the format that raw audio will be converted to. The type used here is single channel, 8000 Hz, Signed 16 bit little-endian pcm:

`export RAW_TYPE="audio/x-raw,rate=8000,channels=1,format=S16LE"`

**Remember to export this environment variable in every new terminal**

### Record Clean Speech to Superimpose

`gst-launch-1.0 -v filesrc location=clean_speech.raw ! $RAW_TYPE ! audioconvert ! autoaudiosink`

### Convert Party Sounds Wav to Raw

`gst-launch-1.0 -v filesrc location= party_sounds.wav ! wavparse ! audioresample ! audioconvert ! $RAW_TYPE ! filesink location=party_sounds.raw`

### Mix Clean Speech and Party Sounds

Make sure to adjust volume of each accordingly

`gst-launch-1.0 -v filesrc location=clean_speech.raw ! $RAW_TYPE ! volume volume=3 ! audiomixer name=mix ! filesink location=party_and_clean.raw filesrc location=party_sounds.raw ! $RAW_TYPE ! volume volume=0.25 ! mix.`

Play mixed file to verify proper mixing and volume blend

`gst-launch-1.0 -v filesrc location=party_and_clean.raw ! $RAW_TYPE ! audioconvert ! autoaudiosink`

## STREAMING OVER UDP WITH OPUS

### Sender

`export RECEIVER_IP=127.0.0.1`

`gst-launch-1.0 -v filesrc location= party_and_clean.raw do-timestamp=true ! $RAW_TYPE ! opusenc bitrate=8000 ! rtpopuspay ! udpsink host=$RECEIVER_IP port=5000`

### Reciever

`export STREAM_CAPS="application/x-rtp,media=(string)audio,payload=(int)96,clock-rate=(int)48000,encoding-params=(int)2,encoding-name=(string)OPUS"`

To play out of the speakers

`gst-launch-1.0 udpsrc port=5000 caps=$STREAM_CAPS ! rtpopusdepay ! opusdec ! audioconvert ! autoaudiosink`

To record to file

`gst-launch-1.0 udpsrc port=5000 caps=$STREAM_CAPS ! rtpopusdepay ! opusdec ! audioconvert ! $RAW_TYPE ! filesink location= party_and_clean_opus.raw`
