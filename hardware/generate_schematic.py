"""Generate the editable KiCad 9 schematic and project-local symbol library.

The generated schematic matches the J1/J2/R1/R2 nets in the PCB. This script
uses only Python's standard library; KiCad validates the resulting files.
"""

from pathlib import Path
from uuid import NAMESPACE_URL, uuid5

ROOT = Path(__file__).resolve().parent
PROJECT = "sensor_carrier"


def uid(name: str) -> str:
    return str(uuid5(NAMESPACE_URL, f"sensor-carrier/{name}"))


def text_effects(size: float = 1.27) -> str:
    return f"(effects (font (size {size} {size})))"


def prop(name: str, value: str, x: float, y: float, hidden: bool = False) -> str:
    effects = text_effects()[:-1] + (" (hide yes))" if hidden else ")")
    return f'(property "{name}" "{value}" (at {x} {y} 0) {effects})'


def pin(number: int, name: str, x: float, y: float, angle: int) -> str:
    return (f'(pin passive line (at {x} {y} {angle}) (length 5.08) '
            f'(name "{name}" {text_effects()}) '
            f'(number "{number}" {text_effects()}))')


def symbol_def(name: str, connector_side: str = "", cached: bool = False) -> str:
    library_name = f"SensorCarrier:{name}" if cached else name
    prefix = f'(symbol "{library_name}" (in_bom yes) (on_board yes) '
    if connector_side:
        x = 7.62 if connector_side == "left" else -7.62
        angle = 180 if connector_side == "left" else 0
        pins = " ".join(pin(i + 1, net, x, 3.81 - i * 2.54, angle)
                        for i, net in enumerate(("3V3", "GND", "SDA", "SCL")))
        geometry = ('(rectangle (start -2.54 5.08) (end 2.54 -5.08) '
                    '(stroke (width 0.254) (type default)) (fill (type background)))')
        footprint = "SensorCarrier:Header_1x04"
        reference = "J"
    else:
        pins = (pin(1, "~", -7.62, 0, 0) + " " + pin(2, "~", 7.62, 0, 180))
        geometry = ('(rectangle (start -2.54 1.27) (end 2.54 -1.27) '
                    '(stroke (width 0.254) (type default)) (fill (type background)))')
        footprint = "SensorCarrier:Pullup_0805"
        reference = "R"
    return (prefix + prop("Reference", reference, 0, 7.62 if connector_side else 3.81)
            + prop("Value", name, 0, -7.62 if connector_side else -3.81)
            + prop("Footprint", footprint, 0, 0, hidden=True)
            + f'(symbol "{name}_1_1" {geometry} {pins}))')


def wire(x1: float, y1: float, x2: float, y2: float, name: str) -> str:
    return (f'(wire (pts (xy {x1} {y1}) (xy {x2} {y2})) '
            f'(stroke (width 0) (type default)) (uuid "{uid(name)}"))')


def label(name: str, x: float, y: float, tag: str) -> str:
    return (f'(label "{name}" (at {x} {y} 0) '
            f'(effects (font (size 1.27 1.27)) (justify left bottom)) '
            f'(uuid "{uid(tag)}"))')


def instance(name: str, ref: str, value: str, x: float, y: float,
             pin_count: int) -> str:
    footprint = "Header_1x04" if ref.startswith("J") else "Pullup_0805"
    pins = " ".join(f'(pin "{i}" (uuid "{uid(ref + "-pin-" + str(i))}"))'
                    for i in range(1, pin_count + 1))
    return (f'(symbol (lib_id "SensorCarrier:{name}") (at {x} {y} 0) '
            f'(unit 1) (in_bom yes) (on_board yes) (dnp no) '
            f'(uuid "{uid(ref)}") '
            + prop("Reference", ref, x, y - (8 if ref.startswith("J") else 4))
            + prop("Value", value, x, y + (8 if ref.startswith("J") else 4))
            + prop("Footprint", f"SensorCarrier:{footprint}", x, y, hidden=True)
            + pins
            + f'(instances (project "{PROJECT}" (path "/{uid("root")}" '
              f'(reference "{ref}") (unit 1)))))')


definitions = [
    ("ConnectorLeft", "left"),
    ("ConnectorRight", "right"),
    ("Resistor", ""),
]
symbol_library = ('(kicad_symbol_lib (version 20231120) (generator "codex") '
                  + " ".join(symbol_def(name, side) for name, side in definitions)
                  + ")\n")
(ROOT / "SensorCarrier.kicad_sym").write_text(symbol_library, encoding="utf-8")
(ROOT / "sym-lib-table").write_text(
    '(sym_lib_table (lib (name "SensorCarrier") (type "KiCad") '
    '(uri "${KIPRJMOD}/SensorCarrier.kicad_sym") (options "") '
    '(descr "Project-local sensor carrier symbols")))\n', encoding="utf-8")

parts = [
    '(kicad_sch (version 20250114) (generator "codex") '
    f'(uuid "{uid("root")}") (paper "A4") ',
    '(lib_symbols ' + " ".join(symbol_def(name, side, cached=True)
                             for name, side in definitions) + ')',
]

for index, net in enumerate(("3V3", "GND", "SDA", "SCL")):
    y = round(72.39 + 2.54 * index, 2)
    parts += [wire(71.12, y, 101.6, y, f"j1-{net}"),
              wire(101.6, y, 132.08, y, f"j2-{net}"),
              label(net, 101.6, y, f"bus-label-{net}")]

for ref, y, destination in (("R1", 101.6, "SDA"), ("R2", 116.84, "SCL")):
    parts += [wire(93.98, y, 86.36, y, f"{ref}-left"),
              wire(109.22, y, 116.84, y, f"{ref}-right"),
              label("3V3", 86.36, y, f"{ref}-supply"),
              label(destination, 116.84, y, f"{ref}-signal")]

parts += [
    instance("ConnectorLeft", "J1", "ESP32_HOST", 63.5, 76.2, 4),
    instance("ConnectorRight", "J2", "SHT31_BREAKOUT", 139.7, 76.2, 4),
    instance("Resistor", "R1", "4.7k", 101.6, 101.6, 2),
    instance("Resistor", "R2", "4.7k", 101.6, 116.84, 2),
    '(sheet_instances (path "/" (page "1")))',
    ')\n',
]
(ROOT / "sensor_carrier.kicad_sch").write_text("\n".join(parts), encoding="utf-8")
print("Generated KiCad schematic and project-local symbol library")
