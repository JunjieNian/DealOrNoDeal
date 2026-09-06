"""Editable, deterministic studio kit. Run with Blender 4.5 in background mode.
All design measurements below are centimeters; meshes are authored in meters.
No third-party assets, photographs, or human geometry are used.
"""
import bpy
import math
import json
import random
import wave
import struct
from pathlib import Path
from mathutils import Vector

BOARD_Y = 895.0
GEOMETRY_CHECKS = []

def bounds_cm(obj):
    points = [obj.matrix_world @ v.co for v in obj.data.vertices]
    return ([min(p[i] for p in points)*100 for i in range(3)],
            [max(p[i] for p in points)*100 for i in range(3)])

def require_geometry(condition, name, **measurements):
    GEOMETRY_CHECKS.append(dict(check=name, passed=bool(condition), **measurements))
    if not condition:
        raise RuntimeError('Studio geometry validation failed: '+name)

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'ArtSource' / 'Export'
OUT.mkdir(parents=True, exist_ok=True)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system = 'METRIC'
bpy.context.scene.unit_settings.scale_length = 1.0

SPECS = {
    'Obsidian': ([.018, .023, .034, 1], .35, .22, 0),
    'Floor': ([.028, .035, .05, 1], .65, .19, 0),
    'Chrome': ([.62, .69, .76, 1], .92, .22, 0),
    'Aluminium': ([.48, .53, .59, 1], .78, .31, 0),
    'Rubber': ([.009, .012, .017, 1], .05, .62, 0),
    'Velvet': ([.028, .045, .075, 1], .02, .9, 0),
    'Blue': ([.016, .055, .12, 1], .62, .27, 0),
    'Glass': ([.12, .27, .32, .22], .15, .09, 0),
    'LED_Cool': ([.035, .53, 1, 1], .1, .3, 4),
    'LED_Warm': ([1, .56, .13, 1], .1, .3, 3),
    'LED_White': ([.72, .85, 1, 1], .1, .3, 3),
    'LED_Red': ([1, .025, .012, 1], .1, .3, 2),
    'Window': ([.65, .49, .29, 1], .15, .4, 1.5),
}
MATS = {}
for name, (color, metal, rough, glow) in SPECS.items():
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = color
    mat.use_nodes = True
    bs = mat.node_tree.nodes.get('Principled BSDF')
    bs.inputs['Base Color'].default_value = color
    bs.inputs['Metallic'].default_value = metal
    bs.inputs['Roughness'].default_value = rough
    bs.inputs['Emission Color'].default_value = color
    bs.inputs['Emission Strength'].default_value = glow
    MATS[name] = mat

parts = []
def finish(obj, name, material, bevel=0):
    obj.name = name
    obj.data.materials.append(MATS[material])
    if bevel:
        mod = obj.modifiers.new('Manufactured edge radius', 'BEVEL')
        mod.width = bevel / 100
        mod.segments = 3
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=mod.name)
        mod = obj.modifiers.new('Weighted corner normals', 'WEIGHTED_NORMAL')
        mod.keep_sharp = True
        bpy.ops.object.modifier_apply(modifier=mod.name)
    parts.append(obj)
    return obj

def box(name, loc, size, mat='Obsidian', bevel=.6):
    bpy.ops.mesh.primitive_cube_add(size=1, location=[x/100 for x in loc])
    obj = bpy.context.object
    obj.dimensions = [x/100 for x in size]
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish(obj, name, mat, bevel)

def rod(name, a, b, radius=2, mat='Chrome', vertices=12):
    a, b = Vector(a)/100, Vector(b)/100
    d = b-a
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius/100,
        depth=d.length, location=(a+b)/2)
    obj = bpy.context.object
    obj.rotation_euler = d.to_track_quat('Z', 'Y').to_euler()
    return finish(obj, name, mat)

