import sys
import time
import serial

class Thingy:

    def __init__(self, sleep_time=1.0):
        self.ser = serial.Serial(
            port='/dev/ttyACM0',
            baudrate=1000000
        )

        # Check if the serial port is open
        if self.ser.is_open:
            print("Successfully opened serial port")
        else:
            print("Error opening serial port")
        
        # How long we will sleep in between serial calls
        self.sleep_time = sleep_time 

    # Accepts a file descriptor and runs commands from that
    def run_test_file(self, fd):

        # Open our unit testing file
        file = open(fd, 'r')
        lines = file.readlines()

        # Need loop to read from a .txt file
        for line in lines:

            # Can provide the command to exit application
            if line == 'exit\n':

                break
            
            # All other (functional) commands
            else:

                # Need to encode line to UTF-8
                encoded = (line + '\r\n').encode('UTF-8')

                # Send the string to the device
                self.ser.write(encoded)
                print("Sent message: " + line, end='')

                # Let's wait 1 second before sending 
                # another command
                time.sleep(self.sleep_time)

        file.close()


    # Accepts a custom command to send to the thingy
    def run_test(self, command):

        encoded = (command + '\r\n').encode('UTF-8')
        self.ser.write(encoded)
        print("Sent message: " + command, end='\n')

        # Not sure if this is necessary, but give a
        # bit of buffer room just in case
        time.sleep(self.sleep_time)

    def close(self):

        # Close the serial
        self.ser.close()
        print("Closed serial connection")

if __name__ == "__main__":

    tester = Thingy()

    tester.run_test_file("unit_tests.txt")

    print("\nSending custom commands")

    tester.run_test("Hello;")
    tester.run_test("World;")
    tester.run_test("This;")
    tester.run_test("Is;")
    tester.run_test("A;")
    tester.run_test("Test;")

    tester.close()