"""
DEOS Protocol Python C-Bindings (ctypes wrapper)
libdeos_protocol C kütüphanesini doğrudan çağıran resmi Python bağlayıcısıdır.
"""

import os
import re
import sys
import ctypes
import subprocess
from pathlib import Path
from typing import Callable, Optional, Dict, List, Any

# ==============================================================================
# 1. Kütüphane Yükleyici (Shared Library Loader)
# ==============================================================================

def _find_or_build_library() -> ctypes.CDLL:
    env_lib = os.environ.get("DEOS_PROTOCOL_LIB")
    if env_lib:
        lib = ctypes.CDLL(env_lib)
        if not hasattr(lib, "deos_registry_sizeof"):
            raise RuntimeError(f"DEOS_PROTOCOL_LIB ({env_lib}) eksik sembol: deos_registry_sizeof")
        return lib

    current_dir = Path(__file__).resolve().parent
    repo_root = current_dir.parent.parent.parent

    search_paths = [
        current_dir / "libdeos_protocol.so",
        repo_root / "tests/desktop/build/deos_protocol_build/libdeos_protocol.so",
        repo_root / "build/lib/libdeos_protocol.so",
        Path("/tmp/libdeos_protocol.so"),
    ]

    for p in search_paths:
        if p.exists():
            try:
                lib = ctypes.CDLL(str(p))
                # Eski önbelleklenmiş .so sürümlerini ele (deos_registry_sizeof gerekli)
                if hasattr(lib, "deos_registry_sizeof"):
                    return lib
            except Exception:
                pass

    # Eğer .so bulunamazsa C kaynak kodlarından otomatik derle
    c_sources = [
        str(current_dir.parent / "src/deos_frame.c"),
        str(current_dir.parent / "src/deos_codec.c"),
        str(current_dir.parent / "src/deos_registry.c"),
    ]
    include_dir = str(current_dir.parent / "include")
    out_so = "/tmp/libdeos_protocol.so"

    cmd = ["gcc", "-shared", "-fPIC", "-O2", "-o", out_so] + c_sources + [f"-I{include_dir}"]
    try:
        subprocess.check_call(cmd)
        return ctypes.CDLL(out_so)
    except Exception as e:
        raise RuntimeError(f"libdeos_protocol.so derlenemedi veya bulunamadı: {e}")

_lib = _find_or_build_library()

# ==============================================================================
# 2. ICD Sabitleri (Single Source of Truth: include/deos/deos_*.h)
# ==============================================================================

_SHORT_ALIAS_PREFIXES = (
    "PRIO_",
    "NODE_",
    "CLASS_",
    "SERVICE_",
    "CMD_",
    "GEAR_",
    "PARAM_",
    "LIGHT_",
    "STATE_",
    "MODE_",
    "RESULT_",
    "FAULT_",
)

def _load_c_constants() -> Dict[str, int]:
    """
    C başlık dosyalarından (deos_icd.h, deos_frame.h, deos_types.h) tüm
    DEOS_* makro ve enum sabitlerini otomatik okur; manuel kopya tutulmaz.
    """
    include_dir = Path(__file__).resolve().parent.parent / "include" / "deos"
    header_files = [
        include_dir / "deos_icd.h",
        include_dir / "deos_frame.h",
        include_dir / "deos_types.h",
    ]

    comment_re = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)
    define_re  = re.compile(r"^\s*#define\s+(DEOS_[A-Z0-9_]+)\s+(0[xX][0-9a-fA-F]+|\d+)\b", re.MULTILINE)
    enum_re    = re.compile(r"\b(DEOS_[A-Z0-9_]+)\s*=\s*(0[xX][0-9a-fA-F]+|\d+)\b")

    constants: Dict[str, int] = {}
    for hdr in header_files:
        if not hdr.exists():
            raise RuntimeError(f"DEOS C başlık dosyası bulunamadı: {hdr}")
        text = comment_re.sub("", hdr.read_text(encoding="utf-8"))
        for name, val_str in define_re.findall(text):
            constants[name] = int(val_str, 0)
        for name, val_str in enum_re.findall(text):
            constants[name] = int(val_str, 0)

    # Kısa takma adları (ör. DEOS_CMD_TRACTION_SET_GEAR -> CMD_TRACTION_SET_GEAR) otomatik üret
    aliases: Dict[str, int] = {}
    for name, val in constants.items():
        short = name[len("DEOS_"):]
        if short.startswith(_SHORT_ALIAS_PREFIXES):
            aliases[short] = val
    constants.update(aliases)

    required = (
        "DEOS_UNIVERSAL_HEADER_SIZE",
        "DEOS_PROTOCOL_VERSION",
        "DEOS_MAX_PAYLOAD_LEN",
        "NODE_INVALID",
        "NODE_GLOBAL_BROADCAST",
        "PRIO_STATUS",
        "CLASS_COMMAND",
        "SERVICE_SYSTEM",
    )
    missing = [k for k in required if k not in constants]
    if missing:
        raise RuntimeError(f"DEOS C başlıklarından beklenen sabitler okunamadı: {missing}")

    return constants

