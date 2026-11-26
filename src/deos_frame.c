#include <deos/deos_frame.h>
#include <string.h>
#include <errno.h>

int deos_frame_encode(const deos_message_t *msg, uint8_t *buf, size_t buf_max)
{
    if (!msg || !buf) {
        return -EINVAL;
    }
    if (msg->payload_len > DEOS_MAX_PAYLOAD_LEN) {
        return -EMSGSIZE;
    }
    if ((size_t)(DEOS_UNIVERSAL_HEADER_SIZE + msg->payload_len) > buf_max) {
        return -ENOSPC;
    }

    buf[DEOS_UF_DEST_OFFSET]    = (uint8_t)msg->destination;
    buf[DEOS_UF_SRC_OFFSET]     = (uint8_t)msg->source;
    buf[DEOS_UF_PRIO_OFFSET]    = (uint8_t)msg->priority;
    buf[DEOS_UF_VER_OFFSET]     = (msg->version != 0) ? msg->version : DEOS_PROTOCOL_VERSION;
    buf[DEOS_UF_CLASS_OFFSET]   = (uint8_t)msg->message_class;
    buf[DEOS_UF_SERVICE_OFFSET] = (uint8_t)msg->service;
    buf[DEOS_UF_CMD_OFFSET]     = (uint8_t)msg->command;
    buf[DEOS_UF_SEQ_OFFSET]     = (uint8_t)msg->sequence;
    buf[DEOS_UF_LEN_OFFSET]     = (uint8_t)msg->payload_len;

    if (msg->payload_len > 0 && msg->payload_len <= DEOS_MAX_PAYLOAD_LEN) {
        memcpy(&buf[DEOS_UF_PAYLOAD_OFFSET], msg->payload, msg->payload_len);
    }

    return DEOS_UNIVERSAL_HEADER_SIZE + msg->payload_len;
}

int deos_frame_decode(const uint8_t *buf, size_t frame_len, deos_message_t *msg)
{
    if (!buf || !msg) {
        return -EINVAL;
    }
    if (frame_len < DEOS_UNIVERSAL_HEADER_SIZE) {
        return -EINVAL;
    }

    if (buf[DEOS_UF_VER_OFFSET] != DEOS_PROTOCOL_VERSION) {
        return -EPROTONOSUPPORT;
    }

    uint8_t payload_len = buf[DEOS_UF_LEN_OFFSET];
    if (payload_len > DEOS_MAX_PAYLOAD_LEN) {
        return -EMSGSIZE;
    }
    if ((size_t)(DEOS_UNIVERSAL_HEADER_SIZE + payload_len) != frame_len) {
        return -EBADMSG;
    }

    memset(msg, 0, sizeof(*msg));
    msg->destination   = (deos_node_id_t)buf[DEOS_UF_DEST_OFFSET];
    msg->source        = (deos_node_id_t)buf[DEOS_UF_SRC_OFFSET];
    msg->priority      = (deos_priority_t)buf[DEOS_UF_PRIO_OFFSET];
    msg->version       = buf[DEOS_UF_VER_OFFSET];
    msg->message_class = (deos_message_class_t)buf[DEOS_UF_CLASS_OFFSET];
    msg->service       = (deos_service_id_t)buf[DEOS_UF_SERVICE_OFFSET];
    msg->command       = buf[DEOS_UF_CMD_OFFSET];
    msg->sequence      = buf[DEOS_UF_SEQ_OFFSET];
    msg->payload_len   = payload_len;

    if (payload_len > 0) {
        memcpy(msg->payload, &buf[DEOS_UF_PAYLOAD_OFFSET], payload_len);
    }

    return 0;
}

