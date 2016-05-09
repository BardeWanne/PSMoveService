#ifndef CLIENT_PSMOVE_API_H
#define CLIENT_PSMOVE_API_H

//-- includes -----
#include "ClientConfig.h"
#include "ClientLog.h"
#include "ClientControllerView.h"

#ifdef HAS_PROTOCOL_ACCESS
#include "PSMoveProtocol.pb.h"
#endif // HAS_PROTOCOL_ACCESS


//-- pre-declarations -----
class ClientControllerView;
class ClientHMDView;

//-- constants -----
#define PSMOVESERVICE_DEFAULT_ADDRESS   "localhost"
#define PSMOVESERVICE_DEFAULT_PORT      "9512"

// See ControllerManager.h in PSMoveService
#define PSMOVESERVICE_MAX_CONTROLLER_COUNT  5

//-- macros -----
#ifdef HAS_PROTOCOL_ACCESS
#define GET_PSMOVEPROTOCOL_RESPONSE(handle) \
    reinterpret_cast<const PSMoveProtocol::Response *>(handle)
#define GET_PSMOVEPROTOCOL_EVENT(handle) \
    reinterpret_cast<const PSMoveProtocol::Response *>(handle) // events are a special case of responses
#endif // HAS_PROTOCOL_ACCESS

//-- interface -----
class CLIENTPSMOVEAPI ClientPSMoveAPI
{
public:
    enum eClientAPIConstants
    {
        INVALID_REQUEST_ID= -1
    };

    enum eControllerDataStreamFlags
    {
        defaultStreamOptions = 0x00,
        includeRawSensorData = 0x01
    };

    // Service Events
    //--------------
    enum eEventType
    {
        // Client Events
        connectedToService,
        failedToConnectToService,
        disconnectedFromService,

        // Service Events
        opaqueServiceEvent, // Need to have protocol access to see what kind of event this is
        controllerListUpdated,
        trackerListUpdated,
    };

    typedef const void *t_event_data_handle;
    struct EventMessage
    {
        eEventType event_type;

        // Opaque handle that can be converted to a <const PSMoveProtocol::Response *> pointer
        // using GET_PSMOVEPROTOCOL_EVENT(handle) if you linked against the PSMoveProtocol lib.
        t_event_data_handle event_data_handle;
    };

    // Service Responses
    //------------------

    enum eClientPSMoveResultCode
    {
        _clientPSMoveResultCode_ok,
        _clientPSMoveResultCode_error,
        _clientPSMoveResultCode_canceled
    };

    enum eResponsePayloadType
    {
        _responsePayloadType_Empty,
        _responsePayloadType_ControllerCount,

        _responsePayloadType_Count
    };

    typedef int t_request_id;
    typedef const void*t_response_handle;
    typedef void *t_request_handle;

    struct ResponsePayload_ControllerList
    {
        int controller_id[PSMOVESERVICE_MAX_CONTROLLER_COUNT];
        ClientControllerView::eControllerType controller_type[PSMOVESERVICE_MAX_CONTROLLER_COUNT];
        int count;
    };

    struct ResponseMessage
    {
        // Fields common to all responses
        //----
        // The id of the request this response is from
        t_request_id request_id;

        // Whether this request succeeded, failed, or was canceled
        eClientPSMoveResultCode result_code;

        // Opaque handle that can be converted to a <const PSMoveProtocol::Response *> pointer
        // using GET_PSMOVEPROTOCOL_RESPONSE(handle) if you linked against the PSMoveProtocol lib.
        t_response_handle opaque_response_handle;

        // Payload data specific to a subset of the responses
        //----
        union
        {
            ResponsePayload_ControllerList controller_list;
        } payload;
        eResponsePayloadType payload_type;
    };

    // Message Container
    //------------------
    enum eMessagePayloadType
    {
        _messagePayloadType_Event,
        _messagePayloadType_Response,

        _messagePayloadType_Count
    };

    struct Message
    {
        union{
            EventMessage event_data;
            ResponseMessage response_data;
        };
        eMessagePayloadType payload_type;
    };

    // Client Interface
    //-----------------
    static bool startup(
        const std::string &host,
        const std::string &port,
        e_log_severity_level log_level = _log_severity_level_info);
    static bool has_started();

    /**< 
        Process incoming/outgoing networking requests via the network manager. 
        Fires off callbacks for any registered request_id that got responses.
    */
    static void update();  
    /**< Poll the next message from the service in the queue */
    static bool poll_next_message(Message *message, size_t message_size);

    static void shutdown();

    static ClientControllerView *allocate_controller_view(int ControllerID);
    static void free_controller_view(ClientControllerView *view);

    static t_request_id get_controller_list();
    static t_request_id start_controller_data_stream(ClientControllerView *view, unsigned int data_stream_flags);
    static t_request_id stop_controller_data_stream(ClientControllerView *view);
    static t_request_id set_controller_rumble(ClientControllerView *view, float rumble_amount);
    static t_request_id set_led_color(ClientControllerView *view, unsigned char r, unsigned char g, unsigned b);
    static t_request_id reset_pose(ClientControllerView *view);

    /// Used to send requests to the server by clients that have protocol access
    static t_request_id send_opaque_request(t_request_handle request_handle);

    /// Used to send register a callback to get called when an async request is completed
    typedef void(*t_response_callback)(const ClientPSMoveAPI::ResponseMessage *response, void *userdata);
    static bool register_callback(ClientPSMoveAPI::t_request_id request_id, t_response_callback callback, void *callback_userdata);
    static bool cancel_callback(ClientPSMoveAPI::t_request_id request_id);
    static bool eat_response(ClientPSMoveAPI::t_request_id request_id);

private:
    static void null_response_callback(
        const ClientPSMoveAPI::ResponseMessage *response,
        void *userdata)
    { }

    // Not allowed to instantiate
    ClientPSMoveAPI();

    // Singleton private implementation - same lifetime as the ClientPSMoveAPI
    static class ClientPSMoveAPIImpl *m_implementation_ptr;
};

#endif // CLIENT_PSMOVE_API_H
