import serial
import time
import os

# Function to clear the screen
def clear_screen():
    os.system('cls' if os.name == 'nt' else 'clear')

# Open the file containing voter details
voter_data = {}
with open('voters.txt', 'r') as file:
    for line in file:
        line = line.strip()
        if not line:
            continue
        try:
            parts = line.split(',')
            if len(parts) == 5:
                id, name, dob, voter_id, gender= parts
                voter_data[int(id)] = (name, dob, voter_id, gender)
            else:
                print(f"Skipping line: {line} - Unexpected number of values: {len(parts)}")
        except ValueError as e:
            print(f"Error processing line: {line}")
            print(f"Error: {e}")
            continue

# Set up serial communication
ser = serial.Serial('COM3', 9600)  # Adjust COM port as needed
time.sleep(2)  # Wait for the connection to establish

last_activity_time = time.time()
clear_screen_delay = 15.01  # Time in seconds to wait before clearing the screen

while True:
    if ser.in_waiting > 0:
        request = ser.readline().decode('utf-8').strip()
        print(f"{request}")
        last_activity_time = time.time()  # Update the last activity time only when data is received
        if request.startswith('DETAILS_CORRESPONDING_TO_ID '):
            _, id = request.split()
            try:
                id = int(id)
                if id in voter_data:
                    name, dob, voter_id, gender = voter_data[id]
                    response = f'\nName: {name}\nD.O.B.: {dob}\nVoter_ID: {voter_id}\nGender: {gender}\n'
                    ser.write(response.encode('utf-8'))
                else:
                    ser.write(f"No data found for ID {id}".encode('utf-8'))
            except ValueError:
                ser.write(f"Invalid ID received: {id}".encode('utf-8'))
    
    # Check for inactivity and clear the screen if necessary
    if time.time() - last_activity_time > clear_screen_delay:
        clear_screen()
        last_activity_time = time.time()  # Reset the timer to prevent continuous clearing
