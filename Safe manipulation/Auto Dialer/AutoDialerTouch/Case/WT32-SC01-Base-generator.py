"""Enlarged WT32-SC01 desk enclosure base - full parametric build (build123d).
USE coords: desk z=0, up +z, depth +y, width x, screen bottom edge at origin.
Then transform to MODEL coords to mate with existing frame.
"""
import numpy as np
from build123d import *
OUT=r"C:\scratchpad"

# ---------- parameters ----------
HALF_W=47.8; WALL=2.4
c=np.cos(np.radians(45))
# ---- screen (ZX-350 / WT32-SC01): 92 W x 60 H; mount pattern 75.14 x 51.39 ----
SCREEN_W, SCREEN_H = 92.0, 60.0
SCREEN_LEN=SCREEN_H                      # 45deg face length = screen height (was wrongly 62.6)
SCR_TOP=(SCREEN_LEN*c, SCREEN_LEN*c)     # (42.43,42.43) screen top edge on the 45deg face
# mounting holes: horizontally ASYMMETRIC (front-view L/R margins), vertically CENTRED.
# front view: 10.34 margin on LEFT, 6.52 on RIGHT (from diagram back-view, mirrored).
SCREW_MARG_L, SCREW_MARG_R = 10.34, 6.52    # swap these two if the screen mounts the other way
SCREW_X=[-SCREEN_W/2+SCREW_MARG_L, SCREEN_W/2-SCREW_MARG_R]   # [-35.66, +39.48], span 75.14
_sv=(SCREEN_H-51.39)/2                        # 4.305 top/bottom margin
SCREW_S=[_sv, _sv+51.39]                      # [4.305, 55.695], span 51.39
DEPTH=88.0; TOPZ=44.0; EDGE_R=4.0
CHIN=10.0                               # foot below screen bottom edge (desk at z=-CHIN)
Nu=Vector(0,-c,c)                       # screen face outward normal
Su=Vector(0,c,c)                        # up-the-screen direction
# board
BW,BL,BT=88.9,52.1,1.6
SLOT_GAP=2.3                      # rear-rail slot height (board 1.6 + ~0.7 clearance)
WIRE_Z=11.0                      # wire-slot centre, raised ABOVE the board top (7.6)
BC_Y=51.0; SHELF_Z=6.0            # board y 25..77 (front clears screen keepout)
M2X=39.35; M2Y=17.8
# ---------- outer ----------
# side profile (y,z): screen bottom edge at (0,0); foot down to z=-CHIN
prof=[(0,0),(SCR_TOP[0],SCR_TOP[1]),(DEPTH,TOPZ),(DEPTH,-CHIN),(0,-CHIN)]
outer=extrude(Plane.YZ*Polygon(*prof,align=None), amount=HALF_W, both=True)
outer=fillet(outer.edges().filter_by(Axis.Z), radius=EDGE_R)

# ---------- shell (open the screen face) ----------
def front_face(sol):
    best=None;bd=-9
    for f in sol.faces().filter_by(GeomType.PLANE):
        d=f.normal_at(f.center()).dot(Nu)
        if d>bd: bd=d; best=f
    return best
ff=front_face(outer)
shell=offset(outer, amount=-WALL, openings=ff, kind=Kind.INTERSECTION)

# ---------- screw bosses ----------
part=shell
INNER_FLOOR=-CHIN+WALL
for sx in SCREW_X:
    for s in SCREW_S:
        ctr=Vector(sx, s*c, s*c)                 # point on screen face
        pl=Plane(origin=ctr, z_dir=-Nu)          # local z points into cavity
        part += pl*Cylinder(radius=4.4, height=12, align=(Align.CENTER,Align.CENTER,Align.MIN))
for sx in SCREW_X:
    for s in SCREW_S:
        ctr=Vector(sx, s*c, s*c)
        # thru Ø3.0 core along Nu through ctr
        part -= Plane(origin=ctr, z_dir=Nu)*Cylinder(radius=1.5,height=30,
                    align=(Align.CENTER,Align.CENTER,Align.CENTER))
        # counterbore Ø5.5 starting 4.5mm into cavity, opening into cavity
        part -= Plane(origin=ctr, z_dir=-Nu)*Pos(0,0,4.5)*Cylinder(radius=2.75,height=20,
                    align=(Align.CENTER,Align.CENTER,Align.MIN))
