#pragma once

#include <Windows.h>
#include <winhttp.h>

#include <nlohmann/json.hpp>

#include "logger.hpp"

void OnRequestCallback(HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID info, DWORD infoLength);
void OnSocketCallback(HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID info, DWORD infoLength);

class StreamDeck
{
public:
    enum {
        INVALID,
        ACTION_SPEED,
        ACTION_HEADING,
        ACTION_ALTITUDE,
    };

private:
    HINTERNET session_{ nullptr };
    HINTERNET connection_{ nullptr };
    HINTERNET request_{ nullptr };
    HINTERNET socket_{ nullptr };

    int port_{ 0 };
    std::string registerEvent_;
    std::string pluginUUID_;
    
    char buffer_[2048];
    std::string payload_;

    std::map<DWORD, std::string> context_;
    std::map<DWORD, int> tick_;

public:
    StreamDeck(int port, const std::string& registerEvent, const std::string& pluginUUID)
        : port_(port)
        , registerEvent_(registerEvent)
        , pluginUUID_(pluginUUID)
    {
    }

    ~StreamDeck()
    {
    }

    void finalize()
    {
        if (session_)
        {
            WinHttpCloseHandle(session_);
        }

        if (connection_)
        {
            WinHttpCloseHandle(connection_);
        }

        if (request_)
        {
            WinHttpCloseHandle(request_);
        }

        if (socket_)
        {
            WinHttpCloseHandle(socket_);
        }
    }

