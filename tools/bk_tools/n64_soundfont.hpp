#ifndef N64_SOUNDFONT_H
#define N64_SOUNDFONT_H

#include <cstdint>
#include <algorithm>
#include <execution>

#include "json.hpp"

#include "n64_span.h"

using json = nlohmann::json;

typedef int32_t ALMicroTime;
typedef uint8_t  ALPan;

class ALEnvelope{
public:
    ALEnvelope(const n64_span & file, const uint32_t offset){
        uint32_t pos = offset;
        attackTime = file.seq_get<ALMicroTime>(pos);
        decayTime = file.seq_get<ALMicroTime>(pos);
        releaseTime = file.seq_get<ALMicroTime>(pos);
        attackVolume = file.seq_get<uint8_t>(pos);
        decayVolume = file.seq_get<uint8_t>(pos);
    }
    ALMicroTime attackTime;
    ALMicroTime decayTime;
    ALMicroTime releaseTime;
    uint8_t attackVolume;
    uint8_t decayVolume;
};

class ALSound{
public:
    ALSound(const n64_span & file, const uint32_t offset){
        uint32_t pos = offset;
        uint32_t env_offset = file.seq_get<uint32_t>(pos);
        if(env_offset){
            envelope = new ALEnvelope(file, env_offset);
        }
    }

    ALEnvelope *envelope;
};

class ALInstrument{
public:
    ALInstrument(const n64_span & file, const uint32_t offset){
        uint32_t pos = offset;
        volume = file.seq_get<uint8_t>(pos);
        pan = file.seq_get<uint8_t>(pos);
        priority = file.seq_get<uint8_t>(pos);
        flags = file.seq_get<uint8_t>(pos);
        tremType = file.seq_get<uint8_t>(pos);
        tremRate = file.seq_get<uint8_t>(pos);
        tremDepth = file.seq_get<uint8_t>(pos);
        tremDelay = file.seq_get<uint8_t>(pos);
        vibType = file.seq_get<uint8_t>(pos);
        vibRate = file.seq_get<uint8_t>(pos);
        vibDepth = file.seq_get<uint8_t>(pos);
        vibDelay = file.seq_get<uint8_t>(pos);
        bendRange = file.seq_get<int16_t>(pos);
        soundCount = file.seq_get<int16_t>(pos);

        auto sound_const = [&](uint32_t offs){return ALSound(file, offs);};
        std::vector<uint32_t> soundOffsets = file.to_vector<uint32_t>(pos, soundCount);
        soundArray.reserve(soundCount);
        std::transform(soundOffsets.begin(), soundOffsets.end(), soundArray.begin(),
            sound_const
        );
    }

    uint8_t volume;
    uint8_t pan;
    uint8_t priority;
    uint8_t flags;
    uint8_t tremType;
    uint8_t tremRate;
    uint8_t tremDepth;
    uint8_t tremDelay;
    uint8_t vibType;
    uint8_t vibRate;
    uint8_t vibDepth;
    uint8_t vibDelay;
    int16_t bendRange;
    int16_t soundCount;
    std::vector<ALSound> soundArray;
};

class ALBank{
public:
    ALBank(const n64_span & file, const uint32_t offset){
        uint32_t pos = offset;
        instCount = file.seq_get<int16_t>(pos);
        flags = file.seq_get<uint8_t>(pos);
        pad = file.seq_get<uint8_t>(pos);
        sampleRate = file.seq_get<int32_t>(pos);

        auto inst_const = [&](uint32_t offs){return ALInstrument(file, offs);};
        uint32_t percOffset = file.seq_get<uint32_t>(pos);
        if(percOffset){
            percussion = inst_const(percOffset);
        }

        std::vector<uint32_t> instOffsets = file.to_vector<uint32_t>(pos, instCount);
        instArray.reserve(instCount);
        std::transform(instOffsets.begin(), instOffsets.end(), instArray.begin(),
            inst_const
        );
    }

    int16_t instCount;
    uint8_t flags;
    uint8_t pad;
    int32_t sampleRate;
    ALInstrument percussion;
    std::vector<ALInstrument> instArray;
};

class ALBankFile{
public:
    ALBankFile(const n64_span & rom, const uint32_t offset){
        uint32_t pos = offset;
        revision = rom.seq_get<int16_t>(pos);
        bankCount = rom.seq_get<int16_t>(pos);
        bankOffsetArray = rom.to_vector<uint32_t>(pos, bankCount);

        uint32_t _end = *std::max(bankOffsetArray.begin(), bankOffsetArray.end());
        uint32_t inst_cnt = rom.get<int16_t>(offset + _end);
        _end += 0x0C + inst_cnt * 4;
        _buffer = rom.slice(offset , _end - offset);
        
        bankArray.reserve(bankCount);
        auto bank_const = [&](uint32_t offs){return ALBank(_buffer, offs);};
        std::transform(
            bankOffsetArray.begin(), bankOffsetArray.end(), bankArray.begin(),
            bank_const
        );
    }

    ~ALBankFile(){

    }

    void extract(std::string path){
        
    }

    int16_t revision;
    int16_t bankCount;
    std::vector<ALBank> bankArray;
    inline const size_t size(void){return _buffer.size();};

private:
    n64_span _buffer;
    std::vector<uint32_t> bankOffsetArray;
};
#endif
