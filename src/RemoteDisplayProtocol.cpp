#include "RemoteDisplayProtocol.h"

#include "RemoteDisplayConfig.h"

#include <mbedtls/md.h>

#include <string.h>

namespace RemoteDisplayProtocol {
    namespace {
        bool authenticate(const Packet& p, uint8_t out[32]) {
            const mbedtls_md_info_t* i = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
            return i && mbedtls_md_hmac(i, reinterpret_cast <
                const unsigned char*> (REMOTE_DISPLAY_SECRET), strlen(REMOTE_DISPLAY_SECRET), reinterpret_cast <
                const unsigned char*> (&p), offsetof(Packet, hmac), out) == 0;
        }
    }
    void makePacket(Packet& p, MessageType t, RaceState s, uint32_t r, uint32_t q, uint32_t e) {
        memset(&p, 0, sizeof(p));
        p.magic = MAGIC;
        p.version = VERSION;
        p.type = static_cast <uint8_t> (t);
        p.state = static_cast <uint8_t> (s);
        p.raceId = r;
        p.sequence = q;
        p.elapsedMs = e;
#if REMOTE_DISPLAY_HMAC_ENABLED
        authenticate(p, p.hmac);
#endif
    }
    bool isValid(const Packet& p) {
        if (p.magic != MAGIC || p.version != VERSION || p.type < 1 || p.type > 8) return false;
#if REMOTE_DISPLAY_HMAC_ENABLED
        uint8_t expected[32];
        return authenticate(p, expected) && memcmp(expected, p.hmac, sizeof(expected)) == 0;
#else
        return true;
#endif
    }
}