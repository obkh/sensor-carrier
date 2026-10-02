"""Align PCB net/value metadata with the generated KiCad schematic."""

from pathlib import Path
import pcbnew

board_path = Path(__file__).resolve().parent / "sensor_carrier.kicad_pcb"
board = pcbnew.LoadBoard(str(board_path))
for name in ("3V3", "GND", "SDA", "SCL"):
    net = board.FindNet(name)
    if net is not None:
        net.SetNetname(f"/{name}")
for footprint in board.GetFootprints():
    if footprint.GetReference() == "J1":
        footprint.SetValue("ESP32_HOST")
    elif footprint.GetReference() == "J2":
        footprint.SetValue("SHT31_BREAKOUT")
pcbnew.SaveBoard(str(board_path), board)
print("PCB nets and values match the schematic")
