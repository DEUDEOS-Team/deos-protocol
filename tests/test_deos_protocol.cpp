#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <vector>

#include <deos/deos_codec.h>
#include <deos/deos_frame.h>
#include <deos/deos_icd.h>
#include <deos/deos_registry.h>
#include <deos/deos_types.h>

static_assert(sizeof(struct deos_parameter_payload) == 2,
              "deos_parameter_payload header size must be 2 bytes");
static_assert(sizeof(struct deos_subscribe_req_payload) == 6,
              "deos_subscribe_req_payload size must be 6 bytes");
static_assert(sizeof(struct deos_subscribe_resp_payload) == 5,
              "deos_subscribe_resp_payload size must be 5 bytes");

struct DispatchLog {
    std::vector<const char*> calls;
};

static int exact_handler(const deos_message_t*, void* user_data) {
    static_cast<DispatchLog*>(user_data)->calls.push_back("exact");
    return 0;
}

static int wildcard_handler(const deos_message_t*, void* user_data) {
    static_cast<DispatchLog*>(user_data)->calls.push_back("wildcard");
    return 0;
}

static void test_codec() {
    uint8_t buf[8]{};
    deos_put_u16_le(buf, 0x1234);
    assert(buf[0] == 0x34 && buf[1] == 0x12);
    assert(deos_get_u16_le(buf) == 0x1234);

    deos_put_i16_le(buf, -1500);
    assert(deos_get_i16_le(buf) == -1500);

    deos_put_u32_le(buf, 0xAABBCCDDu);
    assert(deos_get_u32_le(buf) == 0xAABBCCDDu);

    deos_put_i32_le(buf, -123456);
    assert(deos_get_i32_le(buf) == -123456);
}

static void test_registry_sizeof() {
    assert(deos_registry_sizeof() == sizeof(deos_registry_t));
}

static void test_frame_encode_decode() {
    deos_message_t tx{};
    tx.destination = DEOS_NODE_IMU;
    tx.source = DEOS_NODE_ROS_BRIDGE;
    tx.priority = DEOS_PRIO_CONFIG;
    tx.version = DEOS_PROTOCOL_VERSION;
    tx.message_class = DEOS_CLASS_REQUEST;
    tx.service = DEOS_SERVICE_SENSORS;
    tx.command = DEOS_CMD_SENSOR_IMU_DATA;
    tx.sequence = 7;
    tx.payload_len = 0;

    uint8_t buf[32]{};
    int len = deos_frame_encode(&tx, buf, sizeof(buf));
    assert(len == DEOS_UNIVERSAL_HEADER_SIZE);

    deos_message_t rx{};
    assert(deos_frame_decode(buf, static_cast<size_t>(len), &rx) == 0);
    assert(rx.version == DEOS_PROTOCOL_VERSION);
    assert(std::strcmp(deos_command_name(DEOS_SERVICE_SENSORS, DEOS_CMD_SENSOR_IMU_DATA),
                       "IMU_DATA") == 0);
    assert(std::strcmp(deos_fault_name(DEOS_FAULT_CAN_BUS_OFF), "CAN_BUS_OFF") == 0);
    assert(std::strcmp(deos_fault_name(DEOS_FAULT_BMS_PACK_OVERVOLTAGE), "BMS_PACK_OVERVOLTAGE") == 0);
    assert(std::strcmp(deos_fault_name(DEOS_FAULT_GW_MAIN_COMM_LOST), "GW_MAIN_COMM_LOST") == 0);
    assert(std::strcmp(deos_fault_severity_name(DEOS_FAULT_SEVERITY_CRITICAL), "CRITICAL") == 0);

    // Trailing byte rejected
    buf[len] = 0xAA;
    assert(deos_frame_decode(buf, static_cast<size_t>(len + 1), &rx) == -EBADMSG);

    // Truncated payload rejected
    buf[DEOS_UF_LEN_OFFSET] = 2;
    assert(deos_frame_decode(buf, static_cast<size_t>(len + 1), &rx) == -EBADMSG);

    // Oversized payload rejected
    buf[DEOS_UF_LEN_OFFSET] = DEOS_MAX_PAYLOAD_LEN + 1;
    assert(deos_frame_decode(buf, static_cast<size_t>(len), &rx) == -EMSGSIZE);

    // Mismatched protocol version rejected
    buf[DEOS_UF_LEN_OFFSET] = 0;
    buf[DEOS_UF_VER_OFFSET] = 0x10;
    assert(deos_frame_decode(buf, static_cast<size_t>(len), &rx) == -EPROTONOSUPPORT);
}