globals().update(_load_c_constants())


# ==============================================================================
# 3. C ctypes Yapıları ve Fonksiyon İmzaları
# ==============================================================================

class _CDeosMessage(ctypes.Structure):
    _fields_ = [
        ("priority",      ctypes.c_int),
        ("message_class", ctypes.c_int),
        ("service",       ctypes.c_int),
        ("destination",   ctypes.c_int),
        ("source",        ctypes.c_int),
        ("version",       ctypes.c_uint8),
        ("sequence",      ctypes.c_uint8),
        ("command",       ctypes.c_uint8),
        ("payload",       ctypes.c_uint8 * DEOS_MAX_PAYLOAD_LEN),
        ("payload_len",   ctypes.c_uint8),
    ]

_C_HANDLER_TYPE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.POINTER(_CDeosMessage), ctypes.c_void_p)

# Fonksiyon Bağlantıları
_lib.deos_frame_encode.argtypes = [ctypes.POINTER(_CDeosMessage), ctypes.c_char_p, ctypes.c_size_t]
_lib.deos_frame_encode.restype  = ctypes.c_int

_lib.deos_frame_decode.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.POINTER(_CDeosMessage)]
_lib.deos_frame_decode.restype  = ctypes.c_int

_lib.deos_node_name.argtypes    = [ctypes.c_uint8]
_lib.deos_node_name.restype     = ctypes.c_char_p

_lib.deos_service_name.argtypes = [ctypes.c_uint8]
_lib.deos_service_name.restype  = ctypes.c_char_p

_lib.deos_class_name.argtypes   = [ctypes.c_uint8]
_lib.deos_class_name.restype    = ctypes.c_char_p

_lib.deos_command_name.argtypes = [ctypes.c_uint8, ctypes.c_uint8]
_lib.deos_command_name.restype  = ctypes.c_char_p

_lib.deos_fault_name.argtypes          = [ctypes.c_uint16]
_lib.deos_fault_name.restype           = ctypes.c_char_p

_lib.deos_fault_severity_name.argtypes = [ctypes.c_int]
_lib.deos_fault_severity_name.restype  = ctypes.c_char_p

_lib.deos_registry_sizeof.argtypes = []
_lib.deos_registry_sizeof.restype  = ctypes.c_size_t

_lib.deos_registry_init.argtypes = [ctypes.c_void_p]
_lib.deos_registry_init.restype  = ctypes.c_int

_lib.deos_registry_register_for_node.argtypes = [
    ctypes.c_void_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint8, _C_HANDLER_TYPE, ctypes.c_void_p
]
_lib.deos_registry_register_for_node.restype = ctypes.c_int

_lib.deos_registry_dispatch.argtypes = [ctypes.c_void_p, ctypes.POINTER(_CDeosMessage)]
_lib.deos_registry_dispatch.restype  = ctypes.c_int

# Codec Fonksiyonları
_lib.deos_put_u16_le.argtypes = [ctypes.c_char_p, ctypes.c_uint16]
_lib.deos_put_u16_le.restype  = None

_lib.deos_get_u16_le.argtypes = [ctypes.c_char_p]
_lib.deos_get_u16_le.restype  = ctypes.c_uint16

_lib.deos_put_i16_le.argtypes = [ctypes.c_char_p, ctypes.c_int16]
_lib.deos_put_i16_le.restype  = None

_lib.deos_get_i16_le.argtypes = [ctypes.c_char_p]
_lib.deos_get_i16_le.restype  = ctypes.c_int16

_lib.deos_put_u32_le.argtypes = [ctypes.c_char_p, ctypes.c_uint32]
_lib.deos_put_u32_le.restype  = None

_lib.deos_get_u32_le.argtypes = [ctypes.c_char_p]
_lib.deos_get_u32_le.restype  = ctypes.c_uint32

_lib.deos_put_i32_le.argtypes = [ctypes.c_char_p, ctypes.c_int32]
_lib.deos_put_i32_le.restype  = None

