#include "program_environment.h"

std::map<std::string, Byte> CPU::registerEncoding 
{
    { "al", 0x00 }, { "bl", 0x01 }, { "cl", 0x02 }, { "dl", 0x03 }, { "ah", 0x04 }, { "bh", 0x05 },
    { "ch", 0x06 }, { "dh", 0x07 }, // 8-bit registers

    { "ax", 0x08 }, { "bx", 0x09 }, { "cx", 0x0A }, { "dx", 0x0B }, { "si", 0x0C }, { "di", 0x0D },
    { "sp", 0x0E }, { "bp", 0x0F }, // 16-bit registers

    { "r8", 0x10 }, { "r9", 0x11 }, { "r10", 0x12 }, { "r11", 0x13 }, { "r12", 0x14 }, 
    { "r13", 0x15 }, { "r14", 0x16 }, { "r15", 0x17 } // 64-bit registers
}; // I check for sizes with these assigned encoding values in the assembler. 
   // THIS IS VERY ERROR PRONE IF I CHANGE THIS MAP IN ANY WAY.

uint64_t CPU::get_register_value(Byte encoding)
{
    switch (encoding)
    {
    case 0x00: return AX.lower();
    case 0x01: return BX.lower();
    case 0x02: return CX.lower();
    case 0x03: return DX.lower();
    case 0x04: return AX.upper();
    case 0x05: return BX.upper();
    case 0x06: return CX.upper();
    case 0x07: return DX.upper();

    case 0x08: return AX.value;
    case 0x09: return BX.value;
    case 0x0A: return CX.value;
    case 0x0B: return DX.value;
    case 0x0C: return SI.value;
    case 0x0D: return DI.value;
    case 0x0E: return SP.value;
    case 0x0F: return BP.value;

    case 0x10: return R8;
    case 0x11: return R9;
    case 0x12: return R10;
    case 0x13: return R11;
    case 0x14: return R12;
    case 0x15: return R13;
    case 0x16: return R14;
    case 0x17: return R15;
    }
}

void CPU::set_register_value(Byte encoding, uint64_t value)
{
    switch (encoding)
    {
    case 0x00: AX.set_lower(value); break;
    case 0x01: BX.set_lower(value); break;
    case 0x02: CX.set_lower(value); break;
    case 0x03: DX.set_lower(value); break;
    case 0x04: AX.set_upper(value); break;
    case 0x05: BX.set_upper(value); break;
    case 0x06: CX.set_upper(value); break;
    case 0x07: DX.set_upper(value); break;

    case 0x08: AX.value = value; break;
    case 0x09: BX.value = value; break;
    case 0x0A: CX.value = value; break;
    case 0x0B: DX.value = value; break;
    case 0x0C: SI.value = value; break;
    case 0x0D: DI.value = value; break;
    case 0x0E: SP.value = value; break;
    case 0x0F: BP.value = value; break;

    case 0x10: R8 = value; break;
    case 0x11: R9 = value; break;
    case 0x12: R10 = value; break;
    case 0x13: R11 = value; break;
    case 0x14: R12 = value; break;
    case 0x15: R13 = value; break;
    case 0x16: R14 = value; break;
    case 0x17: R15 = value; break;
    }
}

ProgramEnvironment::ProgramEnvironment(SleepyDiscord::Message message, std::string programCode, bool dumpMemory, bool dumpFull, BotClient *client)
{
    this->originalMessage = message;
    this->dumpMemory = dumpMemory;
    this->programCode = programCode;
    this->dumpFull = dumpFull;

    this->client = client;
}

ProgramEnvironment::~ProgramEnvironment() {}

