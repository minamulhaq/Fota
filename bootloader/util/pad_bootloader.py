import sys
import os
import argparse

# Define the target padding size (32 KB = 32 * 1024 bytes)
# 0x8000 in hexadecimal is 32768 in decimal, which is 32 KB.
TARGET_SIZE_BYTES = 0x10000

def pad_binary_file(file_path: str, target_size: int = TARGET_SIZE_BYTES):
    """
    Pads a binary file with 0xFF bytes up to the specified target size.
    """
    if not os.path.exists(file_path):
        print(f"Error: File not found at path: {file_path}")
        sys.exit(1)

    try:
        # Read the entire file content
        with open(file_path, "rb") as f:
            raw_file_data = f.read()

        current_size = len(raw_file_data)

        if current_size >= target_size:
            print(f"Warning: File size ({current_size} B) is already >= target size ({target_size} B). No padding applied.")
            return

        bytes_to_pad = target_size - current_size
        # Padding with 0xFF is standard for erasing/unused Flash space
        padding = bytes([0xFF] * bytes_to_pad)

        # Write the original data followed by the padding back to the file
        with open(file_path, "wb") as f:
            f.write(raw_file_data + padding)

        print(f"Successfully padded '{file_path}':")
        print(f"  - Original size: {current_size} B")
        print(f"  - Padded size:   {target_size} B")
        print(f"  - Padding added: {bytes_to_pad} B (0x{bytes_to_pad:X} bytes)")

    except Exception as e:
        print(f"An error occurred during padding: {e}")
        sys.exit(1)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Pads a binary file to a fixed size for embedded systems."
    )
    # The file path is now a required positional argument
    parser.add_argument(
        "bin_file",
        type=str,
        help="Path to the .bin file to be padded."
    )

    args = parser.parse_args()

    # Use the target size of 32 KB (0x8000)
    pad_binary_file(args.bin_file, TARGET_SIZE_BYTES)