!!ARBvp1.0

TEMP	oPos;
ATTRIB	inPos = vertex.position;
ATTRIB	inDiffuseUV = vertex.texcoord[0];
ATTRIB	inNormal = vertex.normal;
ATTRIB	v5 = vertex.texcoord[1];
OUTPUT	oD0 = result.color;
OUTPUT	oT0 = result.texcoord[0];
OUTPUT	oT1 = result.texcoord[1];

PARAM	MVP[4] = { program.env[0..3] };

DP4	oPos.x, inPos, MVP[0];	
DP4	oPos.y, inPos, MVP[1];	
DP4	oPos.z, inPos, MVP[2];	
DP4	oPos.w, inPos, MVP[3];	

# Known vars:                   oD0  oT0  oT1        inPos  inDiffuseUV  inNormal  v5
TEMP	r0;
TEMP	r1;
TEMP	r2;

PARAM	uvTransformMat4[4] = { program.env[4..7] };
PARAM	c9 = program.env[9]; # light ?
PARAM	c10 = program.env[10]; # light ?
PARAM	c11 = program.env[11]; # light ?
PARAM	c12 = program.env[12]; # light ?
PARAM	c13 = program.env[13]; # light ?
PARAM	c14 = program.env[14]; # light ?
PARAM	c15 = program.env[15]; # light ?
PARAM	other_map = program.env[20];

SUB	r2.xyz, c10.xyzw, inPos.xyzw;	
DP3	r2.w, r2.xyzw, r2.xyzw;	
RSQ	r2.w, r2.w;	
MUL	r2.xyz, r2.xyzw, r2.w;	
DP3	r1.x, inNormal, r2;	
MAX	r1.x, r1.x, other_map.y;	
MUL	r2.w, r2.w, c11.w;	
RCP	r2.w, r2.w;	
MIN	r2.w, r2.w, other_map.x;	
MUL	r2.w, r2.w, r2.w;	
SUB	r2.w, other_map.x, r2.w;	
MUL	r2.w, r2.w, c10.w;	
MUL	r1.x, r1.x, r2.w;	
MUL	r0.xyz, c11.xyzw, r1.x;	
SUB	r2.xyz, c12.xyzw, inPos.xyzw;	
DP3	r2.w, r2.xyzw, r2.xyzw;	
RSQ	r2.w, r2.w;	
MUL	r2.xyz, r2.xyzw, r2.w;	
DP3	r1.x, inNormal, r2;	
MAX	r1.x, r1.x, other_map.y;	
MUL	r2.w, r2.w, c13.w;	
RCP	r2.w, r2.w;	
MIN	r2.w, r2.w, other_map.x;	
MUL	r2.w, r2.w, r2.w;	
SUB	r2.w, other_map.x, r2.w;	
MUL	r2.w, r2.w, c12.w;	
MUL	r1.x, r1.x, r2.w;	
MAD	r0.xyz, c13.xyzw, r1.x, r0.xyzw;	
DP3	r2.x, inNormal, c14;	
MAD	r0.xyz, c15.xyzw, r2.x, r0.xyzw;	
ADD	r0.xyz, r0.xyzw, c9.xyzw;	
MUL	r0.xyz, r0.xyzw, other_map.z;	
MIN	r0.xyz, r0.xyzw, other_map.x;	
MAX	r0.xyz, r0.xyzw, other_map.y;	
MOV	oD0.xyz, r0.xyzw;	
MOV	oD0.w, other_map.x;	
#M4X4	oT0, inDiffuseUV, uvTransformMat4;	
DP4	oT0.x, inDiffuseUV, uvTransformMat4[0];	
DP4	oT0.y, inDiffuseUV, uvTransformMat4[1];	
DP4	oT0.z, inDiffuseUV, uvTransformMat4[2];	
DP4	oT0.w, inDiffuseUV, uvTransformMat4[3];	
MOV	oT1, v5;	

MOV result.position, oPos;
END
