#pragma once

#include <Arduino.h>

#include <WiFiUdp.h>

#include "RemoteDisplayProtocol.h"

class RemoteDisplayReceiver {
public: enum class DisplayState : uint8_t {
    IDLE,
    RUNNING,
    FINISHED,
    OFFLINE
};
      using DisplayTimeFn = void(*)(uint32_t, bool force);using DisplayStateFn = void(*)();
      RemoteDisplayReceiver(DisplayTimeFn timeFn, DisplayStateFn idleFn, DisplayStateFn offlineFn);
      void begin();void update();
private: WiFiUDP _udp;DisplayTimeFn _displayTime;DisplayStateFn _displayIdle;DisplayStateFn _displayOffline;
       DisplayState _state = DisplayState::IDLE;uint32_t _raceId = 0;uint32_t _visualBaseMs = 0;uint32_t _visualStartMs = 0;
       uint32_t _lastHelloMs = 0;uint32_t _lastPacketMs = 0;uint32_t _nextSequence = 1;bool _discovered = false;
       bool _acceptCurrentRace = false;
       void sendPacket(RemoteDisplayProtocol::MessageType type, uint32_t raceId);void processPackets();void handlePacket(const RemoteDisplayProtocol::Packet& packet);
};
