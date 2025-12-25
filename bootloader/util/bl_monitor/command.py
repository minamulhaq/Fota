import time
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from enum import Enum, auto
from typing import Optional

from serial import Serial


class ErrorCodes(Enum):
    ERROR_INVALID_COMMAND = 0x11


class ResponseType(Enum):
    B_ACK = 0xE0
    B_NACK = 0xE1
    B_RETRANSMIT = 0xE2


class CommandIDs(Enum):
    B_CMD_RETRANSMIT = 0xB0
    B_CMD_GET_BOOTLOADER_VERSION = auto()
    B_CMD_GET_APP_VERSION = auto()
    B_CMD_GET_CHIP_ID = auto()
    B_CMD_SYNC = auto()
    B_CMD_VERIFY_DEVICE_ID = auto()
    B_CMD_SEND_BIN_SIZE = auto()
    B_CMD_SEND_BIN_IN_PACKETS = auto()
    B_CMD_GET_HELP = auto()
    B_CMD_GET_CID = auto()
    B_CMD_GET_RDP_LVL = auto()
    B_CMD_JMP_TO_ADDR = auto()
    B_CMD_ERASE_FLASH = auto()


@dataclass
class Packet:
    id: int
    length: int = field(default=0)
    payload: None | list[int] = None
    crc32: int = 0

    def __post_init__(self):
        if self.payload is None:
            self.payload = []
            self.length = 0
        else:
            self.length = len(self.payload)

    def __str__(self) -> str:
        response = f"id: 0x{self.id:04X}\n".upper()
        response += f"length: {self.length}\n"
        if self.payload:
            response += f"payload: {[hex(x) for x in self.payload]}\n"
        else:
            response += f"payload: {[]}\n"

        response += f"crc: {hex(self.crc32)}\n"

        return response


TIMEOUT = 300


@dataclass
class CommandInfo:
    id: int
    nemonic: str
    # description: str

    def __str__(self) -> str:
        cid = f"{self.id:#04x}".upper()
        return f"id: {cid} | {self.nemonic}"


@dataclass
class CommandExecutionResponse:
    execution_success: bool = field(default=False)
    data: dict = field(default_factory=lambda: dict())


