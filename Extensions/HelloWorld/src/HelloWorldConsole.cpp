#include "Engine/Source/ConsoleBase/IConsole.h"
#include "WoW/Source/Extension/ExtensionMgr.h"

namespace {

// The console command the extension adds: "helloWorld [text]" (console window or
// "/console helloWorld text") echoes its arguments to the console and the log.
int32_t CCommand_HelloWorld(const char* command, const char* arguments) {
    sExtensionMgr->Log("HelloWorld: command %s \"%s\"", command, arguments ? arguments : "");
    ConsolePrintf("HelloWorld says: %s", arguments && *arguments ? arguments : "hello");

    return 1;
}

// Counts every registered command that runs and logs the first few.
class HelloWorldConsoleScript : public ConsoleScript {
    public:
    HelloWorldConsoleScript() : ConsoleScript("HelloWorldConsoleScript") {}

    void OnConsoleCommand(const char* command, const char* arguments) override {
        m_commands++;

        if (m_commands <= 5) {
            sExtensionMgr->Log("HelloWorld: OnConsoleCommand %s %s", command, arguments ? arguments : "");
        }
    }

    private:
    uint32_t m_commands = 0;
};

}

void AddHelloWorldConsoleScripts() {
    sExtensionMgr->RegisterConsoleCommand("helloWorld", CCommand_HelloWorld, CATEGORY_DEBUG, "HelloWorld: echo the arguments");
    new HelloWorldConsoleScript();
}