    bool start()
    {
        session_ = WinHttpOpen(L"SimConnect", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
        if (!session_)
        {
            LOG_ERROR("Failed to initialize WinHTTP.");
            finalize();
            return false;
        }

        connection_ = WinHttpConnect(session_, L"localhost", port_, 0);
        if (!connection_)
        {
            LOG_ERROR("Failed to connect.");
            finalize();
            return false;
        }

        request_ = WinHttpOpenRequest(connection_, L"GET", L"/", nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        if (!request_)
        {
            LOG_ERROR("Failed to open request.");
            finalize();
            return false;
        }

        WinHttpSetStatusCallback(request_, OnRequestCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0);

        if (!WinHttpSetOption(request_, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0))
        {
            LOG_ERROR("Failed to set request option.");
            finalize();
            return false;
        }

        if (!WinHttpSendRequest(request_, WINHTTP_NO_ADDITIONAL_HEADERS, -1, WINHTTP_NO_REQUEST_DATA, 0, 0, (DWORD_PTR)this))
        {
            LOG_ERROR("Failed to send request.");
            finalize();
            return false;
        }

        return true;
    }

    int getTick(DWORD id)
    {
        auto it = tick_.find(id);
        if (it == tick_.end())
        {
            return 0;
        }

        int v = it->second;

        // Consume.
        tick_[id] = 0;;

        return v;
    }

    bool send(DWORD id, nlohmann::json& root)
    {
        auto it = context_.find(id);
        if (it == context_.end())
        {
            LOG_ERROR("No matching context was found. Abort sending message.");
            return false;
        }

        root["context"] = it->second;

        std::string buffer = root.dump();

        LOG_DEBUG("-> %s", buffer.c_str());

        DWORD result = WinHttpWebSocketSend(socket_, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, (void*)buffer.data(), buffer.size());
        if (result != NO_ERROR)
        {
            LOG_ERROR("Failed to send message. %d", result);
            return false;
        }

        return true;
    }

    void onRequestCallback(DWORD status, LPVOID info, DWORD infoLength)
    {
        LOG_DEBUG("onRequestCallback %#x", status);

        if (status == WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE)
        {
            LOG_DEBUG("HTTP request was successfully sent.");

            WinHttpReceiveResponse(request_, NULL);
        }
        else if (status == WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED)
        {
            LOG_DEBUG("HTTP response was successfully received.");
        }
        else if (status == WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE)
        {
            DWORD code;
            DWORD codeLength = sizeof(code);

            if (!WinHttpQueryHeaders(request_, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &code, &codeLength, WINHTTP_NO_HEADER_INDEX))
            {
                LOG_ERROR("Failed to get header information.");
                finalize();
                return;
            }

            LOG_DEBUG("Status code: %d", code);

            if (code != 101)
            {
                LOG_DEBUG("Return code was not 101.");
                finalize();
                return;
            }

            socket_ = WinHttpWebSocketCompleteUpgrade(request_, (DWORD_PTR)this);
            if (!socket_)
            {
                LOG_ERROR("Failed to complete upgrading to WebSocket.");
                finalize();
                return;
            }

            LOG_DEBUG("HTTP connection was successfully upgraded.");

            WinHttpCloseHandle(request_);

            request_ = nullptr;

            WinHttpSetStatusCallback(socket_, OnSocketCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0);

            DWORD result;

            result = WinHttpWebSocketReceive(socket_, (void*)buffer_, sizeof(buffer_), nullptr, nullptr);
            if (result != NO_ERROR)
            {
                LOG_ERROR("Failed to receive message. %d", result);
                finalize();
                return;
            }

            nlohmann::json root;
            root["event"] = registerEvent_;
            root["uuid"]  = pluginUUID_;

            std::string buffer = root.dump();

            LOG_DEBUG("-> %s", buffer.c_str());

            result = WinHttpWebSocketSend(socket_, WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE, (void*)buffer.data(), buffer.size());
            if (result != NO_ERROR)
            {
                LOG_ERROR("Failed to send message. %d", result);
                finalize();
                return;
            }
        }
    }

    void onSocketCallback(DWORD status, LPVOID info, DWORD infoLength)
    {
        LOG_DEBUG("onSocketCallback %#x", status);

        if (status == WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE)
        {
            auto ss = reinterpret_cast<WINHTTP_WEB_SOCKET_STATUS*>(info);

            LOG_DEBUG("%d bytes sent.", ss->dwBytesTransferred);
        }
        else if (status == WINHTTP_CALLBACK_STATUS_READ_COMPLETE)
        {
            auto ss = reinterpret_cast<WINHTTP_WEB_SOCKET_STATUS*>(info);

            LOG_DEBUG("%d bytes received. %d", ss->dwBytesTransferred, ss->eBufferType);

            if (ss->eBufferType == WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE)
            {
                payload_ += std::string(buffer_, ss->dwBytesTransferred);

                LOG_DEBUG("Fragment received. %d", payload_.size());
            }
            else if (ss->eBufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE)
            {
                payload_ += std::string(buffer_, ss->dwBytesTransferred);

                LOG_DEBUG("<- %s", payload_.c_str());

                nlohmann::json root = nlohmann::json::parse(payload_);

                if (root.find("event") != root.end())
                {
                    auto event = root["event"].get<std::string>();
                    if (event == "willAppear")
                    {
                        auto action = root["action"].get<std::string>();
                        if (action == "net.kuma777.simconnect.sdplugin.speed")
                        {
                            context_.emplace(ACTION_SPEED, root["context"].get<std::string>());
                        }
                        else if (action == "net.kuma777.simconnect.sdplugin.heading")
                        {
                            context_.emplace(ACTION_HEADING, root["context"].get<std::string>());
                        }
                        else if (action == "net.kuma777.simconnect.sdplugin.altitude")
                        {
                            context_.emplace(ACTION_ALTITUDE, root["context"].get<std::string>());
                        }
                    }
                    else if (event == "dialRotate")
                    {
                        DWORD id = INVALID;

                        auto action = root["action"].get<std::string>();
                        if (action == "net.kuma777.simconnect.sdplugin.speed")
                        {
                            id = ACTION_SPEED;
                        }
                        else if (action == "net.kuma777.simconnect.sdplugin.heading")
                        {
                            id = ACTION_HEADING;
                        }
                        else if (action == "net.kuma777.simconnect.sdplugin.altitude")
                        {
                            id = ACTION_ALTITUDE;
                        }

                        if (id != INVALID)
                        {
                            auto it = tick_.find(id);
                            if (it == tick_.end())
                            {
                                tick_.emplace(id, 0);
                            }

                            tick_[id] += root["payload"]["ticks"].get<int>();
                        }
                    }
                }

                payload_.clear();
            }

            WinHttpWebSocketReceive(socket_, (void*)buffer_, sizeof(buffer_), nullptr, nullptr);
        }
    }
};

void OnRequestCallback(HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID info, DWORD infoLength)
{
    reinterpret_cast<StreamDeck*>(context)->onRequestCallback(status, info, infoLength);
}

void OnSocketCallback(HINTERNET handle, DWORD_PTR context, DWORD status, LPVOID info, DWORD infoLength)
{
    reinterpret_cast<StreamDeck*>(context)->onSocketCallback(status, info, infoLength);
}