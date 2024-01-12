#!/bin/bash

#
# -S 옵션은 stdio입력으로 password를 받겠다는 의미로 앞부분의 odroid가 password로 입력된다.
# 
# 이 파일의 permission은 반드시 실행가능하게 되어있어야 한다.
# chmod 755
#
./netinfo_display -t 9 -d 2
# echo odroid | sudo -S ./netinfo_display -D /dev/i2c-1 -a 0x3f -t 9 -d 2

