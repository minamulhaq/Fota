# ============================================================================
# PACKET PARSER
# ============================================================================

from typing import Optional
from .command_creator import Command, CommandGetBootloaderVersion, Packet, CommandIDs


class PacketParser:
    """Parses bootloader response packets"""

    def __init__(self):
        self.crc_helper = Command.__new__(CommandGetBootloaderVersion)

    def parse_response(self, raw_data: bytes) -> Optional[Packet]:
        """Parse raw bytes into Packet structure"""
        if len(raw_data) < 6:  # Minimum: ID(1) + Len(1) + CRC(4)
            print(f"[PARSER] ERROR: Insufficient data length: {len(raw_data)} bytes")
            return None

        # Parse header
        pkt_id = raw_data[0]
        pkt_len = raw_data[1]

        # Validate expected packet size
        expected_size = 2 + pkt_len + 4  # ID + Len + Payload + CRC
        if len(raw_data) < expected_size:
            print(f"[PARSER] ERROR: Expected {expected_size} bytes, got {len(raw_data)}")
            return None

        # Extract payload
        payload = list(raw_data[2:2+pkt_len]) if pkt_len > 0 else []

        # Extract CRC (little-endian)
        crc_offset = 2 + pkt_len
        received_crc = int.from_bytes(raw_data[crc_offset:crc_offset+4], byteorder="little")

        # Create packet
        packet = Packet(id=pkt_id, length=pkt_len, payload=payload, crc32=received_crc)

        # Verify CRC
        computed_crc = self.crc_helper.compute_crc32_for_packet(packet)

        print(f"[PARSER] Packet ID: 0x{pkt_id:02X} ({self._get_id_name(pkt_id)})")
        print(f"[PARSER] Length: {pkt_len} bytes")
        print(f"[PARSER] Payload: {' '.join([f'{b:02X}' for b in payload]) if payload else 'None'}")
        print(f"[PARSER] CRC32: Received=0x{received_crc:08X}, Computed=0x{computed_crc:08X}")

        if received_crc != computed_crc:
            print(f"[PARSER] ERROR: CRC mismatch!")
            return None

        print(f"[PARSER] CRC verification: PASS")
        return packet

    def _get_id_name(self, pkt_id: int) -> str:
        """Get human-readable name for packet ID"""
        for cmd_id in CommandIDs:
            if cmd_id.value == pkt_id:
                return cmd_id.name
        return "UNKNOWN"

    def interpret_response(self, packet: Packet):
        """Interpret and print packet contents based on ID"""
        print(f"\n{'='*60}")
        print(f"RESPONSE INTERPRETATION")
        print(f"{'='*60}")

        if packet.id == CommandIDs.B_ACK.value:
            print(f"[INTERPRET] ACK received")

        elif packet.id == CommandIDs.B_NACK.value:
            print(f"[INTERPRET] NACK received")
            if packet.length > 0:
                assert packet.payload
                error_code = packet.payload[0]
                error_names = {
                    0x00: "SUCCESS",
                    0x01: "INVALID_CMD",
                    0x02: "INVALID_PARAMS",
                    0x03: "EXECUTION_FAILED",
                    0x04: "FLASH_ERROR",
                    0x05: "ADDRESS_ERROR"
                }
                print(f"[INTERPRET] Error Code: 0x{error_code:02X} ({error_names.get(error_code, 'UNKNOWN')})")

        elif packet.id == CommandIDs.B_CMD_GET_VERSION.value:
            if packet.length == 3:
                assert packet.payload
                major, minor, patch = packet.payload[0], packet.payload[1], packet.payload[2]
                print(f"[INTERPRET] Bootloader Version: {major}.{minor}.{patch}")
            else:
                print(f"[INTERPRET] Invalid version response length")

        elif packet.id == CommandIDs.B_CMD_GET_HELP.value:
            if packet.length > 0:
                assert packet.payload
                num_cmds = packet.payload[0]
                cmds = packet.payload[1:1+num_cmds]
                print(f"[INTERPRET] Supported Commands ({num_cmds} total):")
                for cmd in cmds:
                    print(f"  - 0x{cmd:02X} ({self._get_id_name(cmd)})")

        elif packet.id == CommandIDs.B_CMD_GET_CID.value:
            if packet.length == 2:
                assert packet.payload
                chip_id = packet.payload[0] | (packet.payload[1] << 8)
                print(f"[INTERPRET] Chip ID: 0x{chip_id:04X}")

        elif packet.id == CommandIDs.B_CMD_GET_RDP_LVL.value:
            if packet.length == 1:
                assert packet.payload
                rdp_level = packet.payload[0]
                rdp_names = {0xAA: "Level 0 (No Protection)",
                             0x00: "Level 1 (Read Protection)",
                             0xCC: "Level 2 (Permanent Protection)"}
                print(f"[INTERPRET] RDP Level: 0x{rdp_level:02X} ({rdp_names.get(rdp_level, 'Unknown')})")

        print(f"{'='*60}\n")
