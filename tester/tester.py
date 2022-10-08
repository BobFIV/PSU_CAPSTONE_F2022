import time
import serial

# Configure the serial connections (the parameters differs on the device you are connecting to)
ser = serial.Serial(
    port='/dev/ttyACM0',
    baudrate=115200                #TODO: I believe this is correct
)

# Check if the serial port is open
if ser.is_open:
    print("Successfully opened serial port")
else:
    print("Error opening serial port")

# Open our unit testing file
file = open('unit_tests.txt', 'r')
lines = file.readlines()

# Need some kind of loop to read from a .txt file
for line in lines:

    # Can provide the command to exit application
    if line == 'exit\n':

        break
    
    # All other (functional) commands
    else:

        # Need to encode line to UTF-8
        encoded = (line + '\r\n').encode('UTF-8')

        # Send the string to the device
        ser.write(encoded)
        print("Sent message: " + line, end='')

        # Let's wait 1 second before sending 
        # another command
        time.sleep(1)

# Close the serial and file I/O
ser.close()
file.close()
print("Closed serial connection")