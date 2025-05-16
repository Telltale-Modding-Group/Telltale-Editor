!!ARBvp1.0

TEMP	oPos;
ATTRIB	v0 = vertex.position;
ATTRIB	v1 = vertex.texcoord[0];
ATTRIB	v2 = vertex.normal;
ATTRIB	v5 = vertex.texcoord[1];
OUTPUT	oD0 = result.color.primary;
OUTPUT	oD1 = result.color.secondary;
OUTPUT	oT0 = result.texcoord[0];
OUTPUT	oT1 = result.texcoord[1];
OUTPUT	oT2 = result.texcoord[2];
OUTPUT	oT3 = result.texcoord[3];

PARAM	c0[4] = { program.env[0..3] };

DP4	oPos.x, v0, c0[0];	
DP4	oPos.y, v0, c0[1];	
DP4	oPos.z, v0, c0[2];	
DP4	oPos.w, v0, c0[3];	

# Known vars:                     oD0  oD1  oPos  oT0  oT1  oT2  oT3              v0  v1  v2  v5
TEMP	r0;
TEMP	r1;
TEMP	r2;
TEMP	r3;
TEMP	r4;
TEMP	r5;

PARAM	c4[4] = { program.env[4..7] };
PARAM	c8 = program.env[8];
PARAM	c10 = program.env[10];
PARAM	c11 = program.env[11];
PARAM	c16[4] = { program.env[16..19] };
PARAM	c20 = program.env[20];
PARAM	c21 = program.env[21];
PARAM	c22 = program.env[22];
PARAM	c23 = program.env[23];

MOV	oD0.xyz, c20.y;	
MOV	oD0.w, c20.z;	
MOV	oT0, v5;	
DP4	r0.x, v1, c4[0];	
DP4	r0.y, v1, c4[1];	
DP4	r0.z, v1, c4[2];	
DP4	r0.w, v1, c4[3];	
MOV	oT1, r0;	
MOV	oT2, r0;	
DP4	r0.x, v0, c16[0];	
DP4	r0.y, v0, c16[1];	
DP4	r0.z, v0, c16[2];	
DP4	r0.w, v0, c16[3];	
SUB	r2, c22, r0;	
DP3	r4.w, r2.xyzw, r2.xyzw;	
RSQ	r4.w, r4.w;	
MUL	r4.w, r4.w, c11.w;	
RCP	r4.w, r4.w;	
MIN	r4.w, r4.w, c20.x;	
MUL	r4.w, r4.w, r4.w;	
SUB	r4.w, c20.x, r4.w;	
MUL	r4.w, r4.w, c10.w;	
MUL	r5, c23, r4.w;	
MOV	r4, c8;	
MUL	r5, r5, r4.w;	
MAD	r5, c23, r4.x, r5;	
DP3	r2.w, r2.xyzw, r2.xyzw;	
RSQ	r2.w, r2.w;	
MUL	r2.xyz, r2.xyzw, r2.w;	
SUB	r0, r0, c21;	
DP3	r0.w, r0.xyzw, r0.xyzw;	
RSQ	r0.w, r0.w;	
MUL	r0.xyz, r0.xyzw, r0.w;	
DP3	r1.x, v2, c16[0];	
DP3	r1.y, v2, c16[1];	
DP3	r1.z, v2, c16[2];	
DP3	r0.w, r0, r1;	
ADD	r0.w, r0.w, r0.w;	
MAD	r0.xyz, r1.xyzw, -r0.w, r0.xyzw;	
DP3	r0.w, r0.xyzw, r0.xyzw;	
RSQ	r0.w, r0.w;	
MUL	r0.xyz, r0.xyzw, r0.w;	
DP3	r0.w, r0.xyzw, r2.xyzw;	
SGE	r0.w, r0.w, c20.y;	
MUL	oD1.w, r0.w, r5.w;	
MOV	r1, c20.y;	
MOV	r1.x, c20.x;	
MUL	r3, r1.zxyw, -r2.yzxw;	
MAD	r3, r1.yzxw, -r2.zxyw, -r3;	
DP3	r3.w, r3.xyzw, r3.xyzw;	
RSQ	r3.w, r3.w;	
MUL	r3.xyz, r3.xyzw, r3.w;	
MUL	r1, r2.zxyw, -r3.yzxw;	
MAD	r1, r2.yzxw, -r3.zxyw, -r1;	
DP3	r1.w, r1.xyzw, r1.xyzw;	
RSQ	r1.w, r1.w;	
MUL	r1.xyz, r1.xyzw, r1.w;	
DP3	r2.x, r1.xyzw, r0.xyzw;	
DP3	r2.y, r3.xyzw, r0.xyzw;	
MAD	r0, r2, c20.z, c20.z;	
MOV	oT3, r0;	

MOV result.position, oPos;
END
