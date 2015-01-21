# Program that attaches to the serial port that is attached to the Arduino
# Will log data to a no sql DB so that i may be displayed and graphed
# SHould be able to mark start and end times for ferments, Prolly on the web side
# Incoming lines are JSON formatted

import serial
import json
import datetime



ser = serial.Serial(0)

print ser.name

while True:
	# Intake the serial line and then go process it
	# Don't block while processing
	line = ser.readline()
	print line


ser.close()

def process_line(line):
	# Make sure it's nmot an error
	if "ERROR" in line:
		#log the error
	else:
		# Process the line
		# Add a timestamp and write that out
