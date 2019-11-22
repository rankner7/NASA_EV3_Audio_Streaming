import struct

#File converts ITU 16 word files to bits

def convert_bit_list_to_bytearr(bit_list):
	if len(bit_list)%8 != 0:
		print("List is not multiple of 4!!! CANNOT CONVERT TO BYTES")
		return b''
	byte_list = []
	for i in range(0, int(len(bit_list)/8)):
		bite = bit_list[i*8:(i+1)*8]
		mult = 128
		bit_sum = 0
		for bit in bite:
			bit_sum += int(bit*mult)
			mult = int(mult/2)
		byte_list.append(bit_sum)
	return bytearray(byte_list)
			
	
#Get input file to convert
input_file = input("Enter the FULL FILE PATH for the file to be converted:\n")
#input_file = '/home/ronnie/NASA_EV3_Audio_Streaming/ITU_G729_Source_Code/g729/test_vectors/speech.bit'
infile_handle = open(input_file, 'rb')

#Output File creation
output_dir = '/home/ronnie/NASA_EV3_Audio_Streaming/Test_Files/Words2bits/'
path = input_file.split('/')
file_name = path[len(path)-1].split('.')
outfile_name = file_name[0] + '_2bits.' + file_name[1]
outfile_path = output_dir+outfile_name
print("\nSaving to '%s' located at:\n%s\n"%(outfile_name, outfile_path))
outfile_handle = open(outfile_path, 'wb')


code_word = b'\x21\x6b\x50\x00'
zero = b'\x7f\x00'
one = b'\x81\x00'

data_size = 16 #16 bit words
sample_index = int(data_size/8) #index through data 

input_bytes = infile_handle.read()
packets = input_bytes.split(code_word)
print("Input File:  %d Bytes"%(len(input_bytes)))

output_size = 0
packet_cnt = 0
for packet in packets:
	if packet:
		packet_cnt += 1
		bin_list = []
		if len(packet) == sample_index*80:
			for samp in range(0, 80):
				sample = packet[samp*sample_index:(samp+1)*sample_index]
				if sample == one:
					bin_list.append(1)
				elif sample == zero:
					bin_list.append(0)
				else:
					print("UNRECOGNIZED VALUE: ", sample, " | Packet #%d, Sample #%d"%(packet_cnt, samp+1))
		bites = convert_bit_list_to_bytearr(bin_list)
		output_size += len(bites)
		outfile_handle.write(bites)

print("Output File: %d Bytes"%(output_size))
outfile_handle.close()
infile_handle.close()
			

