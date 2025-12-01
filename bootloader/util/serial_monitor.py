import time
from typing import Optional

from bl_monitor import (
    Command,
    CommandEraseFlash,
    CommandGetBootloaderVersion,
    CommandGetChipID,
    CommandGetHelp,
    CommandGetRDPLevel,
    CommandJumpToAddress,
    Packet,
)
from bl_monitor.command_creator import ResponseType
from serial import Serial
from serial.tools import list_ports

# ============================================================================
# SERIAL MONITOR
# ============================================================================

BAUDRATE = 115200
KEYWORD = "STM"  # Changed: More generic to match more devices
TIMEOUT = 12.0  # Response timeout in seconds


class SerialMonitor:
    def __init__(self):
        self.port: Optional[Serial] = None
        self.supported_commands: dict[int, Command] = {
            1: CommandGetBootloaderVersion(),
            # CommandGetHelp(),
            # CommandGetChipID(),
            # CommandGetRDPLevel(),
        }

    def scan_com_ports(self) -> Optional[Serial]:
        """Scan for STM32 device"""
        print(f"\n[SCAN] Scanning for devices containing '{KEYWORD}'...")

        available_ports = list(list_ports.comports())

        if not available_ports:
            print(f"[SCAN] No COM ports found on system")
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
        # Step 1: Display command info
        print(f"\n{'=' * 70}")
        print(f"EXECUTING COMMAND")
        print(f"{'=' * 70}")
        print(cmd.info)
        print()

        # Step 2: Get command bytes
        raw_cmd = cmd.cmd

        print(f"[TX] Command Packet Breakdown:")
        print(f"     Total bytes: {len(raw_cmd)}")
        print(f"     ID:          0x{raw_cmd[0]:02X}")
        print(f"     Length:      {raw_cmd[1]}")

        if raw_cmd[1] > 0:
            payload_bytes = raw_cmd[2 : 2 + raw_cmd[1]]
            print(f"     Payload:     {' '.join([f'{b:02X}' for b in payload_bytes])}")
        else:
            print(f"     Payload:     None")

        crc_bytes = raw_cmd[-4:]
        crc_value = int.from_bytes(crc_bytes, byteorder="little")
        print(
            f"     CRC32:       {' '.join([f'{b:02X}' for b in crc_bytes])} (LE) = 0x{crc_value:08X}"
        )
        print(f"\n[TX] Raw bytes: {' '.join([f'{b:02X}' for b in raw_cmd])}")

        # Step 3: Clear buffers and send command
        self.port.reset_input_buffer()
        self.port.reset_output_buffer()

        bytes_sent = self.port.write(raw_cmd)
        self.port.flush()
        print(f"[TX] ✓ Sent {bytes_sent}/{len(raw_cmd)} bytes")

        # Step 4: Receive response byte-by-byte
        print(f"\n[RX] Receiving response packet...")

        def read_byte_with_timeout(timeout_sec=2.0) -> Optional[int]:
            """Wait for single byte with timeout"""
            start_time = time.time()
            while (time.time() - start_time) < timeout_sec:
                assert self.port
                if self.port.in_waiting > 0:
                    byte = self.port.read(1)
                    return byte[0] if byte else None
                time.sleep(0.01)  # Small delay to prevent busy-waiting
            return None

        response_buffer = bytearray()

        print(f"[RX] Step 1 - Reading ID byte...")
        packet_id = read_byte_with_timeout(TIMEOUT)

        if packet_id is None:
            print(f"[RX] ✗ Timeout waiting for ID byte")
            return None

        response_buffer.append(packet_id)
        print(f"[RX]   ID = 0x{packet_id:02X} ({cmd._get_id_name(packet_id)})")

        print(f"[RX] Step 2 - Reading Length byte...")
        payload_length = read_byte_with_timeout(TIMEOUT)

        if payload_length is None:
            print(f"[RX] ✗ Timeout waiting for Length byte")
            return None

        response_buffer.append(payload_length)
        print(f"[RX]   Length = {payload_length} bytes")

        if payload_length > 0:
            print(f"[RX] Step 3 - Reading {payload_length} payload bytes...")

            for i in range(payload_length):
                payload_byte = read_byte_with_timeout(TIMEOUT)

                if payload_byte is None:
                    print(
                        f"[RX] ✗ Timeout waiting for payload byte {i + 1}/{payload_length}"
                    )
                    return None

                response_buffer.append(payload_byte)

                if (i + 1) % 16 == 0 or (i + 1) == payload_length:
                    payload_so_far = " ".join([f"{b:02X}" for b in response_buffer[2:]])
                    print(
                        f"[RX]   Payload [{i + 1}/{payload_length}]: {payload_so_far}"
                    )
        else:
            print(f"[RX] Step 3 - No payload (length = 0)")

        print(f"[RX] Step 4 - Reading 4 CRC32 bytes...")

        for i in range(4):
            crc_byte = read_byte_with_timeout(TIMEOUT)

            if crc_byte is None:
                print(f"[RX] ✗ Timeout waiting for CRC byte {i + 1}/4")
                return None

            response_buffer.append(crc_byte)

        crc_bytes_rx = response_buffer[-4:]
        crc_value_rx = int.from_bytes(crc_bytes_rx, byteorder="little")
        print(
            f"[RX]   CRC32 = {' '.join([f'{b:02X}' for b in crc_bytes_rx])} (LE) = 0x{crc_value_rx:08X}"
        )

        print(f"\n[RX] ✓ Complete packet received ({len(response_buffer)} bytes)")
        print(f"[RX] Raw data: {' '.join([f'{b:02X}' for b in response_buffer])}")

        validated_packet: Packet | None = cmd.receive_response_packet(
            bytes(response_buffer)
        )

        # print(f"\n\n\nPACKET ID IS: {ResponseType.packet_type(validated_packet.id)}\n\n\n")
        if not validated_packet:
            print(f"\n[ERROR] Packet validation failed (CRC mismatch or malformed)")
            return None
        elif validated_packet.id == ResponseType.B_RETRANSMIT.value:
            print(f"\n[RETRANSMIT] Last packet CRC Failed, retransmit")
            return None

        parsed_result = cmd.handle_response(validated_packet)

        print(f"{'=' * 70}")
        print(f"COMMAND EXECUTION COMPLETE")
        print(f"{'=' * 70}\n")

        return parsed_result

    def run(self):
        """Main interactive loop"""
        print(f"\n{'=' * 70}")
        print(f"           STM32 BOOTLOADER COMMUNICATION MONITOR")
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
                    print(f"  {idx}. {cmd.info.description}")
                print(f"  {len(self.supported_commands) + 1}. Jump to Address (custom)")
                print(f"  {len(self.supported_commands) + 2}. Erase Flash (custom)")
                print(f"  0. Exit")
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
