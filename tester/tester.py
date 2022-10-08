import time
import serial

# configure the serial connections (the parameters differs on the device you are connecting to)
ser = serial.Serial(
    port='/dev/ttyACM0',
    baudrate=9600,                  #TODO
    parity=serial.PARITY_ODD,       #TODO
    stopbits=serial.STOPBITS_TWO,   #TODO
    bytesize=serial.SEVENBITS       #TODO
)

# Check if the serial port is open
if ser.is_open:
    print("Successfully opened serial port")
else:
    print("Error opening serial port")


write_line = "hello world!"

# Need some kind of loop to read from a .txt file
while 1 :

    # Can provide the command to exit application
    if write_line == 'exit':
        ser.close()
        exit()
    
    # All other (functional) commands
    else:

        # Need to encode line to UTF-8
        encoded = (write_line + '\r\n').encode('UTF-8')

        # Send the character to the device
        ser.write(encoded)
        print("Sent message: " + write_line)

        # Let's wait 1 second before sending 
        # another command
        time.sleep(1)