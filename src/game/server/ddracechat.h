#ifndef CHAT_COMMAND
#define CHAT_COMMAND(Name, Params, Flags, Callback, Context, Help)
#endif

CHAT_COMMAND("credits", "", CFGFLAG_CHAT|CFGFLAG_SERVER, ConCredits, this, "Shows the credits of the DDNet mod")
CHAT_COMMAND("freeze", "?i[duration]", CFGFLAG_CHAT|CFGFLAG_SERVER, ConFreeze, this, "Freezes yourself")
CHAT_COMMAND("join", "?i[team]", CFGFLAG_CHAT|CFGFLAG_SERVER, ConTeam, this, "Join team")

#undef CHAT_COMMAND
