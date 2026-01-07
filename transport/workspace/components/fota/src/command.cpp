
#include <format>
#include "command.hpp"

CommandInfo::CommandInfo(int id, const std::string &nemonic) : id(id), nemonic(std::move(nemonic))
{
}

command_id_t CommandRetransmit::get_cmd_id() const
{
    return B_CMD_RETRANSMIT;
}

void CommandRetransmit::cmd(Packet &pkt)
{
}

CommandInfo CommandRetransmit::get_info() const
{
    return CommandInfo{B_CMD_RETRANSMIT, "CommandRetransmit"};
}

uint32_t Command::calculate_stm32_crc(uint8_t id, uint8_t length, const std::vector<uint8_t> &payload)
{
    uint32_t crc = 0xFFFFFFFF;
    crc = esp_rom_crc32_be(crc, &id, 1);
    crc = esp_rom_crc32_be(crc, &length, 1);

    if (length > 0 && !payload.empty())
    {
        size_t actual_len = std::min(static_cast<size_t>(length), payload.size());
        crc = esp_rom_crc32_be(crc, payload.data(), actual_len);
    }
    return crc;
}

CommandGetBootloaderVersion::CommandGetBootloaderVersion() : Command()
{
}

command_id_t CommandGetBootloaderVersion::get_cmd_id() const
{
    return B_CMD_GET_BOOTLOADER_VERSION;
}

CommandInfo CommandGetBootloaderVersion::get_info() const
{
    return CommandInfo(B_CMD_GET_BOOTLOADER_VERSION, "CommandGetBootloaderVersion");
}


Command::Command()
{
}

void CommandGetBootloaderVersion::cmd(Packet &pkt)
{
    pkt.id = B_CMD_GET_BOOTLOADER_VERSION;
    pkt.length = 0;
    pkt.crc32 = pkt.calculate_packet_crc();
}
