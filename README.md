# AHT20-Environmental-Monitor 🌡️

## 🇬🇧 English

A real-time temperature and humidity monitoring system built on the Silicon Labs
EFR32xG21 Wireless Gecko platform, featuring UART-based PC communication, dual
database storage, LCD display, and BLE broadcasting — developed as a Computer
Interfacing lab project at Ho Chi Minh City University of Science.

### Overview

The system reads temperature and humidity data from an AHT20 sensor every 5
seconds via I²C, displays the values on the onboard LCD, streams data to a PC
over UART at 115200 baud, broadcasts via BLE advertising, and stores all
readings into both CSV and SQLite databases.

### Hardware Components
| Component | Role |
|---|---|
| EFR32xG21 (BRD4180B + BRD4001A) | Main microcontroller with BLE 2.4 GHz |
| AHT20 | I²C temperature (±0.3°C) & humidity (±2% RH) sensor |
| Memory LCD 128×128 | Local real-time display (SPI, ultra-low power) |

### Features
- Periodic sensor polling via hardware timer (default 5s, adjustable)
- LCD display: temperature, humidity, sensor period, BLE interval
- UART interrupt-driven command interface at 115200 baud
- BLE advertising (custom manufacturer data with BCD-encoded sensor values)
- Auto-logging mode: continuous data capture without user input
- Dual storage: CSV file and SQLite database with timestamp
- UART command set for live system control

### UART Command Set
| Command | Action |
|---|---|
| `1` | Read sensor immediately |
| `2` / `3` | Increase / Decrease sensor period ±1s |
| `4` / `5` | Increase / Decrease BLE interval ±100ms |
| `6` | Show current configuration |
| `7` | Show database statistics |
| `h` | Show help menu |
| `q` | Quit and save all data |

### Technologies
- C (Simplicity Studio 5, Simplicity SDK Suite v2024.6.2)
- PC logger: C on Windows (Visual Studio Code)
- Database: CSV + SQLite3
- BLE: Silicon Labs BLE stack, Si Connect app
- Protocols: I²C (sensor), SPI (LCD), UART (PC), BLE Advertising

### Authors
- Lê Hoàng Bảo Ngọc – 22207119
- Lê Trần Anh Kiệt – 22207116
- Lê Thanh Vy – 22207107
- Nguyễn Vy Ngọc – 22207064

**Course:** Computer Interfacing Lab — HCMUS  
**Instructor:** Hồ Thanh Bảo | **Class:** 22DTV_CLC1 | **Group:** Nhóm 18

---

## 🇻🇳 Tiếng Việt

Hệ thống giám sát nhiệt độ và độ ẩm thời gian thực trên nền tảng Silicon Labs
EFR32xG21, tích hợp giao tiếp UART với PC, lưu trữ cơ sở dữ liệu kép, hiển thị
LCD và quảng bá BLE — thực hiện trong khuôn khổ đồ án môn Thực hành Giao
tiếp Máy tính tại Trường Đại học Khoa học Tự nhiên TP.HCM.

### Tổng quan

Hệ thống đọc nhiệt độ và độ ẩm từ cảm biến AHT20 mỗi 5 giây qua I²C, hiển thị
lên màn hình LCD, truyền dữ liệu đến PC qua UART ở tốc độ 115200 baud, quảng
bá qua BLE, và lưu toàn bộ dữ liệu vào hai cơ sở dữ liệu CSV và SQLite.

### Linh kiện phần cứng
| Linh kiện | Vai trò |
|---|---|
| EFR32xG21 (BRD4180B + BRD4001A) | Vi điều khiển chính, BLE 2.4 GHz |
| AHT20 | Cảm biến I²C đo nhiệt độ (±0.3°C) và độ ẩm (±2% RH) |
| Memory LCD 128×128 | Hiển thị cục bộ thời gian thực (SPI, siêu tiết kiệm điện) |

### Tính năng
- Đọc cảm biến theo chu kỳ qua hardware timer (mặc định 5s, có thể điều chỉnh)
- Hiển thị LCD: nhiệt độ, độ ẩm, chu kỳ sensor, khoảng quảng bá BLE
- Giao tiếp UART điều khiển qua ngắt ở tốc độ 115200 baud
- Quảng bá BLE (dữ liệu cảm biến mã hóa BCD trong manufacturer data)
- Chế độ auto-logging: ghi dữ liệu liên tục không cần can thiệp
- Lưu trữ kép: file CSV và cơ sở dữ liệu SQLite với timestamp
- Bộ lệnh UART điều khiển hệ thống trực tiếp từ PC

### Bộ lệnh UART
| Lệnh | Chức năng |
|---|---|
| `1` | Đọc cảm biến ngay lập tức |
| `2` / `3` | Tăng / Giảm chu kỳ đọc sensor ±1s |
| `4` / `5` | Tăng / Giảm chu kỳ quảng bá BLE ±100ms |
| `6` | Hiển thị cấu hình hiện tại |
| `7` | Hiển thị thống kê cơ sở dữ liệu |
| `h` | Hiển thị menu trợ giúp |
| `q` | Thoát và lưu toàn bộ dữ liệu |

### Công nghệ
- C (Simplicity Studio 5, Simplicity SDK Suite v2024.6.2)
- PC logger: C trên Windows (Visual Studio Code)
- Cơ sở dữ liệu: CSV + SQLite3
- BLE: Silicon Labs BLE stack, ứng dụng Si Connect
- Giao thức: I²C (cảm biến), SPI (LCD), UART (PC), BLE Advertising

### Thành viên nhóm
- Lê Hoàng Bảo Ngọc – 22207119
- Lê Trần Anh Kiệt – 22207116
- Lê Thanh Vy – 22207107
- Nguyễn Vy Ngọc – 22207064

**Môn học:** Thực hành Giao tiếp Máy tính — ĐHKHTN TP.HCM  
**Giảng viên:** Hồ Thanh Bảo | **Lớp:** 22DTV_CLC1 | **Nhóm:** Nhóm 18
