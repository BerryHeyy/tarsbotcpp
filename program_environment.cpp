#include "program_environment.h"

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

        std::vector<std::string> instructions;

        { // I hate this so much x2
            std::stringstream stream(lines[i]);

            std::string intermediate;

            while (std::getline(stream, intermediate, ' '))
            {
                instructions.push_back(intermediate);
            }
        }

        // I hate this
        if (instructions[0] == "lda")
        {
            if (instructions[1].rfind("$", 0) == 0) // Zero Page or Absolute
            {
                if (tarsutils::get_bits(std::stoi(instructions[1].substr(1), nullptr, 16)) <= 8) // Zero Page
                {
                    memory[compilerPointer] = CPU::INS_LDA_ZP;
                    compilerPointer++;
                    
                    memory[compilerPointer] = (Byte) std::stoi(instructions[1].substr(1), nullptr, 16);
                    compilerPointer++;
                }
                else if (tarsutils::get_bits(std::stoi(instructions[1].substr(1), nullptr, 16)) <= 16) // Absolute
                {
                    memory[compilerPointer] = CPU::INS_LDA_AS;
                    compilerPointer++;

                    Word givenWord = std::stoi(instructions[1].substr(1), nullptr, 16);

                    memory[compilerPointer] = (Byte)(givenWord & 0x00FF);
                    compilerPointer++;
                    memory[compilerPointer] = (Byte)(givenWord >> 8);
                    compilerPointer++;
                }
                else { throw_possible_overflow_exception(lines[i], tarsutils::get_bits(std::stoi(instructions[1].substr(1), nullptr, 16)), 16); return false; }
            }
            else if (instructions[1].rfind("#", 0) == 0) // Immediate
            {
                memory[compilerPointer] = CPU::INS_LDA_IM;
                compilerPointer++;

                if (tarsutils::get_bits(std::stoi(instructions[1].substr(1), nullptr, 16)) <= 8)
                {
                    memory[compilerPointer] = std::stoi(instructions[1].substr(1), nullptr, 16);
                }
                else { throw_possible_overflow_exception(lines[i], tarsutils::get_bits(std::stoi(instructions[1].substr(1), nullptr, 16)), 8); return false; }
            }
        }
        else // Non supported instruction TODO: add support for labels
        {
            client->sendMessage(originalMessage.channelID, "Exception while compiling. At line: `" + lines[i] + "`. OPCODEException: Provided instruction (`" + instructions[0] + "`) not recognised.");
        }
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
        case CPU::INS_LDA_IM:
        {
            processor.A = processor.read_byte(memory);
        }break;
        case CPU::INS_LDA_ZP:
        {
            processor.A = memory[processor.read_byte(memory)];
        }break;
        case CPU::INS_LDA_AS:
        {
            Word value = processor.read_byte(memory) | (processor.read_byte(memory) << 8);
            std::cout << value << std::endl;
            processor.A = memory[value];
        }break;
        case 0x00:
        {
            continue;
        }
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
    ss << "Processor State:\n    Registries:   A = " << std::setw(2) << std::setfill('0') << (int)processor.A << " | " <<
        "X = " << std::setw(2) << std::setfill('0') << (int)processor.X << " | " <<
        "Y = " << std::setw(2) << std::setfill('0') << (int)processor.Y <<
        "\n    Status Flags: C = " << std::setw(1) << (int)processor.C << " | " <<
        "Z = " << std::setw(1) << (int)processor.Z << " | " <<
        "I = " << std::setw(1) << (int)processor.I << "\n" <<
        "                  D = " << std::setw(1) << (int)processor.D << " | " <<
        "B = " << std::setw(1) << (int)processor.B << " | " <<
        "V = " << std::setw(1) << (int)processor.V << "\n" <<
        "                  N = " << std::setw(1) << (int)processor.N << "\n\n";

    ss << "Offset    00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\n\n";

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
    ss << "Exception while compiling code. At line: `" << line << "`. PossibleOverflowException: Max: `" << expected << "`, provided: `" << provided << "`.";

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
