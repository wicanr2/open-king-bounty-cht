import sys
import xml.etree.ElementTree as ET
import numpy as np
import re
import struct

if len(sys.argv) < 3:
	print("Usage: python kbtmx.py SOURCE DESTDIR")
	exit(0)

tmxfile = sys.argv[1]
outdir = sys.argv[2]

tree = ET.parse(tmxfile)
root = tree.getroot()

map = np.zeros([4, 64, 64], int)

continent_names = [ "C0", "C1", "C2", "C3" ]

hotspots = [ 0, 0, 0, 0 ];
num_castles = [ 0, 0, 0, 0 ];
num_towns = [ 0, 0, 0, 0 ];
num_foes = [ 0, 0, 0, 0 ];
cont = 0

ini_banner = ';This file was auto-generated from ' + tmxfile + ', do not edit it directly' + "\n\n"

for layer in root.findall("layer"):
	#print(layer.tag, layer.attrib)

	name = layer.get("name") 

	m = re.match(r"Continent (\d+)", name)
	if m is not None:
		cont = int(m.group(1))
		print("Reading landscape for continent %d" % cont)
	else:
		print("Undefined layer %s" % name)
		continue

	# Read layer properties to determine continent name
	_t = layer.find("properties");
	props = _t.findall("property") if _t is not None else [ ]
	for prop in props:
		if prop.get('name') == 'Name':
			continent_names[cont] = str(prop.get('value'));

	x = 0
	y = 0
	for tile in layer.find("data"):
		#print(tile.attrib, tile.get("gid"))

		#print("Setting %d %d %d " % (cont, y, x))
		
		tile = int(tile.get("gid")) - 1

		if tile < 0 or tile > 72:
			print("Warning: Tile %d/%dx%d is out of range!" % (cont, x, y))
			tile = 0

		if (tile >= 11 and tile <= 15) or (tile >= 18 and tile <= 19):
			#print("Converting tile %d/%dx%d into hotspot" % (cont, x, y))
			tile = 0x8b
			hotspots[cont] = hotspots[cont] + 1

		if tile == 17:
			#print("Converting foe %d/%dx%d" % (cont, x, y))
			tile = 0x91
			num_foes[cont] = num_foes[cont] + 1

		map[cont][y][x] = tile

		x = x + 1
		if x >= 64:
			x = 0
			y = y + 1

# Place salt marks
map[0][63][0] = 0xff
map[1][63][0] = 0xff
map[2][63][0] = 0xff
map[3][63][0] = 0xff

signposts = [ ]
castles = [ ]
towns = [ ]
home = None
alcove = None
continents = [
	{ "name": continent_names[0], "nav_x": 0, "nav_y": 0 },
	{ "name": continent_names[1], "nav_x": 0, "nav_y": 0 },
	{ "name": continent_names[2], "nav_x": 0, "nav_y": 0 },
	{ "name": continent_names[3], "nav_x": 0, "nav_y": 0 },
]