# bridge each screen boss to its nearest side wall (gusset web, 2.4mm)
# web starts at the boss OD (r4.4) - clear of the Ø5.5 counterbore - so it never fills the screw slot
for sx in SCREW_X:
    xw=np.sign(sx)*HALF_W                         # target outer edge -> full merge into wall
    for s in SCREW_S:
        pl=Plane(origin=Vector(sx, s*c, s*c), x_dir=Vector(1,0,0), z_dir=-Nu)
        dx=xw-sx
        x0=np.sign(dx)*3.6                         # weld at boss OD, outside the cbore (r2.75)
        part += pl*Pos((x0+dx)/2,0,6)*Box(abs(dx-x0),2.4,12)

# ---------- board mount: 2 FRONT standoffs (screwed) + REAR side-wall slots ----------
for sx in (-M2X,M2X):
    sy=BC_Y-M2Y                                   # front pair only
    h=SHELF_Z-INNER_FLOOR
    part += Pos(sx,sy,INNER_FLOOR)*Cylinder(radius=3.2,height=h,
             align=(Align.CENTER,Align.CENTER,Align.MIN))
    part -= Pos(sx,sy,SHELF_Z-5.5)*Cylinder(radius=0.85,height=6,
             align=(Align.CENTER,Align.CENTER,Align.MIN))
    # bridge standoff to nearest side wall (gusset web, 2.4mm), welded at the OD only
    xt=np.sign(sx)*HALF_W
    xo=sx+np.sign(xt-sx)*2.4                       # start at standoff OD (r3.2), clear of Ø1.7 pilot
    x0,x1=sorted((xo,xt))
    part += Pos((x0+x1)/2, sy, (INNER_FLOOR+SHELF_Z)/2)*Box(abs(x1-x0),2.4,SHELF_Z-INNER_FLOOR)
# rear retention rails (ledge + lip) on both side inner walls; board rear edge slides in
INW=HALF_W-WALL                                   # inner wall x = 45.4
RAIL_Y0, RAIL_Y1 = 63.0, DEPTH-WALL               # y span of rail
for sign in (-1,1):
    xin=sign*(INW)                                # at inner wall
    xtip=sign*42.0                                # engagement (board edge is +-44.45)
    xc=(xin+xtip)/2; xw=abs(xin-xtip)
    yc=(RAIL_Y0+RAIL_Y1)/2; yl=RAIL_Y1-RAIL_Y0
    part += Pos(xc,yc,(4.5+SHELF_Z)/2)*Box(xw,yl,SHELF_Z-4.5)      # ledge, top at SHELF_Z=6
    part += Pos(xc,yc,SHELF_Z+SLOT_GAP+0.6)*Box(xw,yl,1.2)         # lip; slot=SLOT_GAP above ledge

# ---------- cable slots (enclosed, rounded) ----------
def rslot(center,wx,wy,wz,axis):
    # rounded slot = box + two cylinders; axis is wall-normal ('y' or 'x')
    b=Pos(*center)*Box(wx,wy,wz)
    return b
# (14AWG slot now lives in the removable back panel - see below)
# right side wall 22AWG : 8(y) x 4(z) through x=HALF_W wall, ABOVE board top
part -= Pos(HALF_W-WALL/2, BC_Y, WIRE_Z)*Box(WALL+6,8,4)
part -= Pos(HALF_W-WALL/2, BC_Y-4, WIRE_Z)*Rot(0,90,0)*Cylinder(radius=2,height=WALL+6)
part -= Pos(HALF_W-WALL/2, BC_Y+4, WIRE_Z)*Rot(0,90,0)*Cylinder(radius=2,height=WALL+6)

# ---------- speaker vent: BOTTOM floor, centred under the PCB, fires down ----------
for dx in (-10,-5,0,5,10):
    for dy in (-6,0,6):
        part -= Pos(dx, BC_Y+dy, -CHIN)*Cylinder(radius=1.4,height=WALL+8)   # z-axis, thru floor

# ========== REMOVABLE BACK PANEL ==========
INX=HALF_W-WALL            # 45.4 inner wall x
ZB, ZT = -CHIN+WALL, 41.6  # cavity inner z range at back (-7.6 .. 41.6)
ZC=(ZB+ZT)/2; ZH=(ZT-ZB)   # z centre / full height
PT=2.5                     # panel thickness
YO=DEPTH                   # outer back plane (88)
YR=YO-PT                   # recess front plane (85.5); panel sits YR..YO
LEDGE_W=2.5; YL=YR-2.5     # ledge (panel rests on its face at YR)
def yprism(hx,hz,y0,y1):   # axis-aligned box spanning y0..y1
    return Pos(0,(y0+y1)/2,ZC)*Box(2*hx,(y1-y0),2*hz)
