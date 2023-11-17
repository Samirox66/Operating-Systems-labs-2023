#include <iostream>
#include <csignal>
#include <random>
#include <cstring>
#include <unistd.h>
#include "../connections/conn_interfaces/message.h"
#include <fcntl.h>

#include "client.h"


static void ClientSignalHandler(int signalNum)
{
    Client& inst = Client::GetInstance(0);
    inst.Terminate();
}


Client& Client::GetInstance(pid_t pid)
{
    static Client inst(pid);
    return inst;
}


Client::Client(pid_t pid)
: hostPid(pid)
{
    std::cout << "Client::Client(). Host pid: " << pid << std::endl;
    signal(SIGTERM, ClientSignalHandler);
    signal(SIGINT, ClientSignalHandler);
}


bool Client::OpenConnection()
{
    if ((semaphore_host = sem_open(HOST_SEM_NAME, O_CREAT)) == SEM_FAILED) {
        std::cout << "[ERROR]: Failed to create host semaphore, error: " << strerror(errno) << std::endl;
        return false;
    }

    if ((semaphore_client = sem_open(CLIENT_SEM_NAME, O_CREAT)) == SEM_FAILED) {
        std::cout << "[ERROR]: Failed to create client semaphore, error: " << strerror(errno) << std::endl;
        sem_unlink(HOST_SEM_NAME);
        return false;
    }

    std::cout << "Semaphores created. Client's pid: " << getpid() << std::endl;
    if (!conn.Open(getpid(), false)) {
        std::cout << "[ERROR]: Failed to open connection." << std::endl;
        sem_unlink(HOST_SEM_NAME);
        sem_unlink(CLIENT_SEM_NAME);
        return false;
    }

    kill(hostPid, SIGUSR1);
    return true;
}


void Client::Terminate()
{
    std::cout << "Terminating client..." << std::endl;
    kill(hostPid, SIGUSR2);
    if (semaphore_host != SEM_FAILED && sem_close(semaphore_host) == -1) {
        std::cout << "[ERROR]: Failed to close host semaphore, error: " << strerror(errno) << std::endl;
        exit(errno);
    }

    if (semaphore_client != SEM_FAILED && sem_close(semaphore_client) == -1) {
        std::cout << "[ERROR]: Failed to close client semaphore, error: " << strerror(errno) << std::endl;
        exit(errno);
    }

    if (conn.Close()) {
        exit(SIGTERM);
    }

    std::cout << "[ERROR]: Failed in client terminate, error: " << strerror(errno) << std::endl;
    exit(errno);
}


int Client::GetNewRandomNumber(int rightRandEdge)
{
    std::random_device randDev;
    std::mt19937 mt(randDev());
    std::uniform_int_distribution<int> uniformDist(MIN_RAND, rightRandEdge);
    return uniformDist(mt);
}


void Client::Start()
{
    Message msg;

    sem_wait(semaphore_client);
    msg.status = MSG_STATUS ::ALIVE;
    msg.number = GetNewRandomNumber(ALIVE_MAX_RAND);
    std::cout << "------------GOAT ROUND START-----------" << std::endl;
    std::cout << "Status: ALIVE" << std::endl;
    std::cout << "Goat new number: " << msg.number << std::endl;
    std::cout << "------------GOAT ROUND END-------------" << std::endl;
    if (!conn.Write(&msg)) {
        std::cout << "[ERROR]: Failed to write message in connection interface. Terminating client..." << std::endl;
        Terminate();
    }
    sem_post(semaphore_host);

    while (true) {
        sem_wait(semaphore_client);
        if (conn.Read(&msg)) {
            if (msg.status == MSG_STATUS::ALIVE) {
                msg.number = GetNewRandomNumber(ALIVE_MAX_RAND);
            } else {
                msg.number = GetNewRandomNumber(DEAD_MAX_RAND);
            }

            std::cout << "------------GOAT ROUND START-----------" << std::endl;
            std::cout << "Status: " << ((msg.status == MSG_STATUS::ALIVE) ? "ALIVE" : "DEAD") << std::endl;
            std::cout << "Goat new number: " << msg.number << std::endl;
            std::cout << "------------GOAT ROUND END-------------" << std::endl;

            if (!conn.Write(&msg)) {
                std::cout << "[ERROR]: Failed to write message in connection interface. Terminating client..." << std::endl;
                exit(SIGTERM);
            }
        } else {
            std::cout << "[ERROR]: Failed to read message from connection interface. Terminating client..." << std::endl;
            exit(SIGTERM);
        }
        sem_post(semaphore_host);
    }
}
