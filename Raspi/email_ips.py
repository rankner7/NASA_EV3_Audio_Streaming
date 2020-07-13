import socket #Get Local IP
import os #access environment variables
import subprocess #Get Global IP
import smtplib #email information
from email.mime.text import MIMEText #format email info


def open_smtpserver(gmail_user, gmail_password):
	smtpserver = smtplib.SMTP('smtp.gmail.com', 587) # Server to use

	smtpserver.ehlo()  # Says 'hello' to the server
	smtpserver.starttls()  # Start TLS encryption
	smtpserver.ehlo()
	smtpserver.login(gmail_user, gmail_password)  # Log in to server

	return smtpserver

send_to = 'ronnieankner@gmail.com' # Email to send to

#Gmail Credentials
# --> Make sure to have two factor verification installed
# --> set environment variable GMAIL_USERNAME to your gmail account
# --> in two factor, generate 16 Digit App password for this specific pi
# --> set environment variable GMAIL_PASSWORD to the 16-digit App Password
envi_vars = os.environ
if not(('GMAIL_USERNAME' in envi_vars) and ('GMAIL_PASSWORD' in envi_vars)):
	print("Environment Variables 'GMAIL_USERNAME' and 'GMAIL_PASSWORD' not found!")
	print("\tSet those variables and try again")
	exit(0)

gmail_user = os.environ['GMAIL_USERNAME']
gmail_pass = os.environ['GMAIL_PASSWORD']
pi_name = os.environ['USER']

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
message_text = "Global IP: %s\nLocal IP:  %s"%(ipaddr_global, ipaddr_local)
mime_msg = MIMEText(message_text)

mime_msg['Subject'] = "IP Addresses for '%s' Pi"%(pi_name)
mime_msg['From'] = gmail_user
mime_msg['To'] = send_to

# Sends the message
smtpServer = open_smtpserver(gmail_user, gmail_pass)
try:
	smtpServer.sendmail(gmail_user, [send_to], mime_msg.as_string())
except:
	print("Information in gmail file invalid!! Disabling Email Functionality")
smtpServer.quit()
