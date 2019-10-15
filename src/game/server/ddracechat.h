#ifndef CHAT_COMMAND
#define CHAT_COMMAND(Name, Params, Flags, Callback, Context, Help)
#endif

CHAT_COMMAND("credits", "", CFGFLAG_CHAT|CFGFLAG_SERVER, ConCredits, this, "Shows the credits of the DDNet mod")

#undef CHAT_COMMAND
