import sys
from random import randint

# Default number of tests we will run
testSize = 10

# First command line argument modifies test size
if len(sys.argv) > 1:
	testSize = int(sys.argv[1])

# Open the file we are writing to
file = open("unit_tests.txt", 'w')

for i in range(testSize):

	selection = randint(0, 3)

	if selection == 0:
		file.write("off\n")
	if selection == 1:
		file.write("green_on;\n")
	if selection == 2:
		file.write("yellow_on;\n")
	if selection == 3:
		file.write("red_on;\n")

file.close()