bool ProgramEnvironment::compile()
{

    std::map<std::string, Word> labels;
    std::map<std::string, Word> labelReferences;

    Word compilerPointer = 0x200;

    std::vector<std::string> lines;

    { // I hate this so much
        std::stringstream stream(programCode);

        std::string intermediate;

        while (std::getline(stream, intermediate, '\n'))
        {
            lines.push_back(intermediate);
        }
    }

    for (int i = 0; i < lines.size(); i++)
    {

        std::vector<std::string> tokens;

        { // I hate this so much x2
            std::stringstream stream(lines[i]);

            std::string intermediate;

            while (std::getline(stream, intermediate, ' '))
            {
                if (intermediate.empty()) continue; 
                if (intermediate.rfind(',') == intermediate.length() - 1) intermediate = intermediate.substr(0, intermediate.length() - 1);
                tokens.push_back(intermediate);
            }
        }

        if (tokens.empty()) continue; // Empty line

    begin_check:

        if (tokens[0] == "mov")
        {
            if (CPU::registerEncoding.count(tokens[1]) > 0) // Destination exists
            {
                if (CPU::registerEncoding.count(tokens[2]) > 0) // Value is another register
                {
                    //if (tarsutils::get_register_size(CPU::registerEncoding[tokens[1]]) >=
                    //    tarsutils::get_register_size(CPU::registerEncoding[tokens[2]]))
                    //else // registers different sizes

                    memory[compilerPointer++] = CPU::INS_MOV_RV;
                    memory[compilerPointer++] = CPU::registerEncoding[tokens[1]];
                    memory[compilerPointer++] = CPU::registerEncoding[tokens[2]];
                }
                else if ((tokens[2].find('[') == 0) && (tokens[2].rfind(']') == tokens[2].length() - 1)) // Value is a pointer register
                {
                    memory[compilerPointer++] = CPU::INS_MOV_RV;
                    memory[compilerPointer++] = CPU::registerEncoding[tokens[1]];
                    memory[compilerPointer++] = CPU::POINTER_INDICATOR;
                    memory[compilerPointer++] = CPU::registerEncoding[tokens[2].substr(1, tokens[2].length() - 2)];
                }
                else if (tarsutils::valid_hex_string(tokens[2])) // Value is a number
                {
                    int regBits = tarsutils::get_register_size(CPU::registerEncoding[tokens[1]]);
                    int valueBits = tarsutils::get_bits(std::stoull(tokens[2], nullptr, 16));
                    if (regBits >= valueBits)
                    {
                        memory[compilerPointer++] = CPU::INS_MOV_IM;
                        memory[compilerPointer++] = CPU::registerEncoding[tokens[1]];

                        std::vector<Byte> bytes = tarsutils::int_to_little_endian_byte_array(std::stoull(tokens[2], nullptr, 16));

                        for (int i = 0; i < bytes.size(); i++)
                        {
                            memory[compilerPointer++] = bytes[i];
                        }

                        int remainingBytes = regBits / 8 - bytes.size();

                        if (remainingBytes / 8 != 0)
                        {
                            while (remainingBytes > 0)
                            {
                                memory[compilerPointer++] = 0x00;
                                remainingBytes--;
                            }
                        }
                    }
                    else // Throw overflow
                    {
                        throw_possible_overflow_exception(lines[i], tarsutils::get_bits(std::stoull(tokens[2], nullptr, 16)), tarsutils::get_register_size(CPU::registerEncoding[tokens[1]]));
                        return false;
                    }
                }
                else // Value is a label
                {
                    memory[compilerPointer++] = CPU::INS_MOV_AS;
                    memory[compilerPointer++] = CPU::registerEncoding[tokens[1]];

                    labelReferences[tokens[2]] = compilerPointer;
                    compilerPointer += 2;
                }
            }
            else // Destination does not exist
            {
                std::stringstream ss;
                ss << "Exception while compiling code. At line: `" << lines[i] << "`. ArgumentException: Provided argument `" << tokens[1] << "` is an invalid argument.";

                client->sendMessage(originalMessage.channelID, ss.str());
                return false;
            }
        }
        else if (tokens[0] == "db")
        {
            for (int j = 1; j < tokens.size(); j++) // Things to write
            {
                if (tokens[j].find('"') == 0 && tokens[j].rfind('"') == tokens[j].size() - 1) // String
                {
                    std::string insideString = tokens[j].substr(1, tokens[j].size() - 2);
                    const char* charArray = insideString.c_str();

                    for (int k = 0; k < tokens[j].length() - 2; k++)
                    {
                        memory[compilerPointer++] = charArray[k];
                    }
                }
                else if (tarsutils::valid_hex_string(tokens[j])) // Byte literal
                {
                    if (tarsutils::get_bits(std::stoi(tokens[j], nullptr, 16)) <= 8)
                    {
                        memory[compilerPointer++] = std::stoi(tokens[j], nullptr, 16);
                    }
                    else { throw_possible_overflow_exception(lines[i], tarsutils::get_bits(std::stoi(tokens[j], nullptr, 16)), 8); return false; }
                }
            }
        }
        else // Check for label
        {
            if (tokens[0].rfind(':') == tokens[0].length() - 1) // First token is a label
            {
                if (tokens.size() == 1) // Line is only a token
                {
                    std::string labelName = tokens[0].substr(0, tokens[0].length() - 1);

                    if (labels.count(labelName) == 0) // Label doesnt already exist
                    {
                        labels[labelName] = compilerPointer;
                    }
                    else { throw_label_already_defined_exception(lines[i], labelName, labels[labelName]); return false; }
                }
                else // Line is token + inline line
                {
                    std::string labelName = tokens[0].substr(0, tokens[0].length() - 1);

                    if (labels.count(labelName) == 0) // Label doesnt already exist
                    {
                        labels[labelName] = compilerPointer;
                    }
                    else { throw_label_already_defined_exception(lines[i], labelName, labels[labelName]); return false; }

                    tokens.erase(tokens.begin());

                    goto begin_check;
                }
            }
            else // Throw undefined operation exception
            {

            }
        }
    }

    // Link labels
    for (auto const& x : labelReferences)
    {
        if (labels.count(x.first) > 0) // Label exists
        {
            memory[x.second] = (Byte)(labels[x.first] & 0x00FF);
            memory[x.second + 1] = (Byte)(labels[x.first] >> 8);
        }
        else { throw_label_not_found_exception(x.second, x.first); return false; }
    }

    if (dumpMemory) dump_memory("Pre-execution State of Memory:");
    
    return true;
}

