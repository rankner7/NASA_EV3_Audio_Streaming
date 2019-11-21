import sys



file1 = input("Enter full file path of the first file:\n")
file2 = input("Enter full file path of the first file:\n")
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
	g729_packet_size = 10 #Bytes (80 bits per sample)
	data1 = data1_handle.read()
	data2 = data2_handle.read()

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
			print("Packet #%d Difference!!!\n\tData 1: %s\n\tData 2: %s"%(byte_cnt/g729_packet_size, packet1, packet2))
			no_difference = False

		byte_cnt += g729_packet_size
	if (no_difference):
		print("NO Differences Found! Files are the same")
	
else:
	print("FILE IO Error. Exiting")
	sys.exit()
