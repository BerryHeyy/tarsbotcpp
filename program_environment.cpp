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

        std::vector<std::string> instructions;

        { // I hate this so much x2
            std::stringstream stream(lines[i]);

            std::string intermediate;

            while (std::getline(stream, intermediate, ' '))
            {
                instructions.push_back(intermediate);
            }
        }

        if (instructions.empty()) continue;

        // I hate this
        if (instructions[0] == "lda")
        {
            if (instructions[1].rfind("$", 0) == 0) // Zero Page or Absolute
            {
                if (tarsutils::get_bits(std::stoi(instructions[1].substr(1), nullptr, 16)) <= 8) // Zero Page
                {
                    if (instructions.size() != 2) { throw_instruction_exception(lines[i], instructions.size() - 1, 1); return false; }
                    memory[compilerPointer++] = CPU::INS_LDA_ZP;
                    
                    memory[compilerPointer++] = (Byte) std::stoi(instructions[1].substr(1), nullptr, 16);
                }
                else if (tarsutils::get_bits(std::stoi(instructions[1].substr(1), nullptr, 16)) <= 16) // Absolute
                {
                    if (instructions.size() != 2) { throw_instruction_exception(lines[i], instructions.size() - 1, 1); return false; }
                    memory[compilerPointer++] = CPU::INS_LDA_AS;

                    Word givenWord = std::stoi(instructions[1].substr(1), nullptr, 16);

                    memory[compilerPointer++] = (Byte)(givenWord & 0x00FF);
                    memory[compilerPointer++] = (Byte)(givenWord >> 8);
                }
                else { throw_possible_overflow_exception(lines[i], tarsutils::get_bits(std::stoi(instructions[1].substr(1), nullptr, 16)), 16); return false; }
            }
            else if (instructions[1].rfind("#", 0) == 0) // Immediate
            {
                if (instructions.size() != 2) { throw_instruction_exception(lines[i], instructions.size() - 1, 1); return false; }
                memory[compilerPointer++] = CPU::INS_LDA_IM;

                if (tarsutils::get_bits(std::stoi(instructions[1].substr(1), nullptr, 16)) <= 8)
                {
                    memory[compilerPointer++] = std::stoi(instructions[1].substr(1), nullptr, 16);
                }
                else { throw_possible_overflow_exception(lines[i], tarsutils::get_bits(std::stoi(instructions[1].substr(1), nullptr, 16)), 8); return false; }
            }
        }
        else if (instructions[0] == "pha")
        {
            if (instructions.size() != 1) { throw_instruction_exception(lines[i], instructions.size() - 1, 0); return false;  }
            memory[compilerPointer++] = CPU::INS_PHA;
        }
        else if (instructions[0] == "pla")
        {
            if (instructions.size() != 1) { throw_instruction_exception(lines[i], instructions.size() - 1, 0); return false; }
            memory[compilerPointer++] = CPU::INS_PLA;
        }
        else if (instructions[0] == "smh" || instructions[0] == "smd" || instructions[0] == "smc")
        {
            if (instructions[1].rfind("$", 0) == 0) // Zero Page or Absolute
            {
                if (tarsutils::get_bits(std::stoi(instructions[1].substr(1), nullptr, 16)) <= 8) // Zero Page
                {
                    if (instructions.size() != 2) { throw_instruction_exception(lines[i], instructions.size() - 1, 1); return false; }

                    if (instructions[0] == "smh") memory[compilerPointer++] = CPU::INS_SMH_ZP;
                    else if (instructions[0] == "smd") memory[compilerPointer++] = CPU::INS_SMD_ZP;
                    else if (instructions[0] == "smc") memory[compilerPointer++] = CPU::INS_SMC_ZP;

                    memory[compilerPointer++] = (Byte)std::stoi(instructions[1].substr(1), nullptr, 16);
                }
                else if (tarsutils::get_bits(std::stoi(instructions[1].substr(1), nullptr, 16)) <= 16) // Absolute
                {
                    if (instructions.size() != 2) { throw_instruction_exception(lines[i], instructions.size() - 1, 1); return false; }

                    if (instructions[0] == "smh") memory[compilerPointer++] = CPU::INS_SMH_AS;
                    else if (instructions[0] == "smd") memory[compilerPointer++] = CPU::INS_SMD_AS;
                    else if (instructions[0] == "smc") memory[compilerPointer++] = CPU::INS_SMC_AS;

                    Word givenWord = std::stoi(instructions[1].substr(1), nullptr, 16);

                    memory[compilerPointer++] = (Byte)(givenWord & 0x00FF);
                    memory[compilerPointer++] = (Byte)(givenWord >> 8);
                }
            }
            else if (instructions[1].rfind("#", 0) == 0) // Immediate
            {
                if (instructions.size() != 2) { throw_instruction_exception(lines[i], instructions.size() - 1, 1); return false; }

                if (instructions[0] == "smh") memory[compilerPointer++] = CPU::INS_SMH_IM;
                else if (instructions[0] == "smd") memory[compilerPointer++] = CPU::INS_SMD_IM;
                else if (instructions[0] == "smc") memory[compilerPointer++] = CPU::INS_SMC_IM;

                memory[compilerPointer++] = std::stoi(instructions[1].substr(1), nullptr, 16); //TODO: Not checking for overflow, fix if this breaks it lol

            }
            else if (instructions[1].rfind("A", 0) == 0) // Accumulator
            {
                if (instructions.size() != 2) { throw_instruction_exception(lines[i], instructions.size() - 1, 1); return false; }

                if (instructions[0] == "smh") memory[compilerPointer++] = CPU::INS_SMH_AC;
                else if (instructions[0] == "smd") memory[compilerPointer++] = CPU::INS_SMD_AC;
                else if (instructions[0] == "smc") memory[compilerPointer++] = CPU::INS_SMC_AC;
            }
        }
        else if (instructions[0] == "jmp")
        {
            if (instructions[1].rfind("$", 0) == 0) // Absolute
            {
                if (tarsutils::get_bits(std::stoi(instructions[1].substr(1), nullptr, 16)) <= 16) // Less than 16 bits
                {
                    if (instructions.size() != 2) { throw_instruction_exception(lines[i], instructions.size() - 1, 1); return false; }

                    Word givenWord = std::stoi(instructions[1].substr(1), nullptr, 16);
                    
                    memory[compilerPointer++] = CPU::INS_JMP_AS;

                    memory[compilerPointer++] = (Byte)(givenWord & 0x00FF);
                    memory[compilerPointer++] = (Byte)(givenWord >> 8);
                }
            }
            else // Label
            {
                memory[compilerPointer++] = CPU::INS_JMP_AS;

                labelReferences[instructions[1]] = compilerPointer;
                compilerPointer += 2;
            }                
        }
        else // Non supported instruction TODO: add support for labels
        {
            if (std::count(lines[i].begin(), lines[i].end(), ' ') == 0)
                if (lines[i].rfind(':') == lines[i].length() - 1) // Label
                {
                    if (labels.count(lines[i].substr(0, lines[i].length() - 1)) == 0)
                    {
                        labels[lines[i].substr(0, lines[i].length() - 1)] = compilerPointer;

                    }
                    else { throw_label_already_defined_exception(lines[i], lines[i].substr(0, lines[i].length() - 1), labels[lines[i].substr(0, lines[i].length() - 1)]); return false; }
                }
                else
                {
                    client->sendMessage(originalMessage.channelID, "Exception while compiling. At line: `" + lines[i] + "`. OPCODEException: Provided instruction (`" + instructions[0] + "`) not recognised.");
                }
        }
    }

    // Link labels
    for (auto const& x : labelReferences)
    {
        std::cout << x.first  // string (key)
            << ':'
            << x.second // string's value 
            << std::endl;

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
        case CPU::INS_PHA:
        {
            processor.push_stack(processor.A, memory);
        }break;
        case CPU::INS_PLA:
        {
            processor.A = processor.pull_stack(memory);
        }break;
        case CPU::INS_SMH_IM:
        {
            std::stringstream ss;

            ss << std::hex << std::setw(2) << std::setfill('0') << (int) processor.read_byte(memory);

            client->sendMessage(originalMessage.channelID, ss.str());
        }break;
        case CPU::INS_SMH_AS:
        {
            Word value = processor.read_byte(memory) | (processor.read_byte(memory) << 8);

            std::stringstream ss;

            ss << std::hex << std::setw(2) << std::setfill('0') << (int) value;

            client->sendMessage(originalMessage.channelID, ss.str());
        }break;
        case CPU::INS_SMH_ZP:
        {
            std::stringstream ss;

            ss << std::hex << std::setw(2) << std::setfill('0') << (int)memory[processor.read_byte(memory)];

            client->sendMessage(originalMessage.channelID, ss.str());
        }break;
        case CPU::INS_SMH_AC:
        {
            std::stringstream ss;

            ss << std::hex << std::setw(2) << std::setfill('0') << (int)processor.A;

            client->sendMessage(originalMessage.channelID, ss.str());
        }break;
        case CPU::INS_SMD_IM:
        {
            std::stringstream ss;

            ss << (int)processor.read_byte(memory);

            client->sendMessage(originalMessage.channelID, ss.str());
        }break;
        case CPU::INS_SMD_AS:
        {
            Word value = processor.read_byte(memory) | (processor.read_byte(memory) << 8);

            std::stringstream ss;

            ss << (int)value;

            client->sendMessage(originalMessage.channelID, ss.str());
        }break;
        case CPU::INS_SMD_ZP:
        {
            std::stringstream ss;

            ss << (int)memory[processor.read_byte(memory)];

            client->sendMessage(originalMessage.channelID, ss.str());
        }break;
        case CPU::INS_SMD_AC:
        {
            std::stringstream ss;

            ss << (int)processor.A;

            client->sendMessage(originalMessage.channelID, ss.str());
        }break;
        case CPU::INS_SMC_IM:
        {
            std::stringstream ss;

            ss << processor.read_byte(memory);

            client->sendMessage(originalMessage.channelID, ss.str());
        }break;
        case CPU::INS_SMC_AS:
        {
            //Word value = processor.read_byte(memory) | (processor.read_byte(memory) << 8);

            std::stringstream ss;

            Byte a = processor.read_byte(memory);

            const char userSymbol[] = { processor.read_byte(memory), a, '\x00' };

            ss << userSymbol;

            client->sendMessage(originalMessage.channelID, ss.str());

        }break;
        case CPU::INS_SMC_ZP:
        {
            std::stringstream ss;

            ss << memory[processor.read_byte(memory)];

            client->sendMessage(originalMessage.channelID, ss.str());

        }break;
        case CPU::INS_SMC_AC:
        {
            std::stringstream ss;

            ss << processor.A;

            client->sendMessage(originalMessage.channelID, ss.str());
        }break;
        case CPU::INS_JMP_AS:
        {
            processor.PC = processor.read_byte(memory) | (processor.read_byte(memory) << 8);
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
