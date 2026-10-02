"""Normalize the self-contained PCB's footprints into a local KiCad library.

Run with the Python environment that provides pcbnew (KiCad 9).
"""

from pathlib import Path
import pcbnew


root = Path(__file__).resolve().parent
board_path = root / "sensor_carrier.kicad_pcb"
library = root / "SensorCarrier.pretty"
board = pcbnew.LoadBoard(str(board_path))
library.mkdir(exist_ok=True)
plugin = pcbnew.PCB_IO_MGR.PluginFind(pcbnew.PCB_IO_MGR.KICAD_SEXP)
saved = set()
for footprint in board.GetFootprints():
    name = "Header_1x04" if footprint.GetReference().startswith("J") else "Pullup_0805"
    footprint.SetFPIDAsString(f"SensorCarrier:{name}")
    if name not in saved:
        plugin.FootprintSave(str(library), footprint)
        saved.add(name)
pcbnew.SaveBoard(str(board_path), board)
print(f"Saved {len(saved)} local footprints and updated board IDs")
