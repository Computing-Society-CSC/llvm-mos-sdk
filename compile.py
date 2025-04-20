#!/bin/python3
import sys
import subprocess
import os

def compress_zeros(data):
    result = []
    zero_count = 0

    for byte in data:
        if byte == 0:  # Check for zero byte
            zero_count += 1
        else:
            if zero_count > 3:  # Write compressed format for more than 3 zeros
                result.append(f"z{zero_count}!")
            elif zero_count > 0:
                result.extend(["00"] * zero_count)  # Add uncompressed zeros
            zero_count = 0
            result.append(f"{byte:02x}")  # Add the non-zero byte
    # Handle trailing zeros
    if zero_count > 3:
        result.append(f"z{zero_count}!")
    elif zero_count > 0:
        result.extend(["00"] * zero_count)
    return "".join(result)

def main():
    if len(sys.argv) < 2:
        print("Usage: python script.py <filename>")
        return

    filename = sys.argv[1]

    try:
        # Step 1: Run the mos-6502fun-clang command
        subprocess.run(["mos-6502fun-clang", "-Os", "-o", "test.prg", filename], check=True)

        # Step 2: Read and output the binary contents in the specified format
        with open("test.prg", "rb") as binary_file:
            data = binary_file.read()
            hex_bytes = compress_zeros(data)
            print(f"a{hex_bytes}")
        # Step 3: Delete the file test.prg
        os.remove("test.prg")

    except subprocess.CalledProcessError as e:
        print(f"Error while running the command: {e}")
    except FileNotFoundError:
        print("File test.prg not found.")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

if __name__ == "__main__":
    main()
