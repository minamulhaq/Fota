from pathlib import Path
from typing import Optional

from ..command import Command, CommandExecutionResponse, CommandIDs, CommandInfo, Packet
from .command_fw_send_bin_in_packets import (
    CommandFWSendBinInPackets,
)


class CommandFWSendBinSize(Command):
    def __init__(self) -> None:
        super().__init__()
        self.bin_file: Path = Path(".").joinpath("app", "build", "app.bin")
        assert self.bin_file.exists()

    @property
    def bin_file_size(self) -> int:
        with open(self.bin_file, "rb") as f:
            b = f.read()
            return len(b)

    @property
    def next_command(self) -> list["Command"]:
        return [CommandFWSendBinInPackets(bin_file=self.bin_file)]

    @property
    def cmd_id(self) -> CommandIDs:
        return CommandIDs.B_CMD_SEND_BIN_SIZE

    def packet(self, metadata: dict = {}) -> Packet:
        size = self.bin_file_size
        print(f"Size of binary file is : {hex(size)}")
        size = size.to_bytes(length=4, byteorder="little")
        print(f"File size: {size}")
        return Packet(id=self.cmd_id.value, payload=list(size))

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=self.cmd_id.value,
            nemonic="Command FW Send Binary File Size",
        )

    def getinput(self) -> None:
        input("Enter to send bin size")

    def handle_response(self, response_packet: Packet) -> CommandExecutionResponse:
        response = CommandExecutionResponse()

        if response_packet.payload and len(response_packet.payload) >= 8:
            # Extract 32-bit little-endian values
            addr = int.from_bytes(response_packet.payload[0:4], byteorder="little")
            total = int.from_bytes(response_packet.payload[4:8], byteorder="little")

            print(f"Start flash address: 0x{addr:08X}")
            print(f"Total packets: {total}")

            # Store them inside response object (optional)
            response.data["start_address"] = hex(addr)
            response.data["total_packets"] = hex(total)
            response.execution_success = True

        return response
