#!/bin/bash - 
#prebuild script 
echo prebuild.sh : started > $1/output.txt
ecckey=$1"/../Binary/ECCKEY.txt"
echo "$ecckey" >> $1/output.txt
asmfile=$1/se_key.s
# comment this line to force python
# python is used if  executeable not found
current_directory=`pwd`
cd $1/../../../../../../Middlewares/ST/STM32_Secure_Engine/Utilities/KeysAndImages
basedir=`pwd`
cd $current_directory

# test if window executeable useable
prepareimage=$basedir/win/prepareimage/prepareimage.exe
uname  | grep -i -e windows -e mingw > /dev/null 2>&1

if [ $? == 0 ] && [  -e "$prepareimage" ]; then
  echo "prepareimage with windows executeable" >> $1/output.txt
  echo "prepareimage with windows executeable"
  cmd=""
else
  # line for python
  echo "prepareimage with python script" >> $1/output.txt
  echo "prepareimage with python script"
  prepareimage=$basedir/prepareimage.py
  cmd=python
fi

echo "$cmd $prepareimage" >> $1/output.txt
crypto_h=$1/../Inc/se_crypto_config.h

#clean
if [ -e "$1/crypto.txt" ]; then
  rm $1"/crypto.txt"
fi

if [ -e "$asmfile" ]; then
  rm $asmfile
fi

if [ -e "$1"/postbuild.sh ]; then
  rm $1"/postbuild.sh"
fi

#get crypto name
command="$cmd $prepareimage conf "$crypto_h""
echo $command
crypto=`$command`
echo $crypto > $1"/crypto.txt"
echo "$crypto selected">> $1"/output.txt"
echo $crypto selected
ret=$?
if [ $ret == "0" ]; then
    if [ $crypto == "SECBOOT_AES128_GCM_AES128_GCM_AES128_GCM" ]; then
    #AES128_GCM_AES128_GCM_AES128_GCM
        oemkey="$1/../Binary/OEM_KEY_COMPANY1_key_AES_GCM.bin"
        command=$cmd" "$prepareimage" trans -a GNU -k "$oemkey" -f SE_ReadKey -s .SE_Key_Data -e 1 -v V7M"
        echo $command
        $command > $asmfile
        ret=$?
    #The ECC keys are not needed so the SE_ReadKey_Pub is not generated
    fi

    if [ $crypto == "SECBOOT_ECCDSA_WITH_AES128_CBC_SHA256" ]; then  
        echo "ECCDSA_WITH_AES128_CBC_SHA256"
        oemkey=$1"/../Binary/OEM_KEY_COMPANY1_key_AES_CBC.bin"
        command=$cmd" "$prepareimage" trans  -a GNU -k "$oemkey" -f SE_ReadKey -s .SE_Key_Data -v V7M"
        echo $command
        $command > $asmfile
        ret=$?
        if [ $ret == 0 ]; then
            command=$cmd" "$prepareimage" trans  -a GNU -k "$ecckey" -f SE_ReadKey_Pub -e 1 -v V7M"
            echo $command
            $command >> $asmfile
            ret=$?
        fi
    fi

    if [ $crypto == "SECBOOT_ECCDSA_WITHOUT_ENCRYPT_SHA256" ]; then  
    #ECCDSA_WITHOUT_ENCRYPT_SHA256
        oemkey=""
        command=$cmd" "$prepareimage" trans  -a GNU -k "$ecckey" -f SE_ReadKey_Pub  -s .SE_Key_Data -e 1 -v V7M"
        echo $command
        $command > $asmfile
        ret=$?
    fi
fi

if [ $ret == 0 ]; then
#no error recopy post build script
    uname  | grep -i -e windows -e mingw > /dev/null 2>&1
    if [ $? == 0 ]; then
        echo "recopy postbuild.sh script with "$crypto".sh script"
        command="cat "$1"/"$crypto".sh"
        $command > "$1"/"postbuild.sh"
        ret=$?
    else
        echo "create symbolic link postbuild.sh to "$crypto".sh"
        command="ln -s ./"$crypto".sh "$1"/postbuild.sh"
        $command
        ret=$?
    fi
fi

if [ $ret != 0 ]; then
#an error
echo $command : failed >> $1/output.txt 
echo $command : failed
read -n 1 -s
exit 1
fi
exit 0

