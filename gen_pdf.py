from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.platypus import (
    SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, HRFlowable
)
from reportlab.lib.enums import TA_LEFT, TA_CENTER

OUTPUT = "pico_mst_260219_project_description.pdf"

doc = SimpleDocTemplate(
    OUTPUT,
    pagesize=A4,
    rightMargin=20*mm, leftMargin=20*mm,
    topMargin=20*mm, bottomMargin=20*mm,
)

styles = getSampleStyleSheet()

title_style = ParagraphStyle("Title2", parent=styles["Title"], fontSize=18, spaceAfter=6)
h1_style = ParagraphStyle("H1", parent=styles["Heading1"], fontSize=13, spaceAfter=4, spaceBefore=12, textColor=colors.HexColor("#1a3a5c"))
h2_style = ParagraphStyle("H2", parent=styles["Heading2"], fontSize=11, spaceAfter=3, spaceBefore=8, textColor=colors.HexColor("#2e6da4"))
body_style = ParagraphStyle("Body2", parent=styles["Normal"], fontSize=9.5, leading=14, spaceAfter=4)
code_style = ParagraphStyle("Code", parent=styles["Code"], fontSize=8.5, leading=12, leftIndent=10, backColor=colors.HexColor("#f4f4f4"), borderColor=colors.HexColor("#cccccc"), borderWidth=0.5, borderPadding=4)
note_style = ParagraphStyle("Note", parent=styles["Normal"], fontSize=9, textColor=colors.HexColor("#cc3300"), leftIndent=8)

def tbl(data, col_widths, header=True):
    t = Table(data, colWidths=col_widths)
    style_cmds = [
        ("FONTSIZE", (0,0), (-1,-1), 8.5),
        ("LEADING", (0,0), (-1,-1), 12),
        ("ROWBACKGROUND", (0,0), (-1,0), colors.HexColor("#d0e4f5")),
        ("FONTNAME", (0,0), (-1,0), "Helvetica-Bold"),
        ("GRID", (0,0), (-1,-1), 0.4, colors.HexColor("#aaaaaa")),
        ("VALIGN", (0,0), (-1,-1), "TOP"),
        ("LEFTPADDING", (0,0), (-1,-1), 5),
        ("RIGHTPADDING", (0,0), (-1,-1), 5),
        ("TOPPADDING", (0,0), (-1,-1), 3),
        ("BOTTOMPADDING", (0,0), (-1,-1), 3),
    ]
    if header:
        for i in range(1, len(data)):
            bg = colors.white if i % 2 == 0 else colors.HexColor("#f7fbff")
            style_cmds.append(("ROWBACKGROUND", (0,i), (-1,i), bg))
    t.setStyle(TableStyle(style_cmds))
    return t

W = 170*mm

story = []

# ── Title ─────────────────────────────────────────────────────────────────────
story.append(Paragraph("pico_mst_260219", title_style))
story.append(Paragraph("RP2040 LoRa Master Node Firmware", styles["Heading2"]))
story.append(Paragraph("Project created: 2026-02-19 &nbsp;|&nbsp; Platform: Raspberry Pi Pico (RP2040)", body_style))
story.append(HRFlowable(width=W, thickness=1, color=colors.HexColor("#1a3a5c"), spaceAfter=8))

# ── Overview ──────────────────────────────────────────────────────────────────
story.append(Paragraph("1. Project Overview", h1_style))
story.append(Paragraph(
    "This firmware runs on a Raspberry Pi Pico (RP2040 microcontroller) and implements a "
    "<b>LoRa wireless sensor network master node</b>. The master (MST) bridges a wired UART host "
    "application and a mesh of remote LoRa slave nodes. It handles timekeeping via a DS3231 RTC, "
    "displays status on an I2C LCD, periodically broadcasts beacon frames, automatically polls "
    "slave nodes, and relays host commands to the appropriate slave over the LoRa air link.",
    body_style))
story.append(Paragraph(
    "The radio module is a <b>Semtech SX1262</b> driven by the open-source <b>RadioLib</b> library. "
    "All firmware is written in C/C++ and built with the <b>Raspberry Pi Pico SDK 2.2.0</b> using CMake.",
    body_style))