def ring(name, center, rx, ry, width, height, mat, segments=128):
    verts, faces = [], []
    for i in range(segments):
        a = i*math.tau/segments
        for radius_offset, z in [(0,-height/2),(0,height/2),(-width,-height/2),(-width,height/2)]:
            verts.append(((center[0]+max(0,rx+radius_offset)*math.cos(a))/100,
                          (center[1]+max(0,ry+radius_offset)*math.sin(a))/100,(center[2]+z)/100))
    for i in range(segments):
        n=(i+1)%segments
        for p,q in [(0,1),(1,3),(3,2),(2,0)]: faces.append((i*4+p,n*4+p,n*4+q,i*4+q))
    mesh=bpy.data.meshes.new(name)
    mesh.from_pydata(verts,[],faces)
    obj=bpy.data.objects.new(name,mesh)
    bpy.context.collection.objects.link(obj)
    return finish(obj,name,mat)

def arch(name, x, radius, thickness, depth, mat):
    verts,faces=[],[]
    n=128
    for i in range(n+1):
        a=i*math.pi/n
        for xx,r in [(-depth/2,radius), (depth/2,radius),(-depth/2,radius-thickness),(depth/2,radius-thickness)]:
            verts.append(((x+xx)/100,r*math.cos(a)/100,(35+r*math.sin(a))/100))
    for i in range(n):
        for p,q in [(0,1),(1,3),(3,2),(2,0)]: faces.append((i*4+p,(i+1)*4+p,(i+1)*4+q,i*4+q))
    faces += [(0,2,3,1),(n*4,n*4+1,n*4+3,n*4+2)]
    mesh=bpy.data.meshes.new(name); mesh.from_pydata(verts,[],faces)
    obj=bpy.data.objects.new(name,mesh); bpy.context.collection.objects.link(obj)
    return finish(obj,name,mat)

def export(name):
    global parts
    bpy.ops.object.select_all(action='DESELECT')
    for obj in parts: obj.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj=bpy.context.object
    obj.name=name
    bpy.context.scene.cursor.location=(0,0,0)
    bpy.ops.object.origin_set(type='ORIGIN_CURSOR')
    # Keep meshes separate by material and module; UE assigns the authored PBR family.
    bpy.ops.export_scene.fbx(filepath=str(OUT/(name+'.fbx')), use_selection=True,
        object_types={'MESH'}, add_leaf_bones=False, bake_anim=False,
        axis_forward='Y', axis_up='Z', apply_unit_scale=True,
        use_mesh_modifiers=True, mesh_smooth_type='FACE')
    parts=[]
    return obj

# The physical shell extends underneath every seating bank and all camera views.
box('Polished studio floor',(-500,0,-18),(4000,3100,32),'Floor',1)
box('Upstage acoustic wall',(1030,0,450),(30,2800,940),'Rubber')
for side in [-1,1]:
    box('Side masking wall',(-470,side*1490,450),(3020,24,940),'Rubber')
    for x in range(-1800,951,75):
        box('Acoustic wall rib',(x,side*1468,470),(12,12,860),'Obsidian')
    for x in range(-1450,451,180):
        box('Aisle inset light',(x,side*1210,1),(42,3,1.5),'LED_Warm',.3)
for y in [-645,645]:
    box('Runway edge',(-370,y,1),(1020,3,1.2),'LED_Cool',.2)
export('SM_StudioShell')

# Low central dais with machined perimeter and concentric practical strips.
ring('Dais fascia',(-100,0,11),390,290,390,22,'Obsidian',96)
ring('Chrome lower rim',(-100,0,4),394,294,6,4,'Chrome')
ring('Blue perimeter LED',(-100,0,18),392,292,3,3,'LED_Cool')
ring('Polished upper disc',(-100,0,24),382,282,382,5,'Floor')
ring('Inner chrome inlay',(-100,0,27),330,230,1.5,1,'Chrome')
ring('Inner gold inlay',(-100,0,27),315,215,1,1,'LED_Warm')
for i in range(12):
    a=i*math.tau/12
    x,y=-100+387*math.cos(a),287*math.sin(a)
    rod('Fascia screw',(x,y,5),(x,y,18),.8,'Chrome',8)
