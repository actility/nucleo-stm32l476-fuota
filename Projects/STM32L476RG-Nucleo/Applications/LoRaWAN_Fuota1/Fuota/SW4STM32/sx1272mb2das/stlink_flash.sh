#!/bin/bash 

st-link_cli -P "../../Binary/SBSFU_UserApp.bin" 0x8000000 -V
rm -f "../../Binary/SBSFU_UserApp_1_3_3_0.bin"
mv "../../Binary/SBSFU_UserApp.bin" "../../Binary/SBSFU_UserApp_1_3_3_0.bin"
mv "../../Binary/UserApp.sfb" "../../Binary/UserApp_1_3_3_0.sfb"