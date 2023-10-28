#ifndef DAEMON_H
#define DAEMON_H

#include <string>

void HandleSignal(int sigNum);

class Daemon
{
public:
    void ProtectAgainstRestart();
    void DeleteAllSubfolders();
    static Daemon& getInstance();
private:
    Daemon(std::string path): ABS_PID_PATH(path) {}
    Daemon(const Daemon&) = delete;
    Daemon& operator=(const Daemon&) = delete;
    const std::string ABS_PID_PATH;
};

#endif //DAEMON_H