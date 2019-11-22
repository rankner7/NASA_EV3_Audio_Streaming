import struct

#Get input file to convert
input_file = input("Enter the FULL FILE PATH for the file to be converted:\n")
input_file = '/home/ronnie/NASA_EV3_Audio_Streaming/Test_Files/Raw/test1_2words_decoded.raw'
infile_handle = open(input_file, 'rb')

def convert_S16LE_to_decimal(byte_list):
	#LITTLE ENDIAN SO FIRST BYTE IS LITTLE
	unsigned_val = byte_list[0] + byte_list[1]*256
	max_val = 2**15-1 #0111|1111|1111|1111 (0x7FFF) in binary -> maximum magnitude, anything greater becomes signed
	signed_val = -((max_val + 1) & unsigned_val) + (max_val & unsigned_val) #two's compliment -> largest bit is negative. all else is positive

	#print("Converting %02x%02x -> %d"%(byte_list[1], byte_list[0], signed_val))
	return signed_val 

def display_bins(dist):
	bin_cnt = 0
	for bins in dist:
		bin_cnt += 1
		if bins > 0.000000001:
			print("Bin #%d: %f"%(bin_cnt, bins))

#Output File creation
path = input_file.split('.')[0]
file_name = path+"_NoZeros.raw"
outfile_name = file_name.split('/')
outfile_name = outfile_name[len(outfile_name)-1]
outfile_path = file_name
print("\nSaving to '%s' located at:\n%s\n"%(outfile_name, outfile_path))
outfile_handle = open(outfile_path, 'wb')

look_ahead = 240 #240 sample look ahead -> 30 ms window of zeros
silence_thresh = 100
bin_size = silence_thresh

num_bins = int((2**15-1)/bin_size)+1
dist = [0.0]*num_bins

input_data = infile_handle.read()
num_bytes = len(input_data)
num_samples = int(num_bytes/2)

original_clip_length = num_samples/8000/60

sample_cnt = 0
written_samples = 0
while sample_cnt < num_samples:
	signed_val = convert_S16LE_to_decimal(input_data[sample_cnt*2:(sample_cnt+1)*2])

	key = int(abs(signed_val)/bin_size)
	dist[key] += 1/num_samples

	if abs(signed_val) < silence_thresh:
		all_zeros = True
		if ((num_samples - sample_cnt) < look_ahead):
			break
		for ahead in range(0, look_ahead):
			signed_val = convert_S16LE_to_decimal(input_data[(sample_cnt+ahead)*2:(sample_cnt+ahead+1)*2])
			if abs(signed_val) > silence_thresh:
				all_zeros = False
				break
		if all_zeros:
			sample_cnt += look_ahead
		else:
			outfile_handle.write(bytearray(input_data[sample_cnt*2:(sample_cnt+1)*2]))
			written_samples += 1
			sample_cnt += 1
	else:
		outfile_handle.write(bytearray(input_data[sample_cnt*2:(sample_cnt+1)*2]))
		sample_cnt +=1
		written_samples += 1

#display_bins(dist)

edited_clip_length = written_samples/8000/60
print("Original Length: %f min\nEdited length: %f min"%(original_clip_length, edited_clip_length))
print("Removed %f%%"%((original_clip_length - edited_clip_length)*100/original_clip_length))

infile_handle.close()
outfile_handle.close()