bool ProgramEnvironment::run()
{

    while (processor.PC < MEM::MAX_MEM)
    {
        Byte instruction = processor.read_byte(memory);

        switch (instruction)
        {
        case CPU::INS_MOV_RV:
        {
            Byte destinationEncoding = processor.read_byte(memory);
            Byte sourceEncoding = processor.read_byte(memory);

            if (sourceEncoding == CPU::POINTER_INDICATOR) // Value is address pointer
            {
                Byte registerWithPointer = processor.read_byte(memory);

                processor.set_register_value(destinationEncoding, memory[processor.get_register_value(registerWithPointer)]);
            }
            else
            {
                processor.set_register_value(destinationEncoding, processor.get_register_value(sourceEncoding));
            }

        }break;
        case CPU::INS_MOV_IM:
        {
            Byte destinationEncoding = processor.read_byte(memory);

            std::vector<Byte> bytes;

            for (int i = 0; i < tarsutils::get_register_size(destinationEncoding) / 8; i++)
            {
                bytes.push_back(processor.read_byte(memory));
            }

            uint64_t value;
            std::memcpy(&value, &bytes[0], tarsutils::get_register_size(destinationEncoding) / 8);

            processor.set_register_value(destinationEncoding, value);
        }break;
        case CPU::INS_MOV_AS:
        {
            Byte destinationEncoding = processor.read_byte(memory);
            Word value = processor.read_byte(memory) | (processor.read_byte(memory) << 8);

            processor.set_register_value(destinationEncoding, value);
        }break;
        case 0x00:
        {
            continue;
        }break;
        }
    }


    if (dumpMemory) dump_memory("Post-execution State of Memory:");

    return true;
}

