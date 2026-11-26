# DEOS Protocol Kullanım Rehberi (v1.2 / `0x12`)

Bu rehber, **`deos-protocol`** kütüphanesini **Python**, **C/C++ (ROS 2 / Masaüstü)** ve **Gömülü (STM32 / `deos-core`)** projelerinde doğru ve net biçimde kullanmak için hazırlanmıştır.

---

## 1. `deos-protocol` Ne Yapar, Ne Yapmaz?

`deos-protocol`, DEOS ekosisteminin **işletim sisteminden bağımsız, `malloc` kullanmayan, saf ANSI C99** protokol katmanıdır.

**Yaptıkları (4 ana parça):**
1. **Tek Doğruluk Kaynağı (SSOT) Sözlük & Tipler** (`include/deos/deos_icd.h`, `include/deos/deos_types.h`)
   - Protokol sürümü (`DEOS_PROTOCOL_VERSION = 0x12`), Node ID'ler, Service ID'ler, Command ID'ler, global 16-bit Fault ID'leri (`deos_fault_id_t`) ve tüm `__attribute__((packed))` payload struct'ları.
2. **Evrensel Çerçeve (Universal Frame)** (`include/deos/deos_frame.h`, `src/deos_frame.c`)
   - 9 baytlık sabit başlık + $N$ bayt (0–60) payload kodlama (`deos_frame_encode`) ve sıkı doğrulamalı çözme (`deos_frame_decode`).
   - İnsan okunur string çözücüler: `deos_node_name`, `deos_service_name`, `deos_class_name`, `deos_command_name`, `deos_fault_name`, `deos_fault_severity_name`.
3. **Little-Endian Codec** (`include/deos/deos_codec.h`, `src/deos_codec.c`)
   - Mimariden bağımsız 16/32-bit okuma/yazma (`deos_put_u16_le`, `deos_get_i16_le`, `deos_put_u32_le`, `deos_get_i32_le` vb.).
4. **Opsiyonel Mesaj Handler Registry** (`include/deos/deos_registry.h`, `src/deos_registry.c`)
   - Gelen mesajları `(local_node, message_class, service, command)` anahtarına göre callback fonksiyonlarına yönlendiren sabit boyutlu tablo.

**Yapmadıkları (Diğer katmanların sorumluluğu):**
- UDP, TCP, CAN-FD veya LoRa/UART soket/donanım açmaz; yalnızca ham bayt dizisi üretir ve çözer.
- Routing, abonelik zamanlayıcısı, heartbeat veya STM32 fault kayıt tablosu tutmaz — bunlar Zephyr üzerindeki **`deos-core`** kütüphanesinin görevidir.

---

## 2. 9-Baytlık Evrensel Çerçeve (Universal Frame)

```text
Byte 0: Destination Node (uint8_t)
Byte 1: Source Node      (uint8_t)
Byte 2: Priority         (uint8_t)
Byte 3: Version          (uint8_t, zorunlu: 0x12 = v1.2)
Byte 4: Message Class    (uint8_t)
Byte 5: Service ID       (uint8_t)
Byte 6: Command ID       (uint8_t)
Byte 7: Sequence         (uint8_t, 0..255)
Byte 8: Payload Length   (uint8_t, N = 0..60)
Byte 9..9+N-1: Payload   (uint8_t[N], Little-Endian)
```

