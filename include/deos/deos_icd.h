#ifndef DEOS_ICD_H
#define DEOS_ICD_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * DEOS Protocol Version
 */
#define DEOS_PROTOCOL_VERSION 0x12

/*
 * DEOS Timing & Limits Configuration
 */
/* CAN-FD Physical Layer */
#define DEOS_CANFD_NOMINAL_BITRATE    500000  /* 500 kbps for Arbitration */
#define DEOS_CANFD_DATA_BITRATE       2000000 /* 2 Mbps for Data Phase */

/* Network Management */
#define DEOS_HEARTBEAT_PERIOD_MS      1000    /* Nominal heartbeat interval when subscribed */
#define DEOS_PING_TIMEOUT_MS          100     /* Max wait time for PONG */

/* Common Header Layout */
#define DEOS_COMMON_HEADER_SIZE 4
#define DEOS_VERSION_OFFSET     0
#define DEOS_SEQUENCE_OFFSET    1
#define DEOS_COMMAND_OFFSET     2
#define DEOS_LENGTH_OFFSET      3
#define DEOS_PAYLOAD_OFFSET     4

/*
 * 29-bit CAN Extended Identifier
 * Bits 28..26 : Priority       (3 bit)
 * Bits 25..22 : Message Class  (4 bit)
 * Bits 21..16 : Service ID     (6 bit)
 * Bits 15..8  : Destination    (8 bit)
 * Bits 7..0   : Source         (8 bit)
 */
#define DEOS_CAN_PRIORITY_SHIFT      26
#define DEOS_CAN_MESSAGE_CLASS_SHIFT 22
#define DEOS_CAN_SERVICE_SHIFT       16
#define DEOS_CAN_DESTINATION_SHIFT   8
#define DEOS_CAN_SOURCE_SHIFT        0

#define DEOS_CAN_PRIORITY_MASK       0x07
#define DEOS_CAN_MESSAGE_CLASS_MASK  0x0F
#define DEOS_CAN_SERVICE_MASK        0x3F
#define DEOS_CAN_DESTINATION_MASK    0xFF
#define DEOS_CAN_SOURCE_MASK         0xFF

/*
 * Priority Registry
 */
typedef enum {
    DEOS_PRIO_EMERGENCY        = 0,
    DEOS_PRIO_CRITICAL_CONTROL = 1,
    DEOS_PRIO_CONTROL          = 2,
    DEOS_PRIO_STATUS           = 3,
    DEOS_PRIO_CONFIG           = 4,
    DEOS_PRIO_NETWORK          = 5,
    DEOS_PRIO_LOW              = 6,
    DEOS_PRIO_BACKGROUND       = 7
} deos_priority_t;

/*
 * Node Registry
 */
typedef enum {
    DEOS_NODE_INVALID        = 0x00,
    DEOS_NODE_TEXTUAL        = 0x01,
    DEOS_NODE_GROUND_CONTROL = 0x02,
    DEOS_NODE_MAIN_STM32     = 0x03,
    DEOS_NODE_ROS_BRIDGE     = 0x04,

    DEOS_NODE_TRACTION       = 0x10,
    DEOS_NODE_STEERING       = 0x11,
    DEOS_NODE_BRAKE          = 0x12,
    DEOS_NODE_IMU            = 0x13,

    DEOS_NODE_BMS_MAIN       = 0x20,
    DEOS_NODE_BMS_AUX        = 0x21,

    DEOS_NODE_BMS_GATEWAY    = 0x30,

    DEOS_NODE_DIAG_TOOL      = 0xF0,

    DEOS_NODE_CAN_BROADCAST      = 0xFE,
    DEOS_NODE_GLOBAL_BROADCAST   = 0xFF
} deos_node_id_t;

/*
 * Message Class Registry
 * 
 * REQUEST:   bilgi / durum / işlem sonucu talep eder
 * RESPONSE:  REQUEST cevabını veya cevap gerektiren transactional operation sonucunu taşır
 * COMMAND:   çalışma davranışını / gerçek zamanlı kontrolü değiştirir
 * CONFIG:    çalışma parametrelerini değiştirir
 * STATUS:    unsolicited durum / telemetry bilgisidir
 * EVENT:     asynchronous olay / fault bildirimidir
 */
