from enum import Enum
import os
import time
from typing import Optional

from bl_monitor import PacketParser
from bl_monitor import (
    Packet,
    Command,
    CommandJumpToAddress,
    CommandEraseFlash,
    CommandGetBootloaderVersion,
    CommandGetHelp,
    CommandGetChipID,
    CommandGetRDPLevel,
)
from serial import Serial
from serial.tools import list_ports
# ============================================================================
# SERIAL MONITOR
# ============================================================================

BAUDRATE = 115200
KEYWORD = "STM32"
TIMEOUT = 2.0  # Response timeout in seconds


class SerialMonitor:
    def __init__(self):
        self.port: Optional[Serial] = None
        self.parser = PacketParser()
        self.supported_commands: list[Command] = [
            CommandGetBootloaderVersion(),
            CommandGetHelp(),
            CommandGetChipID(),
            CommandGetRDPLevel(),
        ]

    def scan_com_ports(self) -> Optional[Serial]:
        """Scan for STM32 device"""
        print(f"[SCAN] Searching for '{KEYWORD}' device...")
        for port_info in list_ports.comports():
            if KEYWORD in port_info.description:
                print(f"[SCAN] Found: {port_info.device} - {port_info.description}")
                return Serial(port_info.device, BAUDRATE, timeout=TIMEOUT)
        print(f"[SCAN] No device found")
        return None

    def connect(self) -> bool:
        """Connect to device"""
        try:
            if self.port is None:
                self.port = self.scan_com_ports()
                if not self.port:
                    return False

            if not self.port.is_open:
                self.port.open()
                print(f"[CONNECT] Port opened: {self.port.name}")

            return True
        except Exception as e:
            print(f"[CONNECT] ERROR: {e}")
            return False

    def send_command(self, cmd: Command) -> Optional[Packet]:
        """Send command and receive response"""
        if not self.port or not self.port.is_open:
            print("[SEND] ERROR: Port not open")
            return None

        print(f"\n{'=' * 60}")
        print(f"SENDING COMMAND")
        print(f"{'=' * 60}")
        print(cmd.info)

        raw_cmd = cmd.cmd
        print(
            f"\n[TX] Raw bytes ({len(raw_cmd)}): {' '.join([f'{b:02X}' for b in raw_cmd])}"
        )
        print(f"[TX] Breakdown:")
        print(f"     ID:      0x{raw_cmd[0]:02X}")
        print(f"     Length:  {raw_cmd[1]}")
        if raw_cmd[1] > 0:
            print(
                f"     Payload: {' '.join([f'{b:02X}' for b in raw_cmd[2 : 2 + raw_cmd[1]]])}"
            )
        crc_bytes = raw_cmd[-4:]
        print(f"     CRC32:   {' '.join([f'{b:02X}' for b in crc_bytes])} (LE)")

        # Clear buffers
        self.port.reset_input_buffer()
        self.port.reset_output_buffer()

        # Send command
        bytes_sent = self.port.write(raw_cmd)
        self.port.flush()
        print(f"[TX] Sent {bytes_sent} bytes")

        # Wait for response
        print(f"[RX] Waiting for response (timeout={TIMEOUT}s)...")
        time.sleep(0.2)  # Give STM32 time to process

        if self.port.in_waiting == 0:
            print(f"[RX] No data received (timeout)")
            return None

        # Read response
        response_bytes = self.port.read(self.port.in_waiting)
        print(
            f"[RX] Received {len(response_bytes)} bytes: {' '.join([f'{b:02X}' for b in response_bytes])}"
        )

        # Parse response
        packet = self.parser.parse_response(response_bytes)

        if packet:
            self.parser.interpret_response(packet)

        return packet

    def run(self):
        """Main interactive loop"""
        print(f"\n{'=' * 60}")
        print(f"STM32 BOOTLOADER MONITOR")
        print(f"{'=' * 60}\n")

        while True:
            try:
                if not self.connect():
                    print("\n[ERROR] Device not connected. Retrying in 3s...")
                    time.sleep(3)
                    continue

                # Display menu
                print(f"\n{'=' * 60}")
                print("AVAILABLE COMMANDS")
                print(f"{'=' * 60}")
                for idx, cmd in enumerate(self.supported_commands, 1):
                    print(f"{idx}. {cmd.info.description}")
                print(f"{len(self.supported_commands) + 1}. Jump to Address (custom)")
                print(f"{len(self.supported_commands) + 2}. Erase Flash (custom)")
                print("0. Exit")
                print(f"{'=' * 60}")

                choice = input("\nEnter choice: ").strip()

                if choice == "0":
                    print("Exiting...")
                    break

                if not choice.isdigit():
                    print("[ERROR] Invalid input")
                    continue

                choice_idx = int(choice)

                # Handle predefined commands
                if 1 <= choice_idx <= len(self.supported_commands):
                    cmd = self.supported_commands[choice_idx - 1]
                    self.send_command(cmd)

                # Handle Jump to Address
                elif choice_idx == len(self.supported_commands) + 1:
                    addr_str = input("Enter address (hex, e.g., 0x08000000): ").strip()
                    try:
                        address = int(addr_str, 16)
                        cmd = CommandJumpToAddress(address)
                        self.send_command(cmd)
                    except ValueError:
                        print("[ERROR] Invalid address format")

                # Handle Erase Flash
                elif choice_idx == len(self.supported_commands) + 2:
                    erase_type = input("Mass erase? (y/n): ").strip().lower()
                    if erase_type == "y":
                        cmd = CommandEraseFlash(mass_erase=True)
                        self.send_command(cmd)
                    else:
                        sectors_str = input(
                            "Enter sector numbers (comma-separated, e.g., 1,2,3): "
                        ).strip()
                        try:
                            sectors = [int(s.strip()) for s in sectors_str.split(",")]
                            cmd = CommandEraseFlash(sectors=sectors)
                            self.send_command(cmd)
                        except ValueError:
                            print("[ERROR] Invalid sector format")

                else:
                    print("[ERROR] Invalid choice")

                input("\nPress Enter to continue...")

            except KeyboardInterrupt:
                print("\n\n[EXIT] Interrupted by user")
                break
            except Exception as e:
                print(f"\n[ERROR] {e}")
                import traceback

                traceback.print_exc()
                input("\nPress Enter to continue...")

        if self.port and self.port.is_open:
            self.port.close()
            print("[DISCONNECT] Port closed")


if __name__ == "__main__":
    monitor = SerialMonitor()
    monitor.run()
