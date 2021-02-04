#!/bin/bash - 
#Post build for SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256
# arg1 is the build directory
# arg2 is the elf file path+name
# arg3 is the bin file path+name
# arg4 is the version
# arg5 when present forces "bigelf" generation
projectdir=$1
FileName=${3##*/}
execname=${FileName%.*}
elf=$2
bin=$3
version=$4

SecureEngine=${0%/*} 

userAppBinary=$projectdir"/../Binary"

sfu=$userAppBinary"/"$execname".sfu"
sfb=$userAppBinary"/"$execname".sfb"
sign=$userAppBinary"/"$execname".sign"
headerbin=$userAppBinary"/"$execname"sfuh.bin"
bigbinary=$userAppBinary"/SBSFU_"$execname".bin"

iv=$SecureEngine"/../Binary/iv.bin"
oemkey=$SecureEngine"/../Binary/OEM_KEY_COMPANY1_key_AES_CBC.bin"
ecckey=$SecureEngine"/../Binary/ECCKEY.txt"
current_directory=`pwd`
cd  $SecureEngine"/../../"
SecureDir=`pwd`
cd $current_directory
sbsfuelf="$SecureDir/2_Images_SBSFU/SW4STM32/2_Images_SBSFU/Debug/SBSFU.elf"

current_directory=`pwd`
cd $1/../../../../../../Middlewares/ST/STM32_Secure_Engine/Utilities/KeysAndImages
basedir=`pwd`
cd $current_directory
# test if window executeable useable
prepareimage=$basedir"/win/prepareimage/prepareimage.exe"
uname | grep -i -e windows -e mingw > /dev/null 2>&1

if [ $? == 0 ] && [   -e "$prepareimage" ]; then
  echo "prepareimage with windows executeable"
  export PATH=$basedir"\win\prepareimage";$PATH > /dev/null 2>&1
  cmd=""
  prepareimage="prepareimage.exe"
else
  # line for python
  #. $basedir/venv/bin/activate
  echo "prepareimage with python script"
  prepareimage=$basedir/prepareimage.py
  cmd="python"
fi

echo "$cmd $prepareimage" >> $1/output.txt
# Make sure we have a Binary sub-folder in UserApp folder
if [ ! -e $userAppBinary ]; then
mkdir $userAppBinary
fi


command=$cmd" "$prepareimage" enc -k "$oemkey" -i "$iv" "$bin" "$sfu
$command > "$projectdir"/output.txt
ret=$?
if [ $ret == 0 ]; then
  command=$cmd" "$prepareimage" sha256 "$bin" "$sign
  $command >> $projectdir"/output.txt"
  ret=$?
  if [ $ret == 0 ]; then 
    command=$cmd" "$prepareimage" pack -k "$ecckey" -r 28 -v "$version" -i "$iv" -f "$sfu" -t "$sign" "$sfb" -o 512"
    $command >> $projectdir"/output.txt"
    ret=$?
    if [ $ret == 0 ]; then
      command=$cmd" "$prepareimage" header -k  "$ecckey" -r 28 -v "$version"  -i "$iv" -f "$sfu" -t "$sign" -o 512 "$headerbin
      $command >> $projectdir"/output.txt"
      ret=$?
      if [ $ret == 0 ]; then
        command=$cmd" "$prepareimage" merge -v 0 -e 1 -i "$headerbin" -s "$sbsfuelf" "$elf" "$bigbinary
        $command >> $projectdir"/output.txt"
        ret=$?
        if [ $ret == 0 ] && [ $# == 5 ]; then
          echo "Generating the global elf file SBSFU and userApp"
          echo "Generating the global elf file SBSFU and userApp" >> $projectdir"/output.txt"
          uname | grep -i -e windows -e mingw > /dev/null 2>&1
          if [ $? == 0 ]; then
            # Set to the default installation path of the Cube Programmer tool
            # If you installed it in another location, please update PATH.
            PATH="C:\\Program Files\\STMicroelectronics\\STM32Cube\\STM32CubeProgrammer\\bin";$PATH > /dev/null 2>&1
            programmertool="STM32_Programmer_CLI.exe"
          else
            echo "fix access path to STM32_Programmer_CLI.exe "
          fi
          command=$programmertool" -ms "$elf" "$headerbin" "$sbsfuelf
          $command >> $projectdir"/output.txt"
          ret=$?
        fi
      fi
    fi
  fi
fi

if [ $ret == 0 ]; then
  rm $sign
  rm $sfu
  rm $headerbin
  #deactivate
  exit 0
else 
  echo "$command : failed" >> $projectdir"/output.txt"
  if [ -e  "$elf" ]; then
    rm  $elf
  fi
  if [ -e "$elfbackup" ]; then 
    rm  $elfbackup
  fi
  echo $command : failed
  read -n 1 -s
  #deactivate
  exit 1
fi