const char *deos_node_name(uint8_t node_id)
{
    switch (node_id) {
        case DEOS_NODE_INVALID:          return "INVALID";
        case DEOS_NODE_TEXTUAL:          return "TEXTUAL";
        case DEOS_NODE_GROUND_CONTROL:   return "GROUND_CONTROL";
        case DEOS_NODE_MAIN_STM32:       return "MAIN_STM32";
        case DEOS_NODE_ROS_BRIDGE:       return "ROS_BRIDGE";
        case DEOS_NODE_TRACTION:         return "TRACTION";
        case DEOS_NODE_STEERING:         return "STEERING";
        case DEOS_NODE_BRAKE:            return "BRAKE";
        case DEOS_NODE_IMU:              return "IMU";
        case DEOS_NODE_BMS_MAIN:         return "BMS_MAIN";
        case DEOS_NODE_BMS_AUX:          return "BMS_AUX";
        case DEOS_NODE_BMS_GATEWAY:      return "BMS_GATEWAY";
        case DEOS_NODE_DIAG_TOOL:        return "DIAG_TOOL";
        case DEOS_NODE_CAN_BROADCAST:    return "CAN_BROADCAST";
        case DEOS_NODE_GLOBAL_BROADCAST: return "GLOBAL_BROADCAST";
        default:                         return "UNKNOWN_NODE";
    }
}

const char *deos_transport_name(deos_transport_t transport)
{
    switch (transport) {
        case DEOS_TRANSPORT_LOCAL:          return "LOCAL";
        case DEOS_TRANSPORT_CAN_FD:         return "CAN_FD";
        case DEOS_TRANSPORT_ETH_TEXTUAL:    return "ETH_TEXTUAL";
        case DEOS_TRANSPORT_ETH_ROS_BRIDGE: return "ETH_ROS_BRIDGE";
        case DEOS_TRANSPORT_UART_LORA:      return "UART_LORA";
        case DEOS_TRANSPORT_BROADCAST_ALL:  return "BROADCAST_ALL";
        default:                            return "UNKNOWN_TRP";
    }
}

const char *deos_service_name(uint8_t service)
{
    switch (service) {
        case DEOS_SERVICE_SYSTEM:      return "SYSTEM";
        case DEOS_SERVICE_TRACTION:    return "TRACTION";
        case DEOS_SERVICE_STEERING:    return "STEERING";
        case DEOS_SERVICE_BRAKE:       return "BRAKE";
        case DEOS_SERVICE_BMS:         return "BMS";
        case DEOS_SERVICE_CALIBRATION: return "CALIBRATION";
        case DEOS_SERVICE_DIAGNOSTIC:  return "DIAGNOSTIC";
        case DEOS_SERVICE_SENSORS:     return "SENSORS";
        case DEOS_SERVICE_LIGHTING:    return "LIGHTING";
        default:                       return "UNKNOWN_SVC";
    }
}

const char *deos_class_name(uint8_t msg_class)
{
    switch (msg_class) {
        case DEOS_CLASS_SAFETY:   return "SAFETY";
        case DEOS_CLASS_COMMAND:  return "COMMAND";
        case DEOS_CLASS_STATUS:   return "STATUS";
        case DEOS_CLASS_EVENT:    return "EVENT";
        case DEOS_CLASS_REQUEST:  return "REQUEST";
        case DEOS_CLASS_RESPONSE: return "RESPONSE";
        case DEOS_CLASS_CONFIG:   return "CONFIG";
        case DEOS_CLASS_NETWORK:  return "NETWORK";
        default:                  return "UNKNOWN_CLS";
    }
}