for objgroup in root.findall("objectgroup"):

	name = objgroup.get("name")
	
	#print("Reading '%s'" % name)

	cont = 0

	m = re.match(r".+ (\d+)", name)
	if m is not None:
		cont = int(m.group(1))
		print("Reading objects for continent %d (layer '%s')" % (cont, name))
	else:
		print("Undefined layer '%s'" % name)
		continue

	for object in objgroup:
		#print(object.tag, object.attrib)
		
		type = object.get("type")
		_t = object.find("properties");
		props = _t.findall("property") if _t is not None else [ ]

		x = 0
		y = 0
		if type == "Town" or type == "Castle" or type == "Signpost" or type == "Nav" or type == "Alcove":
			x = int(object.get("x")) / 48
			y = int(object.get("y")) / 36 - 1
			if (x >= 64 or x < 0 or y >= 64 or y < 0):
				print("%s object out of bounds, skipping" % type)
				continue
			y = 63 - y

		if type == "Nav":
			#print("Nav point")
			continents[cont]["nav_y"] = y;
			continents[cont]["nav_x"] = x;

		if type == "Alcove":
			#print("Alcove")
			alcove = {
				"x": x,
				"y": y,
				"cont": cont,
			}

			alcove["name"] = object.get("name");

			map[cont][63-y][x] = 0x8E

		if type == "Town":
			#print("Town")
			#for prop in props:
			#	print("!", prop.tag, prop.attrib)

			town = {
				"x": x,
				"y": y,
				"cont": cont,
				"boat_x": 0,
				"boat_y": 0,
				"gate_x": 0,
				"gate_y": 0,
				"castle_name": None,
				"castle": None,
				"order": -1,
			}

			town["name"] = object.get("name");

			if not town["name"]:
				town["name"] = "!NONAME!"
				print("Error! Town at CONT=%d,X=%d Y=%d has no name!" % (cont,63-y,x))

			map[cont][63-y][x] = 0x8A

			for prop in props:
				if prop.get('name') == 'BoatX':
					town["boat_x"] = x + int(prop.get('value'));
				if prop.get('name') == 'BoatY':
					town["boat_y"] = y - int(prop.get('value'));
				if prop.get('name') == 'GateX':
					town["gate_x"] = x + int(prop.get('value'));
				if prop.get('name') == 'GateY':
					town["gate_y"] = y - int(prop.get('value'));
				if prop.get('name') == 'Castle':
					town["castle_name"] = prop.get('value');

			#signpost["order"] = signpost["cont"] * 1024 + signpost["y"] * 64 + signpost["x"];

			#print(signpost)
			towns.append(town)

			num_towns[cont] = num_towns[cont] + 1

		if type == "Castle":
			#print("Castle")
			#for prop in props:
			#	print("!", prop.tag, prop.attrib)

			castle = {
				"x": x,
				"y": y,
				"cont": cont,
				"town": None,
				"order": -1,
			}
			#print("x:", x, "y:", y, "cont:",cont)
			castle["name"] = object.get("name");

			if castle["name"] is None:
				castle["name"] = "z"

			map[cont][63-y][x] = 0x85

			is_home = False

			for prop in props:
				if prop.get('name') == 'Home' and prop.get('value') == 'yes':
					castle["home"] = "yes";
					is_home = True;
					home = castle;
					continue
			#	if prop.get('name') == 'Text2':
			#		signpost["line2"] = prop.get('value');

			#signpost["order"] = signpost["cont"] * 1024 + signpost["y"] * 64 + signpost["x"];

			if is_home:
				continue

			#print(castle)
			castles.append(castle)

			num_castles[cont] = num_castles[cont] + 1

		if type == "Signpost":
			#print("Signpost")
			#for prop in props:
			#	print("!", prop.tag, prop.attrib)

			signpost = {
				"x": x,
				"y": y,
				"cont": cont,
				"line1": "",
				"line2": "",
			}

			map[cont][63-y][x] = 0x90

			for prop in props:
				if prop.get('name') == 'Text1':
					signpost["line1"] = prop.get('value');
				if prop.get('name') == 'Text2':
					signpost["line2"] = prop.get('value');

			if not signpost["line1"] and not signpost["line1"]:
				print("Warning! Signpost at CONT=%d X=%d tY=%d has no text" % (signpost["cont"], signpost["x"], 63-signpost["y"]))

			signpost["order"] = signpost["cont"] * 1024 + signpost["y"] * 64 + signpost["x"];

			#print(signpost)
			signposts.append(signpost)

print("!!! Stats !!!")
for i in range(0,4):
	print("%d) Castles: %d, Towns: %d, Hotspots: %d, Foes: %d" % (i, num_castles[i], num_towns[i], hotspots[i], num_foes[i]))
print("Total:")
print("~) Castles: %d, Towns: %d, Hotspots: %d, Foes: %d" % (len(castles), len(towns), sum(hotspots), sum(num_foes)))
print("Recommended:")
print("~) Castles: 26, Towns: 26, Hotspots: %d(21*4)-241, Foes: 87" % (21 * 4))

castles = sorted(castles, key=lambda k: k['name'])
towns = sorted(towns, key=lambda k: k['name'])
signposts = sorted(signposts, key=lambda k: k['order'])

def find_castle_by_name(name):
	for castle in castles:
		if (castle["name"] == name):
			return castle
	return None

def find_closest_castle(cont, x, y):
	for castle in castles:
		if castle["town"] is not None:
			castle["dist"] = 4096 * 512
		elif castle["cont"] != cont:
			castle["dist"] = 4096
		else:
			diff_x = abs(castle["x"] - x)
			diff_y = abs(castle["y"] - y)
			castle["dist"] = diff_x + diff_y
	sorted_castles = sorted(castles, key=lambda k: k['dist'])
	castle = sorted_castles[0]
	if castle["dist"] > 4096:
		return None
	return castle

# Link towns to castles
i = 0
for castle in castles:
	if castle["name"] is None:
		castle["name"] = "Castle %c" % (65+i)
	castle["order"] = i
	i = i + 1

i = -1
for town in towns:
	i = i + 1
	town["order"] = i
	if town["castle_name"] is None:
		print("Warning! Town %d %s doesn't have 'Castle' property." % (i, town["name"]))
		continue
	castle = find_castle_by_name(town["castle_name"]);
	if castle is None:
		print("Warning! Town %d %s can't find castle '%s'." % (i, town["name"], town["castle_name"]))
		continue
	if castle["town"]:
		print("Warning! Overwriting castle ownership over town")
	castle["town"] = town
	if town["castle"]:
		print("Warning! Overwriting town link to castle")
	town["castle"] = castle

# Auto-link
for town in towns:
	if town["castle"] is None:
		castle = find_closest_castle(town["cont"], town["x"], town["y"])
		if castle is None:
			print("Warning! Unable to auto-link town %s" % town["name"])
		castle["town"] = town
		town["castle"] = castle
		print("Auto-linked town %s to castle %s (dist:%d)" % (town["name"], castle["name"], castle["dist"]))

