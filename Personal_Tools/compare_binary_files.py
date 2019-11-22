import sys
import struct
#================================= Macros =================================
code_word = b'\x21\x6b\x50\x00'
zero = b'\x7f\x00'
one = b'\x81\x00'

#============================== Functions =================================
def prRed(skk): print("\033[91m {}\033[00m" .format(skk), end="") 
def prGreen(skk): print("\033[92m {}\033[00m" .format(skk), end="") 
def prYellow(skk): print("\033[93m {}\033[00m" .format(skk), end="") 
def prLightPurple(skk): print("\033[94m {}\033[00m" .format(skk), end="") 
def prPurple(skk): print("\033[95m {}\033[00m" .format(skk), end="") 
def prCyan(skk): print("\033[96m {}\033[00m" .format(skk), end="") 
def prLightGray(skk): print("\033[97m {}\033[00m" .format(skk), end="") 
def prBlack(skk): print("\033[98m {}\033[00m" .format(skk), end="") 
  
def create_diff_list(ent1, ent2):
	if len(ent1) != len(ent2):
		print("CANNOT COMPARE ENTITIES -> different lengthS")
		return []
	num_elements = len(ent1)
	diff_list = []
	for i in range(0, num_elements):
		if ent1[i] == ent2[i]:
			diff_list.append(0)
		else:
			diff_list.append(1)

	return diff_list


def print_difference(byte_str1, byte_str2):
	diff_list = create_diff_list(byte_str1, byte_str2)
	if len(diff_list) == 0:
		return
	print("\tData 1: ", end="")
	for i in range(0, len(byte_str1)):
		if diff_list[i]:
			prRed("%02x"%(byte_str1[i]))
		else:
			prGreen("%02x"%(byte_str1[i]))
	print("\n")
	print("\tData 2: ", end="")
	for i in range(0, len(byte_str2)):
		if diff_list[i]:
			prRed("%02x"%(byte_str2[i]))
		else:
			prGreen("%02x"%(byte_str2[i]))
	print("\n")
		
	


#file1 = input("Enter full file path of the first file:\n")
#file2 = input("Enter full file path of the second file:\n")

file2 = '/home/ronnie/NASA_EV3_Audio_Streaming/Test_Files/Words2bits/p501_E_8kHz_16bit_ITU_enc_2bits.bit'
file1 = '/home/ronnie/NASA_EV3_Audio_Streaming/Test_Files/Encoded/p501_E_8kHz_16bit_enc.bit'

path1_names = file1.split("/")
path2_names = file2.split("/")

file1_name = path1_names[(len(path1_names)-1)]
file2_name = path2_names[(len(path2_names)-1)]

valid_data = True
try:
	data1_handle = open(file1, 'rb')
except:
	print("Error opening File 1")
	valid_data = False

try:
	data2_handle = open(file2, 'rb')
except:
	print("Error opening File 2")
	valid_data = False

if (valid_data):
	print("\n================== Analyzing ===================")
	g729_packet_size = 1 #Bytes -> default byte by byte
	data1 = data1_handle.read()
	data2 = data2_handle.read()
	
	if data1.find(code_word) != -1:
		g729_packet_size = 164 
	else:
		g729_packet_size = 10 #Bytes (80 bits per sample)

	no_difference = True
	byte_cnt = 0

	data1_size = len(data1)
	data2_size = len(data2)

	if (data1_size != data2_size):
		print("Data Sizes are Different!:\n\tFile 1: %d (%s)\n\tFile 2: %d (%s)"%(len(data1), file1_name, len(data2), file2_name))
		print("Not comparing, Exiting")
		sys.exit()

	for i in range(0,int(data1_size/g729_packet_size)):
		packet1 = data1[g729_packet_size*i:g729_packet_size*(i+1)]
		packet2 = data2[g729_packet_size*i:g729_packet_size*(i+1)]

		if packet1 != packet2:
			print("Packet #%d Difference!!!"%((byte_cnt/g729_packet_size)+1))
			print_difference(packet1, packet2)
			no_difference = False

		byte_cnt += g729_packet_size
	if (no_difference):
		print("NO Differences Found! Files are the same")
	
else:
	print("FILE IO Error. Exiting")
	sys.exit()
