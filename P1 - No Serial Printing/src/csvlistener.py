import serial
import csv
from datetime import datetime

# Adjust port and baudrate as needed
ser = serial.Serial('COM8', 9600)  # Replace 'COM3' with your Arduino port
csv_file = 'arduino_log.csv'

header = ['average', 'ambient', 'target', 'voltage', 'temp1', 'temp2', 'temp3', 'temp4']

with open(csv_file, 'a', newline='') as f:
    writer = csv.writer(f)
    
    if f.tell() == 0:  # Write header if file is empty
        writer.writerow(['timestamp'] + header)
    
    print("Listening for data... Press Ctrl+C to stop.")
    
    try:
        while True:
            line = ser.readline().decode('utf-8').strip()
            if line:
                data = line.split(',')
                timestamp = datetime.now().isoformat()
                writer.writerow([timestamp] + data)
                print(f"[{timestamp}] Logged: {data}")
    except KeyboardInterrupt:
        print("Stopped logging.")
