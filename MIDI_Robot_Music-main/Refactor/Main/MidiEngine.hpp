    #ifndef MIDIENGINE_HPP
    #define MIDIENGINE_HPP
    #include <Arduino.h>
    #include <LittleFS.h>
    uint32_t readBE32(File &f);
    uint16_t readBE16(File &f);
    uint32_t readVarLen(File &f);
    bool skipN(File &f, uint32_t n);
    void delayByTicks(uint32_t dticks, uint32_t tempo_us_per_qn, uint16_t ppqn);
    static bool bufReadVarLen(TrackBuf& tb, uint32_t& out);
    inline bool bufReadByte(TrackBuf& tb, uint8_t& out);
    inline bool bufSkipN(TrackBuf& tb, uint32_t n);
    bool scheduleNextEvent(TrackBuf& tb);
    #endif