# ── Hardware ──────────────────────────────────────────────────────────────────
story.append(Paragraph("2. Hardware Configuration", h1_style))

story.append(Paragraph("2.1 Pin Assignment", h2_style))
hw_data = [
    ["Peripheral", "Interface", "GPIO Pins", "Notes"],
    ["SX1262 LoRa Radio", "SPI1", "MISO=12, MOSI=11, SCK=10, CS=13", "1 MHz SPI clock"],
    ["SX1262 Control", "GPIO", "DIO1=20, RST=15, BUSY=2, CS=3", "IRQ on rising edge of DIO1"],
    ["I2C LCD Display", "I2C1", "SDA=6, SCL=7", "400 kHz, I2C address auto-detect"],
    ["DS3231 RTC", "I2C1", "SDA=6, SCL=7", "Shared I2C bus with LCD"],
    ["UART Host", "UART1", "TX=8, RX=9", "115200 baud, interrupt-driven RX"],
    ["Buttons", "GPIO", "BTN_U=18, BTN_D=19", "Defined; not yet active in firmware"],
    ["Onboard LED", "GPIO", "PICO_DEFAULT_LED_PIN", "1 Hz heartbeat blink"],
]
story.append(tbl(hw_data, [28*mm, 18*mm, 72*mm, 52*mm]))

story.append(Spacer(1, 4*mm))
story.append(Paragraph("2.2 Radio Parameters", h2_style))
radio_data = [
    ["Parameter", "Value"],
    ["Frequency", "915.0 MHz"],
    ["Bandwidth", "125.0 kHz"],
    ["Spreading Factor", "7"],
    ["Coding Rate", "5 (4/5)"],
    ["Sync Word", "0x12"],
    ["TX Power", "20 dBm"],
    ["Mode", "Continuous receive; transmit on demand"],
]
story.append(tbl(radio_data, [60*mm, 110*mm]))

# ── Architecture ──────────────────────────────────────────────────────────────
story.append(Paragraph("3. Software Architecture", h1_style))
story.append(Paragraph(
    "The firmware uses a <b>cooperative event-flag state machine</b> in the main loop. "
    "Interrupt service routines (ISRs) set bits in a global flag register "
    "<code>gPARAM_actflag</code>; the main loop polls this register and dispatches handlers "
    "one event per iteration. This avoids preemptive concurrency while keeping ISRs short.",
    body_style))

story.append(Paragraph("3.1 Event Flags (gPARAM_actflag)", h2_style))
flag_data = [
    ["Bit Mask", "Event", "Source", "Action"],
    ["0x01", "1-second tick", "100 ms repeating timer (÷10)", "Toggle LED, refresh LCD second counter"],
    ["0x02", "UART RX complete", "UART1 RX ISR", "Parse host command, generate response/RF action"],
    ["0x04", "Send UART TX", "RF cmd handler or host cmd", "Write response buffer to UART1"],
    ["0x10", "Beacon due", "Timer: seconds 0, 16, 32", "Compose beacon frame, trigger RF TX (0x20)"],
    ["0x20", "RF TX request", "Beacon, poll, or host relay", "Transmit gPARAM_RFTX_buf via SX1262"],
    ["0x40", "LCD refresh", "Timer: every 4 seconds", "Update slave data display rows on LCD"],
    ["0x80", "Auto-poll slave", "Timer: every 2 seconds", "Compose poll for next slave SID, trigger 0x20"],
]
story.append(tbl(flag_data, [18*mm, 30*mm, 52*mm, 70*mm]))

story.append(Spacer(1, 4*mm))
story.append(Paragraph("3.2 Timer Schedule (1-second counter gPARAM_fcnt_1s)", h2_style))
story.append(Paragraph(
    "The 100 ms hardware timer increments a sub-counter; every 10 ticks (1 s) it increments "
    "<code>gPARAM_fcnt_1s</code> (0–59) and sets the 1-second event. Additional periodic events "
    "are derived from the second counter modulo arithmetic:",
    body_style))
sched_data = [
    ["Condition", "Period", "Flag set"],
    ["fcnt_1s == 0, 16, or 32", "≈16 s", "0x10 (beacon)"],
    ["fcnt_1s % 4 == 2", "4 s", "0x40 (LCD refresh)"],
    ["fcnt_1s % 2 == 1", "2 s", "0x80 (auto-poll)"],
]
story.append(tbl(sched_data, [70*mm, 30*mm, 70*mm]))

