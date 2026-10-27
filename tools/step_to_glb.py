"""Converts Waveshare's ESP32-S3-Zero STEP model to the GLB the settings page shows.

The STEP file is resources/ESP32-S3-Zero_v2-*.stp on the ws-esp32s3-zero branch.
Needs the OpenCASCADE Python bindings (OCP), which aren't in the standard library:

    uv venv --python 3.12 /tmp/ocp && uv pip install --python /tmp/ocp/bin/python cadquery-ocp
    /tmp/ocp/bin/python -I tools/step_to_glb.py path/to/ESP32-S3-Zero_v2.stp models/esp32-s3-zero.glb

Parts are recoloured to look like the real board (the STEP's colours are CAD
defaults) and merged by material, except for the nodes the page animates:
  boot  — the BOOT button's plunger (pressed down by the animation)
  led   — the WS2812B RGB LED (its material is recoloured live)
Units are millimetres, Y up, the board centred on the origin with the USB-C
connector towards -Z.
"""
import json
import struct
import sys
from array import array

from OCP.BRep import BRep_Tool
from OCP.BRepAdaptor import BRepAdaptor_Surface
from OCP.BRepLib import BRepLib_ToolTriangulatedShape
from OCP.BRepMesh import BRepMesh_IncrementalMesh
from OCP.BRepPrimAPI import BRepPrimAPI_MakeBox
from OCP.gp import gp_Pnt
from OCP.Bnd import Bnd_Box
from OCP.BRepBndLib import BRepBndLib
from OCP.GeomAbs import GeomAbs_Cylinder, GeomAbs_Plane
from OCP.IFSelect import IFSelect_RetDone
from OCP.STEPCAFControl import STEPCAFControl_Reader
from OCP.TCollection import TCollection_ExtendedString
from OCP.TDataStd import TDataStd_Name
from OCP.TDocStd import TDocStd_Document
from OCP.TopAbs import TopAbs_FACE, TopAbs_REVERSED
from OCP.TopExp import TopExp_Explorer
from OCP.TopLoc import TopLoc_Location
from OCP.TopoDS import TopoDS
from OCP.XCAFDoc import XCAFDoc_DocumentTool
from OCP.XCAFPrs import XCAFPrs_DocumentExplorer, XCAFPrs_DocumentExplorerFlags_OnlyLeafNodes

# name → (base colour sRGB hex, metallic, roughness)
MATERIALS = {
    'pcb':     ('#1d4596', 0.0, 0.55),   # Waveshare blue solder mask
    'gold':    ('#d9a441', 1.0, 0.30),   # castellated pads (ENIG)
    'steel':   ('#c8ccd2', 1.0, 0.35),   # USB-C shell, button frames, cap terminals
    'black':   ('#1a1b1e', 0.0, 0.45),   # ESP32-S3, inductors, resistors
    'ceramic': ('#b39064', 0.0, 0.60),   # capacitor bodies
    'antenna': ('#b8352a', 0.0, 0.50),   # chip antenna
    'plunger': ('#26272b', 0.0, 0.60),   # button plungers
    'led':     ('#f2f2ee', 0.0, 0.30),   # WS2812B body: the page drives its emissive colour
}

DEFLECTION = 0.02    # mm
ANGLE = 0.35         # rad
BOARD_CENTRE = (9.0, 11.75)   # STEP x, y of the PCB's centre
BOOT_MAX_X = 9.0              # the BOOT button is the left one (x < 9), RESET the right
PLUNGER_MIN_Z = 1.3           # plunger faces start on top of the button body
SMALL_PART_MM = 1.0           # parts smaller than this are drawn as their bounding box


def label_name(label):
    attr = TDataStd_Name()
    return attr.Get().ToExtString() if label.FindAttribute(TDataStd_Name.GetID_s(), attr) else ''


def bounds(shape):
    box = Bnd_Box()
    BRepBndLib.Add_s(shape, box)
    lo, hi = box.CornerMin(), box.CornerMax()
    return (lo.X(), lo.Y(), lo.Z()), (hi.X(), hi.Y(), hi.Z())


