
# 🛡️ Edge-AI 기반 로봇 팔 텔레메트리 보안 파이프라인 (CPS)

## 📌 프로젝트 제1원칙 (First Principles)
* **기존의 관성:** "모든 에지(Edge) 데이터를 무거운 프레임워크(ROS 2 등)에 실어 중앙 서버로 다 보낸다. 보안은 소프트웨어 계층(L7)에서만 처리한다."
* **물리적 본질:** 에지 디바이스(ESP32)는 램과 연산력이 극도로 부족하며, 실시간 쏟아지는 정상 데이터를 모두 전송하면 네트워크 병목과 백엔드(LLM) 토큰 낭비가 발생함. 또한 소프트웨어 암호화는 물리적 메모리 덤프에 취약함.
* **혁신적 설계:** 1. **오프로딩(Offloading):** 에지에서는 초경량 UDP로 직렬화된 JSON만 송출. ROS 2 변환 및 라우팅은 라즈베리파이 게이트웨이가 전담.
  2. **HRoT (Hardware Root of Trust):** ATECC608A 물리 보안 칩을 도입해 해커의 데이터 탈취를 하드웨어 단에서 원천 차단.
  3. **4-Tier 상태 머신 기반 바이패스:** 정상(Safe) 데이터는 10초 단위로 압축(Batch) 전송하고, 충돌/과부하 데이터(Danger, Error)는 즉시 우회(Bypass) 전송하는 스마트 라우팅 구현.

---

## 🛠️ 기술 스택 (Tech Stack)

### 💻 Edge Node (센서 및 액추에이터 계층)
* **MCU:** ESP32 NodeMCU-32S (Multi-Node 분산 제어)
* **Language:** C/C++ (Arduino IDE)
* **Security:** ATECC608A (하드웨어 암호화 전용 Co-Processor)
* **Hardware:** DYNAMIXEL XL430-W250-T (스마트 다관절 서보모터 - 실시간 토크/온도 피드백)
* **Library:** `ArduinoJson` (데이터 직렬화)

### 🍓 Gateway (L7 스마트 라우터 및 방화벽)
* **Device:** Raspberry Pi 4 (Ubuntu/Raspberry Pi OS)
* **Language:** Python 3
* **Middleware:** ROS 2 (UDP to DDS 브릿지 및 다중 토픽 라우팅)
* **Edge AI:** Google Coral USB Accelerator (비정상 트래픽 탐지 NPU)

### 🧠 Backend (분석 및 모니터링)
* **Device:** Ubuntu 22.04 (RTX 4070 탑재 랩탑)
* **Middleware:** ROS 2 (데이터 수신)
* **Analysis:** 로컬 LLM (물리적 충돌 원인 실시간 분석 리포트 생성)

### 🗡️ Red Team (모의 해킹 및 검증)
* **Device:** ESP8266 NodeMCU V1.0
* **Tool:** Deauther Firmware (Wi-Fi 교란 및 망분리 결함 격리 테스트용)

---


## 🏗️ 시스템 아키텍처

1. **데이터 수집:** 다이나믹셀 모터에서 발생하는 실제 물리 데이터(Torque, Temperature)를 ESP32가 수집.
2. **하드웨어 암호화:** ATECC608A 칩을 통해 페이로드를 AES 암호화 후 UDP 송출.
3. **투트랙 라우팅 (Gateway):** 라즈베리파이 게이트웨이가 패킷 수신 후 상태(Safe, Warning, Danger, Error)를 판별.
   * `SAFE`: 버퍼에 적재 후 10초 단위 압축 전송 (`/safe_bulk_data`)
   * `ERROR/DANGER`: 즉각적인 ROS 2 토픽으로 바이패스 전송 (`/alert_error`, `/alert_danger`)
4. **AI 침입 탐지:** Google Coral TPU가 네트워크 패킷을 감시하여 Deauth 등 비정상 공격 방어.

---
## 📁 폴더 구조 (Directory Structure)

```text
SRAM_ROUTER/
├── sram_exp32tsetdata/
│   └── RobotARMTestData.ino   # 로봇 팔 관절 모터 제어 및 원시(Raw) 데이터 수집용 테스트 코드
├── esp32_senddata/            # ESP32에서 수집된 데이터를 UDP 패킷으로 변환하여 송출하는 에지 노드 코드
├── sram_rasberrydata/         # 라즈베리파이 도커 환경 및 ROS 2 L7 상태 분류 게이트웨이 소스 코드
└── Readme.md                  # 프로젝트 통합 설명서
```
---

## 🚀 시작하기 (Runbook)

네트워크 무결성(수신 대기 선행)을 위해 **반드시 역순(Backend ➡️ Gateway ➡️ Edge)**으로 가동합니다.

### 1. Backend 수신 노드 가동 (Ubuntu)
```bash
export ROS_DOMAIN_ID=30
cd ~/llm_ws
source install/setup.bash
# //여기까진 ROS 2 주파수 동기화 및 LLM 수신 작업 공간 환경 변수 등록에 대한 내용

ros2 run llm_pkg llm_analyzer_node
# //여기까진 4단계 필터링 데이터를 대기하는 LLM 분석 노드 실행에 대한 내용
```

### 2. Gateway 라우터 가동 (Raspberry Pi)
```bash
export ROS_DOMAIN_ID=30
cd ~/sec_filter_ws
source install/setup.bash
# //여기까진 라즈베리파이 네트워크 주파수 세팅 및 필터 패키지 경로 등록에 대한 내용

ros2 run sec_filter_pkg filter_node
# //여기까진 UDP 8888 포트를 개방하고 투트랙 라우팅 게이트웨이를 활성화하는 것에 대한 내용
```
### 3. Edge Node 전원 인가 (ESP32)
```
Gateway 정상 가동(Listening) 확인 후, ESP32에 전원을 인가하여 암호화된 센서 데이터 송출 시작.
EOF
```