story.append(Spacer(1, 4*mm))
story.append(Paragraph("3.3 ISR Summary", h2_style))
isr_data = [
    ["ISR", "Trigger", "Action"],
    ["irqRadio()", "GPIO rising edge on DIO1 (pin 20)", "Sets receivedFlag = true (if rf_state==0); else resets rf_state and restarts receive"],
    ["on_uart_rx()", "UART1 RX FIFO not empty", "Assembles byte-stream into gPARAM_UR1RX_buf using length-prefixed framing; sets 0x02 on frame complete"],
    ["repeating_timer_callback()", "Hardware repeating timer, every 100 ms", "Advances sub-counters, sets event flags 0x01 / 0x10 / 0x40 / 0x80"],
]
story.append(tbl(isr_data, [40*mm, 60*mm, 70*mm]))

# ── Protocol ──────────────────────────────────────────────────────────────────
story.append(Paragraph("4. Communication Protocol", h1_style))
story.append(Paragraph(
    "All packets (both over UART and RF) share the same <b>12-byte fixed-length frame format</b>:",
    body_style))
proto_data = [
    ["Byte(s)", "Field", "Description"],
    ["0", "hdr / cmd", "Command identifier (see table below)"],
    ["1", "length", "Always 12 (0x0C)"],
    ["2", "mid", "Master ID — fixed 0x02"],
    ["3", "sid", "Slave ID: 0x81–0x84, or 0xFF (broadcast)"],
    ["4", "subcmd", "Sub-command (0x00=echo, 0xAA=poll, 0x11=write, 0x35=read)"],
    ["5", "addr", "Register address (0=time/HH:MM:SS, 1=date, 4=relay/IO)"],
    ["6–11", "data[0..5]", "Payload bytes — content depends on cmd/subcmd/addr"],
]
story.append(tbl(proto_data, [18*mm, 22*mm, 130*mm]))

story.append(Spacer(1, 3*mm))
story.append(Paragraph("Command Codes", h2_style))
cmd_data = [
    ["hdr (hex)", "Direction", "Meaning"],
    ["0x55", "MST → SLV (RF)", "Beacon (sid=0xFF) or Poll request (sid=target)"],
    ["0x69", "Host → MST (UART)", "Write master register (time, date)"],
    ["0x78", "Host → MST (UART)", "Write slave register via master relay (e.g. relay control)"],
    ["0xAA", "SLV → MST (RF)", "Slave acknowledge / data report"],
    ["0x89", "SLV → MST (RF)", "Slave write acknowledgement"],
    ["0xAA", "MST → Host (UART)", "Echo / acknowledgement response"],
    ["0x7A", "MST → Host (UART)", "Master register read-back response"],
]
story.append(tbl(cmd_data, [25*mm, 45*mm, 100*mm]))

story.append(Spacer(1, 3*mm))
story.append(Paragraph("Auto-Poll Slave List", h2_style))
story.append(Paragraph(
    "The master cycles through up to 4 slave IDs: <b>0x81, 0x82, 0x83, 0x84</b>. "
    "Every 2 seconds it polls the next slave in the list, wrapping at "
    "<code>gPARAM_APOLL_cnt_lmt</code> (default 3). Slave responses are parsed by "
    "<code>mst_rf_cmd_process()</code> and displayed on the LCD.",
    body_style))

# ── Files ─────────────────────────────────────────────────────────────────────
story.append(Paragraph("5. Source File Structure", h1_style))
files_data = [
    ["File", "Language", "Purpose"],
    ["pico_mst_260219.cpp", "C++", "Entry point: hardware init, ISRs, event-loop state machine"],
    ["global_def.h", "C/C++", "GPIO pin defines, packed structs (TimeData, BcnData, PollData, slv_Data, dec3/5Dig)"],
    ["main.h", "C/C++", "Global variable definitions (all buffers, counters, auto-poll config)"],
    ["mst_ur_proc.cpp / .h", "C++", "UART command parser and packet composers (beacon, poll, write relay, responses)"],
    ["mst_rf_cmd_proc.cpp / .h", "C++", "RF receive parser; UART response composer; LCD display buffer fill"],
    ["usr_lib/i2c_lcd.cpp / .h", "C++", "I2C LCD driver (16×4 character display)"],
    ["usr_lib/ds3231.c / .h", "C", "DS3231 RTC driver (time read/write, EEPROM)"],
    ["RadioLib/", "C++", "Third-party SX126x radio library (vendored, Sep 2025)"],
    ["CMakeLists.txt", "CMake", "Build definition: SDK 2.2.0, toolchain 14.2, RADIOLIB_BUILD_RPI_PICO"],
    ["pico_sdk_import.cmake", "CMake", "Pico SDK import helper"],
]
story.append(tbl(files_data, [52*mm, 18*mm, 100*mm]))