typedef enum {
    DEOS_CLASS_SAFETY   = 0x0,
    DEOS_CLASS_COMMAND  = 0x1,
    DEOS_CLASS_STATUS   = 0x2,
    DEOS_CLASS_EVENT    = 0x3,
    DEOS_CLASS_REQUEST  = 0x4,
    DEOS_CLASS_RESPONSE = 0x5,
    DEOS_CLASS_CONFIG   = 0x6,
    DEOS_CLASS_NETWORK  = 0x7
} deos_message_class_t;

/*
 * Service Registry
 */
typedef enum {
    DEOS_SERVICE_SYSTEM       = 0x00,
    DEOS_SERVICE_TRACTION     = 0x01,
    DEOS_SERVICE_STEERING     = 0x02,
    DEOS_SERVICE_BRAKE        = 0x03,
    DEOS_SERVICE_BMS          = 0x04,
    DEOS_SERVICE_CALIBRATION  = 0x05,
    DEOS_SERVICE_DIAGNOSTIC   = 0x06,
    DEOS_SERVICE_SENSORS      = 0x07,
    DEOS_SERVICE_LIGHTING     = 0x08
} deos_service_id_t;

/*
 * SYSTEM Commands
 */
typedef enum {
    DEOS_CMD_SYSTEM_GET_STATE     = 0x01,
    DEOS_CMD_SYSTEM_SET_STATE     = 0x02,
    DEOS_CMD_SYSTEM_GET_MODE      = 0x03,
    DEOS_CMD_SYSTEM_SET_MODE      = 0x04,
    DEOS_CMD_SYSTEM_RESET_NODE    = 0x05,
    DEOS_CMD_SYSTEM_GET_NODE_INFO = 0x06,
    DEOS_CMD_SYSTEM_HEARTBEAT          = 0x07,
    DEOS_CMD_SYSTEM_PING               = 0x08,
    DEOS_CMD_SYSTEM_PONG               = 0x09,
    DEOS_CMD_SYSTEM_SUBSCRIBE          = 0x0A,
    DEOS_CMD_SYSTEM_UNSUBSCRIBE        = 0x0B,
    DEOS_CMD_SYSTEM_GET_SUBSCRIPTIONS  = 0x0C
} deos_system_command_t;

/*
 * TRACTION Commands
 */
typedef enum {
    DEOS_CMD_TRACTION_SET_TARGET_SPEED = 0x01,
    DEOS_CMD_TRACTION_GET_STATUS       = 0x02,
    /* 0x03 reserved */
    DEOS_CMD_TRACTION_SET_GEAR         = 0x04,
    DEOS_CMD_TRACTION_SET_TORQUE_LIMIT = 0x05
} deos_traction_command_t;

/*
 * TRACTION Gears
 */
typedef enum {
    DEOS_GEAR_NEUTRAL = 0x00,
    DEOS_GEAR_DRIVE   = 0x01,
    DEOS_GEAR_REVERSE = 0x02,
    DEOS_GEAR_PARK    = 0x03
} deos_traction_gear_t;

/*
 * TRACTION Parameters
 */
typedef enum {
    DEOS_PARAM_TRACTION_PID_KP               = 0x0001,
    DEOS_PARAM_TRACTION_PID_KI               = 0x0002,
    DEOS_PARAM_TRACTION_PID_KD               = 0x0003,

    DEOS_PARAM_TRACTION_MAX_CURRENT          = 0x0010,
    DEOS_PARAM_TRACTION_MAX_SPEED            = 0x0011,
    DEOS_PARAM_TRACTION_ACCEL_LIMIT          = 0x0012,
    DEOS_PARAM_TRACTION_CMD_TIMEOUT_MS       = 0x0013,
    DEOS_PARAM_TRACTION_POLE_PAIRS           = 0x0014,
    DEOS_PARAM_TRACTION_GEAR_RATIO           = 0x0015,

    DEOS_PARAM_TRACTION_WHEEL_CIRCUM_MM      = 0x0020,
    DEOS_PARAM_TRACTION_REGEN_CUTOFF_SPD     = 0x0022
} deos_traction_parameter_t;

/*
 * STEERING Commands
 */
