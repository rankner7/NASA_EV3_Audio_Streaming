#!/bin/bash
ubuntu='Ubuntu'
red_hat='redhat'
debian='Debian'
raspi='Raspbian'

function install_gcc {
	case $1 in
	$ubuntu|$debian)
		echo " Installing gcc for Debian/Ubuntu"
		apt install build-essential
		;;
	$red_hat)
		echo " Installing gcc for Red Hat/CentOS"
		yum group install "Development Tools"
		;;
	$raspi)
		echo " Installing gcc for Raspbian"
		apt install build-essential
		;;
	esac	
}

function install_gstreamer {
	case $1 in
	$ubuntu|$debian)
		echo " Installing Gstreamer-1.0 for Debian/Ubuntu"
		apt-get install libgstreamer1.0-0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio
		;;
	$red_hat)
		echo " Installing Gstreamer-1.0 for Red Hat/CentOS"
		yum install epel-release
		yum install snapd
		systemctl enable --now snapd.socket
		ln -s /var/lib/snapd/snap /snap
		sudo snap install gstreamer --edge
		;;
	$raspi)
		echo " Installing Gstreamer for Raspbian"
		apt-get install gstreamer-1.0-tools
		;;
	esac	
}

function install_git {
	case $1 in
	$ubuntu|$debian)
		echo " Installing Git for Debian/Ubuntu"
		apt install git
		;;
	$red_hat)
		echo " Installing Git for Red Hat/CentOS"
		yum install git
		;;
	$raspi)
		echo " Installing Git for Raspbian"
		apt install git
		;;
	esac	
}

#======= Enforce Root User ===============
if [[ "$EUID" -ne 0 ]]; then
	echo "Please run with sudo :)"
	exit 0
fi

#======= Get Operating System Info ===============
os_dist=$(cat /etc/*-release)
os_name=''

echo "Finding Operating System"

for OS_NAME in $ubuntu $debian $red_hat $raspi
do
	if [[ "$os_dist" == *"$OS_NAME"* ]]; then
		echo "You have $OS_NAME"
		os_name=$ubuntu
	else
		echo "  You do not have $OS_NAME"
	fi
done

#======= Check GCC Existence ===============
gcc_out=$(gcc --version)
good_check='Copyright (C)'

echo ""
if [[ "$gcc_out" == *"$good_check"* ]]; then
	echo "You have GCC ------------> Good!"
else
	echo "You do not have GCC. I am installing it for you"
	install_gcc $os_name
fi

#====== Check Gstreamer-1.0 Existence ======
gst_out=$(gst-launch-1.0 --gst-version)
good_check='GStreamer Core Library'

if [[ "$gst_out" == *"$good_check"* ]]; then
	echo "You have Gstreamer-1.0 --> Good!"
else
	echo "You do not have Gstreamer-1.0. I am installing it for you"
	install_gstreamer $os_name
fi

#============ Check Git ===================
git_out=$(git --version)
good_check='git version'

if [[ "$git_out" == *"$good_check"* ]]; then
	echo "You have Git ------------> Good!"
else
	echo "You do not have Git --> I am installing it for you"
	install_gstreamer $os_name
fi

echo $HOME
echo "=============== REVOKING SUDO ACCESS ======================"
sudo -k
echo $HOME
exit 0
#======= Make Folder for all code and download =========
code_folder="/gstreamer_client"
g729_plugin_folder="/g729_plugin"
g_client_folder="/g_client"
g_client_code="/Gstreamer_Applications/gstreamer_client.c"

echo ""
echo "Making Overall Folder for code: ${HOME}${code_folder}"
mkdir ${HOME}${code_folder}

echo "  Making G729 Plugin Folder: ${HOME}${code_folder}${g729_plugin_folder}"
mkdir ${HOME}${code_folder}${g729_plugin_folder}

echo "  Cloning G729 Plugin Code to ${HOME}${code_folder}${g729_plugin_folder}"
echo ""
echo " ================ G729 Plugin Clone ================== "
git clone --recursive https://github.com/sdroege/gladstone.git ${HOME}${code_folder}${g729_plugin_folder}
echo " ================ G729 Plugin Clone ================== "

echo ""
echo "  Making Temp Gstreamer Client directory ${HOME}${code_folder}${g_client_folder}"
mkdir ${HOME}${code_folder}${g_client_folder}

echo "  Cloning Gstreamer Client Code to ${HOME}${code_folder}${g_client_folder}"
echo ""
echo " ================ Gstreamer Client Clone ================== "
git clone --recursive https://github.com/rankner7/NASA_EV3_Audio_Streaming.git ${HOME}${code_folder}${g_client_folder}
echo " ================ Gstreamer Client Clone ================== "

echo ""
echo "  Extracting only gstreamer_client.c to ${HOME}${code_folder}"
echo "  Removing Temp Folder: ${HOME}${code_folder}${g_client_folder}"
mv ${HOME}${code_folder}${g_client_folder}${g_client_code} ${HOME}${code_folder}
rm -r ${HOME}${code_folder}${g_client_folder}

#======== Install G729 Plugin ==============
cd ${HOME}${code_folder}${g729_plugin_folder}
echo ""
echo "Installing G729 Plugin"

config_file="/configure"
autogen_file="/autogen.sh"

echo "================= Running autogen.sh ==============================="
.${autogen_file}

echo ""
echo "=========== Running configure with ITU source code download ==========="
.${config_file} --enable-refcode-download

echo ""
echo "=================== Running make ==============================="
make

echo ""
echo "=================== Running make install ==============================="
install_res=$(make install)
cut1=${install_res#*----------------------------------------------------------------------}
cut2=${cut1%If*}
plugin_install_loc=${cut2#*in:}

echo "============= Linking Plugin Library ============================="
#Libraries have been installed in:
#   /usr/local/lib/gstreamer-1.0
find_loc=$(find $plugin_install_loc -name libgstg729.so)

if [[ ${#find_loc} -gt 0 ]]; then
	plugin_install_loc=${find_loc%/*} #gets rid of any weird formatting
	echo "Found the Plugin Directory! --> $plugin_install_loc"
else
	echo "Could Not Find Plugin Directory --> Exiting"
	exit 0
fi

#set plugin path to above location to ensure
export GST_PLUGIN_PATH=$plugin_install_loc
echo "setting GST_PLUGIN_PATH=$GST_PLUGIN_PATH"
echo " --> Adding 'export GST_PLUGIN_PATH=$GST_PLUGIN_PATH' to .bashrc"
cd
echo "export GST_PLUGIN_PATH=$GST_PLUGIN_PATH" >> .bashrc

gst_out=$(gst-inspect-1.0 | grep g729enc)

if [[ ${#gst_out} -gt 0 ]]; then
	echo "Plugin linked properly!"
else
	echo "Error linking and finding plugin :/"
	exit 0
fi

echo "================ Compiling Gstreamer Client ================"
cd ${HOME}${code_folder}
complile_out=$(gcc gstreamer_client.c -o G_CLIENT -lpthread $(pkg-config --cflags --libs gstreamer-1.0))
if [[ ${#compile_out} -gt 0 ]]; then
	echo "======================= Compilation Output Begin ==============="
	echo $compile_out
	echo "======================= Compilation Output End ==============="
	echo "Error compile, something went wrong"
	echo "Diagnose above output and adjust accordingly"
else
	echo "COMPILATION SUCCEDED!"
	echo "To use just type ./G_CLIENT from ${HOME}${code_folder}"
fi