_lib.deos_get_i32_le.argtypes = [ctypes.c_char_p]
_lib.deos_get_i32_le.restype  = ctypes.c_int32


# ==============================================================================
# 4. Yüksek Seviye Pythonic Arayüz (Wrapper)
# ==============================================================================

class DeosMessage:
    """
    C kütüphanesindeki `deos_message_t` yapısının yüksek seviyeli Python sarmalayıcısı.
    Tüm encode/decode işlemleri C kütüphanesi (src/deos_frame.c) üzerinden yürütülür.
    """
    def __init__(self,
                 destination: int = NODE_INVALID,
                 source: int = NODE_INVALID,
                 priority: int = PRIO_STATUS,
                 version: int = DEOS_PROTOCOL_VERSION,
                 message_class: int = CLASS_COMMAND,
                 service: int = SERVICE_SYSTEM,
                 command: int = 0,
                 sequence: int = 0,
                 payload: bytes = b""):
        self._c = _CDeosMessage()
        self.destination = destination
        self.source = source
        self.priority = priority
        self.version = version
        self.message_class = message_class
        self.service = service
        self.command = command
        self.sequence = sequence
        self.payload = payload

    @property
    def destination(self) -> int: return self._c.destination
    @destination.setter
    def destination(self, val: int): self._c.destination = int(val)

    @property
    def source(self) -> int: return self._c.source
    @source.setter
    def source(self, val: int): self._c.source = int(val)

    @property
    def priority(self) -> int: return self._c.priority
    @priority.setter
    def priority(self, val: int): self._c.priority = int(val)

    @property
    def version(self) -> int: return self._c.version
    @version.setter
    def version(self, val: int): self._c.version = int(val)

    @property
    def message_class(self) -> int: return self._c.message_class
    @message_class.setter
    def message_class(self, val: int): self._c.message_class = int(val)

    @property
    def service(self) -> int: return self._c.service
    @service.setter
    def service(self, val: int): self._c.service = int(val)

    @property
    def command(self) -> int: return self._c.command
    @command.setter
    def command(self, val: int): self._c.command = int(val)

    @property
    def sequence(self) -> int: return self._c.sequence
    @sequence.setter
    def sequence(self, val: int): self._c.sequence = int(val)

    @property
    def payload(self) -> bytes:
        return bytes(self._c.payload[:self._c.payload_len])

    @payload.setter
    def payload(self, data: bytes):
        if len(data) > DEOS_MAX_PAYLOAD_LEN:
            raise ValueError(f"Payload boyutu izin verilen {DEOS_MAX_PAYLOAD_LEN} bayttan büyük: {len(data)}")
        self._c.payload_len = len(data)
        for i, b in enumerate(data):
            self._c.payload[i] = b

    def encode(self, buf_max: int = 128) -> bytes:
        """
        C fonksiyonu `deos_frame_encode` çağırarak mesajı Universal Frame baytlarına dönüştürür.
        """
        out_buf = ctypes.create_string_buffer(buf_max)
        res = _lib.deos_frame_encode(ctypes.byref(self._c), out_buf, buf_max)
        if res < 0:
            raise RuntimeError(f"deos_frame_encode C hatası döndürdü: {res}")
        return out_buf.raw[:res]

    @classmethod
    def decode(cls, frame_bytes: bytes) -> Optional["DeosMessage"]:
        """
        C fonksiyonu `deos_frame_decode` çağırarak bayt dizisinden DeosMessage nesnesi oluşturur.
        """
        c_msg = _CDeosMessage()
        res = _lib.deos_frame_decode(frame_bytes, len(frame_bytes), ctypes.byref(c_msg))
        if res != 0:
            return None
        msg = cls()
        msg._c = c_msg
        return msg

    @property
    def destination_name(self) -> str:
        return _lib.deos_node_name(self.destination).decode("utf-8")

    @property
    def source_name(self) -> str:
        return _lib.deos_node_name(self.source).decode("utf-8")

    @property
    def service_name(self) -> str:
        return _lib.deos_service_name(self.service).decode("utf-8")

    @property
    def class_name(self) -> str:
        return _lib.deos_class_name(self.message_class).decode("utf-8")

    @property
    def command_name(self) -> str:
        return _lib.deos_command_name(self.service, self.command).decode("utf-8")

    def __repr__(self) -> str:
        return (f"<DeosMessage Src:{self.source_name} -> Dest:{self.destination_name} | "
                f"Class:{self.class_name} Svc:{self.service_name} Cmd:{self.command_name} | "
                f"Len:{len(self.payload)} Payload:{self.payload.hex()}>")


