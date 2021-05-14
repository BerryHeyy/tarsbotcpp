#pragma once

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <map>
#include "sleepy_discord/sleepy_discord.h"
#include "bot_client.h"
#include "util.h"

using Byte = uint8_t; 
using Word = uint16_t;

class BotClient;

struct MEM
{
    static constexpr Word MAX_MEM = 0xFFFF;
    Byte* data = new Byte[MAX_MEM];

    MEM()
    {
        for (Word i = 0; i < MAX_MEM; i++)
        {
            data[i] = 0;
        }
    }

    ~MEM()
    {
        delete[] data;
    }

    Byte operator[](Word address) const // Read byte
    {
        if (address < MAX_MEM) return data[address];
        else throw std::overflow_error("tried indexing address greater than 0xFFFF");
    }

    Byte& operator[](Word address) // Write byte
    {
        if (address < MAX_MEM) return data[address];
        else throw std::overflow_error("tried indexing address greater than 0xFFFF");
    }
};

struct Register
{
    Word value;

    Register()
    {
        value = 0;
    }

    Byte lower()
    {
        return (Byte)value & 0x00FF;
    }

    Byte upper()
    {
        return (Byte)value >> 8;
    }

    void set_lower(Byte lower)
    {
        value = (value & 0xFF00) | lower;
    }

    void set_upper(Byte upper)
    {
        value = (value & 0x00FF) | (upper << 8);
    }
};

struct CPU
{
public:

    Word PC; // Program Counter

    Register AX, BX, CX, DX, SI, DI, SP,
        BP;

    uint64_t R8, R9, R10, R11, R12, R13,
        R14, R15;

    Byte CF : 1;

public:

    static constexpr Byte // Op Codes
        INS_MOV_AS = 0x01,
        INS_MOV_IM = 0x02,
        INS_CMP_IM = 0x03,
        INS_JNE = 0x04,
        INS_PUSHA = 0x05,
        INS_POPA = 0x06,
        INS_JMP = 0x07,
        INS_CALL = 0x08,
        INS_RET = 0x09,
        INS_PRTC_IM = 0x0A,
        INS_ADD_IM = 0x0B,
        INS_MOV_RV = 0x0C,
        INS_PRTC_RV = 0x0D,
        // Pseudo Ops
        INS_DB = 0xF0;

    static std::map<std::string, Byte> registerEncoding;
    static constexpr Byte POINTER_INDICATOR = 0xF8;

    CPU()
    {
        reset();
    }

    void reset()
    {
        PC = 0x0200;
        SP.value = 0x0100;
        R8 = R9 = R10 = R11 = R12 = R13 = R14 = R15 = 0;
        CF = 0;
    }

    Byte read_byte(MEM &memory)
    {
        return memory[PC++];
    }

    void write_byte(Byte toWrite, MEM& memory)
    {
        memory[PC++] = toWrite;
    }

    Byte pull_stack(MEM& memory)
    {
        SP.value--;
        Byte toReturn = memory[SP.value];
        memory[SP.value] = 0x00;
        return toReturn;
    }

    void push_stack(Byte toWrite, MEM& memory)
    {
        memory[SP.value++] = toWrite;
    }

    uint64_t get_register_value(Byte encoding);
    void set_register_value(Byte encoding, uint64_t value);
};

class ProgramEnvironment
{
private:
    BotClient* client;

    SleepyDiscord::Message originalMessage;
    std::string programCode;
    bool dumpMemory, dumpFull;

    std::string consoleBuffer;

    CPU processor;
    MEM memory;

public:
    ProgramEnvironment(SleepyDiscord::Message message, std::string programCode, bool dumpMemory, bool dumpFull, BotClient* client);
    ~ProgramEnvironment();

public:
    bool compile();

    bool run();

    void dump_memory(std::string messageContent);

private:

    void throw_possible_overflow_exception(std::string line, int provided, int expected);
    void throw_instruction_exception(std::string line, int provided, int expected);
    void throw_hex_string_format_exception(std::string line, std::string provided);
    void throw_label_already_defined_exception(std::string line, std::string label, Word address);
    void throw_label_not_found_exception(Word address, std::string label);
};