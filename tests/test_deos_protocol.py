import ctypes
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "python"))
import deos


class TestDeosProtocolPython(unittest.TestCase):
    def test_icd_ssot_constants(self):
        self.assertEqual(deos.DEOS_PROTOCOL_VERSION, 0x12)
        self.assertEqual(deos.DEOS_UNIVERSAL_HEADER_SIZE, 9)
        self.assertEqual(deos.DEOS_MAX_PAYLOAD_LEN, 60)
        self.assertEqual(deos.CMD_TRACTION_SET_GEAR, 0x04)
        self.assertEqual(deos.CMD_TRACTION_SET_TORQUE_LIMIT, 0x05)
        self.assertEqual(deos.CMD_SYSTEM_GET_SUBSCRIPTIONS, 0x0C)
        self.assertEqual(deos.CMD_SENSOR_IMU_DATA, 0x01)
        self.assertEqual(deos.GEAR_DRIVE, 0x01)
        self.assertEqual(deos.RESULT_UNSUPPORTED, 0x01)
        self.assertEqual(deos.FAULT_CAN_BUS_OFF, 0x0047)
        self.assertEqual(deos.FAULT_BMS_PACK_OVERVOLTAGE, 0x0400)
        self.assertEqual(deos.FAULT_GW_MAIN_COMM_LOST, 0x0500)
        self.assertEqual(deos.FAULT_SEVERITY_CRITICAL, 0x04)
        self.assertEqual(deos.FAULT_STATE_LATCHED, 0x02)
        self.assertEqual(deos.fault_name(deos.FAULT_CAN_BUS_OFF), "CAN_BUS_OFF")
        self.assertEqual(deos.fault_name(deos.FAULT_BMS_PACK_OVERVOLTAGE), "BMS_PACK_OVERVOLTAGE")
        self.assertEqual(deos.fault_name(deos.FAULT_GW_MAIN_COMM_LOST), "GW_MAIN_COMM_LOST")
        self.assertEqual(deos.fault_severity_name(deos.FAULT_SEVERITY_ERROR), "ERROR")

        self.assertEqual(
            deos.DeosMessage(service=deos.SERVICE_TRACTION, command=deos.CMD_TRACTION_SET_GEAR).command_name,
            "SET_GEAR",
        )
        self.assertEqual(
            deos.DeosMessage(service=deos.SERVICE_TRACTION, command=deos.CMD_TRACTION_SET_TORQUE_LIMIT).command_name,
            "SET_TORQUE_LIMIT",
        )
        self.assertEqual(
            deos.DeosMessage(service=deos.SERVICE_SENSORS, command=deos.CMD_SENSOR_IMU_DATA).command_name,
            "IMU_DATA",
        )

    def test_registry_abi_and_dynamic_sizing(self):
        argtypes = deos._lib.deos_registry_register_for_node.argtypes
        self.assertIs(argtypes[1], ctypes.c_int)
        self.assertIs(argtypes[2], ctypes.c_int)
        self.assertIs(argtypes[3], ctypes.c_int)

        # local_node is mandatory in DeosRegistry constructor and cannot be NODE_INVALID
        with self.assertRaises(TypeError):
            deos.DeosRegistry()
        with self.assertRaises(ValueError):
            deos.DeosRegistry(deos.NODE_INVALID)

        reg_size = deos._lib.deos_registry_sizeof()
        reg = deos.DeosRegistry(deos.NODE_TRACTION)
        self.assertEqual(len(reg._reg_buf), reg_size)

        with self.assertRaises(ValueError):
            reg.register_for_node(deos.NODE_INVALID, deos.CLASS_COMMAND, deos.SERVICE_TRACTION, 0, lambda _m: None)

        # Fill capacity dynamically (works for both 32 and 64 handler builds)
        registered = 0
        for cmd_id in range(256):
            rc = reg.register(
                deos.CLASS_COMMAND, deos.SERVICE_TRACTION, cmd_id,
                lambda _m: None
            )
            if rc != 0:
                break
            registered += 1
        self.assertGreaterEqual(registered, 32)
        self.assertEqual(len(reg._c_callbacks), registered)

        # Duplicate registration must fail and not leak callback reference
        dup_rc = reg.register(
            deos.CLASS_COMMAND, deos.SERVICE_TRACTION, 0,
            lambda _m: None
        )
        self.assertNotEqual(dup_rc, 0)
        self.assertEqual(len(reg._c_callbacks), registered)

    def test_little_endian_codec_bounds(self):
        self.assertEqual(deos.get_u16_le(deos.put_u16_le(0x1234)), 0x1234)
        self.assertEqual(deos.get_i16_le(deos.put_i16_le(-1500)), -1500)
        self.assertEqual(deos.get_u32_le(deos.put_u32_le(0xAABBCCDD)), 0xAABBCCDD)
        self.assertEqual(deos.get_i32_le(deos.put_i32_le(-123456)), -123456)

        for fn, bad_inputs in [
            (deos.get_u16_le, [b"", b"\x01"]),
            (deos.get_i16_le, [b"", b"\x01"]),
            (deos.get_u32_le, [b"", b"\x01", b"\x01\x02", b"\x01\x02\x03"]),
            (deos.get_i32_le, [b"", b"\x01", b"\x01\x02", b"\x01\x02\x03"]),
        ]:
            for bad in bad_inputs:
                with self.assertRaises(ValueError):
                    fn(bad)

    def test_strict_frame_decode(self):
        msg = deos.DeosMessage(
            destination=deos.NODE_TRACTION,
            source=deos.NODE_GROUND_CONTROL,
            message_class=deos.CLASS_COMMAND,
            service=deos.SERVICE_TRACTION,
            command=deos.CMD_TRACTION_SET_GEAR,
            payload=b"\x01",
        )
        wire = msg.encode()
        decoded = deos.DeosMessage.decode(wire)
        self.assertIsNotNone(decoded)
        self.assertEqual(decoded.payload, b"\x01")
        self.assertIsNone(deos.DeosMessage.decode(wire + b"\x00"))

        bad_ver = bytearray(wire)
        bad_ver[3] = 0x10
        self.assertIsNone(deos.DeosMessage.decode(bytes(bad_ver)))

    def test_unicast_dispatch_order_independence(self):
        msg = deos.DeosMessage(
            destination=deos.NODE_TRACTION,
            source=deos.NODE_GROUND_CONTROL,
            message_class=deos.CLASS_COMMAND,
            service=deos.SERVICE_TRACTION,
            command=deos.CMD_TRACTION_SET_GEAR,
            payload=b"\x01",
        )
        reg = deos.DeosRegistry(deos.NODE_TRACTION)
        calls = []
        self.assertEqual(
            reg.register_for_node(deos.NODE_GLOBAL_BROADCAST, deos.CLASS_COMMAND, deos.SERVICE_TRACTION,
                                  deos.CMD_TRACTION_SET_GEAR, lambda _m: calls.append("wildcard")),
            0,
        )
        self.assertEqual(
            reg.register(deos.CLASS_COMMAND, deos.SERVICE_TRACTION,
                         deos.CMD_TRACTION_SET_GEAR, lambda _m: calls.append("exact")),
            0,
        )
        self.assertEqual(reg.dispatch(msg), 1)
        self.assertEqual(calls, ["exact"])


if __name__ == "__main__":
    unittest.main()

