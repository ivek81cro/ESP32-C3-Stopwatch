#include "RemoteDisplayReceiver.h"

#include "RemoteDisplayConfig.h"

#include <WiFi.h>

using namespace RemoteDisplayProtocol;
RemoteDisplayReceiver::RemoteDisplayReceiver(DisplayTimeFn a, DisplayStateFn b, DisplayStateFn c) : _displayTime(a), _displayIdle(b), _displayOffline(c) {}
void RemoteDisplayReceiver::begin() {
    _udp.begin(REMOTE_DISPLAY_PORT);
}
void RemoteDisplayReceiver::sendPacket(MessageType type, uint32_t raceId) {
    Packet p;
    makePacket(p, type, RaceState::IDLE, raceId, _nextSequence++, 0);
    const IPAddress target = _discovered ? _udp.remoteIP() : IPAddress(255, 255, 255, 255);
    _udp.beginPacket(target, REMOTE_DISPLAY_PORT);
    _udp.write(reinterpret_cast <
        const uint8_t*> (&p), sizeof(p));
    _udp.endPacket();
}
void RemoteDisplayReceiver::handlePacket(const Packet& p) {
    const MessageType t = static_cast <MessageType> (p.type);
    _lastPacketMs = millis();
    if (t == MessageType::HELLO_ACK) {
        _discovered = true;
        Serial.println("Stopwatch discovered");
        return;
    }
    if (t == MessageType::RESET) {
        _state = DisplayState::IDLE;
        _visualBaseMs = 0;
        _acceptCurrentRace = false;
        _displayIdle();
        return;
    }
    if (p.raceId < _raceId) return;
    if (t == MessageType::START) {
        if (p.raceId > _raceId) {
            _raceId = p.raceId;
            _visualBaseMs = p.elapsedMs;
            _visualStartMs = millis();
            _state = DisplayState::RUNNING;
            _acceptCurrentRace = true;
            Serial.printf("START race=%lu\n", _raceId);
        }
        if (p.raceId == _raceId && _acceptCurrentRace && _state == DisplayState::RUNNING) sendPacket(MessageType::START_ACK, p.raceId);
    }
    else if (t == MessageType::STATUS && static_cast <RaceState> (p.state) == RaceState::RUNNING) {
        if (p.raceId > _raceId) {
            _raceId = p.raceId;
            _visualBaseMs = p.elapsedMs;
            _visualStartMs = millis();
            _state = DisplayState::RUNNING;
            _acceptCurrentRace = true;
            Serial.printf("Recovered running race=%lu\n", _raceId);
        }
    }
    else if (t == MessageType::STOP) {
        if (p.raceId > _raceId) {
            _raceId = p.raceId;
            _acceptCurrentRace = true;
        }
        if (p.raceId == _raceId && _acceptCurrentRace) {
            _visualBaseMs = p.elapsedMs;
            _state = DisplayState::FINISHED;
            _displayTime(_visualBaseMs, true);
            sendPacket(MessageType::STOP_ACK, p.raceId);
            Serial.printf("STOP race=%lu time=%lu\n", _raceId, _visualBaseMs);
        }
    }
}
void RemoteDisplayReceiver::processPackets() {
    for (uint8_t i = 0; i < 4; ++i) {
        int n = _udp.parsePacket();
        if (n <= 0) break;
        if (n != static_cast <int>(sizeof(Packet))) {
            while (_udp.available()) _udp.read();
            continue;
        }
        Packet p;
        if (_udp.read(reinterpret_cast <uint8_t*>(&p), sizeof(p)) == sizeof(p) && isValid(p)) handlePacket(p);
    }
}
void RemoteDisplayReceiver::update() {
    processPackets();
    uint32_t now = millis();
    if (!_discovered && WiFi.status() == WL_CONNECTED && now - _lastHelloMs >= REMOTE_DISPLAY_HELLO_INTERVAL_MS) {
        _lastHelloMs = now;
        sendPacket(MessageType::HELLO, 0);
    }
    if (_state == DisplayState::RUNNING) _displayTime(_visualBaseMs + (now - _visualStartMs), false);
    if (_discovered && _state != DisplayState::FINISHED && now - _lastPacketMs > REMOTE_DISPLAY_OFFLINE_TIMEOUT_MS) {
        _discovered = false;
        _state = DisplayState::OFFLINE;
        _displayOffline();
        Serial.println("Stopwatch offline");
    }
}
