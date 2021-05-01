#include "tarsbotcpp.h"

#if 0
class MyClientClass : public SleepyDiscord::DiscordClient {
public:
    using SleepyDiscord::DiscordClient::DiscordClient;

    void onMessage(SleepyDiscord::Message message) override {
        if (message.author.bot) return;

        std::string messageContent = message.content;

        if (messageContent.rfind("$", 0) == 0)
        {
            messageContent = messageContent.substr(1);

            bool dumpMemory = messageContent.rfind("--dumpMemory") != std::string::npos;
            bool dumpFull = messageContent.rfind("--dumpMemoryFull") != std::string::npos;

            if (messageContent.find("-") != std::string::npos)
                messageContent = messageContent.substr(0, messageContent.find("-") - 1); // Get space too

            ProgramEnvironment environment(message, dumpMemory, dumpFull, this);

            if (environment.compile()) environment.run();
        }
    }
};
#endif

int main() {
    std::ifstream infile("token.txt");
    std::stringstream ss;

    ss << infile.rdbuf();

    infile.close();

    BotClient client(ss.str());

    client.run();
}