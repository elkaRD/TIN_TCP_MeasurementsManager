#include "SensorConnectionHandler.h"

using namespace std;

namespace sc
{
    const int Client::MAX_MSG = 512;

    Client::Client()
    {
        reset();
    }

    void Client::reset()
    {
        socket = -1;
        remainingIn = 0;
        remainingInLen = 0;
        listener = nullptr;

        inBuffer.clear();
        outBuffer.clear();
    }

    void Client::setListener(IRequestListener *listener)
    {
        this->listener = listener;
    }

    bool Client::isConnected()
    {
        return socket > -1;
    }

    void Client::connected(int socket, int clientId)
    {
        this->socket = socket;
        this->clientId = clientId;
    }

    void Client::disconnected()
    {
        reset();
    }

    bool Client::isSomethingToSend()
    {
        if (socket < 0)
            return false;

        bool result;
        sendLock.lock();
        result = !outBuffer.empty();
        sendLock.unlock();

        return result;
    }

    bool Client::isSomethingToRecv()
    {
        bool result = socket >= 0;
        return result;
    }

    void Client::addOutMsg(std::vector<unsigned char> msg)
    {
        int msgLen = msg.size();
        BytesParser::appendFrontBytes<int32_t>(msg, (int32_t) msgLen);

        sendLock.lock();
        outBuffer.insert(outBuffer.end(), make_move_iterator(msg.begin()), make_move_iterator(msg.end()));
        sendLock.unlock();

        sendData();
    }

    void Client::gotMsg(std::vector<unsigned char> &msg)
    {
        if (listener == nullptr)
            return;

        addOutMsg(listener->onGotRequest(clientId, msg));
    }

    int Client::sendData()
    {
        sendLock.lock();
        int sent = send(socket, reinterpret_cast<const char *>(outBuffer.data()), outBuffer.size(), 0);
        outBuffer.erase(outBuffer.begin(), outBuffer.begin() + sent);
        sendLock.unlock();

        return sent;
    }

    int Client::recvData()
    {
        if (remainingIn == 0 && remainingInLen == 0)
        {
            inBuffer.clear();
            remainingInLen = sizeof(int32_t);
        }

        int toReceive = max(remainingIn, remainingInLen);

        char *data = new char[toReceive];
        int received = recv(socket, data, toReceive, 0);

        if (received <= 0)
            return received;

        BytesParser::appendBytes(inBuffer, (unsigned char *)data, received);

        if (remainingInLen > 0)
        {
            remainingInLen -= received;

            if (remainingInLen == 0)
            {
                remainingIn = BytesParser::parse<int32_t>(inBuffer);
                inBuffer.clear();
                if (remainingIn > MAX_MSG)
                    throw ConnectionException(ConnectionException::DATA_LEN, "Message too long");
                if (remainingIn <= 0)
                    throw ConnectionException(ConnectionException::DATA_LEN, "Message too short");
            }
        }
        else
        {
            remainingIn -= received;
            if (remainingIn == 0)
            {
                gotMsg(inBuffer);
            }
        }

        return received;
    }

    int Client::getSocket()
    {
        return socket;
    }

    SensorConnectionHandler::SensorConnectionHandler() : CLIENTS(100), DELAY_SELECT_SEC(5), DELAY_SELECT_MICROS(0)
    {
        for (int i = 0; i < CLIENTS; ++i)
        {
            clientHandlers.push_back(shared_ptr<Client>(new Client()));
        }
    }

    void SensorConnectionHandler::addListener(IRequestListener *requestListener)
    {
        listener = requestListener;
    }

    void SensorConnectionHandler::handleSensor(int socketDescriptor)
    {

    }

    void SensorConnectionHandler::acceptSensors(std::string ipAddress, int port)
    {
        acceptingSocket = getAcceptingSocket(ipAddress, port);
        nfds = acceptingSocket + 1;

        do
        {
            freeHandler = getFreeHandler();

            if (setReadyHandlers(acceptingSocket) == 0)
                cout << "Timeout, restarting select..." << endl;

            tryAccept();
            tryRecv();
            trySend();
        }
        while(true);
    }

    int SensorConnectionHandler::getFreeHandler()
    {
        for (int i = 0; i < clientHandlers.size(); ++i)
            if ( clientHandlers[i].get()->getSocket() < 0 )
                return i;

        return -1;
    }

    int SensorConnectionHandler::setReadyHandlers(int acceptingSocket)
    {
        FD_ZERO(&readyOut);
        FD_ZERO(&readyIn);

        if (freeHandler >= 0)
            FD_SET(acceptingSocket, &readyIn);

        for (const auto handler : clientHandlers)
        {
            bool condition = handler.get()->isSomethingToSend();
            if (condition)
                FD_SET(handler.get()->getSocket(), &readyOut);
        }

        for (const auto handler : clientHandlers)
        {
            bool condition = handler.get()->isSomethingToRecv();
            if (condition)
                FD_SET(handler.get()->getSocket(), &readyIn);
        }

        int nactive;
        struct timeval to;
        to.tv_sec = DELAY_SELECT_SEC;
        to.tv_usec = DELAY_SELECT_MICROS;
        if ( (nactive = select(nfds, &readyIn, &readyOut, (fd_set *)0, &to) ) == -1)
        {
            throw ConnectionException(ConnectionException::SELECT);
        }

        return nactive;
    }

    void SensorConnectionHandler::tryAccept()
    {
        if ( FD_ISSET(acceptingSocket, &readyIn))
        {
            int clientSocket = accept(acceptingSocket, NULL, NULL);
            if (clientSocket == -1)
                throw ConnectionException(ConnectionException::ACCEPT);
            nfds = max(clientSocket + 1, nfds);
            clientHandlers[freeHandler].get()->connected(clientSocket, freeHandler);
            clientHandlers[freeHandler].get()->setListener(listener);
            cout << "Accepted client" << endl;
        }
    }

    void SensorConnectionHandler::tryRecv()
    {
        for (auto handler : clientHandlers) {
            if (!handler.get()->isConnected())
                continue;

            if (FD_ISSET(handler.get()->getSocket(), &readyIn))
            {
                int received = handler.get()->recvData();

                if (received <= 0)
                    disconnectHandler(handler.get());
            }
        }
    }

    void SensorConnectionHandler::trySend()
    {
        for (auto handler : clientHandlers)
        {
            if (!handler.get()->isConnected())
                continue;

            if (FD_ISSET(handler.get()->getSocket(), &readyOut))
            {
                int sent = handler.get()->sendData();

                if (sent <= 0)
                    disconnectHandler(handler.get());
            }
        }
    }

    void SensorConnectionHandler::disconnectHandler(Client *client)
    {
        closeSocket(client->getSocket());
        client->disconnected();
        cout << "Client disconnected" << endl;
    }
}