# Authentic scale desk, clear slab and separated supports rather than emissive glass.
box('Desk base',(-140,0,31),(76,128,7),'Chrome',3)
for y in [-57,57]:
    box('Acrylic desk fin',(-140,y,81),(63,3.5,96),'Glass',1)
    box('Fin cap',(-140,y,80),(5,5,97),'Chrome',1)
box('Desk glass slab',(-140,0,132),(112,178,5),'Glass',2)
for y in [-86,86]: box('Desk perimeter rail',(-140,y,128),(108,2,2),'Chrome',.5)
for x in [-193,-87]: box('Desk perimeter rail',(x,0,128),(2,170,2),'Chrome',.5)
box('Button mount',(-159,-40,138),(29,34,6),'Chrome',2)
rod('Red deal button',(-159,-40,141),(-159,-40,146),10,'LED_Red',48)
box('Button clear guard',(-140,-40,151),(2,34,22),'Glass',1)
export('SM_GamePlatform')

# Telephone model including receiver, dial keys and coiled lead, in world space.
box('Phone base',(-163,45,139),(28,24,8),'Rubber',3)
for i in range(12):
    box('Telephone keypad',(-171+(i//3)*3.8,39+(i%3)*4,143.5),(2.8,2.9,.8),'Aluminium',.3)
box('Receiver bridge',(-153,45,147),(7,30,5),'Obsidian',2)
for y in [31,59]: box('Receiver earpiece',(-153,y,146),(11,8,7),'Rubber',2)
for i in range(95):
    a=i*.53; b=(i+1)*.53
    rod('Coiled phone cord',(-151+i*.3,63+1.2*math.cos(a),143+1.2*math.sin(a)),
        (-151+(i+1)*.3,63+1.2*math.cos(b),143+1.2*math.sin(b)),.25,'Rubber',6)
export('SM_BankerPhone')

for row,(count,x,z) in enumerate(zip([6,7,7,6],[285,405,525,645],[55,135,215,295])):
    box('Tier structure',(x,0,z/2),(120,1070,z),'Obsidian',2)
    box('Stage tread',(x,0,z),(120,1070,3),'Floor',1)
    # The first show tier is only 55 cm high. Keep its trim above the floor.
    fascia_bottom=max(4,z-68)
    fascia_top=z-4
    box('Brushed fascia',(x-60,0,(fascia_bottom+fascia_top)/2),(2,1068,fascia_top-fascia_bottom),'Blue',.4)
    for zz in [z-4,max(4,z-65)]: box('Step LED',(x-62,0,zz),(1.5,1062,2),'LED_Cool',.2)
    box('Chrome nosing',(x-62,0,z),(4,1072,2),'Chrome',.5)
    for y in range(-480,481,80):
        box('Fascia joint',(x-61.3,y,(fascia_bottom+fascia_top)/2),(1,1,fascia_top-fascia_bottom-3),'Chrome',.1)
    for pos in range(count):
        y=(pos-(count-1)/2)*145
        box('Case plinth foot',(x,y,z+4),(48,58,6),'Chrome',2)
        box('Case plinth column',(x+5,y,z+44),(10,12,80),'Obsidian',1)
        box('Plinth light',(x-1,y,z+44),(1,3,68),'LED_Cool',.2)
        box('Case shelf',(x,y,z+89),(27,59,4),'Chrome',1)

# Continuous mirrored stairs with actual tread-mounted posts. Each fourth tread
# meets its show tier at the finished surface, including the 3 cm top slab.
for side in [-1,1]:
    previous_top=0.0
    stair_nodes=[]
    stair_meshes=[]
    for row,(x,z) in enumerate(zip([285,405,525,645],[55,135,215,295])):
        landing_top=z+1.5
        rise=(landing_top-previous_top)/4
        require_geometry(0 < rise <= 20.01,'Uniform flight rise',side=side,flight=row,rise_cm=rise)
        for k in range(4):
            height=previous_top+rise*(k+1)
            cx=x-45+k*30
            stair=box('Access stair',(cx,side*580,height/2),(30,88,height),'Obsidian',.35)
            stair_meshes.append(stair)
            # Narrow inset marker sits on the tread, clear of the handrail flange.
            box('Access stair marker',(cx-13,side*580,height+.25),(2,66,.5),'LED_Warm',.15)
            stair_nodes.append((cx,side*617,height))
        previous_top=landing_top
    for i,(cx,cy,top) in enumerate(stair_nodes):
        if i in (0,3,7,11,15):
            flange=box('Handrail mounting flange',(cx,cy,top+1),(8,8,2),'Chrome',.3)
            rod('Handrail upright',(cx,cy,top+1),(cx,cy,top+90),2,'Chrome')
            lo,hi=bounds_cm(stair_meshes[i]); flo,fhi=bounds_cm(flange)
            require_geometry(abs(flo[2]-hi[2])<.02 and all(flo[a]>=lo[a] and fhi[a]<=hi[a] for a in (0,1)),
                'Post flange supported by actual tread',side=side,step=i)
        if i:
            a=stair_nodes[i-1]
            rod('Continuous handrail',(a[0],a[1],a[2]+90),(cx,cy,top+90),2,'Chrome')
    require_geometry(len(stair_nodes)==16,'Continuous 16-step access flight',side=side)
export('SM_CaseTerraces')

# Smooth continuous three-band arch, with real separators and a layered city cyclorama.
arch('Arch structural spine',790,683,43,64,'Obsidian')
arch('Outer polished moulding',752,686,6,10,'Chrome')
arch('Outer blue practical',744,675,7,5,'LED_Cool')
arch('Brushed arch face',748,662,17,8,'Aluminium')
arch('Inner warm practical',741,642,4,5,'LED_Warm')
for i in range(25):
    a=i*math.pi/24
    rod('Arch divider',(738,645*math.cos(a),35+645*math.sin(a)),
        (738,675*math.cos(a),35+675*math.sin(a)),1,'Chrome',8)
rng=random.Random(2026)
for layer in range(2):
    for i in range(17):
        y=-620+i*77 + layer*20
        h=rng.randint(230,570); w=rng.randint(48,83); x=905-layer*55
        box('City scenic tower',(x,y,h/2),(36,w,h),'Blue' if layer else 'Obsidian',.8)
        box('Rooftop coping',(x,y,h+2),(38,w+3,4),'Aluminium',.3)
        if i%4==0: rod('Skyline antenna',(x,y,h),(x,y,h+35),.7,'Chrome',6)
        for yy in range(-int(w/2)+10,int(w/2)-6,13):
            for zz in range(22,h-12,21):
                if rng.random()<.62:
                    box('Night window',(x-19,y+yy,zz),(1,6,10),'Window',0)
export('SM_ArchSkyline')

box('Banker tower',(625,-845,235),(280,420,470),'Obsidian',2)
for y in range(-1035,-650,32):
    box('Tower vertical reveal',(482,y,239),(2,9,443),'Blue',.5)
box('Booth floor',(507,-845,472),(280,422,12),'Chrome',1)
box('Booth soffit',(525,-845,694),(280,422,16),'Obsidian',2)
box('Booth back',(663,-845,579),(10,414,215),'Rubber',1)
for side in [-1,1]:
    box('Window jamb',(401,-845+side*201,580),(20,14,215),'Chrome',1)
    box('Booth side return',(550,-845+side*210,580),(260,10,215),'Obsidian',1)
box('Smoked glazing',(394,-845,585),(2,391,194),'Glass',.5)
for y in [-910,-780]: box('Glazing mullion',(391,y,581),(7,4,209),'Chrome',.4)
for z in range(495,680,13): box('Venetian blind',(404,-845,z),(4,382,3),'Obsidian',.3)
box('Banker desk',(480,-845,542),(70,155,5),'Chrome',1)
box('Desk monitor',(495,-845,568),(9,53,33),'Obsidian',2)
box('Booth warm strip',(400,-845,686),(5,388,3),'LED_Red',.3)
export('SM_BankerSuite')

# Screen enclosure dimensions contain both columns (the old housing was narrower than its text).
box('Amount screen enclosure',(435,BOARD_Y,350),(40,430,702),'Obsidian',5)
for y in [BOARD_Y-215,BOARD_Y+215]:
    box('Screen chrome stile',(410,y,350),(12,6,696),'Chrome',1)
    box('Screen perimeter practical',(402,y,350),(2,2,689),'LED_Warm',.4)
for z in [6,694]: box('Screen horizontal frame',(408,BOARD_Y,z),(10,428,8),'Chrome',1)
for col in [-1,1]:
    for row in range(13):
        box('Amount cell bezel',(394,BOARD_Y+col*98,603-row*46),(5,189,39),'Chrome',1)
export('SM_AmountDisplay')

# Real stepped bleachers and rails. Chairs remain GPU-instanced by the runtime module.
for row in range(5):
    x=-720-row*90; top=row*28
    box('Downstage bleacher',(x,0,top/2-2),(90,1830,max(4,top)),'Rubber',.6)
    box('Bleacher aisle tread',(x-42,0,top),(3,1750,1),'Aluminium',.2)
for side in [-1,1]:
    for row in range(4):
        y=side*(810+row*88); top=row*28
        # Seats end at X=160. Stop the platform at 270, leaving a real gap
        # before the board and booth instead of extending solid risers into them.
        box('Side bleacher',(-187.5,y,top/2-2),(915,88,max(4,top)),'Rubber',.6)
    for x in [-640,-485,-330,-175,-20,135,270]:
        rod('Audience rail post',(x,side*1168,84),(x,side*1168,179),2,'Chrome')
    rod('Audience safety rail',(-640,side*1168,179),(270,side*1168,179),2,'Chrome')
export('SM_AudienceArchitecture')

# Visible square box-truss and lamp bodies aligned with the physical light positions.
for x in [-450,450]:
    for dx in [-16,16]:
        for z in [865,897]: rod('Truss chord',(x+dx,-1050,z),(x+dx,1050,z),2.1,'Aluminium')
    for y in range(-1050,1001,60):
        for dx in [-16,16]:
            rod('Truss diagonal',(x+dx,y,865),(x+dx,y+60,897),1,'Chrome',8)
        rod('Truss crossbar',(x-16,y,897),(x+16,y,897),1,'Chrome',8)
for x,y,z in [(-450,-620,850),(-450,620,850),(450,-600,900),(450,600,900)]:
    a=Vector((x,y,z)); d=(Vector((250,0,100))-a).normalized()
    rod('Moving head body',a-d*10,a+d*22,15,'Obsidian',32)
    rod('Lamp lens',a+d*22,a+d*23,12,'LED_White',32)
    box('Fixture yoke',(x,y,z+20),(40,10,34),'Rubber',2)
export('SM_LightingHardware')

# A reusable chair, facing local +X, with separate upholstery, arms and steel underframe.
box('Seat cushion',(0,0,43),(43,46,10),'Velvet',4)
back=box('Seat back',(-20,0,67),(9,46,45),'Velvet',4)
back.rotation_euler.y=math.radians(-8)
for y in [-27,27]:
    box('Arm rest',(0,y,60),(41,6,5),'Rubber',2)
    rod('Arm support',(-8,y,6),(-8,y,60),1.8,'Chrome')
for x in [-14,16]:
    for y in [-18,18]: rod('Seat leg',(x,y,2),(x,y,40),1.7,'Obsidian')
rod('Seat cross brace',(-14,-20,24),(-14,20,24),1.5,'Chrome')
export('SM_StudioChair')

# Case lower body and handle, at local origin. Front is -X; lower lid hinge is Z=-17.
box('Aluminium case shell',(0,0,0),(14,48,34),'Aluminium',2.6)
box('Recessed black seam',(-6.8,0,0),(.8,46,32),'Rubber',1.8)
for y in [-22,22]:
    box('Case corner protector',(0,y,0),(14,3,33),'Chrome',1)
for y in [-8,8]: box('Handle foot',(0,y,19),(5,4,6),'Chrome',.8)
box('Suitcase handle',(0,0,23),(5,21,4),'Rubber',1.6)
for y in [-15,15]: box('Case latch',(-8,y,11),(2.5,5,7),'Chrome',.8)
box('Case interior lining',(-7.5,0,0),(.5,41,26),'Velvet',1)
export('SM_BriefcaseBody')
# Lid mesh geometry is authored about its hinge, for a real opening rotation.
box('Case lid',(-.6,0,17),(2,47,33),'Aluminium',2)
box('Lid face inset',(-1.75,0,17),(.4,41,26),'Chrome',1.4)
for y in [-18,18]: box('Hinge barrel',(.2,y,.8),(3,5,2),'Chrome',.7)
export('SM_BriefcaseLid')

# Check measured mesh envelopes, not just authored coordinate constants.
bpy.context.view_layer.update()
for left,right in [('SM_CaseTerraces','SM_AmountDisplay'),('SM_CaseTerraces','SM_BankerSuite'),
                   ('SM_CaseTerraces','SM_ArchSkyline'),('SM_AudienceArchitecture','SM_AmountDisplay'),
                   ('SM_AudienceArchitecture','SM_BankerSuite')]:
    amin,amax=bounds_cm(bpy.data.objects[left]); bmin,bmax=bounds_cm(bpy.data.objects[right])
    gaps=[max(bmin[i]-amax[i],amin[i]-bmax[i]) for i in range(3)]
    require_geometry(max(gaps)>1,'Separate mesh envelopes',left=left,right=right,axis_gaps_cm=gaps)
(OUT/'geometry-validation.json').write_text(json.dumps(dict(checks=GEOMETRY_CHECKS,
    passed=all(c['passed'] for c in GEOMETRY_CHECKS)),indent=2))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'ArtSource'/'DealStudio.blend'))
(OUT/'materials.json').write_text(json.dumps(SPECS,indent=2))

# Original short cues, deliberately restrained and without sampled show music.
audio=ROOT/'ArtSource'/'Audio'; audio.mkdir(exist_ok=True)
def cue(name,duration,notes):
    rate=22050; data=[]
    for i in range(int(rate*duration)):
        t=i/rate; value=0
        for start,length,freq,gain in notes:
            u=t-start
            if 0<=u<length:
                env=min(u/.012,1)*min((length-u)/.08,1)*math.exp(-u*2)
                value+=gain*env*(math.sin(math.tau*freq*u)+.15*math.sin(math.tau*freq*2*u))
        data.append(struct.pack('<h',int(max(-.9,min(.9,value))*32767)))
    with wave.open(str(audio/(name+'.wav')),'wb') as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(rate);w.writeframes(b''.join(data))
cue('Select',.15,[(0,.14,660,.13)])
cue('Reveal',.8,[(0,.4,220,.16),(.14,.5,440,.13),(.28,.45,660,.08)])
cue('Ring',1.6,[(s,.24,f,.11) for s in [0,.32,.9,1.22] for f in [440,480]])
cue('Deal',1.2,[(0,.9,262,.12),(.12,.9,330,.12),(.24,.9,392,.12),(.4,.7,523,.1)])
print('[StudioArt] COMPLETE - editable Blender scene, 12 meshes, PBR recipe and original audio')