const char *deos_command_name(uint8_t service, uint8_t cmd)
{
    switch (service) {
        case DEOS_SERVICE_SYSTEM:
            switch (cmd) {
                case DEOS_CMD_SYSTEM_GET_STATE:         return "GET_STATE";
                case DEOS_CMD_SYSTEM_SET_STATE:         return "SET_STATE";
                case DEOS_CMD_SYSTEM_GET_MODE:          return "GET_MODE";
                case DEOS_CMD_SYSTEM_SET_MODE:          return "SET_MODE";
                case DEOS_CMD_SYSTEM_RESET_NODE:        return "RESET_NODE";
                case DEOS_CMD_SYSTEM_GET_NODE_INFO:     return "GET_NODE_INFO";
                case DEOS_CMD_SYSTEM_HEARTBEAT:         return "HEARTBEAT";
                case DEOS_CMD_SYSTEM_PING:              return "PING";
                case DEOS_CMD_SYSTEM_PONG:              return "PONG";
                case DEOS_CMD_SYSTEM_SUBSCRIBE:         return "SUBSCRIBE";
                case DEOS_CMD_SYSTEM_UNSUBSCRIBE:       return "UNSUBSCRIBE";
                case DEOS_CMD_SYSTEM_GET_SUBSCRIPTIONS: return "GET_SUBSCRIPTIONS";
                default: break;
            }
            break;
        case DEOS_SERVICE_TRACTION:
            switch (cmd) {
                case DEOS_CMD_TRACTION_SET_TARGET_SPEED: return "SET_TARGET_SPEED";
                case DEOS_CMD_TRACTION_GET_STATUS:       return "GET_STATUS";
                case DEOS_CMD_TRACTION_SET_GEAR:         return "SET_GEAR";
                case DEOS_CMD_TRACTION_SET_TORQUE_LIMIT: return "SET_TORQUE_LIMIT";
                default: break;
            }
            break;
        case DEOS_SERVICE_STEERING:
            switch (cmd) {
                case DEOS_CMD_STEERING_SET_TARGET_ANGLE: return "SET_TARGET_ANGLE";
                case DEOS_CMD_STEERING_GET_STATUS:       return "GET_STATUS";
                case DEOS_CMD_STEERING_SET_MODE:         return "SET_MODE";
                default: break;
            }
            break;
        case DEOS_SERVICE_BRAKE:
            switch (cmd) {
                case DEOS_CMD_BRAKE_SET_TARGET:  return "SET_TARGET";
                case DEOS_CMD_BRAKE_GET_STATUS:  return "GET_STATUS";
                case DEOS_CMD_BRAKE_SET_MODE:    return "SET_MODE";
                case DEOS_CMD_BRAKE_SET_PARK:    return "SET_PARK";
                case DEOS_CMD_BRAKE_SET_PREFILL: return "SET_PREFILL";
                default: break;
            }
            break;
        case DEOS_SERVICE_BMS:
            switch (cmd) {
                case DEOS_CMD_BMS_GET_STATUS:        return "GET_STATUS";
                case DEOS_CMD_BMS_STATUS:            return "STATUS";
                case DEOS_CMD_BMS_GET_CELL_VOLTAGES: return "GET_CELL_VOLTAGES";
                case DEOS_CMD_BMS_CELL_VOLTAGE:      return "CELL_VOLTAGE";
                case DEOS_CMD_BMS_GET_TEMPERATURES:  return "GET_TEMPERATURES";
                case DEOS_CMD_BMS_TEMPERATURE:       return "TEMPERATURE";
                case DEOS_CMD_BMS_GET_BALANCING:     return "GET_BALANCING";
                case DEOS_CMD_BMS_BALANCING:         return "BALANCING";
                default: break;
            }
            break;
        case DEOS_SERVICE_LIGHTING:
            switch (cmd) {
                case DEOS_CMD_LIGHTING_TURN_ON:    return "TURN_ON";
                case DEOS_CMD_LIGHTING_TURN_OFF:   return "TURN_OFF";
                case DEOS_CMD_LIGHTING_GET_STATUS: return "GET_STATUS";
                case DEOS_CMD_LIGHTING_STATUS:     return "STATUS";
                default: break;
            }
            break;
        case DEOS_SERVICE_DIAGNOSTIC:
            switch (cmd) {
                case DEOS_CMD_DIAG_GET_HEALTH:   return "GET_HEALTH";
                case DEOS_CMD_DIAG_GET_FAULTS:   return "GET_FAULTS";
                case DEOS_CMD_DIAG_CLEAR_FAULTS: return "CLEAR_FAULTS";
                default: break;
            }
            break;
        case DEOS_SERVICE_CALIBRATION:
            switch (cmd) {
                case DEOS_CMD_CAL_START:        return "CAL_START";
                case DEOS_CMD_CAL_ABORT:        return "CAL_ABORT";
                case DEOS_CMD_CAL_GET_STATUS:   return "CAL_GET_STATUS";
                case DEOS_CMD_CAL_SAVE_RESULT:  return "CAL_SAVE_RESULT";
                case DEOS_CMD_CAL_CLEAR_RESULT: return "CAL_CLEAR_RESULT";
                default: break;
            }
            break;
        case DEOS_SERVICE_SENSORS:
            switch (cmd) {
                case DEOS_CMD_SENSOR_IMU_DATA: return "IMU_DATA";
                default: break;
            }
            break;
        default:
            break;
    }
    return "UNKNOWN_CMD";
}

