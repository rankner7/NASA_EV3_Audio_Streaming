import socket #Get Local IP
import os #access environment variables
import subprocess #Get Global IP

#Get Global IP
ipaddr_global = subprocess.Popen(['curl', 'ifconfig.me'], stdout=subprocess.PIPE).communicate()[0]
ipaddr_global = ipaddr_global.decode('utf-8')

#Get Local IP
ipaddr_local = ipaddr = socket.gethostbyname(socket.gethostname())
if '127.0' in ipaddr_local:
	print("Found Local Host, checking another method")
	ipaddr_local = subprocess.Popen(["hostname", "-I"], stdout=subprocess.PIPE).communicate()[0].decode('utf-8').split('\n')[0].strip()
	print("IP Address from hostname: |%s|"%(ipaddr_local))

#Form Message
print("Global IP: %s\nLocal IP:  %s"%(ipaddr_global, ipaddr_local))
