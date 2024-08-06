# netinfo_display
Net Info LCD Display.

현재 시간 및 자기자신의 IP를 표시하는 프로그램.

LCD Shield는 wiringPi를 사용하여 구현. I2C LCD는 직접 Control방식으로 구현.

wiringPi package : https://wiki.odroid.com/odroid-c4/application_note/gpio/wiringpi#tab__ubuntu_ppa

필요 package : 
```
// wiring-pi install
sudo apt install software-properties-common
sudo add-apt-repository ppa:hardkernel/ppa
sudo apt update
sudo apt install odroid-wiringpi

// package install
sudo apt install build-essential i2c-tools ethtool vim git nmap
```

ODROID에서 판매하는 제품 16x2 LCD Shield 또는 I2C LCD를 사용할 수 있도록 구현함.

Usage: ./netinfo_display [-Dawhtd]   
  -D --device        device name. (default /dev/i2c-0).   
  -a --i2c_addr      i2c chip address. (default 0x3f).   
  -w --width         lcd width.(default w = 16)   
  -h --height        lcd height.(default h = 2)   
  -t --time_offset   Display current time & time offset.(default false)   
  -d --delay         Display Switching delay (time & net info, default = 1)   

LCD Shield를 사용하는 경우 아래와 같이 사용한다.   
-t 옵션은 기준시간에서 9시간을 더함 (한국표준시), -d 2는 2초가 표시 후 전환 (시계/IP)   
sudo ./netinfo_display -t 9 -d 2   

I2C LCD를 사용시 (I2C1번 0x3f device를 사용함.)   
sudo ./netinfo_display -D /dev/i2c-1 -a 0x3f -t 9 -d 2   

