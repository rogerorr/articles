/**
 * This is a minimal program ...
 * It contains an entry point with a return, and no dependent DLLs
 */

static char const szRCSID[] =
    "$Id: trivialProgram.cpp 299 2021-07-07 21:42:42Z roger $";

#pragma comment(linker, "/nodefaultlib")

int main() { return 0; }

int mainCRTStartup() { return 0; }
