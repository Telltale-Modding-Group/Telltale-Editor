!!ARBvp1.0

TEMP	oPos;
ATTRIB	v0 = vertex.position;
ATTRIB	v1 = vertex.texcoord[0];
OUTPUT	oD0 = result.color;
OUTPUT	oT0 = result.texcoord[0];

PARAM	c0[4] = { program.env[0..3] };


# Known vars:           oD0  oPos  oT0    v0  v1
TEMP	r0;

PARAM	c10 = program.env[10];
PARAM	c11 = program.env[11];
PARAM	c12 = program.env[12];
PARAM	c20 = program.env[20];

MOV	r0, v0;	
MUL	r0.xy, r0.xyzw, c11.xyzw;	
ADD	r0.xy, r0.xyzw, c10.xyzw;	
DP4	oPos.x, r0, c0[0];	
DP4	oPos.y, r0, c0[1];	
DP4	oPos.z, r0, c0[2];	
DP4	oPos.w, r0, c0[3];	
MOV	oD0.xyzw, c12;	
MOV	oD0.w, c20.x;	
MOV	oT0, v1;	

MOV result.position, oPos;
END
