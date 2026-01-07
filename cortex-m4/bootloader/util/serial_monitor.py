from typing import Optional

from bl_monitor import (
    Command,
    CommandFWSendBinSize,
    CommandFWUpdateSync,
    CommandFWVerifyDeviceID,
    CommandGetAppVersion,
    CommandGetBootloaderVersion,
    CommandGetChipID,
    CommandGetHelp,
    CommandGetRDPLevel,
    CommandJumpToAddress,
    CommandRetransmit,
    Packet,
    ResponseType,
)
from serial import Serial
from serial.tools import list_ports

# ============================================================================
# SERIAL MONITOR
# ============================================================================

BAUDRATE = 115200
KEYWORD = "STM"  # Changed: More generic to match more devices
TIMEOUT = 5


class SerialMonitor:
    def __init__(self):
        self.port: Optional[Serial] = None
        self.supported_commands: dict[int, Command] = {
            1: CommandRetransmit(),
            2: CommandGetBootloaderVersion(),
            3: CommandGetAppVersion(),
            4: CommandGetChipID(),
            5: CommandFWUpdateSync(),
            6: CommandFWVerifyDeviceID(),
            7: CommandFWSendBinSize(),
        }

    def scan_com_ports(self) -> Optional[Serial]:
        """Scan for STM32 device"""
        print(f"\n[SCAN] Scanning for devices containing '{KEYWORD}'...")

        available_ports = list(list_ports.comports())

        if not available_ports:
            print("[SCAN] No COM ports found on system")
            return None

        print(f"[SCAN] Found {len(available_ports)} COM port(s):")
        for idx, port_info in enumerate(available_ports, 1):
            print(f"  [{idx}] {port_info.device} - {port_info.description}")

        # Search for keyword match
        for port_info in available_ports:
            if KEYWORD.lower() in port_info.description.lower():
                print(f"\n[SCAN] ✓ Matched device: {port_info.device}")
                print(f"[SCAN]   Description: {port_info.description}")
                try:
                    return Serial(port_info.device, BAUDRATE, timeout=TIMEOUT)
                except Exception as e:
                    print(f"[SCAN] ERROR: Failed to open {port_info.device}: {e}")
                    continue

        # If no keyword match, offer manual selection
        print(f"\n[SCAN] No device matching '{KEYWORD}' found")
        choice = input(
            f"[SCAN] Enter port number to connect manually (or 0 to skip): "
        ).strip()

        if choice.isdigit() and 1 <= int(choice) <= len(available_ports):
            selected_port = available_ports[int(choice) - 1]
            print(f"[SCAN] Manually selected: {selected_port.device}")
            try:
                return Serial(selected_port.device, BAUDRATE, timeout=TIMEOUT)
            except Exception as e:
                print(f"[SCAN] ERROR: Failed to open {selected_port.device}: {e}")

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
                print(f"[CONNECT] ✓ Port opened: {self.port.name}")
                print(f"[CONNECT]   Baudrate: {BAUDRATE}")
                print(f"[CONNECT]   Timeout: {TIMEOUT}s")

            return True
        except Exception as e:
            print(f"[CONNECT] ERROR: {e}")
            return False

    def send_command(self, cmd: Command) -> Optional[dict]:
        """
        Send command and receive response with byte-by-byte parsing.
        Matches STM32 transmission: ID → Length → Payload (if len>0) → CRC32

        Returns:
            dict: Parsed response from command handler, or None on failure
        """
        if not self.port or not self.port.is_open:
            print("[SEND] ERROR: Port not open")
            return None

        assert self.port
        cmd.process_commmand(port=self.port)

    def run(self):
        """Main interactive loop"""
        print(f"\n{'=' * 70}")
        print("           STM32 BOOTLOADER COMMUNICATION MONITOR")
        print(f"{'=' * 70}\n")

        while True:
            try:
                # Connect to device
                if not self.connect():
                    print("\n[ERROR] Device not connected")
                    retry = input("Retry connection? (y/n): ").strip().lower()
                    if retry != "y":
                        break
                    continue

                # Display menu
                print(f"\n{'=' * 70}")
                print("AVAILABLE COMMANDS")
                print(f"{'=' * 70}")
                for idx, cmd in enumerate(self.supported_commands.values(), 1):
                    print(f"  {idx}. {cmd.info}")
                print("  0. Exit")
                print(f"{'=' * 70}")

                choice = input("\nEnter choice: ").strip()

                # Handle exit
                if choice == "0":
                    print("\n[EXIT] Exiting monitor...")
                    break

                # Validate input
                if not choice.isdigit():
                    print("[ERROR] Invalid input - please enter a number")
                    continue

                choice_idx = int(choice)

                if choice_idx in self.supported_commands.keys():
                    cmd = self.supported_commands[choice_idx]
                    self.send_command(cmd)

                # Pause before showing menu again
                input("\n⏎ Press Enter to continue...")

            except KeyboardInterrupt:
                print("\n\n[EXIT] Interrupted by user (Ctrl+C)")
                break
            except Exception as e:
                print(f"\n[ERROR] Unexpected exception: {e}")
                import traceback

                traceback.print_exc()
                input("\n⏎ Press Enter to continue...")

        # Cleanup
        if self.port and self.port.is_open:
            self.port.close()
            print("[DISCONNECT] ✓ Port closed")


if __name__ == "__main__":
    monitor = SerialMonitor()
    monitor.run()