static void test_registry_dispatch_and_find() {
    deos_message_t unicast{};
    unicast.destination = DEOS_NODE_TRACTION;
    unicast.message_class = DEOS_CLASS_COMMAND;
    unicast.service = DEOS_SERVICE_TRACTION;
    unicast.command = DEOS_CMD_TRACTION_SET_GEAR;

    deos_message_t other_unicast = unicast;
    other_unicast.destination = DEOS_NODE_STEERING;

    deos_message_t broadcast = unicast;
    broadcast.destination = DEOS_NODE_GLOBAL_BROADCAST;

    // Order A: wildcard before exact
    {
        deos_registry_t reg{};
        assert(deos_registry_init(&reg) == 0);
        DispatchLog log{};
        assert(deos_registry_register(&reg, DEOS_CLASS_COMMAND, DEOS_SERVICE_TRACTION,
                                      DEOS_CMD_TRACTION_SET_GEAR, wildcard_handler, &log) == 0);
        assert(deos_registry_register_for_node(&reg, DEOS_NODE_TRACTION, DEOS_CLASS_COMMAND,
                                               DEOS_SERVICE_TRACTION, DEOS_CMD_TRACTION_SET_GEAR,
                                               exact_handler, &log) == 0);

        void* ud = nullptr;
        assert(deos_registry_find(&reg, DEOS_NODE_TRACTION, DEOS_CLASS_COMMAND,
                                  DEOS_SERVICE_TRACTION, DEOS_CMD_TRACTION_SET_GEAR,
                                  &ud) == exact_handler);
        assert(deos_registry_find(&reg, DEOS_NODE_STEERING, DEOS_CLASS_COMMAND,
                                  DEOS_SERVICE_TRACTION, DEOS_CMD_TRACTION_SET_GEAR,
                                  &ud) == wildcard_handler);

        assert(deos_registry_dispatch(&reg, &unicast) == 1);
        assert(log.calls.size() == 1 && std::strcmp(log.calls[0], "exact") == 0);

        log.calls.clear();
        assert(deos_registry_dispatch(&reg, &other_unicast) == 1);
        assert(log.calls.size() == 1 && std::strcmp(log.calls[0], "wildcard") == 0);

        log.calls.clear();
        assert(deos_registry_dispatch(&reg, &broadcast) == 2);
        assert(log.calls.size() == 2);
    }

    // Order B: exact before wildcard
    {
        deos_registry_t reg{};
        assert(deos_registry_init(&reg) == 0);
        DispatchLog log{};
        assert(deos_registry_register_for_node(&reg, DEOS_NODE_TRACTION, DEOS_CLASS_COMMAND,
                                               DEOS_SERVICE_TRACTION, DEOS_CMD_TRACTION_SET_GEAR,
                                               exact_handler, &log) == 0);
        assert(deos_registry_register(&reg, DEOS_CLASS_COMMAND, DEOS_SERVICE_TRACTION,
                                      DEOS_CMD_TRACTION_SET_GEAR, wildcard_handler, &log) == 0);

        assert(deos_registry_dispatch(&reg, &unicast) == 1);
        assert(log.calls.size() == 1 && std::strcmp(log.calls[0], "exact") == 0);

        log.calls.clear();
        assert(deos_registry_dispatch(&reg, &other_unicast) == 1);
        assert(log.calls.size() == 1 && std::strcmp(log.calls[0], "wildcard") == 0);

        log.calls.clear();
        assert(deos_registry_dispatch(&reg, &broadcast) == 2);
        assert(log.calls.size() == 2);
    }
}

int main() {
    test_codec();
    test_registry_sizeof();
    test_frame_encode_decode();
    test_registry_dispatch_and_find();
    std::cout << "ALL deos_protocol C++17 tests passed.\n";
    return 0;
}

