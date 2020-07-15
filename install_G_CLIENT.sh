#!/bin/bash
declare -a OSArray=("Ubuntu" "Debian" "Red Hat" "Raspbian")
declare -a GstPkgArray=("libgstreamer1.0-0" "gstreamer1.0-plugins-base" "gstreamer1.0-plugins-good" "gstreamer1.0-plugins-bad" "gstreamer1.0-plugins-ugly" "gstreamer1.0-libav" "gstreamer1.0-doc" "gstreamer1.0-tools" "gstreamer1.0-x" "gstreamer1.0-alsa" "gstreamer1.0-gl" "gstreamer1.0-gtk3" "gstreamer1.0-qt5" "gstreamer1.0-pulseaudio" "libgstreamer-plugins-base1.0-dev" "libgstreamer-plugins-good1.0-0")
declare -a PluginTools=("automake" "autoconf" "libtool" "pkg-config")

function install_gcc {
	pkg_name="GCC"
	echo " Installing $pkg_name for ${OSArray[$1]}"
	case $1 in
	 0|1|3)
		install_res=$(apt install -y build-essential)
		;;
	2)
		install_res=$(yum group install "Development Tools")
		;;
	esac

	if [[ "$install_res" == *"newly installed"* ]]; then
		echo " --> $pkg_name Installed Properly"
	else
		echo "ERROR Installing $pkg_name!!!"
		echo "============ $pkg_name Error Report ================="
		echo $(install_res)
		echo "============ $pkg_name Error Report ================="
		exit 0
	fi	
}

function install_gstreamer_by_tool {
	case $1 in
	0|1|3)
		echo "Installing Gstreamer piece by piece"
		for TOOL_NAME in "${GstPkgArray[@]}"; do
			install_res=$(apt-get install -y $TOOL_NAME)
			if [[ "$install_res" == *"newly installed"* ]]; then
				echo " --> Good: $TOOL_NAME"
			else
				echo "ERROR INSTALLING $TOOL_NAME"
			fi
		done
		;;
	2)
		install_gstreamer $1
		;;
	esac
		
}

function install_gstreamer {
	pkg_name="Gstreamer-1.0"
	echo " Installing $pkg_name for ${OSArray[$1]}"
	case $1 in
	0|1)
		install_res=$(apt-get install -y libgstreamer1.0-0 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-doc gstreamer1.0-tools gstreamer1.0-x gstreamer1.0-alsa gstreamer1.0-gl gstreamer1.0-gtk3 gstreamer1.0-qt5 gstreamer1.0-pulseaudio)
		;;
	2)
		yum install epel-release
		yum install snapd
		systemctl enable --now snapd.socket
		ln -s /var/lib/snapd/snap /snap
		sudo snap install gstreamer --edge
		;;
	3)
		install_res=$(apt-get install -y gstreamer1.0-tools)
		;;
	esac
	
	if [[ "$install_res" == *"newly installed"* ]]; then
		echo " --> $pkg_name Installed Properly"
	else
		echo "ERROR Installing $pkg_name!!!"
		echo "============ $pkg_name Error Report ================="
		echo $(install_res)
		echo "============ $pkg_name Error Report ================="
		case $1 in
		0|1|3)
			install_gstreamer_by_tool $1
			;;
		2)
			echo "Don't know what to do for Red Hat in this situation :/"
			;;
		esac
	fi	
}

function install_git {
	pkg_name="Git"
	echo " Installing $pkg_name for ${OSArray[$1]}"
	case $1 in
	0|1|3)
		install_res=$(apt install -y git)
		;;
	2)
		install_res=$(yum install git)
		;;
	esac
	if [[ "$install_res" == *"newly installed"* ]]; then
		echo " --> $pkg_name Installed Properly"
	else
		echo "ERROR Installing $pkg_name!!!"
		echo "============ $pkg_name Error Report ================="
		echo $(install_res)
		echo "============ $pkg_name Error Report ================="
		exit 0
	fi	
}

