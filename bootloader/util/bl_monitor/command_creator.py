from abc import ABC, abstractmethod
from dataclasses import dataclass
from enum import Enum, auto
from typing import Optional


class ResponseType(Enum):
    B_ACK = 0xE0
    B_NACK = 0xE1
    B_RETRANSMIT = 0xE2


class CommandIDs(Enum):
    B_CMD_RETRANSMIT = 0xB0
    B_CMD_GET_BOOTLOADER_VERSION = 0xB1
    B_CMD_GET_APP_VERSION = auto()
    B_CMD_GET_HELP = auto()
    B_CMD_GET_CID = auto()
    B_CMD_GET_RDP_LVL = auto()
    B_CMD_JMP_TO_ADDR = auto()
    B_CMD_ERASE_FLASH = auto()


@dataclass
class Packet:
    id: int
    length: int = 0
    payload: None | list[int] = None
    crc32: int = 0

    def __post_init__(self):
        if self.payload is None:
            self.payload = []


@dataclass
class CommandInfo:
    id: int
    nemonic: str
    description: str

    def __str__(self) -> str:
        response = f"Command: {self.nemonic}\n"
        response += f"ID: {self.id}\n"
        response += f"Description: {self.description}"

        return response


class Command(ABC):
    @property
    @abstractmethod
    def cmd_id(self) -> CommandIDs: ...

    @property
    @abstractmethod
    def packet(self) -> Packet: ...

    def validate_id(self, id: int) -> bool:
        return self.packet.id == id

    @property
    @abstractmethod
    def info(self) -> CommandInfo: ...

    def compute_crc32_for_packet(self, pkt: Packet) -> int:
        # Build data exactly like STM32 expects
        data = bytearray()
        data.append(pkt.id & 0xFF)
        data.append(pkt.length & 0xFF)

        if pkt.length > 0 and pkt.payload:
            data.extend([b & 0xFF for b in pkt.payload[: pkt.length]])

        return self.crc32_stm32_style(data)

    def crc32_stm32_style(self, data: bytes) -> int:
        crc = 0xFFFFFFFF

        for byte in data:
            crc ^= byte
            for _ in range(8):
                if crc & 1:
                    crc = (crc >> 1) ^ 0xEDB88320
                else:
                    crc >>= 1

        return (~crc) & 0xFFFFFFFF

    @property
    def len_crc(self) -> int:
        return 4

    @abstractmethod
    def handle_response(self, response_packet: Packet) -> dict:
        """
        Handle the validated response packet.
        Each command implements its own response handling logic.

        Args:
            response_packet: Validated packet with correct CRC

        Returns:
            dict: Parsed response data specific to this command
        """
        ...

    def _get_id_name(self, packet_id: int) -> str:
        """Get human-readable name for packet ID"""
        for cmd_id in CommandIDs:
            if cmd_id.value == packet_id:
                return cmd_id.name
        return "UNKNOWN"

    @property
    def cmd(self) -> bytearray:
        """Generate raw byte array to send: id + length + payload + crc (little-endian)"""
        pkt: Packet = self.packet
        pkt.crc32 = self.compute_crc32_for_packet(pkt)
        raw = bytearray([pkt.id & 0xFF, pkt.length & 0xFF])
        if pkt.length > 0:
            assert pkt.payload, "Payload can't be None"
            raw.extend([b & 0xFF for b in pkt.payload])
        raw.extend(pkt.crc32.to_bytes(4, byteorder="little"))
        return raw

    def receive_response_packet(self, raw_data: bytes) -> Optional[Packet]:
        """
        Generic response packet receiver and validator.
        Parses raw bytes and validates CRC.

        Packet structure:
        - Byte 0: ID (ACK/NACK)
        - Byte 1: Payload length
        - Bytes 2 to (2+length-1): Payload (if length > 0)
        - Last 4 bytes: CRC32 (little-endian)

        Args:
            raw_data: Raw bytes received from bootloader

        Returns:
            Packet object if valid, None if invalid
        """
        print(f"\n{'=' * 60}")
        print(f"RECEIVING RESPONSE PACKET")
        print(f"{'=' * 60}")
        print(
            f"[RX] Raw data ({len(raw_data)} bytes): {' '.join([f'{b:02X}' for b in raw_data])}"
        )

        # Minimum packet: ID(1) + Length(1) + CRC(4) = 6 bytes
        if len(raw_data) < 6:
            print(
                f"[RX] ERROR: Insufficient data (minimum 6 bytes, got {len(raw_data)})"
            )
            return None

        # Step 1: Parse ID (byte 0)
        offset = 0
        packet_id = raw_data[offset]
        offset += 1

        id_name = self._get_id_name(packet_id)
        print(f"[RX] Step 1 - Parse ID:")
        print(f"     Byte[0] = 0x{packet_id:02X} ({id_name})")

        # Step 2: Parse length (byte 1)
        payload_length = raw_data[offset]
        offset += 1

        print(f"[RX] Step 2 - Parse Length:")
        print(f"     Byte[1] = {payload_length} bytes")

        # Validate total packet size
        expected_size = 1 + 1 + payload_length + 4  # ID + Len + Payload + CRC
        if len(raw_data) < expected_size:
            print(f"[RX] ERROR: Incomplete packet")
            print(f"     Expected: {expected_size} bytes")
            print(f"     Received: {len(raw_data)} bytes")
            return None

        # Step 3: Parse payload (if length > 0)
        payload = []
        if payload_length > 0:
            payload = list(raw_data[offset : offset + payload_length])
            print(f"[RX] Step 3 - Parse Payload:")
            print(
                f"     Bytes[2:{2 + payload_length}] = {' '.join([f'{b:02X}' for b in payload])}"
            )
            offset += payload_length
        else:
            print(f"[RX] Step 3 - Parse Payload:")
            print(f"     No payload (length = 0)")

        # Step 4: Parse CRC32 (last 4 bytes, little-endian)
        crc_bytes = raw_data[offset : offset + 4]
        received_crc = int.from_bytes(crc_bytes, byteorder="little")

        print(f"[RX] Step 4 - Parse CRC32:")
        print(
            f"     Bytes[{offset}:{offset + 4}] = {' '.join([f'{b:02X}' for b in crc_bytes])} (LE)"
        )
        print(f"     CRC32 value = 0x{received_crc:08X}")

        # Step 5: Verify CRC
        # CRC is computed over: [ID][Length][Payload]
        crc_data = bytearray()
        crc_data.append(packet_id)
        crc_data.append(payload_length)
        if payload_length > 0:
            crc_data.extend(payload)

        computed_crc = self.crc32_stm32_style(crc_data)

        print(f"[RX] Step 5 - Verify CRC:")
        print(f"     CRC computed over: {' '.join([f'{b:02X}' for b in crc_data])}")
        print(f"     Computed CRC = 0x{computed_crc:08X}")
        print(f"     Received CRC = 0x{received_crc:08X}")

        if computed_crc != received_crc:
            print(f"[RX] ERROR: CRC MISMATCH!")
            print(f"     ✗ Packet validation FAILED")
            return None

        print(f"     ✓ CRC Valid")
        print(f"[RX] ✓ Packet validation SUCCESS")
        print(f"{'=' * 60}\n")

        # Create and return validated packet
        packet = Packet(
            id=packet_id,
            length=payload_length,
            payload=payload if payload else None,
            crc32=received_crc,
        )

        return packet
