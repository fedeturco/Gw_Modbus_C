# Gw Modbus TCP-RTU

          TCP
           |
       -------------
       | GW Modbus |
       -------------
           |
    ----------------------------------- 485 - RTU
     |            |                 |
    Slave ID 1   Slave ID 2        Slave ID N
   
La configurazione avviene tramite il file /etc/gwModbus/gwModbus.ini riportato a seguire.
  
## File di configurazione: gwModbus.ini

verbose = 2
# 0 -> No output
# 1 -> Info
# 2 -> TX, RX bytes
# 3 -> All 

[tcp]
address = 0.0.0.0
port    = 504

[serial]
device          = /dev/ttyUSB0
baud            = 9600
configuration   = 8N1
timeout         = 2000

tty_VTIME       = 1
tty_VMIN        = 0    