# 1) open back + recess: remove full-inner slab from YR outward
part -= yprism(INX, ZH/2, YR, YO+2)
# 2) inward ledge ring for the panel to rest on
part += yprism(INX, ZH/2, YL, YR) - yprism(INX-LEDGE_W, ZH/2-LEDGE_W, YL-1, YR+1)
# 3) four M3 self-tap corner BLOCKS (tie the boss into the side wall + roof/floor + ledge)
PB=[(sx,sz) for sx in (-40,40) for sz in (ZB+5, ZT-5)]
GUSS_Y=YR-9.0
for sx,sz in PB:
    xs = 1 if sx>0 else -1
    zs = 1 if sz>ZC else -1
    x0,x1 = sorted((sx-5*xs, xs*INX))              # boss -> side wall
    z0,z1 = sorted((sz-5*zs, ZT if zs>0 else ZB))  # boss -> roof/floor
    part += Pos((x0+x1)/2,(GUSS_Y+YR)/2,(z0+z1)/2)*Box(abs(x1-x0), YR-GUSS_Y, abs(z1-z0))
for sx,sz in PB:
    part -= Pos(sx, YR+0.1, sz)*Rot(90,0,0)*Cylinder(radius=1.25,height=8.5,
             align=(Align.CENTER,Align.CENTER,Align.MIN))        # pilot, opens at panel face

# 4) notch the back lip at board level (both sides) so the PCB slides in from the back
for xs in (-1,1):
    part -= Pos(xs*43.2, (YL-1+YO+2)/2, SHELF_Z+SLOT_GAP/2)*Box(4.4, (YO+2)-(YL-1), SLOT_GAP+2.5)

# ---------- the panel (separate part) ----------
panel = yprism(INX-0.3, ZH/2-0.3, YR, YO)
for sx,sz in PB:
    panel -= Pos(sx,(YR+YO)/2,sz)*Rot(90,0,0)*Cylinder(radius=1.7,height=PT+2,
              align=(Align.CENTER,Align.CENTER,Align.CENTER))     # M3 clearance
    panel -= Pos(sx,YO,sz)*Rot(90,0,0)*Cylinder(radius=3.1,height=2.2,
              align=(Align.CENTER,Align.CENTER,Align.MIN))        # head counterbore (outer)
# 14AWG slot in panel, near -x, raised ABOVE board top
panel -= Pos(-30,(YR+YO)/2,WIRE_Z)*Box(12,PT+4,5)
panel -= Pos(-36,(YR+YO)/2,WIRE_Z)*Rot(90,0,0)*Cylinder(radius=2.5,height=PT+4)
panel -= Pos(-24,(YR+YO)/2,WIRE_Z)*Rot(90,0,0)*Cylinder(radius=2.5,height=PT+4)

# ---------- OPEN-SLOT panel variant (alternative to the closed-slot panel above) ----------
# identical, but the cable notch runs out to the -x edge so the panel can be slid off the
# cable bundle without disconnecting / unthreading it.
panel_open = panel - Pos(-38,(YR+YO)/2,WIRE_Z)*Box(16,PT+4,5)   # channel x=-46..-30 -> past edge

# ---------- handedness (user asked to flip 2026-07-14) ----------
MIRROR=False                       # False = opposite hand from the previously printed body
if MIRROR:
    part  = mirror(part,  about=Plane.YZ)
    panel = mirror(panel, about=Plane.YZ)
    panel_open = mirror(panel_open, about=Plane.YZ)
print("USE body vol",round(part.volume,0),"panel vol",round(panel.volume,0),
      "panel_open vol",round(panel_open.volume,0))
export_stl(part, OUT+r"\_use.stl")

# ---------- transform to MODEL coords + export ----------
def to_model(o): return o.rotate(Axis.X,45).translate((0,12,-31.3))
mbody=to_model(part); mpanel=to_model(panel); mpanel_open=to_model(panel_open)
export_stl(mbody, OUT+r"\base_new.stl");   export_step(mbody, OUT+r"\base_new.step")
export_stl(mpanel, OUT+r"\panel_new.stl"); export_step(mpanel, OUT+r"\panel_new.step")
export_stl(mpanel_open, OUT+r"\panel_open_new.stl"); export_step(mpanel_open, OUT+r"\panel_open_new.step")
print("bbox body",[round(v,1) for v in tuple(mbody.bounding_box().min)+tuple(mbody.bounding_box().max)])
print("exported base_new + panel_new")