!!ARBvp1.0
# Known vars: 1      oD0  oPos  oT0  v0  v1

TEMP	oPos;
ATTRIB	v0 = vertex.position;
ATTRIB	v1 = vertex.texcoord[0];
OUTPUT	oD0 = result.color;
OUTPUT	oT0 = result.texcoord[0];

PARAM	world[4] = { program.env[0..3] };
PARAM	color = program.env[9];

#LINE	1 "Text.vsh";	
DP4	oPos.x, v0, world[0];	
DP4	oPos.y, v0, world[1];	
DP4	oPos.z, v0, world[2];	
DP4	oPos.w, v0, world[3];	
MOV	oT0, v1;	
MOV	oD0, color;	

MOV result.position, oPos;
END
