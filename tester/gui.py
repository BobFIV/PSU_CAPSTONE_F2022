from tkinter import *
from tkinter import ttk
import tester
import threading

# Our tester
thingy = tester.Thingy()

# Tkinter requires a root node that all
# child elements will attach to. This is our 'window'
root = Tk()
root.title("Thingy:91 upper tester")

# In the window, we have a frame we want to look at
mainframe = ttk.Frame(root, padding="3 3 12 12")
# Organizing scheme is a grid
mainframe.grid(column=0, row=0, sticky=(N, W, E, S))
root.columnconfigure(0, weight=1)
root.rowconfigure(0, weight=1)

# Status is one of the things we want to be able to display. Initally,
# this will just be that we have successfully opened the serial port
status = StringVar()
status.set("Serial port open!")

# Because some of our button may take a while we want to be
# able to open up threads. This is a blank thread that
# we will assign tasks to later
t1 = None

# Create a label which displays the status
ttk.Label(mainframe, textvariable=status).grid(column=3, row=1, sticky=W)

# Run the test file
def run_test_file():

	# Ensure that the sleep time has been updated
	thingy.sleep_time = float(sleep_entry.get())

	# Open a thread to run the test file
	t1 = threading.Thread(target=thingy.run_test_file, args=("unit_tests.txt",))
	t1.start()

	# Change what status says
	status.set("Running test file...")

	# Update how the window looks
	root.update_idletasks()	

# Run an arbitrary command
def run_command(command):
	
	thingy.sleep_time = float(sleep_entry.get())

	t1 = threading.Thread(target=thingy.run_test, args=(command,))
	t1.start()

	status.set("Running command \"{}\"".format(command))

	root.update_idletasks()

# Create a button that says allows us to run the test file
ttk.Button(mainframe,
	text="Run test file",
	command=run_test_file).grid(column=3, row=2, sticky=W)

# Create a button that says allows us to run command "reset"
ttk.Button(mainframe,
	text="reset",
	command=lambda: run_command("reset")).grid(column=4, row=1, sticky=W)

# Create a button that says allows us to run command "register"
ttk.Button(mainframe,
	text="register",
	command=lambda: run_command("register")).grid(column=4, row=2, sticky=W)

# Create a button that says allows us to run command "createDataModel"
ttk.Button(mainframe,
	text="createDataModel",
	command=lambda: run_command("createDataModel")).grid(column=4, row=3, sticky=W)

# Create a button that says allows us to run command "createDataModel"
ttk.Button(mainframe,
	text="createDataModel",
	command=lambda: run_command("createDataModel")).grid(column=4, row=4, sticky=W)

# Create a button that says allows us to run command "retrieveNotifications"
ttk.Button(mainframe,
	text="retrieveNotifications",
	command=lambda: run_command("retrieveNotifications")).grid(column=5, row=1, sticky=W)

# Create a button that says allows us to run command "retrieveNotifications"
ttk.Button(mainframe,
	text="retrieveNotifications",
	command=lambda: run_command("retrieveNotifications")).grid(column=5, row=2, sticky=W)

# Create a button that says allows us to run command "updateDataModel"
ttk.Button(mainframe,
	text="updateDataModel",
	command=lambda: run_command("updateDataModel")).grid(column=5, row=3, sticky=W)

# Create a button that says allows us to run command "deregister"
ttk.Button(mainframe,
	text="deregister",
	command=lambda: run_command("deregister")).grid(column=5, row=4, sticky=W)

# Widget to modify the sleep time between each command
sleep_time = StringVar()
sleep_time.set(1.0)
sleep_entry = ttk.Entry(mainframe, width=7, textvariable=sleep_time)
sleep_entry.grid(column=2, row=1, sticky=(W, E))

# Label to display sleep time
ttk.Label(mainframe, text="Sleep time:").grid(column=1, row=2, sticky=E)
ttk.Label(mainframe, textvariable=sleep_time).grid(column=2, row=2, sticky=W)

# Add padding to our items
for child in mainframe.winfo_children():
	child.grid_configure(padx=5, pady=5)

# Main loop
root.mainloop()