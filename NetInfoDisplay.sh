#!/bin/bash

#
# -S 옵션은 stdio입력으로 password를 받겠다는 의미로 앞부분의 odroid가 password로 입력된다.
# 
echo odroid | sudo -S ./NetInfoDisplay

