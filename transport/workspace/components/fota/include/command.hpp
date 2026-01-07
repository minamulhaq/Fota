#ifndef INCLUDE_COMMAND_HPP
#define INCLUDE_COMMAND_HPP

#include <vector>
#include <iostream>
#include <iomanip>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <map>
#include <optional>
#include <string>
#include "esp_rom_crc.h"
#include "packet.hpp"

typedef enum error_code
{
    ERROR_INVALID_COMMAND = 0x11
} error_code_t;

typedef enum response_type
{
    B_ACK = 0xE0,
    B_NACK = 0xE1,
    B_RETRANSMIT = 0xE2
} response_type_t;

typedef enum command_id
{
    B_CMD_RETRANSMIT = 0xB0,
    B_CMD_GET_BOOTLOADER_VERSION,
    B_CMD_GET_APP_VERSION,
    B_CMD_GET_CHIP_ID,
    B_CMD_SYNC,
    B_CMD_VERIFY_DEVICE_ID,
    B_CMD_SEND_BIN_SIZE,
    B_CMD_SEND_BIN_IN_PACKETS,
    B_CMD_GET_HELP,
    B_CMD_GET_CID,
    B_CMD_GET_RDP_LVL,
    B_CMD_JMP_TO_ADDR,
    B_CMD_ERASE_FLASH,
    B_CMD_MAX
} command_id_t;

class CommandInfo
{
public:
    int id;
    std::string nemonic;
    CommandInfo(int id, const std::string &nemonic);
    ~CommandInfo() = default;
    friend inline std::ostream &
    operator<<(std::ostream &os, const CommandInfo &p)
    {
        os << "ID: " << p.id << " | " << p.nemonic;
        return os;
    }
};

class Command
{
public:
    Command();
    virtual ~Command() = default;

    virtual command_id_t get_cmd_id() const = 0;
    virtual CommandInfo get_info() const = 0;
    // virtual std::map<std::string, std::string> parse_command(const Packet_t &packet) = 0;
    // virtual std::vector<std::unique_ptr<Command>> get_next_commands() = 0;
    // virtual Packet get_packet(const std::map<std::string, std::vector<uint8_t>> &metadata = {}) = 0;
    // virtual void get_input() = 0;
    // virtual response_type_t hndandle_response(const Packet &response_packet) = 0;

    // // Concrete methods
    virtual void cmd(Packet &pkt) = 0;
    // uint32_t compute_crc32_for_packet(const Packet &pkt);
    // std::optional<Packet> receive_response_packet(const std::vector<uint8_t> &raw_data);

protected:
    uint32_t calculate_stm32_crc(uint8_t id, uint8_t length, const std::vector<uint8_t> &payload);
    static constexpr uint32_t TIMEOUT_MS = 2000;
};

class CommandRetransmit : public Command
{
public:
    command_id_t get_cmd_id() const;
    void cmd(Packet &pkt);
    CommandInfo get_info() const;
    // std::map<std::string, std::string> parse_command(const Packet_t &packet) override;

private:
};

class CommandGetBootloaderVersion : public Command
{
public:
    CommandGetBootloaderVersion();
    virtual ~CommandGetBootloaderVersion() = default;
    void cmd(Packet &pkt);
    command_id_t get_cmd_id() const;
    CommandInfo get_info() const;
    // std::map<std::string, std::string> parse_command(const Packet_t &packet) override;

private:
};

#endif // INCLUDE_COMMAND_HPP