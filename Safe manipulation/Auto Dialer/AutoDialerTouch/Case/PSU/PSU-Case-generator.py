"""PSU enclosure for the Auto Dialer: holds an Arcity 160x98.5x39 arcade PSU + a
screw-mount fused/switched IEC inlet. Removable honeycomb-vented lid, cable notch to the lid.
Axis-aligned box. X=length, Y=width(depth), Z=up. Floor top at z=0."""
import numpy as np
from build123d import *
OUT=r"C:\scratchpad"

# ---------- parameters ----------
PL,PW,PH = 160.0, 98.5, 39.0      # PSU L x W x H
SH = 4.0                          # standoff height (airflow under PSU)
FA = 22.0                         # front access clearance (terminal block side, +Y)
BC = 6.0                          # back clearance (-Y)
IG = 32.0                         # inlet-end gap (-X)  (inlet body ~30mm deep)
EC = 44.0                         # +X end: 6 base + 38 (1.5") spade/wire access. Terminal block faces +X.
TC = 16.0                         # clearance above PSU (top terminals/wires)
WT, FT = 2.5, 2.5                 # wall / floor thickness
IX = IG+PL+EC                     # internal length 198
IY = BC+PW+FA                     # internal width  126.5
IZ = SH+PH+TC                     # internal height  59
EX, EY = IX+2*WT, IY+2*WT
# PSU footprint in internal coords (centred box): X[-IX/2..],Y[-IY/2..]
PX0 = -IX/2+IG; PX1 = PX0+PL      # -67 .. 93
PY0 = -IY/2+BC; PY1 = PY0+PW      # -57.25 .. 41.25  (front edge PY1)
# PSU mount holes: 5mm from short ends, 6mm from long edges -> diagonal pair
SOCK_INSET = 4.0                  # socket-end posts moved 1mm OUT (long-way spacing 150 -> 151)
FAR_INSET  = 5.0
HOLES = [(PX0+SOCK_INSET, PY0+6), (PX1-FAR_INSET, PY1-6)]      # screwed
SUPPORTS = [(PX0+SOCK_INSET, PY1-6), (PX1-FAR_INSET, PY0+6)]   # plain support pads
# inlet (screw-mount) on -X wall
IN_CUT = (47.1+0.6, 27.6+0.6)     # cutout Y x Z (+clearance)
IN_SCREW = 40.0                   # screw spacing (vertical, along Z)
IN_ZC = 26.0                       # inlet centre height (top screw stays clear of the lid-ledge ramp)
# lid rabbet
LID_T=3.0; LEDGE_W=4.0; LEDGE_H=3.0
LZ = IZ-LID_T                      # lid rests here (flange top)

# ---------- body: floor + 4 walls, open top ----------
part = Pos(0,0,(IZ-FT)/2)*Box(EX,EY,IZ+FT)             # z from -FT to IZ
part -= Pos(0,0,(IZ+2)/2)*Box(IX,IY,IZ+3)             # cavity, open top (z 0..IZ+)

# ---------- PSU standoffs + corner guides (all overlap into the floor so they fuse) ----------
for (hx,hy) in HOLES:
    part += Pos(hx,hy,-2)*Cylinder(radius=4,height=SH+2,align=(Align.CENTER,Align.CENTER,Align.MIN))   # z -2..SH
    part -= Pos(hx,hy,SH+0.1)*Cylinder(radius=1.25,height=SH,align=(Align.CENTER,Align.CENTER,Align.MAX))  # M3 self-tap, from top
for (hx,hy) in SUPPORTS:
    part += Pos(hx,hy,-2)*Cylinder(radius=4,height=SH+2,align=(Align.CENTER,Align.CENTER,Align.MIN))
# (PSU is fully located + held by the 2 diagonal screws + lid; no separate corner guides needed)

# (inlet cutout + screw holes are cut LAST, after the flange/ramp, so nothing backfills them)

# ---------- honeycomb vent helper ----------
def hex_centers(uext, vext, af=9.0, rib=2.6):
    dx=af+rib; dy=dx*np.sqrt(3)/2; cs=[]
    jmax=int(vext/2/dy)+1; imax=int(uext/2/dx)+2
    for j in range(-jmax,jmax+1):
        vv=j*dy
        if abs(vv)>vext/2-af*0.4: continue
        xoff=(dx/2) if (j%2) else 0.0
        for i in range(-imax,imax+1):
            uu=i*dx+xoff
            if abs(uu)>uext/2-af*0.4: continue
            cs.append((uu,vv))
    return cs
def hc_sketch(centers, af=9.0):
    R=af/np.sqrt(3); s=None
    for u,v in centers:
        f=Pos(u,v)*RegularPolygon(R,6,rotation=30)
        s=f if s is None else s+f
    return s
def vent(plane, centers, at, thru=WT*3, af=9.0):
    prisms=extrude(plane*hc_sketch(centers,af), amount=thru, both=True).translate(at)
    return prisms

# back wall (-Y), front wall (+Y, avoid notch), +X end, floor, lid
VENT_ZC, VENT_H = 28.0, 30.0      # z13..43: stays BELOW the lid-ledge ramp (bottom z46) so the
                                  # ramp (added later) can never backfill a vent hole