typedef enum {
    DEOS_CMD_STEERING_SET_TARGET_ANGLE = 0x01,
    DEOS_CMD_STEERING_GET_STATUS       = 0x02,
    /* 0x03 reserved */
    DEOS_CMD_STEERING_SET_MODE         = 0x04
} deos_steering_command_t;

/*
 * STEERING Parameters
 */
typedef enum {
    DEOS_PARAM_STEERING_PID_KP               = 0x0001,
    DEOS_PARAM_STEERING_PID_KI               = 0x0002,
    DEOS_PARAM_STEERING_PID_KD               = 0x0003,
    DEOS_PARAM_STEERING_PID_I_MAX            = 0x0004,
    DEOS_PARAM_STEERING_FEEDFORWARD_K        = 0x0005,
    DEOS_PARAM_STEERING_DEADBAND             = 0x0006,

    DEOS_PARAM_STEERING_ZERO_OFFSET          = 0x0010,
    DEOS_PARAM_STEERING_MIN_ANGLE            = 0x0011,
    DEOS_PARAM_STEERING_MAX_ANGLE            = 0x0012,
    DEOS_PARAM_STEERING_CURRENT_LIMIT        = 0x0013,
    DEOS_PARAM_STEERING_MAX_ANGULAR_VEL      = 0x0014,
    DEOS_PARAM_STEERING_MAX_ANGULAR_ACC      = 0x0015,
    DEOS_PARAM_STEERING_SOFT_STOP_MARGIN     = 0x0016,
    DEOS_PARAM_STEERING_MAX_TRACK_ERR        = 0x0017,
    DEOS_PARAM_STEERING_TRACK_TIMEOUT_MS     = 0x0018
} deos_steering_parameter_t;

/*
 * BRAKE Commands
 */
typedef enum {
    DEOS_CMD_BRAKE_SET_TARGET   = 0x01,
    DEOS_CMD_BRAKE_GET_STATUS   = 0x02,
    /* 0x03 reserved */
    DEOS_CMD_BRAKE_SET_MODE     = 0x04,
    DEOS_CMD_BRAKE_SET_PARK     = 0x05,
    DEOS_CMD_BRAKE_SET_PREFILL  = 0x06
} deos_brake_command_t;

/*
 * BRAKE Parameters
 */
typedef enum {
    DEOS_PARAM_BRAKE_PID_KP                  = 0x0001,
    DEOS_PARAM_BRAKE_PID_KI                  = 0x0002,
    DEOS_PARAM_BRAKE_PID_KD                  = 0x0003,

    DEOS_PARAM_BRAKE_MIN_POSITION            = 0x0010,
    DEOS_PARAM_BRAKE_MAX_POSITION            = 0x0011,
    DEOS_PARAM_BRAKE_CURRENT_LIMIT           = 0x0012,
    DEOS_PARAM_BRAKE_SLEW_RATE_LIMIT         = 0x0013,
    DEOS_PARAM_BRAKE_CMD_TIMEOUT_MS          = 0x0014,
    DEOS_PARAM_BRAKE_KISS_POINT_OFFSET       = 0x0015,
    DEOS_PARAM_BRAKE_MAX_FORCE_LIMIT         = 0x0016
} deos_brake_parameter_t;

/*
 * BMS Commands
 */
typedef enum {
    DEOS_CMD_BMS_GET_STATUS          = 0x01,
    DEOS_CMD_BMS_STATUS              = 0x02,
    DEOS_CMD_BMS_GET_CELL_VOLTAGES   = 0x03,
    DEOS_CMD_BMS_CELL_VOLTAGE        = 0x04,
    DEOS_CMD_BMS_GET_TEMPERATURES    = 0x05,
    DEOS_CMD_BMS_TEMPERATURE         = 0x06,
    DEOS_CMD_BMS_GET_BALANCING       = 0x07,
    DEOS_CMD_BMS_BALANCING           = 0x08
} deos_bms_command_t;

/*
 * CALIBRATION Commands
 */
typedef enum {
    DEOS_CMD_CAL_START        = 0x01,
    DEOS_CMD_CAL_ABORT        = 0x02,
    DEOS_CMD_CAL_GET_STATUS   = 0x03,
    DEOS_CMD_CAL_SAVE_RESULT  = 0x04,
    DEOS_CMD_CAL_CLEAR_RESULT = 0x05
} deos_calibration_command_t;

