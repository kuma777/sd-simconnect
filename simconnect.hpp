#pragma once

#include <Windows.h>

#include <SimConnect.h>

#include "logger.hpp"

void OnReceive(SIMCONNECT_RECV* data, DWORD length, void* context);

class SimConnect
{
    enum {
        EVENT_AP_MASTER,
        EVENT_AP_SPD_VAR_SET,
    };

    enum {
        DEF_AP_HDG_LCK,
    };

    enum {
        REQ_AP_HDG_LCK,
    };

private:
    HANDLE handle_{ nullptr };

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

        SimConnect_AddToDataDefinition(handle_, DEF_AP_HDG_LCK, "AUTOPILOT HEADING LOCK DIR", "degrees");

        SimConnect_RequestDataOnSimObject(handle_, REQ_AP_HDG_LCK, DEF_AP_HDG_LCK, SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SECOND);

        SimConnect_MapClientEventToSimEvent(handle_, EVENT_AP_MASTER, "AP_MASTER");
        SimConnect_MapClientEventToSimEvent(handle_, EVENT_AP_SPD_VAR_SET, "AP_SPD_VAR_SET");

        LOG_DEBUG("SimConnect was successfully started.");

        return true;
    }

    void poll()
    {
        SimConnect_CallDispatch(handle_, OnReceive, this);
    }

    void onReceive(SIMCONNECT_RECV* data, DWORD length)
    {
        LOG_DEBUG("RECV: %d", data->dwID);

        if (data->dwID == SIMCONNECT_RECV_ID_SIMOBJECT_DATA)
        {
            auto p = static_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*>(data);
            LOG_DEBUG("RECV SIMOBJECT DATA: DEF=%d");
            for (int i = 0; i < p->dwDefineCount; ++i)
            {
                LOG_DEBUG(" [%d] %.1f", i, *(double*)(&p->dwData));
            }
        }
    }
};

void OnReceive(SIMCONNECT_RECV* data, DWORD length, void* context)
{
    reinterpret_cast<SimConnect*>(context)->onReceive(data, length);
}