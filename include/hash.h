// Hashing function to speedup access to cvars, aliases, commands.
unsigned char Hash(char *str);

// Hash table size
#define HT_SIZE 256
