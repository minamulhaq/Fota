from bl_monitor.command_creator import (
    Command,
    CommandIDs,
    CommandInfo,
    Packet,
)


class CommandEraseFlash(Command):
    def __init__(self, sectors: list[int] | None = None, mass_erase: bool = False):
        self.sectors = sectors if sectors else []
        self.mass_erase = mass_erase

    @property
    def packet(self) -> Packet:
        if self.mass_erase:
            payload = [0xFF, 0xFF]
            length = 2
        else:
            payload = [len(self.sectors)] + self.sectors
            length = len(payload)

        return Packet(
            id=CommandIDs.B_CMD_ERASE_FLASH.value, length=length, payload=payload
        )

    @property
    def info(self) -> CommandInfo:
        erase_type = (
            "Mass Erase" if self.mass_erase else f"Erase Sectors: {self.sectors}"
        )
        return CommandInfo(
            id=CommandIDs.B_CMD_ERASE_FLASH.value,
            description=f"ID: {CommandIDs.B_CMD_ERASE_FLASH.value} | {CommandIDs.B_CMD_ERASE_FLASH.value:#04X} | {erase_type}",
            nemonic="CommandEraseFlash",
        )
