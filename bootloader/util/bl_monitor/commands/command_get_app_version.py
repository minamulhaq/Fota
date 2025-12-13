from dataclasses import dataclass
from typing import Optional

from ..command import (
    Command,
    CommandExecutionResponse,
    CommandIDs,
    CommandInfo,
    Packet,
)


@dataclass
class AppVersion:
    Major: int
    Minor: int
    Patch: int

    def __str__(self) -> str:
        return f"Application Version: {self.Major}.{self.Minor}.{self.Patch}"

    @staticmethod
    def from_packet(packet: Packet) -> "AppVersion":
        assert packet.payload
        return AppVersion(
            Major=packet.payload[0], Minor=packet.payload[1], Patch=packet.payload[2]
        )


class CommandGetAppVersion(Command):
    @property
    def cmd_id(self) -> CommandIDs:
        return CommandIDs.B_CMD_GET_APP_VERSION

    def packet(self, metadata: dict = {}) -> Packet:
        return Packet(id=self.cmd_id.value, length=0)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=self.cmd_id.value,
            nemonic="Command Get App Version",
        )

    def handle_response(
        self, response_packet: Packet
    ) -> CommandExecutionResponse:
        response = CommandExecutionResponse()
        print(f"{'=' * 60}")
        print(f"HANDLING GET APP_VERSION RESPONSE")
        print(f"{'=' * 60}")

        response.data = {}
        response.data["version"] = AppVersion.from_packet(packet=response_packet)
        response.execution_success = True
        return response

    def getinput(self) -> None:
        return


    @property
    def next_command(self) -> list["Command"]:
        return []