function install_plugin_tool {
	pkg_name=$2
	echo " Installing $pkg_name for ${OSArray[$1]}"
	case $1 in
	0|1|3)
		install_res=$(apt-get install -y $pkg_name)
		;;
	2)
		install_res=$(yum group install "Development Tools")
		;;
	esac
	if [[ "$install_res" == *"newly installed"* ]]; then
		echo " --> $pkg_name Installed Properly"
	else
		echo "ERROR Installing $pkg_name!!!"
		echo "============ $pkg_name Error Report ================="
		echo $(install_res)
		echo "============ $pkg_name Error Report ================="
		exit 0
	fi	
}


#======= Enforce Root User ===============
if [[ "$EUID" -ne 0 ]]; then
	echo "Please run with sudo :)"
	exit 0
fi

#======= Get Operating System Info ===============
os_dist=$(cat /etc/*-release)
os_name=-1

echo "Finding Operating System"

for OS_NAME_IND in "${!OSArray[@]}"
do
	if [[ "$os_dist" == *"${OSArray[OS_NAME_IND]}"* ]]; then
		echo "You have ${OSArray[OS_NAME_IND]}"
		os_name=$OS_NAME_IND
	else
		echo "  You do not have ${OSArray[OS_NAME_IND]}"
	fi
done

if [[ $os_name -lt 0 ]]; then
	echo "COULD NOT DETERMINE OS --> exiting"
	exit 0
fi

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

echo " Checking for important Gstreamer libraries"

declare -a GstLibs=("gstreamer1.0-pulseaudio" "gstreamer1.0-plugins-base" "gstreamer1.0-plugins-good" "gstreamer1.0-plugins-bad" "gstreamer1.0-plugins-ugly" "libgstreamer-plugins-base1.0-dev" "libgstreamer1.0-dev")
search_out=$(dpkg -l | grep gstreamer)
all_exist=1
for GST_LIB in "${GstLibs[@]}"; do
	if [[ "$search_out" == *"$GST_LIB"* ]]; then
		echo "  $GST_LIB --> Good!"
	else
		echo " COULD NOT FIND $GST_LIB"
		all_exist=0
	fi
done

search_out=$(dpkg -L libgstreamer1.0-dev | grep gstbin.h)
if [[ ${#search_out} -eq 0 ]]; then
	echo "GST_BASE directory not found or library not installed"
	all_exist=0
else
	gst_base=${search_out%gstreamer-1.0*}
	echo "   GST_BASE found --> $gst_base"
	
fi 

search_out=$(dpkg -L libgstreamer-plugins-base1.0-dev | grep audio.h)
if [[ ${#search_out} -eq 0 ]]; then
	echo "GST_PLUGINS_BASE directory not found or library not installed"
	all_exist=0
else
	gst_plugins_base=${search_out%gstreamer-1.0*}
	echo "   GST_PLUGINS_BASE found --> $gst_plugins_base"
	
fi 

gst_both=""
if [[ "$gst_plugins_base" == "$gst_base" ]]; then
	echo "  Base and plugins path the same --> consolidating"
	gst_both=$gst_base
fi

if [[ $all_exist -eq 0 ]]; then
	install_gstreamer_by_tool $os_name
	echo "PLEASE RUN AGAIN --> Bye :p"
	exit 0
fi

#============ Check Git ===================
git_out=$(git --version)
good_check='git version'

if [[ "$git_out" == *"$good_check"* ]]; then
	echo "You have Git ------------> Good!"
else
	echo "You do not have Git --> I am installing it for you"
	install_git $os_name
fi

#============== Check Plugin Tools =================
for PLUGIN in "${PluginTools[@]}"; do
	exist_query=$(dpkg -l ${PLUGIN})
	if [[ "${exist_query}" == *"<none>"* ]]; then
		echo "You do not have $PLUGIN --> I am installing it for you"
		install_plugin_tool $os_name $PLUGIN
	else
		echo "You have $PLUGIN -------> Good!"
	fi
done

if [[ $os_name -eq 3 ]]; then
	HOME=/home/pi
fi


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
if [[ ${#gst_both} -eq 0 ]]; then
	.${config_file} --enable-refcode-download GST_LIBS="${gst_base};${gst_plugins_base}"
else
	.${config_file} --enable-refcode-download  GST_LIBS=${gst_both}
fi
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
	echo "Could Not Find Plugin Directory --> Removing Folder and Exiting"
	rm -r ${HOME}${code_folder}
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
	echo "Error linking and finding plugin :/--> Removing Folder and Exiting"
	rm -r ${HOME}${code_folder}
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