part -= vent(Plane.XZ, hex_centers(IX-30, VENT_H), (0,-EY/2, VENT_ZC))             # back
part -= vent(Plane.XZ, hex_centers(IX-70, VENT_H), (-15,  EY/2, VENT_ZC))          # front (offset -X, clear of notch)
part -= vent(Plane.YZ, hex_centers(IY-28, VENT_H), ( EX/2, 0, VENT_ZC))            # +X end
_fc=[c for c in hex_centers(IX-60, IY-40)
     if all((c[0]-sx)**2+(c[1]-sy)**2 > 10.0**2 for sx,sy in HOLES+SUPPORTS)]      # keep floor solid at standoffs
part -= vent(Plane.XY, _fc, (0,0,-FT/2), thru=FT*3)                                # floor (central)

# ---------- top rabbet flange (lid seat) + corner bosses ----------
def frame(outer_hx,outer_hy,inner_hx,inner_hy,z0,z1):
    return (Pos(0,0,(z0+z1)/2)*Box(2*outer_hx,2*outer_hy,z1-z0)
            - Pos(0,0,(z0+z1)/2)*Box(2*inner_hx,2*inner_hy,z1-z0+2))
part += frame(IX/2+2,IY/2+2, IX/2-LEDGE_W,IY/2-LEDGE_W, LZ-LEDGE_H, LZ)   # outer embeds into walls
# 45deg support ramp under the flange inner edge -> self-supporting (no print supports)
zb=LZ-LEDGE_H
# ramp embeds +1 into flange (z) and +1 into wall (x/y) so it FUSES (not just touches).
# drop 8 over run 5 => ~58deg from horizontal, comfortably steeper than the 45deg print limit
RAMP_DROP=8.0
for s in (1,-1):
    part += extrude(Plane.XZ*Polygon((s*(IX/2-LEDGE_W),zb+1),(s*(IX/2+1),zb+1),(s*(IX/2+1),zb+1-RAMP_DROP),align=None),
                    amount=IY/2+2, both=True)
    part += extrude(Plane.YZ*Polygon((s*(IY/2-LEDGE_W),zb+1),(s*(IY/2+1),zb+1),(s*(IY/2+1),zb+1-RAMP_DROP),align=None),
                    amount=IX/2+2, both=True)
# lid screw posts: FULL-HEIGHT corner columns merged into the walls -> zero overhang, very sturdy
# posts sit 4.5 in from the walls so the lid counterbore clears the lid edge; r6.6 still
# reaches the corner apex (6.36) so no sealed sliver forms between post and walls
LB=[(sx*(IX/2-4.5), sy*(IY/2-4.5)) for sx in (-1,1) for sy in (-1,1)]
for bx,by in LB:
    part += Pos(bx,by,-2)*Cylinder(radius=6.6,height=LZ+2,align=(Align.CENTER,Align.CENTER,Align.MIN))    # floor..flange, fills the square corner
    part -= Pos(bx,by,LZ+0.1)*Cylinder(radius=1.3,height=10,align=(Align.CENTER,Align.CENTER,Align.MAX))   # M3 self-tap pilot

# ---------- cable notch: front (+Y) wall, open to the top; inboard of the corner screw ----------
NW=16.0; NX=PX1+20                # in the +X access bay, near the terminal block, clear of the corner column
part -= Pos(NX, EY/2, IZ-11)*Box(NW, WT*3, 24)     # slot up to top (z IZ-23..IZ+1)
part -= Pos(NX, IY/2-LEDGE_W/2, IZ-11)*Box(NW, LEDGE_W+1, 24)   # clear the ledge there too

# ---------- lid (separate) ----------
lid = Pos(0,0,LZ+LID_T/2)*Box(IX-0.6, IY-0.6, LID_T)
for bx,by in LB:
    lid -= Pos(bx,by,LZ+LID_T/2)*Cylinder(radius=1.7,height=LID_T+2)
    lid -= Pos(bx,by,LZ+LID_T)*Cylinder(radius=3.1,height=1.6,align=(Align.CENTER,Align.CENTER,Align.MAX))  # cbore top
lid -= vent(Plane.XY, hex_centers(IX-40, IY-40), (0,0,LZ+LID_T/2), thru=LID_T*3)
# (no lid relief over the cable notch - the cable passes fine without it)

# ---------- inlet cut LAST so the flange/ramp cannot backfill the screw holes ----------
xw=-EX/2
part -= Pos(xw, 0, IN_ZC)*Box(WT*4, IN_CUT[0], IN_CUT[1])
for dz in (-IN_SCREW/2, IN_SCREW/2):
    part -= Pos(xw, 0, IN_ZC+dz)*Rot(0,90,0)*Cylinder(radius=1.65,height=WT*4)  # M3 clearance

print("internal",round(IX,1),round(IY,1),round(IZ,1),"external",round(EX,1),round(EY,1))
print("body vol",round(part.volume,0),"lid vol",round(lid.volume,0),
      "body wt?",part.is_manifold,"lid wt?",lid.is_manifold)
export_stl(part, OUT+r"\psu_body.stl"); export_step(part, OUT+r"\psu_body.step")
export_stl(lid,  OUT+r"\psu_lid.stl");  export_step(lid,  OUT+r"\psu_lid.step")
print("exported")
