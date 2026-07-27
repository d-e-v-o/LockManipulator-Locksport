"""TPU strain-relief grommet for the PSU case cable exit slot (3x 14 AWG).
Slot: 16 wide x 2.5mm wall, open at the top. Local coords: x=width, y=thru-wall, z=height.
Drops into the open-top slot; flanges grip the wall front/back+bottom; lid traps the top;
3 stacked wire holes with a top slit so the wires side-load."""
import numpy as np
from build123d import *
OUT=r"C:\scratchpad"

SLOT_W=16.0; WALL=2.5; SLOT_H=19.0     # from PSU generator (NW, WT, notch height below lid)
CL=0.5                                  # total side clearance (snug TPU fit in the 16mm slot)
PW=SLOT_W-CL                            # plug width 15.5
FL_OUT=3.0                              # OUTER flange overhang onto the wall
FL_IN=2.0                               # INNER flange overhang - reduced so it inserts more easily
FLT=2.0                                 # flange thickness each face
GW=PW+2*FL_OUT                          # overall width 21.5 (set by the outer flange)
WD=4.0                                  # wire hole dia (14 AWG insulated ~4mm; tune to your wire)
SW=1.3                                  # top slit width
HZ=[-5.5,0.0,5.5]                       # 3 wire holes, stacked in z

# The lid ledge + its 58deg ramp protrude ~4mm into the cavity for case z46..56, and are only
# cleared INSIDE the 16mm notch. So the gripping flanges must stop BELOW that (case z45.5) or they
# collide beside the notch; above it only the (in-slot, clear) plug continues up to the lid.
# Grommet local z runs -SLOT_H/2..+SLOT_H/2 == case 36..55, i.e. case_z = local_z + 45.5.
FTOP=0.0                                 # local z (=case 45.5) where flanges stop, clear of the ledge

# plug fills the slot gap, full height
g = Box(PW, WALL, SLOT_H)
# OUTER flange (+y, faces outside the case): FULL height, 3mm overhang - nothing out there to hit
z0o=-(SLOT_H/2+FL_OUT)
g += Pos(0, +(WALL/2+FLT/2), (z0o+SLOT_H/2)/2)*Box(PW+2*FL_OUT, FLT, SLOT_H/2-z0o)
# INNER flange (-y, cavity side): relieved height + 2mm overhang (clears the ledge, easier insert)
z0i=-(SLOT_H/2+FL_IN)
g += Pos(0, -(WALL/2+FLT/2), (z0i+FTOP)/2)*Box(PW+2*FL_IN, FLT, FTOP-z0i)
GD=WALL+2*FLT                           # overall thru depth 6.5
# NOTE: part is handed - install with the FULL-height flange facing OUT (away from the cavity).

# 3 wire holes through y, stacked in z
for zc in HZ:
    g -= Pos(0,0,zc)*Rot(90,0,0)*Cylinder(radius=WD/2, height=GD+2)
# top slit: from top down through all holes so wires push in from the top
slit_top=SLOT_H/2+1; slit_bot=HZ[0]-WD/2+0.5
g -= Pos(0,0,(slit_top+slit_bot)/2)*Box(SW, GD+2, slit_top-slit_bot)

print("overall WxDxH:",round(GW,1),round(GD,1),round(SLOT_H,1),
      "| manifold",g.is_manifold,"vol",round(g.volume,0))
export_stl(g, OUT+r"\grommet.stl"); export_step(g, OUT+r"\grommet.step")
print("exported")
