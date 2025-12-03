from dataclasses import dataclass

from bl_monitor.command_creator import (
    Command,
    CommandIDs,
    CommandInfo,
    Packet,
    ResponseType,
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

    @property
    def packet(self) -> Packet:
        return Packet(id=self.cmd_id.value, length=0)

    @property
    def info(self) -> CommandInfo:
        return CommandInfo(
            id=self.cmd_id.value,
            description=f"ID: {self.cmd_id.value} | {self.cmd_id.value:#04X} | Get Application Version",
            nemonic="CommandGetAppVersion",
        )

    def handle_response(self, response_packet: Packet) -> dict:
        print(f"{'=' * 60}")
        print(f"HANDLING GET APP_VERSION RESPONSE")
        print(f"{'=' * 60}")

        result = {}
        version = AppVersion.from_packet(packet=response_packet)
        print(version)
        return result