def classify(part, face):
    """(node, material) for one face of a STEP part."""
    surface = BRepAdaptor_Surface(face).GetType()
    (x0, _, z0), (x1, _, _) = bounds(face)
    if part == 'BOARD':
        return 'board', 'gold' if surface == GeomAbs_Cylinder else 'pcb'
    if part.startswith('TYPE-C'):
        return 'board', 'steel'
    if part.startswith('BUTTON'):
        if z0 < PLUNGER_MIN_Z - 1e-3 or (surface == GeomAbs_Plane and z0 < PLUNGER_MIN_Z + 1e-3):
            return 'board', 'steel'
        return ('boot' if x1 < BOOT_MAX_X else 'board'), 'plunger'
    if part == 'EXTRUDED-59609':
        return 'led', 'led'
    if part == 'C3':
        return 'board', 'antenna'
    if part == '_1_10001':
        return 'board', 'ceramic'
    if part in ('3_1_A', '2_100'):
        return 'board', 'steel'
    return 'board', 'black'


def to_scene(p):
    """STEP point (mm, Z up) → scene (mm, Y up, board centred, USB-C towards -Z)."""
    return p[0] - BOARD_CENTRE[0], p[2], -(p[1] - BOARD_CENTRE[1])


def direction_to_scene(d):
    """STEP direction → scene: the same rotation as to_scene(), without the centring."""
    return d[0], d[2], -d[1]


class Primitive:
    def __init__(self):
        self.positions = array('f')
        self.normals = array('f')
        self.indices = array('I')

    def add_face(self, face):
        loc = TopLoc_Location()
        tri = BRep_Tool.Triangulation_s(face, loc)
        if tri is None:
            return
        if not tri.HasNormals():
            BRepLib_ToolTriangulatedShape.ComputeNormals_s(face, tri)
        trsf = loc.Transformation()
        flip = face.Orientation() == TopAbs_REVERSED
        base = len(self.positions) // 3
        for i in range(1, tri.NbNodes() + 1):
            p = tri.Node(i).Transformed(trsf)
            n = tri.Normal(i).Transformed(trsf)
            if flip:
                n.Reverse()
            self.positions.extend(to_scene((p.X(), p.Y(), p.Z())))
            self.normals.extend(direction_to_scene((n.X(), n.Y(), n.Z())))
        for i in range(1, tri.NbTriangles() + 1):
            a, b, c = tri.Triangle(i).Get()
            if flip:
                b, c = c, b
            self.indices.extend((base + a - 1, base + b - 1, base + c - 1))


def read_parts(step_path):
    doc = TDocStd_Document(TCollection_ExtendedString('XmlOcaf'))
    reader = STEPCAFControl_Reader()
    reader.SetNameMode(True)
    if reader.ReadFile(step_path) != IFSelect_RetDone or not reader.Transfer(doc):
        sys.exit(f'Could not read {step_path} as STEP')
    shapes = XCAFDoc_DocumentTool.ShapeTool_s(doc.Main())
    explorer = XCAFPrs_DocumentExplorer(doc, XCAFPrs_DocumentExplorerFlags_OnlyLeafNodes)
    while explorer.More():
        node = explorer.Current()
        yield label_name(node.RefLabel), shapes.GetShape_s(node.RefLabel).Moved(node.Location)
        explorer.Next()


def build(step_path):
    nodes = {}   # node → material → Primitive
    parts = 0
    for part, shape in read_parts(step_path):
        parts += 1
        # 0402 capacitors are modelled down to their fillets: thousands of triangles
        # for half a millimetre, invisible at this scale
        lo, hi = bounds(shape)
        if max(h - l for l, h in zip(lo, hi)) < SMALL_PART_MM:
            shape = BRepPrimAPI_MakeBox(gp_Pnt(*lo), gp_Pnt(*hi)).Shape()
        BRepMesh_IncrementalMesh(shape, DEFLECTION, False, ANGLE, True)
        faces = TopExp_Explorer(shape, TopAbs_FACE)
        while faces.More():
            face = TopoDS.Face(faces.Current())
            node, material = classify(part, face)
            nodes.setdefault(node, {}).setdefault(material, Primitive()).add_face(face)
            faces.Next()
    for required in ('board', 'boot', 'led'):
        if required not in nodes:
            sys.exit(f'No "{required}" geometry found: is this the ESP32-S3-Zero STEP model?')
    print(f'{parts} parts')
    return nodes


