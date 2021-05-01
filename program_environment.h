#pragma once

#include <iostream>
#include <stdexcept>
#include "sleepy_discord/sleepy_discord.h"
#include "bot_client.h"
#include "util.h"

using Byte = unsigned char;
using Word = unsigned short;

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

struct CPU
{
public:

    Word PC; // Program Counter
    Word SP; // Stack Pointer

    Byte A, X, Y; // Registers

    Byte C : 1, // Status Flags
        Z : 1,
        I : 1,
        D : 1,
        B : 1,
        V : 1,
        N : 1;

public:

    static constexpr Byte // Op Codes
        // LDA
        INS_LDA_IM = 0xA9,
        INS_LDA_ZP = 0xA5,
        INS_LDA_AS = 0xAD,
        // PHA
        INS_PHA = 0x48,
        // PLA
        INS_PLA = 0x68,
        // Custom Op Codes 0x*F, 0x*B, 0x*7, 0x*3
        INS_PUS_ST = 0x1F,
        INS_BUP_ST = 0x2F,
        INS_SMH_IM = 0x3F,
        INS_SMH_ZP = 0x4F,
        INS_SMD_IM = 0x5F,
        INS_SMD_ZP = 0x6F,
        INS_SMC_IM = 0x7F,
        INS_SMC_ZP = 0x8F;

    CPU()
    {
        reset();
    }

    void reset()
    {
        PC = 0x0200;
        SP = 0x0100;
        C = Z = I = D = B = V = N = 0;
        A = X = Y = 0;
    }

    Byte read_byte(MEM &memory)
    {
        return memory[PC++];
    }

    void write_byte(Byte toWrite, MEM& memory)
    {
        memory[PC++] = toWrite;
    }
};

class ProgramEnvironment
{
private:
    BotClient* client;

    SleepyDiscord::Message originalMessage;
    std::string programCode;
    bool dumpMemory, dumpFull;

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

};