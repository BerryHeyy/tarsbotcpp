#include "bot_client.h"

BotClient::BotClient(const char* token) : SleepyDiscord::DiscordClient(token, SleepyDiscord::USER_CONTROLED_THREADS)
{

}

void BotClient::onMessage(SleepyDiscord::Message message)
{
    if (message.author.bot) return;

    std::string messageContent = message.content;

    if (messageContent.find("&", 0) == 0)
    {
        messageContent = messageContent.substr(1);

        bool dumpMemory = messageContent.rfind("--dumpMemory") != std::string::npos;
        bool dumpFull = messageContent.rfind("--dumpMemoryFull") != std::string::npos;

        if (messageContent.find("-") != std::string::npos)
            messageContent = messageContent.substr(0, messageContent.find("-") - 1); // Get space too

        ProgramEnvironment environment(message, messageContent, dumpMemory, dumpFull, this);

        if (environment.compile()) environment.run();
    }
}