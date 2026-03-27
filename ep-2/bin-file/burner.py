import serial
import time
import os
import subprocess

def handle_asm_conversion(filepath):
    """If the file is .s or .asm, assemble it and return the path to the new .bin"""
    name, ext = os.path.splitext(filepath)
    
    if ext.lower() in ['.s', '.asm']:
        bin_path = name + "_compiled.bin"
        print(f"--- Assembling {ext} file to {bin_path} ---")
        
        try:
            # Running vasm from your global alias/path
            subprocess.run(["vasm6502_oldstyle", "-dotdir", "-Fbin", "-o", bin_path, filepath], check=True)
            return bin_path
        except Exception as e:
            print(f"Error during assembly: {e}")
            return None
            
    return filepath # It's already a .bin, return unchanged

def cleanup_temp_bin(original_path, current_path):
    """Deletes the bin file ONLY if it was generated from an assembly file."""
    if original_path != current_path and os.path.exists(current_path):
        os.remove(current_path)
        print(f"--- Cleaned up temporary file: {current_path} ---")

def get_windows_file():
    """Opens a Windows File Picker and returns the WSL path."""
    print("Opening Windows File Picker...")
    
    ps_script = (
        "[System.Reflection.Assembly]::LoadWithPartialName('System.Windows.Forms') | Out-Null; "
        "$f = New-Object System.Windows.Forms.OpenFileDialog; "
        "$f.Filter = '6502 Files (*.bin, *.s, *.asm)|*.bin;*.s;*.asm|All Files (*.*)|*.*'; "
        "if($f.ShowDialog() -eq 'OK'){ $f.FileName }"
    )

    cmd = ["powershell.exe", "-NoProfile", "-Command", ps_script]
    
    try:
        win_path = subprocess.check_output(cmd).decode().strip()
        if not win_path: return None

        # --- REFINED PATH CLEANING ---
        
        # Case A: File is inside WSL (Starts with \\wsl.localhost or \\wsl$)
        if win_path.startswith("\\\\wsl"):
            # We need to strip the network prefix to get the local path
            # Example: \\wsl.localhost\Ubuntu-24.04\home\taha\rom.bin
            # 1. Split by backslashes
            parts = win_path.split('\\')
            # 2. Rejoin starting from the 4th element (the actual Linux path)
            # parts[0-3] are "", "", "wsl.localhost", "Ubuntu-24.04"
            linux_path = "/" + "/".join(parts[4:])
            return linux_path

        # Case B: File is on a Windows Drive (Starts with C:, D:, etc.)
        if ":" in win_path:
            drive = win_path[0].lower()
            path = win_path[3:].replace("\\", "/")
            return f"/mnt/{drive}/{path}"
            
        return win_path
    except Exception as e:
        print(f"Error resolving path: {e}")
        return None

def verify_rom(ser, original_data):
    print("-----------------Starting Verification-----------------")
    ser.write(b'R') # Trigger the Raw Read

    old_timeout = ser.timeout
    ser.timeout = 10 # Increase timeout for verification
    
    # Read exactly 32768 bytes from the serial port
    received_data = ser.read(32768)

    ser.timeout = old_timeout # Restore original timeout
    
    if len(received_data) < 32768:
        print(f"Error: Only received {len(received_data)} bytes.")
        return False

    errors = 0
    # Compare byte by byte
    for i in range(len(original_data)):
        if original_data[i] != received_data[i]:
            if errors < 10: # Only print the first 10 errors
                print(f"Mismatch at {hex(i)}: Expected {hex(original_data[i])}, Got {hex(received_data[i])}")
            errors += 1
    
    if errors == 0:
        print("✅ VERIFICATION SUCCESSFUL! EEPROM matches Binary 100%.")
        return True
    else:
        print(f"❌ VERIFICATION FAILED! Found {errors} mismatches.")
        return False

def upload_rom():
    # 1. Select the file
    selectedFile = get_windows_file()
    if not selectedFile:
        print("No file selected. Exiting.")
        return

    print(f"Selected: {selectedFile}")
    filepath = handle_asm_conversion(selectedFile) # Convert if it's an assembly file

    if filepath is None:
        print("Failed to process the selected file. Exiting.")
        return
    # 2. Read the binary
    with open(filepath, 'rb') as f:
        rom_data = f.read()

    # 3. Setup Serial (Update PORT to your Arduino's port)
    # Inside WSL, your COM8 is usually /dev/ttyS8 or /dev/ttyACM0
    try:
        # Use your port (likely /dev/ttyS8 for COM8)
        ser = serial.Serial('/dev/ttyACM0', 115200, timeout=3)
    
        print("Waiting for Arduino to wake up...")
    
        # --- NEW HANDSHAKE LOGIC ---
        while True:
            line = ser.readline().decode('ascii', errors='ignore').strip()
            if line:
                print(f"Arduino says: {line}")
            if "Awaiting Commands..." in line:
                print("Handshake Complete! Arduino is ready.")
            break
        # ---------------------------

        time.sleep(2) # Wait for reset
        ser.reset_input_buffer()

        if len(rom_data) != 32768:
            print("Error: file size should be exactly 32768 bytes.")
            return

        print("Burning EEPROM...")
        for addr in range(0, len(rom_data), 64):
            chunk = rom_data[addr:addr+64]
            
            # Ensure 64-byte page alignment
            # if len(chunk) < 64: i don't think so
            #     chunk = chunk.ljust(64, b'\xff')

            # Packet: 'w' (1) + Addr (2) + Data (64) + Checksum (1)
            checksum = sum(chunk) & 0xFF
            packet = b'w' + addr.to_bytes(2, 'big') + chunk + bytes([checksum])

            ser.write(packet)
            
            # Wait for Arduino's "OK ACK"
            response = ser.readline().decode().strip()
            
            if "OK ACK" in response:
                print(f"Written: {hex(addr)} | Checksum: {hex(checksum)}", end='\r')
            else:
                print(f"\nFailed at {hex(addr)}: {response}")
                return

        print("\nSuccess! NitroBurner 65 task complete.")
        verify_rom(ser, rom_data)
    except Exception as e:
        print(f"\nError: {e}")
    finally:
        if 'ser' in locals(): ser.close()
        cleanup_temp_bin(selectedFile, filepath) # Clean up if we created a temp bin

if __name__ == "__main__":
    upload_rom()