# Set boat/gate coords
def find_nearby_water(cont, x, y):
	offsets = [
		(1, 0), (-1, 0), (0, 1), (0, -1),
		(1, 1), (-1, 1), (-1, -1), (1, -1),
	]
	for i, j in offsets:
		my = y + j
		mx = x + i
		if (mx < 0): mx = 0
		if (mx > 63): mx = 63
		if (my < 0): my = 0
		if (my > 63): my = 63
		if map[cont][63-my][mx] >= 20 and map[cont][63-my][mx] <= 32:
			return mx, my
	return x, y

def find_nearby_grass(cont, x, y):
	offsets = [
		(1, 0), (-1, 0), (0, 1), (0, -1),
		(1, 1), (-1, 1), (-1, -1), (1, -1),
	]
	for i, j in offsets:
		my = y + j
		mx = x + i
		if (mx < 0): mx = 0
		if (mx > 63): mx = 63
		if (my < 0): my = 0
		if (my > 63): my = 63
		if map[cont][63-my][mx] == 0:
			return mx, my
	return x, y

for town in towns:
	if not town["boat_x"] and not town["boat_y"]:
		town["boat_x"], town["boat_y"] = find_nearby_water(town["cont"], town["x"], town["y"])
	if not town["gate_x"] and not town["gate_y"]:
		town["gate_x"], town["gate_y"] = find_nearby_grass(town["cont"], town["x"], town["y"])

# Verify
for castle in castles:
	if castle["town"] is None:
		print("Castle %s lacks town link" % castle["town"])
for town in towns:
	if town["castle"] is None:
		print("Town %s lacks castle link" % town["castle"])

# Reorder towns!
towns = sorted(towns, key=lambda k: k['castle']['order'])

def write_coords(fh, obj):
	fh.write("x = %d\n" % obj["x"])
	fh.write("y = %d\n" % obj["y"])
	fh.write("continent = %d\n" % obj["cont"])

# Save generics!
landfile = outdir + "/" + "land.ini"
print("Saving generics to %s" % landfile)
fh = open(landfile, "w")
i = 0
fh.write(ini_banner)
fh.write("[land]\n")
fh.write("name = Royal Reward\n")
fh.write("\n")

if home:
	fh.write("[special0]\n")
	fh.write(";home\n")
	fh.write("name = of the King\n")
	write_coords(fh, home)
	fh.write("\n")

if alcove:
	fh.write("[special1]\n")
	fh.write(";alcove\n")
	fh.write("name = Magic Alcove\n")
	write_coords(fh, alcove)
	fh.write("\n")

for continent in continents:
	fh.write("[continent%d]\n" % i)
	fh.write("name = %s\n" % continent["name"])
	fh.write("nav_x = %d\n" % continent["nav_x"])
	fh.write("nav_y = %d\n" % continent["nav_y"])
	fh.write("\n")
	i = i + 1

fh.close()


# Save castles!
castlefile = outdir + "/" + "castles.ini"
print("Saving castles to %s" % castlefile)
fh = open(castlefile, "w")
fh.write(ini_banner)
i = 0
for castle in castles:
	#print("%c) %s" % (65 + i, castle["name"])
	fh.write("[castle%d]\n" % i)
	fh.write("name = %s\n" % castle["name"])
	fh.write("continent = %d\n" % castle["cont"])
	fh.write("x = %d\n" % castle["x"])
	fh.write("y = %d\n" % castle["y"])
	fh.write("difficulty = 1\n")
	fh.write("dark = no\n")
	fh.write("\n")
	i = i + 1
fh.close()

# Save towns!
castlefile = outdir + "/" + "towns.ini"
print("Saving towns to %s" % castlefile)
fh = open(castlefile, "w")
fh.write(ini_banner)
i = 0
for town in towns:
	fh.write("[town%d]\n" % i)
	fh.write("name = %s\n" % town["name"])
	fh.write("continent = %d\n" % town["cont"])
	fh.write("x = %d\n" % town["x"])
	fh.write("y = %d\n" % town["y"])
	fh.write("boat_x = %d\n" % town["boat_x"])
	fh.write("boat_y = %d\n" % town["boat_y"])
	fh.write("gate_x = %d\n" % town["gate_x"])
	fh.write("gate_y = %d\n" % town["gate_y"])
	fh.write("spell = 7\n")
	#fh.write("castle_id = %d\n" % town["castle"]["order"])
	fh.write("invert_id = %d\n" % town["order"])
	fh.write("\n")
	i = i + 1
fh.close()

# Save signposts!
signfile = outdir + "/" + "signs.txt"
print("Saving sign texts to %s" % signfile)
fh = open(signfile, "w")
for sign in signposts:
	#fh.write("; hey\n");
	fh.write(sign["line1"])
	fh.write("\n")
	fh.write(sign["line2"])
	fh.write("\n")
fh.close()

# Save!

mapfile = outdir + "/" + "land.org"

#print(map)

print("Saving map data to %s" % mapfile)

fh = open(mapfile, "wb")
for cont in range(0, 4):
	for y in range(0, 64):
		for x in range(0, 64):
			fh.write(struct.pack("B", map[cont][63-y][x]))
fh.close()
