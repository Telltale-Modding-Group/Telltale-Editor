!!ARBvp1.0

TEMP	oPos;
ATTRIB	inPosition = vertex.position;
ATTRIB	inDiffuseUV = vertex.texcoord[0];
ATTRIB	inNormal = vertex.normal;
ATTRIB	inBoneWeight = vertex.attrib[6];	# weight
ATTRIB	inBoneIndices = vertex.color;		# indices (really)
ATTRIB	inUV2 = vertex.texcoord[1];
OUTPUT	oD0 = result.color;
OUTPUT	oT0 = result.texcoord[0];
OUTPUT	oT1 = result.texcoord[1];

ADDRESS a0;
PARAM	boneMatrixRows[] = { program.env[24..95] };
PARAM	MVP[4] = { program.env[0..3] };

# Known vars: a0                      oD0  oPos  oT0  oT1              inPosition  inDiffuseUV  inNormal  inBoneWeight  inBoneIndices  v5
TEMP	r1;
TEMP	r2;
TEMP	r3;
TEMP	correctBoneIndices;
TEMP	r5;
TEMP	r6;

PARAM	amb = program.env[9]
PARAM	light0_pos_and_intensity = program.env[10];
PARAM	light0_color_and_atten = program.env[11];
PARAM	light1_pos_and_intensity = program.env[12];
PARAM	light1_color_and_atten = program.env[13];
PARAM	directionLightPos = program.env[14];
PARAM	directionLightCol = program.env[15];
PARAM	const_Translate_1_0_05_255 = program.env[20];
PARAM	bone_index_map = program.env[21]; # seems to be 256, 256, 256, 256 Vec4

# indices problem...
# xyzw
# wzyx
# zwxy
# x seems unused
# y appears to match pc .x
# z appears to match pc .x, maybe a little closer tho still some artifacts
# z appears to match pc .y very closely, no artifacts ****
# w appears to match pc .z very closely, no artifacts ****

# weights problem
# x checks out
# y checks out
# z checks out

MUL	correctBoneIndices, inBoneIndices.yzwx, bone_index_map;	# this translation converts color order to original pc

# test
#MOV oD0, inBoneIndices.yzwx;
#SGE r2, correctBoneIndices, {68,68,68,68};
#ADD oD0, r2, inBoneIndices.yzwx;

# SECT 1: SKIN MATRIX?
ARL	a0.x, correctBoneIndices.z; # z 	
DP4	r2.x, inPosition, boneMatrixRows[a0.x+0];	# mat4 prod with in position ??
DP4	r2.y, inPosition, boneMatrixRows[a0.x+1];	
DP4	r2.z, inPosition, boneMatrixRows[a0.x+2];	
DP4	r2.w, inPosition, boneMatrixRows[a0.x+3];	
MUL	r5, r2, inBoneWeight.x;
DP3	r2.x, inNormal, boneMatrixRows[a0.x+0];	
DP3	r2.y, inNormal, boneMatrixRows[a0.x+1];	
DP3	r2.z, inNormal, boneMatrixRows[a0.x+2];	
DP3	r2.w, inNormal, boneMatrixRows[a0.x+3];	
MUL	r6, r2, inBoneWeight.x;	 # * bone weight

ARL	a0.x, correctBoneIndices.y;
DP4	r2.x, inPosition, boneMatrixRows[a0.x+0];	
DP4	r2.y, inPosition, boneMatrixRows[a0.x+1];	
DP4	r2.z, inPosition, boneMatrixRows[a0.x+2];	
DP4	r2.w, inPosition, boneMatrixRows[a0.x+3];	
MAD	r5, r2, inBoneWeight.y, r5;	
DP3	r2.x, inNormal, boneMatrixRows[a0.x+0];	
DP3	r2.y, inNormal, boneMatrixRows[a0.x+1];	
DP3	r2.z, inNormal, boneMatrixRows[a0.x+2];	
DP3	r2.w, inNormal, boneMatrixRows[a0.x+3];	
MAD	r6, r2, inBoneWeight.y, r6;	