void ProgramEnvironment::dump_memory(std::string messageContent)
{
    std::string fileName = "memoryDump.txt";

    // Remove file if exists
    if (static_cast<bool>(std::ifstream(fileName))) std::remove(fileName.c_str());

    std::stringstream ss;
    ss << std::hex;

    // Processor State
    ss << "Processor State:\n    16-bit Registers:   AX = " << std::setw(4) << std::setfill('0') << processor.AX.value << " | " <<
        "BX = " << std::setw(4) << std::setfill('0') << processor.BX.value << " | " <<
        "CX = " << std::setw(4) << std::setfill('0') << processor.CX.value <<
        "\n                        DX = " << std::setw(4) << processor.DX.value << " | " <<
        "SI = " << std::setw(4) << std::setfill('0') << processor.SI.value << " | " <<
        "DI = " << std::setw(4) << std::setfill('0') << processor.DI.value <<
        "\n                        SP = " << std::setw(4) << std::setfill('0') << processor.SP.value << " | " <<
        "BP = " << std::setw(4) << std::setfill('0') << processor.BP.value << " | " <<
        "\n    64-bit Registers:   R8  = " << std::setw(16) << std::setfill('0') << processor.R8 << " | " <<
        "R9  = " << std::setw(16) << std::setfill('0') << processor.R9 <<
        "\n                        R10 = " << std::setw(16) << std::setfill('0') << processor.R10 << " | " <<
        "R11 = " << std::setw(16) << std::setfill('0') << processor.R11 <<
        "\n                        R12 = " << std::setw(16) << std::setfill('0') << processor.R12 << " | " <<
        "R13 = " << std::setw(16) << std::setfill('0') << processor.R13 <<
        "\n                        R14 = " << std::setw(16) << std::setfill('0') << processor.R14 << " | " <<
        "R15 = " << std::setw(16) << std::setfill('0') << processor.R15 << "\n\n";

    ss << "Offset    00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n";

    for (int i = 0; i < MEM::MAX_MEM; i++)
    {
        if (i % 16 == 0) ss << "\n" << "0x" << std::setw(4) << std::setfill('0') << i << "    ";
        ss << std::setw(2) << std::setfill('0') << (int)memory[i] << " ";
    }
    
    std::ofstream outfile(fileName);
    outfile << ss.str();
    outfile.close();

    client->uploadFile(originalMessage.channelID, fileName, messageContent);
}

void ProgramEnvironment::throw_possible_overflow_exception(std::string line, int provided, int expected)
{
    std::stringstream ss;
    ss << "Exception while compiling code. At line: `" << line << "`. PossibleOverflowException: Max: `" << expected << "` bits, provided: `" << provided << "` bits.";

    client->sendMessage(originalMessage.channelID, ss.str());
}

void ProgramEnvironment::throw_instruction_exception(std::string line, int provided, int expected)
{
    std::stringstream ss;
    ss << "Exception while compiling code. At line: `" << line << "`. InstructionException: Provided: `" << provided << "` instructions, expected: `" << expected << " instructions`.";

    client->sendMessage(originalMessage.channelID, ss.str());
}

void ProgramEnvironment::throw_hex_string_format_exception(std::string line, std::string provided)
{
    std::stringstream ss;
    ss << "Exception while compiling code. At line: `" << line << "`. HexStringFormatException: Provided (`" << provided << "`) hex number is not a valid hex number.";

    client->sendMessage(originalMessage.channelID, ss.str());
}

void ProgramEnvironment::throw_label_already_defined_exception(std::string line, std::string label, Word address)
{
    std::stringstream ss;
    ss << "Exception while compiling code. At line: `" << line << "`. LabelAlreadyProvidedException: Provided label (`" << label << "`) is already defined to point at 0x" << std::hex << std::setw(4) << std::setfill('0') << address << ".";

    client->sendMessage(originalMessage.channelID, ss.str());
}

void ProgramEnvironment::throw_label_not_found_exception(Word address, std::string label)
{
    std::stringstream ss;
    ss << "Exception while compiling code. At address: `" << std::hex << std::setw(4) << std::setfill('0') << address << "`. LabelNotFoundException: Provided label (`" << label << "`) has not been defined.";

    client->sendMessage(originalMessage.channelID, ss.str());
}
