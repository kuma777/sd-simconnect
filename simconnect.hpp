#pragma once

#include <map>

#include <Windows.h>

#include <SimConnect.h>

#include "logger.hpp"

void OnReceive(SIMCONNECT_RECV* data, DWORD length, void* context);

class SimConnect
{
    enum {
        REQ_AP_SPEED,
        REQ_AP_HEADING,
        REQ_AP_ALTITUDE,
    };

public:
    enum
    {
        DEF_AP_SPEED_VALUE,
        DEF_AP_HEADING_VALUE,
        DEF_AP_ALTITUDE_VALUE,
    };

private:
    HANDLE handle_{ nullptr };

    std::map<SIMCONNECT_DATA_DEFINITION_ID, double> data_;

public:
    SimConnect()
    {
    }

    ~SimConnect()
    {
        if (handle_)
        {
            SimConnect_Close(handle_);
        }
    }

    bool start()
    {
        HRESULT result = SimConnect_Open(&handle_, "sd-simconnect", NULL, 0, 0, SIMCONNECT_OPEN_CONFIGINDEX_LOCAL);
        if (result != S_OK)
        {
            LOG_ERROR("Failed to open SimCoonect.");
            return false;
        }

        SimConnect_AddToDataDefinition(handle_, DEF_AP_HEADING_VALUE, "AUTOPILOT HEADING LOCK DIR", "degrees");
        SimConnect_AddToDataDefinition(handle_, DEF_AP_ALTITUDE_VALUE, "AUTOPILOT ALTITUDE LOCK VAR", "feet");

        SimConnect_RequestDataOnSimObject(handle_, REQ_AP_HEADING, DEF_AP_HEADING_VALUE, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_VISUAL_FRAME);
        SimConnect_RequestDataOnSimObject(handle_, REQ_AP_ALTITUDE, DEF_AP_ALTITUDE_VALUE, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_VISUAL_FRAME);

        LOG_DEBUG("SimConnect was successfully started.");

        return true;
    }

    void poll()
    {
        SimConnect_CallDispatch(handle_, OnReceive, this);
    }

    template <typename T>
    T get(SIMCONNECT_DATA_DEFINITION_ID id)
    {
        auto it = data_.find(id);
        if (it == data_.end())
        {
            return 0;
        }

        return *(T*)(&it->second);
    }

    template <typename T>
    void set(SIMCONNECT_DATA_DEFINITION_ID id, T value)
    {
        SimConnect_SetDataOnSimObject(handle_, id, SIMCONNECT_OBJECT_ID_USER, 0, 1, sizeof(T), &value);
    }

    void onReceive(SIMCONNECT_RECV* data, DWORD length)
    {
        if (data->dwID == SIMCONNECT_RECV_ID_SIMOBJECT_DATA)
        {
            auto p = static_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*>(data);

            auto it = data_.find(p->dwDefineID);
            if (it == data_.end())
            {
                data_.emplace(p->dwDefineID, 0);
            }

            data_[p->dwDefineID] = *(double*)(&p->dwData);
        }
    }
};

void OnReceive(SIMCONNECT_RECV* data, DWORD length, void* context)
{
    reinterpret_cast<SimConnect*>(context)->onReceive(data, length);
}