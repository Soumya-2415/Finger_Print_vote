import serial
import time

arduino_port = 'COM3' # Change this to your Arduino's serial port
baud_rate = 9600
trigger_command = 'E'

def send_trigger(port, command):
    port.write(command.encode())

def read_serial(port):
    while port.in_waiting > 0:
        line = port.readline().decode('utf-8').strip()
        print(line)
        if "Stored!" in line:
            return True
        if "Enrollment failed" in line:
            return False
    return None

def main():
    with serial.Serial(arduino_port, baud_rate, timeout=1) as ser:
        time.sleep(2)  # Wait for the serial connection to initialize
        # Read and print initial setup messages from Arduino
        start_time = time.time()
        while time.time() - start_time < 5:  # Read for 5 seconds
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                print(line)
        
        # Input validation loop
        while True:
            try:
                current_id = int(input("Enter the starting ID for fingerprint enrollment : "))
                if current_id in [1, 2, 3]:
                    print("Invalid stating ID. Please choose another number (except 1, 2 & 3).")
                else:
                    break
            except ValueError:
                print("Invalid input. Please enter a valid integer.")

        ser.write(f'S{current_id}'.encode())  # Send the starting ID to the Arduino
        
        while True:
            input("Press Enter to enroll a new fingerprint...")
            success = False
            while not success:
                send_trigger(ser, trigger_command)
                print(f"Attempting to enroll fingerprint with ID #{current_id}")
                
                # Wait and read serial output from Arduino for 10 seconds
                end_time = time.time() + 10
                while time.time() < end_time:
                    result = read_serial(ser)
                    if result is not None:
                        success = result
                        break
                    time.sleep(0.1)  # Short delay to avoid busy-waiting
                
                if success:
                    current_id += 1
                    break

if __name__ == '__main__':
    main()
