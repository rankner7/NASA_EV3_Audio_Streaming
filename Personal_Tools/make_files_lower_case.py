import os

file_location = input("Enter full file path for folder to correct files, or type 'here' for current folder\n")
file_location = str(file_location)
pwd_out = True

if file_location == "here":

	pwd_out_file = "/home/pi/pwd_out.txt"
	cmd = "pwd > " + pwd_out_file
	os.system(cmd)

	ls_out_file = "/home/pi/ls_out.txt"
	cmd = "ls > " + ls_out_file
	os.system(cmd)

	pwd_in = open(pwd_out_file, 'r')
	ls_in = open(ls_out_file, 'r')

	pwd = pwd_in.read().split('\n')[0]
	ls_str = ls_in.read()
	ls_lst = ls_str.split('\n')

else:
	pwd = file_location
	pwd_out = False
	
	ls_out_file = "/home/pi/ls_out.txt"
	cmd = "ls " + pwd + " > " + ls_out_file
	os.system(cmd)
	
	ls_in = open(ls_out_file, 'r')
	ls_lst = ls_in.read().split('\n')

print("Number of files in the folder %s: %d"%(pwd, len(ls_lst)))
cnt = 1
correction_lst = []
for files in ls_lst:
	if (len(files) > 0 and files != "make_files_lower_case.py" and files.islower() == False):
		name = files
		correction_lst.append(name)
		print("Correcting File #%d: %s --> %s"%(cnt, name, name.lower()))
		cnt+=1
if (cnt == 1):
	print("No files to change")
else:
	ans = input("Enter y to continue and make changes| n to quit: ")
	if (ans == 'y'):
		for files in correction_lst:
			name = files
			corrected_name = files.lower()
			name = pwd + "/" + name
			corrected_name = pwd + "/" + corrected_name
			cmd = "mv " + name + " " + corrected_name
			print("Running: %s"%(cmd))
			os.system(cmd)
if pwd_out:
	pwd_in.close()
	cmd = "rm -r " + pwd_out_file
	os.system(cmd)
	
ls_in.close()
cmd = "rm -r " + ls_out_file
os.system(cmd)

