#pragma once

#include "sleepy_discord/sleepy_discord.h"
#include "program_environment.h"

class BotClient : public SleepyDiscord::DiscordClient
{
public:
    using SleepyDiscord::DiscordClient::DiscordClient;

    BotClient(const char* token);

    void onMessage(SleepyDiscord::Message message) override;
};