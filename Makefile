
make:
	gcc main.c -o gwModbus

install:
	sudo cp gwModbus /usr/bin/gwModbus
	sudo mkdir -p /etc/gwModbus
	sudo cp config.ini /etc/gwModbus/gwModbus.ini

clean:
	rm gwModbus