class DeosRegistry:
    """
    C kütüphanesindeki `deos_registry_t` mekanizmasını doğrudan çalıştırır.
    Python callback fonksiyonlarını C fonksiyon işaretçilerine bağlar.
    `deos-core` ile aynı modelde çalışır: varsayılan yerel düğüm kurucuda verilir.
    """
    def __init__(self, local_node: int):
        if int(local_node) == NODE_INVALID:
            raise ValueError("local_node NODE_INVALID (0x00) olamaz")
        self.local_node = int(local_node)
        reg_size = _lib.deos_registry_sizeof()
        self._reg_buf = ctypes.create_string_buffer(reg_size)
        res = _lib.deos_registry_init(self._reg_buf)
        if res != 0:
            raise RuntimeError(f"deos_registry_init C hatası döndürdü: {res}")
        # Python callback'lerinin garbage collector tarafından silinmesini önlemek için referans tablosu:
        self._c_callbacks: List[Any] = []

    def register_for_node(self, local_node: int, message_class: int, service: int,
                          command: int, callback: Callable[[DeosMessage], None]) -> int:
        """
        C motoruna belirli bir mantıksal düğüm (local_node) için callback kaydeder.
        """
        if int(local_node) == NODE_INVALID:
            raise ValueError("local_node NODE_INVALID (0x00) olamaz")

        def _c_shim(msg_ptr, user_data):
            msg = DeosMessage()
            msg._c = msg_ptr.contents
            try:
                callback(msg)
                return 0
            except Exception as e:
                print(f"[DeosRegistry Error in Python Callback]: {e}", file=sys.stderr)
                return -1

        c_func = _C_HANDLER_TYPE(_c_shim)
        res = _lib.deos_registry_register_for_node(
            self._reg_buf, int(local_node), message_class, service, command, c_func, None
        )
        if res == 0:
            self._c_callbacks.append(c_func)
        return res

    def register(self, message_class: int, service: int, command: int,
                 callback: Callable[[DeosMessage], None]) -> int:
        """
        C motoruna bu registry'nin birincil düğümü (`self.local_node`) için callback kaydeder.
        """
        return self.register_for_node(self.local_node, message_class, service, command, callback)

    def dispatch(self, message: DeosMessage) -> int:
        """
        C `deos_registry_dispatch` fonksiyonunu çağırarak mesajı ilgili handler'a yönlendirir.
        """
        return _lib.deos_registry_dispatch(self._reg_buf, ctypes.byref(message._c))


# ==============================================================================
# 5. Little-Endian Codec Yardımcıları (C Kütüphanesini Çağırır)
# ==============================================================================

def put_u16_le(val: int) -> bytes:
    buf = ctypes.create_string_buffer(2)
    _lib.deos_put_u16_le(buf, val)
    return buf.raw[:2]

def get_u16_le(data: bytes) -> int:
    if len(data) < 2:
        raise ValueError(f"get_u16_le requires at least 2 bytes, got {len(data)}")
    return _lib.deos_get_u16_le(data[:2])

def put_i16_le(val: int) -> bytes:
    buf = ctypes.create_string_buffer(2)
    _lib.deos_put_i16_le(buf, val)
    return buf.raw[:2]

def get_i16_le(data: bytes) -> int:
    if len(data) < 2:
        raise ValueError(f"get_i16_le requires at least 2 bytes, got {len(data)}")
    return _lib.deos_get_i16_le(data[:2])

def put_u32_le(val: int) -> bytes:
    buf = ctypes.create_string_buffer(4)
    _lib.deos_put_u32_le(buf, val)
    return buf.raw[:4]

def get_u32_le(data: bytes) -> int:
    if len(data) < 4:
        raise ValueError(f"get_u32_le requires at least 4 bytes, got {len(data)}")
    return _lib.deos_get_u32_le(data[:4])

def put_i32_le(val: int) -> bytes:
    buf = ctypes.create_string_buffer(4)
    _lib.deos_put_i32_le(buf, val)
    return buf.raw[:4]

def get_i32_le(data: bytes) -> int:
    if len(data) < 4:
        raise ValueError(f"get_i32_le requires at least 4 bytes, got {len(data)}")
    return _lib.deos_get_i32_le(data[:4])


def fault_name(fault_id: int) -> str:
    return _lib.deos_fault_name(int(fault_id)).decode("utf-8")


def fault_severity_name(severity: int) -> str:
    return _lib.deos_fault_severity_name(int(severity)).decode("utf-8")
