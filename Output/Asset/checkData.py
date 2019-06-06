
file = open('Vincent.xml', 'r')

vertices = []

for line in file:
	#print (line)
	# Search x
	indexStartX = line.find('x="')
	line = line[indexStartX + 3:]
	indexEndX = line.find('"')
	xString = line[:indexEndX]
	line = line[indexEndX + 1:]
	# Search y
	indexStartY = line.find('y="')
	line = line[indexStartY + 3:]
	indexEndY = line.find('"')
	yString = line[:indexEndY]
	line = line[indexEndY + 1:]
	# Search z
	indexStartZ = line.find('z="')
	line = line[indexStartZ + 3:]
	indexEndZ = line.find('"')
	zString = line[:indexEndZ]
	line = line[indexEndZ + 1:]

	#print(xString, yString, zString)
	vertex = [float(xString), float(yString), float(zString)]
	vertices.append(vertex)



for u in vertices:
	for v in vertices:
		if u != v:
			difference = [u[0] - v [0], u[1] - v [1], u[2] - v [2]]
			sqrDistance = difference[0]*difference[0] + difference[1]*difference[1] + difference[2]*difference[2]
			if sqrDistance < 1.0:
				vertices.remove(v)
				print("These points are too close : ")
				print(u, v)


outputFile = ""
for v in vertices:
	line = '<noi:GEO_POINT x="' + str(v[0]) + '" y="'+ str(v[1]) +'" z="' + str(v[2]) + '" />\n'
	outputFile += line


fileOutput = open('Vincent2.xml', 'w')

fileOutput.write(outputFile)
			