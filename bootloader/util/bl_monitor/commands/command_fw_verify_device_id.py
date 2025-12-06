from typing import Optional

from bl_monitor.command import (
    Command,
    CommandExecutionResponse,
    CommandIDs,
    CommandInfo,
    Packet,
    ResponseType,
)


class CommandFWVerifyDeviceID(Command):
    def __init__(self) -> None:
        super().__init__()
        self.device_id: int

    @property
    def next_command(self) -> Optional["Command"]:
        return None

    @property
    def cmd_id(self) -> CommandIDs:
        return CommandIDs.B_CMD_VERIFY_DEVICE_ID

    @property
    def packet(self) -> Packet:
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
        print(f"{'=' * 60}")
        print(f"HANDLING {self.info.nemonic}")
        print(f"{'=' * 60}")

        response = CommandExecutionResponse()
        if response_packet.payload:
            response.execution_status = (
                response_packet.payload[0] == ResponseType.B_ACK.value
            )

        print(f"{'=' * 70}")
        print("COMMAND EXECUTION COMPLETE")
        print(f"{'=' * 70}\n")

        return CommandExecutionResponse(execution_status=True, run_next_command=True)

    def getinput(self) -> None:
        self.device_id = 0x6415
        input("Enter Device ID: 0x")
        print(f"Device ID is set to: {hex(self.device_id)}")