/*
 * DIAGNOSTIC Commands
 */
typedef enum {
    DEOS_CMD_DIAG_GET_HEALTH   = 0x01,
    DEOS_CMD_DIAG_GET_FAULTS   = 0x02,
    DEOS_CMD_DIAG_CLEAR_FAULTS = 0x03
} deos_diagnostic_command_t;

/*
 * SENSORS Commands
 */
typedef enum {
    DEOS_CMD_SENSOR_IMU_DATA   = 0x01
} deos_sensors_command_t;

/*
 * LIGHTING Commands
 */
typedef enum {
    DEOS_CMD_LIGHTING_TURN_ON    = 0x01, /* Far Ac */
    DEOS_CMD_LIGHTING_TURN_OFF   = 0x02, /* Far Kapat */
    DEOS_CMD_LIGHTING_GET_STATUS = 0x03, /* Durum Sorgula (REQUEST) */
    DEOS_CMD_LIGHTING_STATUS     = 0x04  /* Durum Telemetrisi (STATUS/RESPONSE) */
} deos_lighting_command_t;

/*
 * LIGHTING Far ID Registry
 */
typedef enum {
    DEOS_LIGHT_HEADLIGHT  = 0x01, /* On Farlar */
    DEOS_LIGHT_BRAKE      = 0x02, /* Fren Lambasi */
    DEOS_LIGHT_TURN_LEFT  = 0x03, /* Sol Sinyal */
    DEOS_LIGHT_TURN_RIGHT = 0x04, /* Sag Sinyal */
    DEOS_LIGHT_ALL        = 0xFF  /* Tum Isiklar */
} deos_light_id_t;

/*
 * LIGHTING Durum Kodlari
 */
typedef enum {
    DEOS_LIGHT_STATE_OFF      = 0x00, /* Kapali */
    DEOS_LIGHT_STATE_ON       = 0x01, /* Acik (Surekli yanan) */
    DEOS_LIGHT_STATE_BLINKING = 0x02, /* Yanip sonuyor (Flasor) */
    DEOS_LIGHT_STATE_FAULT    = 0x03  /* Ariza (Acik/kisa devre) */
} deos_light_state_t;

/*
 * Parameter Commands
 */
typedef enum {
    DEOS_CMD_GET_PARAMETER      = 0xE0,
    DEOS_CMD_SET_PARAMETER      = 0xE1,
    DEOS_CMD_SAVE_PARAMETERS    = 0xE2,
    DEOS_CMD_RESTORE_DEFAULTS   = 0xE3,
    DEOS_CMD_GET_PARAMETER_INFO = 0xE4
} deos_parameter_command_t;

/*
 * State Registry
 */
typedef enum {
    DEOS_STATE_INIT        = 0x00,
    DEOS_STATE_STANDBY     = 0x01,
    DEOS_STATE_READY       = 0x02,
    DEOS_STATE_ACTIVE      = 0x03,
    DEOS_STATE_CALIBRATING = 0x04,
    DEOS_STATE_SAFE        = 0x05,
    DEOS_STATE_FAULT       = 0x06,
    DEOS_STATE_UNKNOWN     = 0xFF
} deos_state_t;

/*
 * Mode Registry
 */
typedef enum {
    DEOS_MODE_NORMAL     = 0x00,
    DEOS_MODE_MANUAL     = 0x01,
    DEOS_MODE_AUTONOMOUS = 0x02,
    DEOS_MODE_SERVICE    = 0x03,
    DEOS_MODE_TEST       = 0x04,
    DEOS_MODE_UNKNOWN    = 0xFF
} deos_mode_t;

/*
 * Response Result Code Registry
 */
typedef enum {
    DEOS_RESULT_SUCCESS           = 0x00,
    DEOS_RESULT_UNSUPPORTED       = 0x01,
    DEOS_RESULT_INVALID_PARAMETER = 0x02,
    DEOS_RESULT_OUT_OF_RANGE      = 0x03,
    DEOS_RESULT_NOT_ALLOWED       = 0x04,
    DEOS_RESULT_BUSY              = 0x05,
    DEOS_RESULT_NOT_READY         = 0x06,
    DEOS_RESULT_INTERNAL_ERROR    = 0x07
} deos_result_t;

