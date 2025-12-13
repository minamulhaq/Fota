from typing import Optional

from ..command import (
    Command,
    CommandExecutionResponse,
    CommandIDs,
    CommandInfo,
    Packet,
    ResponseType,
)
from .command_fw_send_bin_size import CommandFWSendBinSize


class CommandFWVerifyDeviceID(Command):
    def __init__(self) -> None:
        super().__init__()
        self.device_id: int

    @property
    def next_command(self) -> list["Command"]:
        return [CommandFWSendBinSize()]

    @property
    def cmd_id(self) -> CommandIDs:
        return CommandIDs.B_CMD_VERIFY_DEVICE_ID

    def packet(self, metadata: dict = {}) -> Packet:
        return Packet(
            id=self.cmd_id.value,
            payload=list(self.device_id.to_bytes(2, byteorder="little")),
            length=2,
        )

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=self.cmd_id.value,
            nemonic="Command FW Verify Device ID",
        )

    def handle_response(self, response_packet: Packet) -> CommandExecutionResponse:
        response = CommandExecutionResponse()
        if response_packet.payload:
            response.execution_success = (
                response_packet.payload[0] == ResponseType.B_ACK.value
            )

        return CommandExecutionResponse(
            execution_success=True,
        )

    def getinput(self) -> None:
        self.device_id = 0x6415
        input("Enter Device ID: 0x")
        print(f"Device ID is set to: {hex(self.device_id)}")
