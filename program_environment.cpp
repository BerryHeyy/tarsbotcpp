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

ProgramEnvironment::ProgramEnvironment(SleepyDiscord::Message message, std::string programCode, bool dumpMemory, bool dumpFull, BotClient *client)
{
    this->originalMessage = message;
    this->dumpMemory = dumpMemory;
    this->programCode = programCode;
    this->dumpFull = dumpFull;

    this->client = client;
}

ProgramEnvironment::~ProgramEnvironment()
{

}

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
                    memory[compilerPointer++] = CPU::registerEncoding[tokens[2].substr(1, tokens[2].length() - 1)];
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
                else if (labels.count(tokens[2]) > 1) // Value is a label
                {
                    
                }
                else // Unrecognised value, throw error
                {
                    std::stringstream ss;
                    ss << "Exception while compiling code. At line: `" << lines[i] << "`. ArgumentException: Provided argument `" << tokens[2] << "` is an invalid argument.";

                    client->sendMessage(originalMessage.channelID, ss.str());
                    return false;
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
        else // Check for label
        {

        }
    }

    if (dumpMemory) dump_memory("Pre-execution State of Memory:");
    
    return true;
}

bool ProgramEnvironment::run()
{

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
