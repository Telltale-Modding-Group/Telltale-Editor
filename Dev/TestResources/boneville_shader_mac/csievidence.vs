!!ARBvp1.0

TEMP	oPos;
ATTRIB	v0 = vertex.position;
ATTRIB	v1 = vertex.texcoord[0];
ATTRIB	v2 = vertex.normal;
ATTRIB	v6 = vertex.attrib[6]; # ALPHA in
OUTPUT	oD0 = result.color;
OUTPUT	oT0 = result.texcoord[0];

PARAM	c0[4] = { program.env[0..3] };

DP4	oPos.x, v0, c0[0];	
DP4	oPos.y, v0, c0[1];	
DP4	oPos.z, v0, c0[2];	
DP4	oPos.w, v0, c0[3];	

# Known vars:                   oD0  oT0        v0  v1  v2  v6
TEMP	r0;
TEMP	r1;
TEMP	r2;

PARAM	c4[4] = { program.env[4..7] };
PARAM	c9 = program.env[9];
PARAM	c10 = program.env[10];
PARAM	c11 = program.env[11];
PARAM	c12 = program.env[12];
PARAM	c13 = program.env[13];
PARAM	c14 = program.env[14];
PARAM	c15 = program.env[15];
PARAM	c20 = program.env[20];

SUB	r2.xyz, c10.xyzw, v0.xyzw;	
DP3	r2.w, r2.xyzw, r2.xyzw;	
RSQ	r2.w, r2.w;	
MUL	r2.xyz, r2.xyzw, r2.w;	
DP3	r1.x, v2, r2;	
MAX	r1.x, r1.x, c20.y;	
MUL	r2.w, r2.w, c11.w;	
RCP	r2.w, r2.w;	
MIN	r2.w, r2.w, c20.x;	
MUL	r2.w, r2.w, r2.w;	
SUB	r2.w, c20.x, r2.w;	
MUL	r2.w, r2.w, c10.w;	
MUL	r1.x, r1.x, r2.w;	
MUL	r0.xyz, c11.xyzw, r1.x;	
SUB	r2.xyz, c12.xyzw, v0.xyzw;	
DP3	r2.w, r2.xyzw, r2.xyzw;	
RSQ	r2.w, r2.w;	
MUL	r2.xyz, r2.xyzw, r2.w;	
DP3	r1.x, v2, r2;	
MAX	r1.x, r1.x, c20.y;	
MUL	r2.w, r2.w, c13.w;	
RCP	r2.w, r2.w;	
MIN	r2.w, r2.w, c20.x;	
MUL	r2.w, r2.w, r2.w;	
SUB	r2.w, c20.x, r2.w;	
MUL	r2.w, r2.w, c12.w;	
MUL	r1.x, r1.x, r2.w;	
MAD	r0.xyz, c13.xyzw, r1.x, r0.xyzw;	
DP3	r2.x, v2, c14;	
MAD	r0.xyz, c15.xyzw, r2.x, r0.xyzw;	
ADD	r0.xyz, r0.xyzw, c9.xyzw;	
MUL	r0.xyz, r0.xyzw, c20.z;	
MIN	r0.xyz, r0.xyzw, c20.x;	
MAX	r0.xyz, r0.xyzw, c20.y;	
MOV	oD0.xyz, r0.xyzw;	
MOV	oD0.w, v6.x;	
#M4X4	oT0, v1, c4;	
DP4	oT0.x, v1, c4[0];	
DP4	oT0.y, v1, c4[1];	
DP4	oT0.z, v1, c4[2];	
DP4	oT0.w, v1, c4[3];	

MOV result.position, oPos;
END