ARL	a0.x, correctBoneIndices.x;
DP4	r2.x, inPosition, boneMatrixRows[a0.x+0];	
DP4	r2.y, inPosition, boneMatrixRows[a0.x+1];	
DP4	r2.z, inPosition, boneMatrixRows[a0.x+2];	
DP4	r2.w, inPosition, boneMatrixRows[a0.x+3];	
MAD	r5, r2, inBoneWeight.z, r5;	
DP3	r2.x, inNormal, boneMatrixRows[a0.x+0];	
DP3	r2.y, inNormal, boneMatrixRows[a0.x+1];	
DP3	r2.z, inNormal, boneMatrixRows[a0.x+2];	
DP3	r2.w, inNormal, boneMatrixRows[a0.x+3];	
MAD	r6, r2, inBoneWeight.z, r6;	

#MOV r5, inPosition;
DP4	oPos.x, r5, MVP[0];	
DP4	oPos.y, r5, MVP[1];	
DP4	oPos.z, r5, MVP[2];	
DP4	oPos.w, r5, MVP[3];	

SUB	r2.xyz, light0_pos_and_intensity.xyzw, r5.xyzw;	
DP3	r2.w, r2.xyzw, r2.xyzw;	
RSQ	r2.w, r2.w;	
MUL	r2.xyz, r2.xyzw, r2.w;	
DP3	r1.x, r6, r2;	
MAX	r1.x, r1.x, const_Translate_1_0_05_255.y;	
MUL	r2.w, r2.w, light0_color_and_atten.w;	
RCP	r2.w, r2.w;	
MIN	r2.w, r2.w, const_Translate_1_0_05_255.x;	
MUL	r2.w, r2.w, r2.w;	
SUB	r2.w, const_Translate_1_0_05_255.x, r2.w;	
MUL	r2.w, r2.w, light0_pos_and_intensity.w;	
MUL	r1.x, r1.x, r2.w;	
MUL	r3.xyz, light0_color_and_atten.xyzw, r1.x;	
SUB	r2.xyz, light1_pos_and_intensity.xyzw, r5.xyzw;	
DP3	r2.w, r2.xyzw, r2.xyzw;	
RSQ	r2.w, r2.w;	
MUL	r2.xyz, r2.xyzw, r2.w;	
DP3	r1.x, r6, r2;	
MAX	r1.x, r1.x, const_Translate_1_0_05_255.y;	
MUL	r2.w, r2.w, light1_color_and_atten.w;	
RCP	r2.w, r2.w;	
MIN	r2.w, r2.w, const_Translate_1_0_05_255.x;	
MUL	r2.w, r2.w, r2.w;	
SUB	r2.w, const_Translate_1_0_05_255.x, r2.w;	
MUL	r2.w, r2.w, light1_pos_and_intensity.w;	
MUL	r1.x, r1.x, r2.w;	
MAD	r3.xyz, light1_color_and_atten.xyzw, r1.x, r3.xyzw;	
DP3	r2.x, r6, directionLightPos;	
MAD	r3.xyz, directionLightCol.xyzw, r2.x, r3.xyzw;	
ADD	r3.xyz, r3.xyzw, amb.xyzw;	
MUL	r3.xyz, r3.xyzw, const_Translate_1_0_05_255.z;	
MIN	r3.xyz, r3.xyzw, const_Translate_1_0_05_255.x;	
MAX	r3.xyz, r3.xyzw, const_Translate_1_0_05_255.y;	
MOV	oD0.xyz, r3.xyzw;	
MOV	oD0.w, const_Translate_1_0_05_255.x;	
MOV	oT0, inDiffuseUV;	
MOV	oT1, inUV2;	

MOV result.position, oPos;
END
