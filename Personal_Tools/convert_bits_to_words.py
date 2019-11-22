import struct
#================================= Macros =================================
code_word = b'\x21\x6b\x50\x00'
zero = b'\x7f\x00'
one = b'\x81\x00'

#================================ Functions ===============================
def convert_byte_to_words(bite):
	if type(bite) == bytes:
		bite_val = struct.unpack('h', bite)
		
	elif type(bite) == int:
		bite_val = bite
	else:
		print("UNRECOGNIZED TYPE ", type(bite), " -> returning")
		return b''
	mask = 128 #10000000 in binary
	word_bytes = b''
	bit_list = []
	for i in range(0, 8):
		res = int(bite_val & mask) #isolates single bit
		if res == 0:
			word_bytes += zero
			bit_list.append(0)
		elif res > 0:
			word_bytes += one
			bit_list.append(1)
		else:
			print("Some how didnt get a positive number or a zero")
		mask = int(mask/2) #left shifting

	return word_bytes
		


#Get input file to convert
input_file = input("Enter the FULL FILE PATH for the file to be converted:\n")
#input_file = '/home/ronnie/NASA_EV3_Audio_Streaming/Test_Files/Encoded/p501_E_8kHz_16bit_enc.bit'
infile_handle = open(input_file, 'rb')

#Output File creation
output_dir = '/home/ronnie/NASA_EV3_Audio_Streaming/Test_Files/Bits2words/'
path = input_file.split('/')
file_name = path[len(path)-1].split('.')
outfile_name = file_name[0] + '_2words.' + file_name[1]
outfile_path = output_dir+outfile_name
print("\nSaving to '%s' located at:\n%s\n"%(outfile_name, outfile_path))
outfile_handle = open(outfile_path, 'wb')

data = infile_handle.read()

print("Input File Size:  %d"%(len(data)))
byte_cnt = 0
out_bytes = 0

for bite in data:
	if byte_cnt%10 == 0:
		#beginning of a new packet
		outfile_handle.write(code_word)
		out_bytes += 4

	word_bytes = convert_byte_to_words(bite)
	outfile_handle.write(word_bytes)
	out_bytes += 16
	byte_cnt += 1

print("Output File Size: %d"%(out_bytes))

outfile_handle.close()
infile_handle.close()


