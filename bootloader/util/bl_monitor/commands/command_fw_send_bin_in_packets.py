import time
from dataclasses import dataclass, field
from pathlib import Path

from serial import Serial

from ..command import Command, CommandExecutionResponse, CommandIDs, CommandInfo, Packet

MAX_PAYLOAD = 16


@dataclass
class BinFWUpdateMetaData:
    bin_file_path: Path
    current_packet_count: int = 0
    total_packets: int = field(init=False)
    bin_size: int = field(init=False)
    raw_bytes: bytes = field(init=False)

    def __post_init__(self):
        with open(self.bin_file_path, "rb") as f:
            self.raw_bytes = f.read()
            self.bin_size = len(self.raw_bytes)
            self.total_packets = (self.bin_size + MAX_PAYLOAD - 1) // MAX_PAYLOAD

    def __str__(self) -> str:
        return (
            f"BinFWUpdateMetaData(\n"
            f"  bin_file_path       = {self.bin_file_path},\n"
            f"  current_packet_count= {self.current_packet_count},\n"
            f"  total_packets       = {self.total_packets},\n"
            f"  bin_size            = {self.bin_size},\n"
            f"  raw_bytes(len)      = {len(self.raw_bytes)}\n"
            f")"
        )

    def generator_bin_bytes_for_packet(self):
        """
        Yields packets of MAX_PAYLOAD bytes.
        Updates current_packet_count automatically.
        """
        for i in range(0, self.bin_size, MAX_PAYLOAD):
            packet = self.raw_bytes[i : i + MAX_PAYLOAD]
            self.current_packet_count += 1
            yield packet


class CommandFWSendBinInPackets(Command):
    def __init__(self, bin_file: Path) -> None:
        super().__init__()
        self.bin_file: Path = bin_file
        assert self.bin_file.exists()
        self.bin_fw_update_metadata: BinFWUpdateMetaData = BinFWUpdateMetaData(
            bin_file_path=self.bin_file
        )
        print(self.bin_fw_update_metadata)

    @property
    def next_command(self) -> list["Command"]:
        return []

    @property
    def bin_file_size(self) -> int:
        return self.bin_fw_update_metadata.bin_size

    @property
    def cmd_id(self) -> CommandIDs:
        return CommandIDs.B_CMD_SEND_BIN_IN_PACKETS

    def packet(self, metadata: dict = {}) -> Packet:
        return Packet(id=self.cmd_id.value, payload=list(metadata["bin_bytes"]))

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=self.cmd_id.value,
            nemonic="Command FW Send Binary File In Packets",
        )

    def getinput(self) -> None:
        input("Enter to send bin size")

    def handle_response(self, response_packet: Packet) -> CommandExecutionResponse:
        response = CommandExecutionResponse()
        if response_packet.payload and len(response_packet.payload) >= 8:
            addr = int.from_bytes(response_packet.payload[0:4], byteorder="little")
            current = int.from_bytes(response_packet.payload[4:8], byteorder="little")
            response.data["start_address"] = hex(addr)
            response.data["current_packet"] = hex(current)
            response.execution_success = True

        return response

    def cmd(self, pkt: Packet) -> bytearray:
        """Generate raw byte array to send: id + length + payload + crc (little-endian)"""
        pkt.crc32 = self.compute_crc32_for_packet(pkt)
        raw = bytearray([pkt.id & 0xFF, pkt.length & 0xFF])
        if pkt.length > 0:
            assert pkt.payload, "Payload can't be None"
            raw.extend([b & 0xFF for b in pkt.payload])
        raw.extend(pkt.crc32.to_bytes(4, byteorder="little"))
        return raw

    def process_commmand(self, port: Serial) -> CommandExecutionResponse:
        response: CommandExecutionResponse = CommandExecutionResponse()
        response.execution_success = True
        input("Enter to Start update ? ")

        start = time.time()
        for i, bb in enumerate(
            self.bin_fw_update_metadata.generator_bin_bytes_for_packet()
        ):
            packet = self.packet(metadata={"bin_bytes": bb})
            print(f"Raw Packet no: {hex(i)}\n{packet}")
            response = self.send_command(
                port=port,
                raw_cmd=self.cmd(pkt=packet),
                expect_response=True,
                show_debug=True,
            )

        end = time.time()
        print(f"Total time: {end - start}")
        return response