# ── Build ─────────────────────────────────────────────────────────────────────
story.append(Paragraph("6. Build Configuration", h1_style))
build_data = [
    ["Setting", "Value"],
    ["Target board", "pico (RP2040)"],
    ["Pico SDK version", "2.2.0"],
    ["Toolchain", "ARM GCC 14.2 Rel1"],
    ["C standard", "C11"],
    ["C++ standard", "C++17"],
    ["stdio via USB", "Enabled (pico_enable_stdio_usb)"],
    ["stdio via UART", "Disabled"],
    ["RadioLib compile def", "RADIOLIB_BUILD_RPI_PICO"],
    ["Linked libraries", "pico_stdlib, hardware_spi, hardware_i2c, hardware_timer, hardware_uart, hardware_pwm, pico_multicore, RadioLib"],
]
story.append(tbl(build_data, [60*mm, 110*mm]))

# ── Bugs ──────────────────────────────────────────────────────────────────────
story.append(Paragraph("7. Known Issues & Bugs", h1_style))
story.append(Paragraph(
    "The following defects were identified during code analysis:", body_style))

bugs = [
    ("<b>[BUG — compile error]</b> <code>pico_mst_260219.cpp:106</code> — "
     "<code>conv4byte_4decdig()</code> references undefined variable <code>intval</code>; "
     "should be the parameter <code>inval</code>. The file will not compile."),
    ("<b>[BUG — logic error]</b> <code>pico_mst_260219.cpp:290</code> — "
     "<code>if(gPARAM_APOLL_en = 1)</code> uses assignment instead of comparison (<code>==</code>). "
     "Auto-poll is always enabled regardless of the variable's value."),
    ("<b>[BUG — wrong field]</b> <code>mst_ur_proc.cpp:139</code> — "
     "<code>mst_ur_rsp_write_mst_a1()</code> writes <code>Stru_curTime.day</code> to both "
     "byte 8 and byte 9. Byte 9 should carry <code>Stru_curTime.dow</code> (day-of-week)."),
    ("<b>[BUG — unconditional display fill]</b> <code>mst_rf_cmd_proc.cpp:46–55</code> — "
     "The LCD display buffer is always filled from <code>inbuf</code> even when the RF packet "
     "failed to parse. Garbage data may appear on the LCD after a bad receive."),
    ("<b>[BUG — auto-poll counter wrap]</b> <code>pico_mst_260219.cpp:292–296</code> — "
     "When <code>cnt >= lmt</code>, the counter is reset to 0 without incrementing, so the "
     "same slave (index 0) is polled twice in a row after each wrap."),
    ("<b>[INFO — inefficient math]</b> <code>pico_mst_260219.cpp:167–226</code> — "
     "<code>conv1byte_3dig()</code> performs decimal digit extraction by repeated subtraction "
     "instead of using <code>%</code> and <code>/</code> operators. Functionally correct but slow."),
]
for b in bugs:
    story.append(Paragraph(b, note_style))
    story.append(Spacer(1, 2*mm))

# ── Footer ────────────────────────────────────────────────────────────────────
story.append(HRFlowable(width=W, thickness=0.5, color=colors.HexColor("#aaaaaa"), spaceBefore=10))
story.append(Paragraph(
    "Generated by Claude Code — 2026-04-22",
    ParagraphStyle("Footer", parent=styles["Normal"], fontSize=8, textColor=colors.grey, alignment=TA_CENTER)))

doc.build(story)
print(f"PDF written to {OUTPUT}")
