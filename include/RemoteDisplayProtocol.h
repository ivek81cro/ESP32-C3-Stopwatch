#pragma once
#include <Arduino.h>
namespace RemoteDisplayProtocol {
constexpr uint32_t MAGIC = 0x52445331UL;
constexpr uint8_t VERSION = 1;
enum class MessageType : uint8_t { HELLO = 1, HELLO_ACK, START, START_ACK, STOP, STOP_ACK, RESET, STATUS };
enum class RaceState : uint8_t { IDLE = 0, RUNNING = 1, FINISHED = 2 };
struct __attribute__((packed)) Packet {
    uint32_t magic; uint8_t version; uint8_t type; uint8_t state; uint8_t reserved;
    uint32_t raceId; uint32_t sequence; uint32_t elapsedMs; uint8_t hmac[32];
};
void makePacket(Packet& packet, MessageType type, RaceState state, uint32_t raceId, uint32_t sequence, uint32_t elapsedMs);
bool isValid(const Packet& packet);
}
