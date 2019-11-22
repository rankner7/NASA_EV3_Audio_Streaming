import os

file_name = input("Enter the full name for the file to be played:\n")
cmd = "gst-launch-1.0 filesrc location=%s ! audio/x-raw,rate=8000,format=S16LE,channels=1 ! audioconvert ! autoaudiosink"%(file_name)

os.system(cmd)