const char *deos_fault_name(uint16_t fault_id)
{
    switch (fault_id) {
        case DEOS_FAULT_ID_END_OF_LIST:             return "END_OF_LIST";
        /* System */
        case DEOS_FAULT_INTERNAL_SOFTWARE_ERROR:    return "INTERNAL_SOFTWARE_ERROR";
        case DEOS_FAULT_WATCHDOG_RESET:             return "WATCHDOG_RESET";
        case DEOS_FAULT_INVALID_CONFIGURATION:      return "INVALID_CONFIGURATION";
        case DEOS_FAULT_COMMUNICATION_TIMEOUT:      return "COMMUNICATION_TIMEOUT";
        case DEOS_FAULT_LOCAL_STORAGE_ERROR:        return "LOCAL_STORAGE_ERROR";
        case DEOS_FAULT_ESTOP_TRIGGERED:            return "ESTOP_TRIGGERED";
        case DEOS_FAULT_MEMORY_ALLOCATION_FAILED:   return "MEMORY_ALLOCATION_FAILED";
        /* Subscriptions */
        case DEOS_FAULT_SUB_TABLE_FULL:             return "SUB_TABLE_FULL";
        case DEOS_FAULT_SUB_BROADCAST_REJECTED:     return "SUB_BROADCAST_REJECTED";
        case DEOS_FAULT_SUB_MALFORMED_PAYLOAD:      return "SUB_MALFORMED_PAYLOAD";
        case DEOS_FAULT_SUB_HANDLER_NOT_FOUND:      return "SUB_HANDLER_NOT_FOUND";
        case DEOS_FAULT_SUB_DISPATCH_FAILED:        return "SUB_DISPATCH_FAILED";
        /* Router */
        case DEOS_FAULT_ROUTER_NO_POLICY:           return "ROUTER_NO_POLICY";
        case DEOS_FAULT_ROUTER_UNMAPPED_ROUTE:      return "ROUTER_UNMAPPED_ROUTE";
        case DEOS_FAULT_ROUTER_TRP_UNAVAILABLE:     return "ROUTER_TRP_UNAVAILABLE";
        case DEOS_FAULT_ROUTER_LOOPBACK_DETECTED:   return "ROUTER_LOOPBACK_DETECTED";
        case DEOS_FAULT_ROUTER_QUEUE_FULL:          return "ROUTER_QUEUE_FULL";
        /* Protocol */
        case DEOS_FAULT_PROTOCOL_VERSION_MISMATCH:  return "PROTOCOL_VERSION_MISMATCH";
        case DEOS_FAULT_FRAME_HEADER_TOO_SHORT:     return "FRAME_HEADER_TOO_SHORT";
        case DEOS_FAULT_PAYLOAD_TOO_LARGE:          return "PAYLOAD_TOO_LARGE";
        case DEOS_FAULT_FRAME_CRC_MISMATCH:         return "FRAME_CRC_MISMATCH";
        case DEOS_FAULT_FRAME_DECODE_FAILED:        return "FRAME_DECODE_FAILED";
        /* CAN */
        case DEOS_FAULT_CAN_DEV_NOT_READY:          return "CAN_DEV_NOT_READY";
        case DEOS_FAULT_CAN_MODE_NOT_SUPPORTED:     return "CAN_MODE_NOT_SUPPORTED";
        case DEOS_FAULT_CAN_FILTER_INSTALL_FAILED:  return "CAN_FILTER_INSTALL_FAILED";
        case DEOS_FAULT_CAN_RX_QUEUE_FULL:          return "CAN_RX_QUEUE_FULL";
        case DEOS_FAULT_CAN_TX_QUEUE_FULL:          return "CAN_TX_QUEUE_FULL";
        case DEOS_FAULT_CAN_TX_SEND_FAILED:         return "CAN_TX_SEND_FAILED";
        case DEOS_FAULT_CAN_FRAME_DECODE_ERR:       return "CAN_FRAME_DECODE_ERR";
        case DEOS_FAULT_CAN_BUS_OFF:                return "CAN_BUS_OFF";
        /* LoRa */
        case DEOS_FAULT_LORA_DEV_NOT_READY:         return "LORA_DEV_NOT_READY";
        case DEOS_FAULT_LORA_TX_QUEUE_FULL:         return "LORA_TX_QUEUE_FULL";
        case DEOS_FAULT_LORA_RX_BUFFER_OVERFLOW:    return "LORA_RX_BUFFER_OVERFLOW";
        case DEOS_FAULT_LORA_CRC_MISMATCH:          return "LORA_CRC_MISMATCH";
        case DEOS_FAULT_LORA_FRAME_MALFORMED:       return "LORA_FRAME_MALFORMED";
        case DEOS_FAULT_LORA_ACK_TIMEOUT:           return "LORA_ACK_TIMEOUT";
        /* Ethernet */
        case DEOS_FAULT_ETH_LINK_DOWN:              return "ETH_LINK_DOWN";
        case DEOS_FAULT_ETH_SOCKET_CREATE_FAILED:   return "ETH_SOCKET_CREATE_FAILED";
        case DEOS_FAULT_ETH_SOCKET_BIND_FAILED:     return "ETH_SOCKET_BIND_FAILED";
        case DEOS_FAULT_ETH_TX_QUEUE_FULL:          return "ETH_TX_QUEUE_FULL";
        case DEOS_FAULT_ETH_TX_SEND_FAILED:         return "ETH_TX_SEND_FAILED";
        case DEOS_FAULT_ETH_CLIENT_DISCONNECTED:    return "ETH_CLIENT_DISCONNECTED";
        case DEOS_FAULT_ETH_FRAME_DECODE_ERR:       return "ETH_FRAME_DECODE_ERR";
        /* Dispatcher */
        case DEOS_FAULT_DISPATCH_TABLE_FULL:        return "DISPATCH_TABLE_FULL";
        case DEOS_FAULT_DISPATCH_DUPLICATE_HANDLER: return "DISPATCH_DUPLICATE_HANDLER";
        case DEOS_FAULT_DISPATCH_HANDLER_NOT_FOUND: return "DISPATCH_HANDLER_NOT_FOUND";
        /* BMS */
        case DEOS_FAULT_BMS_PACK_OVERVOLTAGE:       return "BMS_PACK_OVERVOLTAGE";
        case DEOS_FAULT_BMS_PACK_UNDERVOLTAGE:      return "BMS_PACK_UNDERVOLTAGE";
        case DEOS_FAULT_BMS_CHARGE_OVERCURRENT:     return "BMS_CHARGE_OVERCURRENT";
        case DEOS_FAULT_BMS_DISCHARGE_OVERCURRENT:  return "BMS_DISCHARGE_OVERCURRENT";
        case DEOS_FAULT_BMS_OVERTEMPERATURE:        return "BMS_OVERTEMPERATURE";
        case DEOS_FAULT_BMS_UNDERTEMPERATURE:       return "BMS_UNDERTEMPERATURE";
        case DEOS_FAULT_BMS_CELL_IMBALANCE:         return "BMS_CELL_IMBALANCE";
        case DEOS_FAULT_BMS_INTERNAL_ERROR:         return "BMS_INTERNAL_ERROR";
        case DEOS_FAULT_BMS_INVALID_MEASUREMENT:    return "BMS_INVALID_MEASUREMENT";
        /* BMS Gateway */
        case DEOS_FAULT_GW_MAIN_COMM_LOST:          return "GW_MAIN_COMM_LOST";
        case DEOS_FAULT_GW_AUX_COMM_LOST:           return "GW_AUX_COMM_LOST";
        case DEOS_FAULT_GW_CAN_BUS_OFF:             return "GW_CAN_BUS_OFF";
        case DEOS_FAULT_GW_FD_BUS_OFF:              return "GW_FD_BUS_OFF";
        case DEOS_FAULT_GW_INVALID_FRAME:           return "GW_INVALID_FRAME";
        case DEOS_FAULT_GW_PROTOCOL_ERROR:          return "GW_PROTOCOL_ERROR";
        default:                                    return "UNKNOWN_FAULT";
    }
}

const char *deos_fault_severity_name(deos_fault_severity_t severity)
{
    switch (severity) {
        case DEOS_FAULT_SEVERITY_INFO:     return "INFO";
        case DEOS_FAULT_SEVERITY_WARNING:  return "WARNING";
        case DEOS_FAULT_SEVERITY_ERROR:    return "ERROR";
        case DEOS_FAULT_SEVERITY_CRITICAL: return "CRITICAL";
        default:                           return "UNKNOWN_SEV";
    }
}

