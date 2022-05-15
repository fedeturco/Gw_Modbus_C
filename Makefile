
# Compilatore arm Raspberry-Pi
CC_CROSS = /opt/pi/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc
CC_CROSS_FLAGS = -g -std=c99

# Root filesystem Raspberry
SYSROOT_CROSS = /opt/pi/tools/arm-bcm2708/arm-rpi-4.9.3-linux-gnueabihf/arm-linux-gnueabihf/sysroot

# Credenziali raspberry
TARGET_USER = pi
TARGET_PASSWD = RaspDemo15
TARGET_IP_ADDRESS = 192.168.1.149

make:
	gcc src/main.c src/crc.c src/config.c -o build/gwModbus

cross:
	$(CC_CROSS) main.c crc.c config.c $(CC_CROSS_FLAGS) -o build/gwModbus --sysroot=$(SYSROOT_CROSS)

install:
	sudo cp build/gwModbus /usr/bin/gwModbus
	sudo mkdir -p /etc/gwModbus
	sudo cp config_files/config.ini /etc/gwModbus/gwModbus.ini

copy:
	scp build/gwModbus $(TARGET_USER)@$(TARGET_IP_ADDRESS):/usr/bin/
	scp config_files/gwModbus.ini $(TARGET_USER)@$(TARGET_IP_ADDRESS):/etc/gwModbus/

clean:
	rm build/gwModbus