**Sıkı `deos_frame_decode()` kuralları:**
- `Version != 0x12` → `-EPROTONOSUPPORT` (Python'da `None`)
- `Payload Length > 60` → `-EMSGSIZE` (Python'da `None`)
- `9 + Payload Length != frame_len` (eksik veya fazla bayt) → `-EBADMSG` (Python'da `None`)

---

## 3. Python ile Kullanım (`python/deos.py`)

### Arkada Ne Çalışır?
`python/deos.py` protokolü Python'da yeniden yazmaz; derlenmiş native C kütüphanesini (`libdeos_protocol.so`) **`ctypes`** ile aynı bellek alanında doğrudan çağırır:
- `import deos` yapıldığında tüm `DEOS_*` sabitleri otomatik olarak `include/deos/*.h` C başlıklarından okunur (elle kopya yoktur).
- `libdeos_protocol.so` yoksa kaynak koddan `gcc` ile otomatik derlenir (özel bir `.so` yolu vermek için `DEOS_PROTOCOL_LIB=/yol/libdeos_protocol.so` ortam değişkeni kullanılabilir).
- `encode()`, `decode()`, `put_*_le()`, `get_*_le()`, `DeosRegistry.dispatch()` ve `fault_name()` çağrılarının tamamı doğrudan **C kodu** çalıştırır.

```python
import sys
sys.path.insert(0, "/yol/deos-protocol/python")
import deos
```

### 3.1. Mesaj Oluşturma ve Gönderme (3 Adım)

```python
import socket
import deos

# 1) Payload'ı Little-Endian hazırla (ör. Traction hedef hız 15.00 m/s, ivme 2.00 m/s²)
payload = deos.put_u16_le(1500) + deos.put_i16_le(200)

# 2) Mesajı oluştur ve C deos_frame_encode() ile Universal Frame baytlarına çevir
msg = deos.DeosMessage(
    destination=deos.NODE_TRACTION,
    source=deos.NODE_ROS_BRIDGE,
    priority=deos.PRIO_CONTROL,
    message_class=deos.CLASS_COMMAND,
    service=deos.SERVICE_TRACTION,
    command=deos.CMD_TRACTION_SET_TARGET_SPEED,
    sequence=1,
    payload=payload,
)
frame: bytes = msg.encode()  # 9 + 4 = 13 bayt (Version otomatik 0x12)

# 3) Kullandığın taşıma hattının (L2) zarfıyla gönder:
# Ethernet UDP (ROS Bridge, Port 5001): [1B uzunluk] + frame
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(bytes([len(frame)]) + frame, ("192.168.1.10", 5001))

# Ethernet TCP (Textual UI, Port 5000): [0xAA, 0x55, 1B uzunluk] + frame
# tcp_sock.sendall(b"\xAA\x55" + bytes([len(frame)]) + frame)
```

Payload gerektirmeyen komutlarda (ör. `GET_FAULTS`) `payload` vermemen yeterlidir (`msg.encode()` 9 bayt üretir).

### 3.2. Gelen Mesajı Çözme

```python
# UDP datagramından 1 baytlık uzunluk önekini doğrula
if datagram and datagram[0] == len(datagram) - 1:
    msg = deos.DeosMessage.decode(datagram[1:])
    if msg is None:
        print("Geçersiz çerçeve veya uyumsuz sürüm (v1.2 değil)!")
    else:
        print(msg)  # <DeosMessage Src:... -> Dest:... | Class:... Svc:... Cmd:... | ...>
        try:
            speed = deos.get_u16_le(msg.payload[0:2])
        except ValueError as err:
            print(f"Eksik payload: {err}")
```

### 3.3. Gelen Mesajları İşleme: Registry Zorunlu mu?

**Hayır, `DeosRegistry` opsiyoneldir.**

#### Yöntem A — Registry kullanmadan (`if / elif`, küçük scriptler için)
```python
msg = deos.DeosMessage.decode(frame)
if msg is None:
    continue

if msg.service == deos.SERVICE_SENSORS and msg.command == deos.CMD_SENSOR_IMU_DATA:
    ax = deos.get_i16_le(msg.payload[0:2]) * 0.01
    print(f"IMU ax = {ax:.2f} m/s²")
elif msg.service == deos.SERVICE_DIAGNOSTIC and msg.command == deos.CMD_DIAG_GET_FAULTS:
    fault_id = deos.get_u16_le(msg.payload[0:2])
    print("Fault:", deos.fault_name(fault_id))
```

#### Yöntem B — `DeosRegistry(local_node)` ile (büyük uygulamalar için)
`DeosRegistry` yalnızca **gelen (RX)** mesajları `(destination, class, service, command)` anahtarıyla eşleştirir (`source` alanına bakmaz). `deos-core` ile aynı mantıkta çalışır:

```python
# Birincil yerel düğüm kurucuda zorunludur (NODE_INVALID kabul edilmez)
reg = deos.DeosRegistry(deos.NODE_ROS_BRIDGE)

def on_pong(msg: deos.DeosMessage) -> None:
    ping_id = deos.get_u32_le(msg.payload)
    print(f"PONG <- {msg.source_name}: 0x{ping_id:08X}")

def on_imu(msg: deos.DeosMessage) -> None:
    ax = deos.get_i16_le(msg.payload[0:2]) * 0.01
    print(f"IMU ax = {ax:.2f} m/s²")

# Normal kayıt (self.local_node = NODE_ROS_BRIDGE için)
reg.register(deos.CLASS_NETWORK, deos.SERVICE_SYSTEM,  deos.CMD_SYSTEM_PONG,     on_pong)
reg.register(deos.CLASS_STATUS,  deos.SERVICE_SENSORS, deos.CMD_SENSOR_IMU_DATA, on_imu)

# Aynı süreçte birden fazla mantıksal düğüm barındırılıyorsa:
# reg.register_for_node(deos.NODE_IMU, deos.CLASS_REQUEST, deos.SERVICE_SENSORS, deos.CMD_SENSOR_IMU_DATA, on_imu_req)

# Gelen mesajı C motoruna ver
matched = reg.dispatch(msg)  # Eşleşen callback sayısını döner
```

**Deterministik dağıtım kuralı:**
- **Unicast** mesajda önce tam düğüm eşleşmesi (`local_node == msg.destination`) çalışır; yalnızca tam eşleşme yoksa ve açıkça `NODE_GLOBAL_BROADCAST` (`0xFF`) kaydı yapılmışsa o fallback çalışır. Her unicast mesaj en fazla **1** callback tetikler.
- **Broadcast** (`0xFE` veya `0xFF`) mesajda eşleşen tüm callback'ler çalışır.

---

## 4. Global Fault Yönetimi (`DEOS_SERVICE_DIAGNOSTIC = 0x06`)

DEOS v1.2'de tüm hata kodları sistem genelinde **tek ve benzersiz 16-bit global ID** (`deos_fault_id_t`) kullanır; nereden geldiğine bakılmaksızın aynı ID her zaman aynı anlama gelir.

| Aralık | Alt-Sistem |
| :--- | :--- |
| `0x0000` | `DEOS_FAULT_ID_END_OF_LIST` (liste sonu) |
| `0x0001–0x00FF` | Core & Transport (System, Sub, Router, Protocol, CAN, LoRa, Eth, Dispatch) |
| `0x0100–0x03FF` | Traction (`0x0100+`), Steering (`0x0200+`), Brake (`0x0300+`) |
| `0x0400–0x04FF` | BMS (`DEOS_FAULT_BMS_*`) |
| `0x0500–0x05FF` | BMS Gateway (`DEOS_FAULT_GW_*`) |
| `0x0600–0x08FF` | Sensors (`0x0600+`), Lighting (`0x0700+`), Calibration (`0x0800+`) |
| `0xF000–0xFFFE` | Geçici / Debug |

### STM32 vs Python Farkı
- **STM32 (`deos-core`):** `GET_FAULTS` (`0x02`) ve `CLEAR_FAULTS` (`0x03`) komutlarını çekirdekte yerleşik olarak otomatik yanıtlar; STM32 uygulamasında registry'ye fault handler'ı kaydedilmez.
- **Python / ROS 2 / Yer İstasyonu:** Hedef düğüme `GET_FAULTS` gönderilir; hedef düğüm her kayıt için 100 ms arayla **10 baytlık** `CLASS_RESPONSE` yollar ve `0x0000` (`END_OF_LIST`) ile bitirir.

```python
# Fault isteme (GET_FAULTS - payload yok)
req = deos.DeosMessage(
    destination=deos.NODE_BMS_MAIN,
    source=deos.NODE_GROUND_CONTROL,
    priority=deos.PRIO_STATUS,
    message_class=deos.CLASS_REQUEST,
    service=deos.SERVICE_DIAGNOSTIC,
    command=deos.CMD_DIAG_GET_FAULTS,
    sequence=1,
)

# Gelen 10 baytlık GET_FAULTS yanıtını işleme (source fark etmeksizin tek callback yeter)
def on_fault_response(msg: deos.DeosMessage) -> None:
    if len(msg.payload) < 10:
        return
    fault_id = deos.get_u16_le(msg.payload[0:2])
    severity = msg.payload[2]
    state    = msg.payload[3]
    count    = deos.get_u16_le(msg.payload[4:6])
    last_ms  = deos.get_u32_le(msg.payload[6:10])

    if fault_id == deos.FAULT_ID_END_OF_LIST:
        print(f"[{msg.source_name}] Fault listesi tamamlandı.")
        return

    print(
        f"[{msg.source_name}] {deos.fault_name(fault_id)} (0x{fault_id:04X}) | "
        f"Seviye={deos.fault_severity_name(severity)} | State={state} | "
        f"Adet={count} | Son={last_ms} ms"
    )
```

---

## 5. C / C++ ile Kullanım

### CMake Entegrasyonu
```cmake
add_subdirectory(vendor/deos_protocol)
target_link_libraries(my_app PRIVATE deos_protocol)
```

### Temel C++ Kullanımı (`-Wall -Wextra -Wpedantic -Werror` uyumlu)
```cpp
#include <deos/deos_frame.h>
#include <deos/deos_codec.h>
#include <deos/deos_registry.h>

deos_registry_t reg{};
deos_registry_init(&reg);

deos_registry_register_for_node(
    &reg,
    DEOS_NODE_ROS_BRIDGE,
    DEOS_CLASS_STATUS,
    DEOS_SERVICE_SENSORS,
    DEOS_CMD_SENSOR_IMU_DATA,
    [](const deos_message_t *msg, void *user_data) -> int {
        (void)user_data;
        int16_t ax = deos_get_i16_le(msg->payload);
        (void)ax;
        return 0;
    },
    nullptr
);

// Gelen ham çerçeveyi çöz ve dağıt
deos_message_t rx{};
if (deos_frame_decode(raw_buf, raw_len, &rx) == 0) {
    deos_registry_dispatch(&reg, &rx);
}
```

---

## 6. Kütüphane Testlerini Çalıştırma

```bash
cmake -S . -B build -DDEOS_PROTOCOL_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

64 handler kapasitesiyle test etmek için:
```bash
cmake -S . -B build64 -DDEOS_PROTOCOL_BUILD_TESTS=ON \
  -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic -Werror -DDEOS_MAX_REGISTRY_HANDLERS=64" \
  -DCMAKE_CXX_FLAGS="-DDEOS_MAX_REGISTRY_HANDLERS=64"
cmake --build build64
ctest --test-dir build64 --output-on-failure
```