/*
 * Global Fault ID Registry (Single Source of Truth, Wire-level ICD)
 *
 * Domain-partitioned 16-bit global fault space (zero collisions across nodes):
 *   0x0000          : End-of-list sentinel
 *   0x0001 - 0x00FF : Core & Transport (System, Sub, Router, Protocol, CAN, LoRa, Eth, Dispatch)
 *   0x0100 - 0x01FF : Traction
 *   0x0200 - 0x02FF : Steering
 *   0x0300 - 0x03FF : Brake
 *   0x0400 - 0x04FF : BMS
 *   0x0500 - 0x05FF : BMS Gateway
 *   0x0600 - 0x06FF : Sensors (IMU, etc.)
 *   0x0700 - 0x07FF : Lighting
 *   0x0800 - 0x08FF : Calibration
 *   0xF000 - 0xFFFE : Experimental / Debug
 */
typedef enum {
    DEOS_FAULT_ID_END_OF_LIST             = 0x0000,

    /* --- Sistem & Donanim Hatalari (0x0001 - 0x000F) --- */
    DEOS_FAULT_INTERNAL_SOFTWARE_ERROR    = 0x0001,
    DEOS_FAULT_WATCHDOG_RESET             = 0x0002,
    DEOS_FAULT_INVALID_CONFIGURATION      = 0x0003,
    DEOS_FAULT_COMMUNICATION_TIMEOUT      = 0x0004,
    DEOS_FAULT_LOCAL_STORAGE_ERROR        = 0x0005,
    DEOS_FAULT_ESTOP_TRIGGERED            = 0x0006,
    DEOS_FAULT_MEMORY_ALLOCATION_FAILED   = 0x0007,

    /* --- Telemetri & Abonelik Hatalari (0x0010 - 0x001F) --- */
    DEOS_FAULT_SUB_TABLE_FULL             = 0x0010,
    DEOS_FAULT_SUB_BROADCAST_REJECTED     = 0x0011,
    DEOS_FAULT_SUB_MALFORMED_PAYLOAD      = 0x0012,
    DEOS_FAULT_SUB_HANDLER_NOT_FOUND      = 0x0013,
    DEOS_FAULT_SUB_DISPATCH_FAILED        = 0x0014,

    /* --- Yonlendirici (Router) Hatalari (0x0020 - 0x002F) --- */
    DEOS_FAULT_ROUTER_NO_POLICY           = 0x0020,
    DEOS_FAULT_ROUTER_UNMAPPED_ROUTE      = 0x0021,
    DEOS_FAULT_ROUTER_TRP_UNAVAILABLE     = 0x0022,
    DEOS_FAULT_ROUTER_LOOPBACK_DETECTED   = 0x0023,
    DEOS_FAULT_ROUTER_QUEUE_FULL          = 0x0024,

    /* --- Protokol & Cerceve Hatalari (0x0030 - 0x003F) --- */
    DEOS_FAULT_PROTOCOL_VERSION_MISMATCH  = 0x0030,
    DEOS_FAULT_FRAME_HEADER_TOO_SHORT     = 0x0031,
    DEOS_FAULT_PAYLOAD_TOO_LARGE          = 0x0032,
    DEOS_FAULT_FRAME_CRC_MISMATCH         = 0x0033,
    DEOS_FAULT_FRAME_DECODE_FAILED        = 0x0034,

    /* --- CAN / CAN-FD Tasiyici Hatalari (0x0040 - 0x005F) --- */
    DEOS_FAULT_CAN_DEV_NOT_READY          = 0x0040,
    DEOS_FAULT_CAN_MODE_NOT_SUPPORTED     = 0x0041,
    DEOS_FAULT_CAN_FILTER_INSTALL_FAILED  = 0x0042,
    DEOS_FAULT_CAN_RX_QUEUE_FULL          = 0x0043,
    DEOS_FAULT_CAN_TX_QUEUE_FULL          = 0x0044,
    DEOS_FAULT_CAN_TX_SEND_FAILED         = 0x0045,
    DEOS_FAULT_CAN_FRAME_DECODE_ERR       = 0x0046,
    DEOS_FAULT_CAN_BUS_OFF                = 0x0047,

    /* --- UART / LoRa Tasiyici Hatalari (0x0060 - 0x007F) --- */
    DEOS_FAULT_LORA_DEV_NOT_READY         = 0x0060,
    DEOS_FAULT_LORA_TX_QUEUE_FULL         = 0x0061,
    DEOS_FAULT_LORA_RX_BUFFER_OVERFLOW    = 0x0062,
    DEOS_FAULT_LORA_CRC_MISMATCH          = 0x0063,
    DEOS_FAULT_LORA_FRAME_MALFORMED       = 0x0064,
    DEOS_FAULT_LORA_ACK_TIMEOUT           = 0x0065,

    /* --- Ethernet / Network Tasiyici Hatalari (0x0080 - 0x009F) --- */
    DEOS_FAULT_ETH_LINK_DOWN              = 0x0080,
    DEOS_FAULT_ETH_SOCKET_CREATE_FAILED   = 0x0081,
    DEOS_FAULT_ETH_SOCKET_BIND_FAILED     = 0x0082,
    DEOS_FAULT_ETH_TX_QUEUE_FULL          = 0x0083,
    DEOS_FAULT_ETH_TX_SEND_FAILED         = 0x0084,
    DEOS_FAULT_ETH_CLIENT_DISCONNECTED    = 0x0085,
    DEOS_FAULT_ETH_FRAME_DECODE_ERR       = 0x0086,

    /* --- Dagitici (Dispatcher) Hatalari (0x00A0 - 0x00AF) --- */
    DEOS_FAULT_DISPATCH_TABLE_FULL        = 0x00A0,
    DEOS_FAULT_DISPATCH_DUPLICATE_HANDLER = 0x00A1,
    DEOS_FAULT_DISPATCH_HANDLER_NOT_FOUND = 0x00A2,

    /* --- BMS Hatalari (0x0400 - 0x04FF) --- */
    DEOS_FAULT_BMS_PACK_OVERVOLTAGE       = 0x0400,
    DEOS_FAULT_BMS_PACK_UNDERVOLTAGE      = 0x0401,
    DEOS_FAULT_BMS_CHARGE_OVERCURRENT     = 0x0402,
    DEOS_FAULT_BMS_DISCHARGE_OVERCURRENT  = 0x0403,
    DEOS_FAULT_BMS_OVERTEMPERATURE        = 0x0404,
    DEOS_FAULT_BMS_UNDERTEMPERATURE       = 0x0405,
    DEOS_FAULT_BMS_CELL_IMBALANCE         = 0x0406,
    DEOS_FAULT_BMS_INTERNAL_ERROR         = 0x0407,
    DEOS_FAULT_BMS_INVALID_MEASUREMENT    = 0x0408,

    /* --- BMS Gateway Hatalari (0x0500 - 0x05FF) --- */
    DEOS_FAULT_GW_MAIN_COMM_LOST          = 0x0500,
    DEOS_FAULT_GW_AUX_COMM_LOST           = 0x0501,
    DEOS_FAULT_GW_CAN_BUS_OFF             = 0x0502,
    DEOS_FAULT_GW_FD_BUS_OFF              = 0x0503,
    DEOS_FAULT_GW_INVALID_FRAME           = 0x0504,
    DEOS_FAULT_GW_PROTOCOL_ERROR          = 0x0505
} deos_fault_id_t;

/*
 * Fault Severity Registry
 */
typedef enum {
    DEOS_FAULT_SEVERITY_INFO     = 0x01,
    DEOS_FAULT_SEVERITY_WARNING  = 0x02,
    DEOS_FAULT_SEVERITY_ERROR    = 0x03,
    DEOS_FAULT_SEVERITY_CRITICAL = 0x04
} deos_fault_severity_t;

/*
 * Fault State Registry
 */
typedef enum {
    DEOS_FAULT_STATE_INACTIVE = 0x00,
    DEOS_FAULT_STATE_ACTIVE   = 0x01,
    DEOS_FAULT_STATE_LATCHED  = 0x02
} deos_fault_state_t;

#ifdef __cplusplus
}
#endif

#endif /* DEOS_ICD_H */
