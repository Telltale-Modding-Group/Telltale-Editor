!!ARBvp1.0

TEMP	oPos;
ATTRIB	inpos = vertex.position;
ATTRIB	diffUV = vertex.texcoord[0];
ATTRIB	normal = vertex.normal;
ATTRIB	lightMapUV = vertex.texcoord[1];
OUTPUT	oD0 = result.color;
OUTPUT	oT0 = result.texcoord[0];
OUTPUT	oT1 = result.texcoord[1];

PARAM	MVP[4] = { program.env[0..3] };
PARAM	UV_Trans[4] = { program.env[4..7] };

DP4	oPos.x, inpos, MVP[0];	
DP4	oPos.y, inpos, MVP[1];	
DP4	oPos.z, inpos, MVP[2];	
DP4	oPos.w, inpos, MVP[3];	

# Known vars:     oD0  oT0  oT1  diffUV  lightMapUV

PARAM	c20 = program.env[20];

MOV	oD0.xyz, c20.y;	
MOV	oD0.w, c20.z;	
DP4	oT1.x, diffUV, UV_Trans[0];	
DP4	oT1.y, diffUV, UV_Trans[1];	
DP4	oT1.z, diffUV, UV_Trans[2];	
DP4	oT1.w, diffUV, UV_Trans[3];	
MOV	oT0, lightMapUV;	

MOV result.position, oPos;
END