def srgb_to_linear(hex_colour):
    c = [int(hex_colour[i:i + 2], 16) / 255 for i in (1, 3, 5)]
    return [x / 12.92 if x <= 0.04045 else ((x + 0.055) / 1.055) ** 2.4 for x in c]


def write_glb(nodes, out_path):
    blob = bytearray()
    views, accessors, meshes, gltf_nodes = [], [], [], []
    material_names = list(MATERIALS)

    def add_view(data, target):
        while len(blob) % 4:
            blob.append(0)
        views.append({'buffer': 0, 'byteOffset': len(blob), 'byteLength': len(data), 'target': target})
        blob.extend(data)
        return len(views) - 1

    def add_accessor(values, kind, component, count, **extra):
        target = 34963 if kind == 'SCALAR' else 34962
        accessors.append({'bufferView': add_view(values.tobytes(), target), 'componentType': component,
                          'count': count, 'type': kind, **extra})
        return len(accessors) - 1

    triangles = 0
    for node, primitives in nodes.items():
        gltf_primitives = []
        for material, prim in primitives.items():
            xs, ys, zs = prim.positions[0::3], prim.positions[1::3], prim.positions[2::3]
            count = len(prim.positions) // 3
            pos = add_accessor(prim.positions, 'VEC3', 5126, count,
                               min=[min(xs), min(ys), min(zs)], max=[max(xs), max(ys), max(zs)])
            nrm = add_accessor(prim.normals, 'VEC3', 5126, count)
            small = count < 65536
            idx_values = array('H', prim.indices) if small else prim.indices
            idx = add_accessor(idx_values, 'SCALAR', 5123 if small else 5125, len(prim.indices))
            gltf_primitives.append({'attributes': {'POSITION': pos, 'NORMAL': nrm}, 'indices': idx,
                                    'material': material_names.index(material)})
            triangles += len(prim.indices) // 3
        meshes.append({'name': node, 'primitives': gltf_primitives})
        gltf_nodes.append({'name': node, 'mesh': len(meshes) - 1})

    materials = [{'name': name, 'pbrMetallicRoughness': {
        'baseColorFactor': srgb_to_linear(colour) + [1.0], 'metallicFactor': metallic, 'roughnessFactor': rough}}
        for name, (colour, metallic, rough) in MATERIALS.items()]

    gltf = {
        'asset': {'version': '2.0', 'generator': 'step_to_glb.py',
                  'copyright': 'ESP32-S3-Zero model: Waveshare'},
        'scene': 0, 'scenes': [{'nodes': list(range(len(gltf_nodes)))}],
        'nodes': gltf_nodes, 'meshes': meshes, 'materials': materials,
        'accessors': accessors, 'bufferViews': views, 'buffers': [{'byteLength': len(blob)}],
    }
    while len(blob) % 4:
        blob.append(0)
    js = json.dumps(gltf, separators=(',', ':')).encode()
    js += b' ' * (-len(js) % 4)
    with open(out_path, 'wb') as f:
        f.write(struct.pack('<III', 0x46546C67, 2, 12 + 8 + len(js) + 8 + len(blob)))
        f.write(struct.pack('<II', len(js), 0x4E4F534A) + js)
        f.write(struct.pack('<II', len(blob), 0x004E4942) + blob)
    print(f'{triangles} triangles, {12 + 16 + len(js) + len(blob)} bytes → {out_path}')


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    write_glb(build(sys.argv[1]), sys.argv[2])
