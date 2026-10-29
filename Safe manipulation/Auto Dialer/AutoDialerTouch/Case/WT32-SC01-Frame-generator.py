"""Generate a correct bezel frame for the 92x60 screen, in MODEL coords (mates base_new).
Screen bolts to the base with 4 M3 into its OWN ears; this bezel is clamped by those
screws via 4 collars behind the screen, and wraps the screen front (window) + sides."""
import numpy as np
from build123d import *
OUT=r"C:\scratchpad"

# ---- geometry in MODEL coords (base front face at y=12, frame at y<12) ----
FACE_Y=12.0
SX=[-35.66, 39.48]                 # screw x (matches base, asymmetric)
SZ=[-26.995, 24.395]               # screw z (= s-31.3 for s=4.305,55.695)
ZC=-1.3                            # screen/face centre in z
SCR_HZ=30.0                        # screen half-height (60)
COLLAR_H=2.0                       # screen stands this far off the base face
SCR_BACK=FACE_Y-COLLAR_H           # 10.0  screen back plane
SCR_T=10.8
SCR_FRONT=SCR_BACK-SCR_T           # -0.8
PLATE_T=2.0
PY1=SCR_FRONT                      # -0.8 plate back
PY0=PY1-PLATE_T                    # -2.8 plate front
AW,AH=74.04,49.56                  # active area
def yprism(hx,hz,zc,y0,y1,r=0.1):
    sk=Plane.XZ*Pos(0,zc)*RectangleRounded(2*hx,2*hz,r)
    return extrude(sk, amount=(y1-y0), dir=(0,1,0)).translate((0,y0,0))

# outer solid — extended 2mm ABOVE the screen at the top to fit a top wall; bottom stays flush
TOP_EXT=2.6
TOPZ_F=ZC+SCR_HZ+TOP_EXT           # 31.3  frame top
BOTZ_F=ZC-SCR_HZ                   # -31.3 frame bottom (flush with screen)
frame = yprism(47.8, (TOPZ_F-BOTZ_F)/2, (TOPZ_F+BOTZ_F)/2, PY0, FACE_Y, r=4)
# hollow the screen pocket: OPEN at the bottom, CLOSED at the top. Pocket top is 0.4mm ABOVE the
# screen top so the screen slides up with clearance and seats under a 2.2mm top wall/cap.
PTOP=ZC+SCR_HZ+0.4                 # 29.1 (0.4mm gap over screen top)
PBOT=BOTZ_F-1.5                    # -32.8 (through the open bottom)
frame -= yprism(46.2, (PTOP-PBOT)/2, (PTOP+PBOT)/2, PY1, FACE_Y+0.1, r=3)
# window through the front plate (active area + 0.75 margin)
frame -= yprism(AW/2+0.75, AH/2+0.75, ZC, PY0-0.2, PY1+0.01, r=3)

# 4 collars + bracing. TOP collars brace to BOTH the side wall and the top cap; BOTTOM collars
# get a curved tapered gusset (cove profile) that flares out of the side wall (no bottom wall).
TOPWALL_Z=PTOP                         # 29.1 underside of the top cap
def curved_gusset(x, z, wx):
    ztop=z+6.0; zbot=max(z-6.0, BOTZ_F+0.8)
    A=(x, z+2.6); B=(wx, ztop); C=(wx, zbot); D=(x, z-2.6); mx=(x+wx)/2
    try:
        with BuildSketch(Plane.XZ) as sk:
            with BuildLine():
                Line(D, A); ThreePointArc(A, (mx, z+2.6), B)
                Line(B, C); ThreePointArc(C, (mx, z-2.6), D)
            make_face()
        return extrude(sk.sketch, amount=COLLAR_H, dir=(0,1,0)).translate((0,FACE_Y-COLLAR_H,0))
    except Exception as ex:
        print("curved gusset failed -> linear taper:",ex)
        tri=Plane.XZ*Polygon(B,C,(x,z),align=None)
        return extrude(tri, amount=COLLAR_H, dir=(0,1,0)).translate((0,FACE_Y-COLLAR_H,0))
for x in SX:
    xs=1 if x>0 else -1; wx=xs*46.8
    for z in SZ:
        frame += Pos(x,FACE_Y,z)*Rot(90,0,0)*Cylinder(radius=2.6,height=COLLAR_H,
                  align=(Align.CENTER,Align.CENTER,Align.MIN))          # collar 10..12
        if z>0:   # TOP: horizontal rib to side wall + vertical bridge up to the top cap
            x0,x1=sorted((x, wx))
            frame += Pos((x0+x1)/2, FACE_Y-COLLAR_H/2, z)*Box(abs(x1-x0), COLLAR_H, 4.0)
            zb0=z-2.6; zb1=TOPWALL_Z+0.2
            frame += Pos(x, FACE_Y-COLLAR_H/2, (zb0+zb1)/2)*Box(5.2, COLLAR_H, zb1-zb0)
        else:     # BOTTOM: curved taper gusset flaring out of the side wall
            frame += curved_gusset(x, z, wx)
# clearance holes through collars (screw passes into the SCREEN ear)
for x in SX:
    for z in SZ:
        frame -= Pos(x,FACE_Y+1,z)*Rot(90,0,0)*Cylinder(radius=1.75,height=6,
                  align=(Align.CENTER,Align.CENTER,Align.MIN))

# chamfer the raised top-front edge of the cap so it blends toward the roofline
CH=2.2
try:
    top=frame.edges().filter_by(Axis.X).group_by(Axis.Z)[-1]  # edges at max Z (top), parallel X
    top_front=top.group_by(Axis.Y)[0]                          # frontmost (min Y)
    frame=chamfer(top_front, length=CH)
    print("chamfered top-front edge via edge-select")
except Exception as ex:
    print("edge chamfer failed -> wedge cut:",ex)
    from build123d import Polygon as _Poly
    tri=Plane.YZ*_Poly((PY0,TOPZ_F),(PY0+CH,TOPZ_F),(PY0,TOPZ_F-CH),align=None)
    frame -= extrude(tri, amount=48, both=True)

export_stl(frame, OUT+r"\frame_new.stl")
export_step(frame, OUT+r"\frame_new.step")
bb=frame.bounding_box()
print("frame bbox",[round(v,1) for v in tuple(bb.min)+tuple(bb.max)],"vol",round(frame.volume,0))

# mock screen (for validation/rendering): 92x60x10.8 with a recessed active area
screen = Pos(0,(SCR_FRONT+SCR_BACK)/2,ZC)*Box(92,SCR_T,60)
screen -= Pos(0,SCR_FRONT+0.4,ZC)*Box(AW,1.0,AH)     # active-area recess (visual)
export_stl(screen, OUT+r"\mock_screen.stl")
print("done")
