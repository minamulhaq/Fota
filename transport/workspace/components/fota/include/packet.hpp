#ifndef _INCLUDE_PACKET_HPP__
#define _INCLUDE_PACKET_HPP__

#include <cstdint>
#include <vector>

#define MAX_PAYLOAD_SIZE 16

typedef struct Packet
{
    uint8_t id;
    uint8_t length;
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint32_t crc32;
    friend std::ostream &operator<<(std::ostream &os, const Packet &p);
    uint32_t calculate_packet_crc() const;

} Packet_t;

#endif // _INCLUDE_PACKET_HPP__
