
make:
	gcc main.c -o gwModbus

copy:
	echo "Still to implement"

install:
	sudo cp gwModbus /usr/bin/gwModbus
	sudo mkdir -p /etc/gwModbus
	sudo cp config.ini /etc/gwModbus/gwModbus.ini

clean:
	rm gwModbus