class Command(ABC):
    def __init__(self) -> None:
        super().__init__()
        print(f"\nHANDLING {self.info.nemonic}")

    @property
    @abstractmethod
    def next_command(self) -> list["Command"]:
        return []

    @property
    @abstractmethod
    def cmd_id(self) -> CommandIDs: ...

    @abstractmethod
    def packet(self, metadata: dict = {}) -> Packet: ...

    def validate_id(self, id: int) -> bool:
        return self.packet().id == id

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
        """
        STM32 CRC peripheral compatible
        CRC-32 / MPEG-2
        Poly: 0x04C11DB7
        Init: 0xFFFFFFFF
        No reflection
        No final XOR
        """
        crc = 0xFFFFFFFF
        poly = 0x04C11DB7

        for byte in data:
            crc ^= byte << 24  # align byte to MSB
            for _ in range(8):
                if crc & 0x80000000:
                    crc = ((crc << 1) ^ poly) & 0xFFFFFFFF
                else:
                    crc = (crc << 1) & 0xFFFFFFFF

        return crc

    @property
    def len_crc(self) -> int:
        return 4

    @abstractmethod
    def handle_response(self, response_packet: Packet) -> CommandExecutionResponse:
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
        print(f"\n\nreceive_response_packet: Parsing into Packet")
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

        # id_name = self._get_id_name(packet_id)
        print(f"[RX] Step 1 - Parse ID:")
        print(f"     Byte[0] = 0x{packet_id:02X} ({ResponseType(packet_id).name})")

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

        # Create and return validated packet
        packet = Packet(
            id=packet_id,
            length=payload_length,
            payload=payload if payload else None,
            crc32=received_crc,
        )

        return packet

    def cmd(self, pkt: Packet) -> bytearray:
        """Generate raw byte array to send: id + length + payload + crc (little-endian)"""
        # pkt: Packet = self.packet()
        pkt.crc32 = self.compute_crc32_for_packet(pkt)
        raw = bytearray([pkt.id & 0xFF, pkt.length & 0xFF])
        if pkt.length > 0:
            assert pkt.payload, "Payload can't be None"
            raw.extend([b & 0xFF for b in pkt.payload])
        raw.extend(pkt.crc32.to_bytes(4, byteorder="little"))
        return raw

    @abstractmethod
    def getinput(self) -> None: ...

    def process_commmand(self, port: Serial) -> CommandExecutionResponse:
        # Step 2: Get command bytes
        self.getinput()
        raw_cmd = self.cmd
        response: CommandExecutionResponse = self.send_command(
            port=port, raw_cmd=raw_cmd(pkt=self.packet())
        )
        if response.execution_success:
            for c in self.next_command:
                response = c.process_commmand(port=port)
        return response

    def send_command(
        self,
        port: Serial,
        raw_cmd: bytearray,
        expect_response: bool = True,
        show_debug: bool = True,
    ) -> CommandExecutionResponse:
        """
        Send command and receive response with byte-by-byte parsing.
        Matches STM32 transmission: ID → Length → Payload (if len>0) → CRC32

        Returns:
            dict: Parsed response from command handler, or None on failure
        """
        response = CommandExecutionResponse()
        if not port or not port.is_open:
            print("[SEND] ERROR: Port not open")
            return response

        assert port

        if show_debug:
            print("[TX] Command Packet Breakdown:")
            print(f"     Total bytes: {len(raw_cmd)}")
            print(f"     ID:          0x{raw_cmd[0]:02X}")
            print(f"     Length:      {raw_cmd[1]}")

            if raw_cmd[1] > 0:
                payload_bytes = raw_cmd[2 : 2 + raw_cmd[1]]
                print(
                    f"     Payload:     {' '.join([f'{b:02X}' for b in payload_bytes])}"
                )
            else:
                print("     Payload:     None")

            crc_bytes = raw_cmd[-4:]
            crc_value = int.from_bytes(crc_bytes, byteorder="little")
            print(
                f"     CRC32:       {' '.join([f'{b:02X}' for b in crc_bytes])} (LE) = 0x{crc_value:08X}"
            )
            print(f"\n[TX] Raw bytes: {' '.join([f'0x{b:02X}' for b in raw_cmd])}")

        # Step 3: Clear buffers and send command
        port.reset_input_buffer()
        port.reset_output_buffer()

        bytes_sent = port.write(raw_cmd)
        port.flush()
        print(f"[TX] ✓ Sent {bytes_sent}/{len(raw_cmd)} bytes")
        # time.sleep(0.01)

        if expect_response:
            # Step 4: Receive response byte-by-byte
            print(f"\n[RX] Receiving response packet...")

            response_buffer = self.receive_raw_packet(port)
            print(f"Reponse buffer : {response_buffer}")
            if response_buffer:
                response: CommandExecutionResponse = self.validate_packet(
                    port=port, response_buffer=response_buffer
                )
                print(response)
        return response

    def read_byte_with_timeout(self, port: Serial, timeout_sec=2.0) -> Optional[int]:
        """Wait for single byte with timeout"""
        start_time = time.time()
        while (time.time() - start_time) < timeout_sec:
            assert port
            if port.in_waiting > 0:
                byte = port.read(1)
                return byte[0] if byte else None
            # time.sleep(0.01)  # Small delay to prevent busy-waiting
        return None

    def receive_raw_packet(self, port: Serial):
        response_buffer = bytearray([])

        print("[RX] - Reading ID byte...")
        packet_id = self.read_byte_with_timeout(port, TIMEOUT)

        if packet_id is None:
            print("[RX] ✗ Timeout waiting for ID byte")
            return None

        response_buffer.append(packet_id)
        print(f"[RX]   ID = 0x{packet_id:02X} ({hex(packet_id)})")

        print("[RX] - Reading Length byte...")
        payload_length = self.read_byte_with_timeout(port, TIMEOUT)

        if payload_length is None:
            print("[RX] ✗ Timeout waiting for Length byte")
            return None

        response_buffer.append(payload_length)
        print(f"[RX]   Length = {payload_length} bytes")

        if payload_length > 0:
            print(f"[RX] - Reading {payload_length} payload bytes...")

            for i in range(payload_length):
                payload_byte = self.read_byte_with_timeout(port, TIMEOUT)

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
            print("[RX] - No payload (length = 0)")

        print("[RX] - Reading 4 CRC32 bytes...")

        for i in range(4):
            crc_byte = self.read_byte_with_timeout(port=port, timeout_sec=TIMEOUT)

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
        print(f"[RX] Raw data: {' '.join([f'0x{b:02X}' for b in response_buffer])}")
        return response_buffer

    def validate_packet(
        self, port: Serial, response_buffer: bytearray
    ) -> CommandExecutionResponse:
        response = CommandExecutionResponse()
        validated_packet: Packet | None = self.receive_response_packet(
            bytes(response_buffer)
        )

        if not validated_packet:
            print(
                "\n[ERROR] Packet validation failed (CRC mismatch or malformed), request retransmit !!!"
            )

            return response
        elif validated_packet.id == ResponseType.B_RETRANSMIT.value:
            print("\n[RETRANSMIT] Last packet CRC Failed, retransmit")
            return response

        if self.is_ack(validated_packet):
            response = self.handle_response(response_packet=validated_packet)
        else:
            response.execution_success = False
            self.handle_nack(validated_packet)
        return response

    def handle_nack(self, packet: Packet):
        assert packet.payload
        print(f"Nack received | Error: {ErrorCodes(packet.payload[0]).name}")

    def is_ack(self, packet: Packet) -> bool:
        return ResponseType(packet.id) == ResponseType.B_ACK
