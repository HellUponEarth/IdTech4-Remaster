
#ifndef __GAMETYPEINFO_H__
#define __GAMETYPEINFO_H__

#include <stddef.h>

/*
===================================================================================

	This file has been generated with the Type Info Generator v1.0 (c) 2004 id Software

	118 constants
	1 enums
	18 classes/structs/unions
	0 templates
	1 max inheritance level for 'idAI_Vagary'

===================================================================================
*/

typedef struct {
	const char * name;
	const char * type;
	const char * value;
} constantInfo_t;

typedef struct {
	const char * name;
	int value;
} enumValueInfo_t;

typedef struct {
	const char * typeName;
	const enumValueInfo_t * values;
} enumTypeInfo_t;

typedef struct {
	const char * type;
	const char * name;
	size_t offset;
	size_t size;
} classVariableInfo_t;

typedef struct {
	const char * typeName;
	const char * superType;
	size_t size;
	const classVariableInfo_t * variables;
} classTypeInfo_t;

static constantInfo_t constantInfo[] = {
	{ "const int", "maxWalkPathIterations", "10" },
	{ "const float", "maxWalkPathDistance", "500.0" },
	{ "const float", "walkPathSampleDistance", "8.0" },
	{ "const int", "maxFlyPathIterations", "10" },
	{ "const float", "maxFlyPathDistance", "500.0" },
	{ "const float", "flyPathSampleDistance", "8.0" },
	{ "const float", "MAX_OBSTACLE_RADIUS", "256.0" },
	{ "const float", "PUSH_OUTSIDE_OBSTACLES", "0.5" },
	{ "const float", "CLIP_BOUNDS_EPSILON", "10.0" },
	{ "const int", "MAX_AAS_WALL_EDGES", "256" },
	{ "const int", "MAX_OBSTACLES", "256" },
	{ "const int", "MAX_PATH_NODES", "256" },
	{ "const int", "MAX_OBSTACLE_PATH", "64" },
	{ "const float", "OVERCLIP", "1.001" },
	{ "const int", "MAX_FRAME_SLIDE", "5" },
	{ "static exporterInterface_t", "Maya_ConvertModel", "0" },
	{ "static exporterShutdown_t", "Maya_Shutdown", "0" },
	{ "static int", "importDLL", "0" },
	{ "static idTypeInfo *", "typelist", "0" },
	{ "static int", "eventCallbackMemory", "0" },
	{ "static bool", "eventError", "0" },
	{ "END_CLASS const float", "ERROR_REDUCTION", "0.5" },
	{ "const float", "ERROR_REDUCTION_MAX", "256.0" },
	{ "const float", "LIMIT_ERROR_REDUCTION", "0.3" },
	{ "const float", "LCP_EPSILON", "1e-7" },
	{ "const float", "LIMIT_LCP_EPSILON", "1e-4" },
	{ "const float", "CONTACT_LCP_EPSILON", "1e-6" },
	{ "const float", "CENTER_OF_MASS_EPSILON", "1e-4" },
	{ "const float", "NO_MOVE_TIME", "1.0" },
	{ "const float", "NO_MOVE_TRANSLATION_TOLERANCE", "10.0" },
	{ "const float", "NO_MOVE_ROTATION_TOLERANCE", "10.0" },
	{ "const float", "MIN_MOVE_TIME", "-1.0" },
	{ "const float", "MAX_MOVE_TIME", "-1.0" },
	{ "const float", "IMPULSE_THRESHOLD", "500.0" },
	{ "const float", "SUSPEND_LINEAR_VELOCITY", "10.0" },
	{ "const float", "SUSPEND_ANGULAR_VELOCITY", "15.0" },
	{ "const float", "SUSPEND_LINEAR_ACCELERATION", "20.0" },
	{ "const float", "SUSPEND_ANGULAR_ACCELERATION", "30.0" },
	{ "const idVec6", "vec6_lcp_epsilon", "0(0,0,0,0,0,0)" },
	{ "static int", "lastTimerReset", "0" },
	{ "static int", "numArticulatedFigures", "0" },
	{ "const float", "AF_VELOCITY_MAX", "16000" },
	{ "const int", "AF_VELOCITY_TOTAL_BITS", "16" },
	{ "const int", "AF_VELOCITY_MANTISSA_BITS", "16-1-0" },
	{ "const float", "AF_FORCE_MAX", "1e20" },
	{ "const int", "AF_FORCE_TOTAL_BITS", "16" },
	{ "const int", "AF_FORCE_MANTISSA_BITS", "16-1-0" },
	{ "END_CLASS const float", "OVERCLIP", "1.001" },
	{ "const float", "MONSTER_VELOCITY_MAX", "4000" },
	{ "const int", "MONSTER_VELOCITY_TOTAL_BITS", "16" },
	{ "const int", "MONSTER_VELOCITY_MANTISSA_BITS", "16-1-0" },
	{ "END_CLASS const float", "PM_STOPSPEED", "100.0" },
	{ "const float", "PM_SWIMSCALE", "0.5" },
	{ "const float", "PM_LADDERSPEED", "100.0" },
	{ "const float", "PM_STEPSCALE", "1.0" },
	{ "const float", "PM_ACCELERATE", "10.0" },
	{ "const float", "PM_AIRACCELERATE", "1.0" },
	{ "const float", "PM_WATERACCELERATE", "4.0" },
	{ "const float", "PM_FLYACCELERATE", "8.0" },
	{ "const float", "PM_FRICTION", "6.0" },
	{ "const float", "PM_AIRFRICTION", "0.0" },
	{ "const float", "PM_WATERFRICTION", "1.0" },
	{ "const float", "PM_FLYFRICTION", "3.0" },
	{ "const float", "PM_NOCLIPFRICTION", "12.0" },
	{ "const float", "MIN_WALK_NORMAL", "0.7" },
	{ "const float", "OVERCLIP", "1.001" },
	{ "const int", "PMF_DUCKED", "1" },
	{ "const int", "PMF_JUMPED", "2" },
	{ "const int", "PMF_STEPPED_UP", "4" },
	{ "const int", "PMF_STEPPED_DOWN", "8" },
	{ "const int", "PMF_JUMP_HELD", "16" },
	{ "const int", "PMF_TIME_LAND", "32" },
	{ "const int", "PMF_TIME_KNOCKBACK", "64" },
	{ "const int", "PMF_TIME_WATERJUMP", "128" },
	{ "const int", "PMF_ALL_TIMES", "(128|32|64)" },
	{ "const float", "PLAYER_VELOCITY_MAX", "4000" },
	{ "const int", "PLAYER_VELOCITY_TOTAL_BITS", "16" },
	{ "const int", "PLAYER_VELOCITY_MANTISSA_BITS", "16-1-0" },
	{ "const int", "PLAYER_MOVEMENT_TYPE_BITS", "3" },
	{ "const int", "PLAYER_MOVEMENT_FLAGS_BITS", "8" },
	{ "END_CLASS const float", "STOP_SPEED", "10.0" },
	{ "const float", "RB_VELOCITY_MAX", "16000" },
	{ "const int", "RB_VELOCITY_TOTAL_BITS", "16" },
	{ "const int", "RB_VELOCITY_MANTISSA_BITS", "16-1-0" },
	{ "const float", "RB_MOMENTUM_MAX", "1e20" },
	{ "const int", "RB_MOMENTUM_TOTAL_BITS", "16" },
	{ "const int", "RB_MOMENTUM_MANTISSA_BITS", "16-1-0" },
	{ "const float", "RB_FORCE_MAX", "1e20" },
	{ "const int", "RB_FORCE_TOTAL_BITS", "16" },
	{ "const int", "RB_FORCE_MANTISSA_BITS", "16-1-0" },
	{ "int", "PUSH_NO", "0" },
	{ "int", "PUSH_OK", "1" },
	{ "int", "PUSH_BLOCKED", "2" },
	{ "END_CLASS static const float", "BOUNCE_SOUND_MIN_VELOCITY", "80.0" },
	{ "static const float", "BOUNCE_SOUND_MAX_VELOCITY", "200.0" },
	{ "END_CLASS const int", "SHARD_ALIVE_TIME", "5000" },
	{ "const int", "SHARD_FADE_START", "2000" },
	{ "static const char *", "brittleFracture_SnapshotName", "_BrittleFracture_Snapshot_" },
	{ "END_CLASS static const float", "BOUNCE_SOUND_MIN_VELOCITY", "80.0" },
	{ "static const float", "BOUNCE_SOUND_MAX_VELOCITY", "200.0" },
	{ "const int", "LADDER_RUNG_DISTANCE", "32" },
	{ "const int", "HEALTH_PER_DOSE", "10" },
	{ "const int", "WEAPON_DROP_TIME", "20*1000" },
	{ "const int", "WEAPON_SWITCH_DELAY", "150" },
	{ "const int", "SPECTATE_RAISE", "25" },
	{ "const int", "HEALTHPULSE_TIME", "333" },
	{ "const float", "MIN_BOB_SPEED", "5.0" },
	{ "END_CLASS const int", "MAX_RESPAWN_TIME", "10000" },
	{ "const int", "RAGDOLL_DEATH_TIME", "3000" },
	{ "const int", "MAX_PDAS", "64" },
	{ "const int", "MAX_PDA_ITEMS", "128" },
	{ "const int", "STEPUP_TIME", "200" },
	{ "const int", "MAX_INVENTORY_ITEMS", "20" },
	{ "const int", "IMPULSE_DELAY", "150" },
	{ "static const int", "BFG_DAMAGE_FREQUENCY", "333" },
	{ "static const float", "BOUNCE_SOUND_MIN_VELOCITY", "200.0" },
	{ "static const float", "BOUNCE_SOUND_MAX_VELOCITY", "400.0" },
	{ "static const char *", "smokeParticle_SnapshotName", "_SmokeParticle_Snapshot_" },
	{ NULL, NULL, NULL }
};

static enumValueInfo_t enum_0_typeInfo[] = {
	{ "PUSH_NO", 0 },
	{ "PUSH_OK", 1 },
	{ "PUSH_BLOCKED", 2 },
	{ NULL, 0 }
};

static enumTypeInfo_t enumTypeInfo[] = {
	{ "enum_0", enum_0_typeInfo },
	{ NULL, NULL }
};

static classVariableInfo_t wallEdge_t_typeInfo[] = {
	{ "int", "edgeNum", offsetof( wallEdge_t, edgeNum ), sizeof( ((wallEdge_t *)0)->edgeNum ) },
	{ "int[2]", "verts", offsetof( wallEdge_t, verts ), sizeof( ((wallEdge_t *)0)->verts ) },
	{ "wallEdge_s *", "next", offsetof( wallEdge_t, next ), sizeof( ((wallEdge_t *)0)->next ) },
	{ NULL, 0 }
};

static classVariableInfo_t obstacle_t_typeInfo[] = {
	{ "idVec2[2]", "bounds", offsetof( obstacle_t, bounds ), sizeof( ((obstacle_t *)0)->bounds ) },
	{ "idWinding2D", "winding", offsetof( obstacle_t, winding ), sizeof( ((obstacle_t *)0)->winding ) },
	{ "idEntity *", "entity", offsetof( obstacle_t, entity ), sizeof( ((obstacle_t *)0)->entity ) },
	{ NULL, 0 }
};

static classVariableInfo_t pathNode_t_typeInfo[] = {
	{ "int", "dir", offsetof( pathNode_t, dir ), sizeof( ((pathNode_t *)0)->dir ) },
	{ "idVec2", "pos", offsetof( pathNode_t, pos ), sizeof( ((pathNode_t *)0)->pos ) },
	{ "idVec2", "delta", offsetof( pathNode_t, delta ), sizeof( ((pathNode_t *)0)->delta ) },
	{ "float", "dist", offsetof( pathNode_t, dist ), sizeof( ((pathNode_t *)0)->dist ) },
	{ "int", "obstacle", offsetof( pathNode_t, obstacle ), sizeof( ((pathNode_t *)0)->obstacle ) },
	{ "int", "edgeNum", offsetof( pathNode_t, edgeNum ), sizeof( ((pathNode_t *)0)->edgeNum ) },
	{ "int", "numNodes", offsetof( pathNode_t, numNodes ), sizeof( ((pathNode_t *)0)->numNodes ) },
	{ "pathNode_s *", "parent", offsetof( pathNode_t, parent ), sizeof( ((pathNode_t *)0)->parent ) },
	{ "pathNode_s *[2]", "children", offsetof( pathNode_t, children ), sizeof( ((pathNode_t *)0)->children ) },
	{ "pathNode_s *", "next", offsetof( pathNode_t, next ), sizeof( ((pathNode_t *)0)->next ) },
	{ NULL, 0 }
};

static classVariableInfo_t pathTrace_t_typeInfo[] = {
	{ "float", "fraction", offsetof( pathTrace_t, fraction ), sizeof( ((pathTrace_t *)0)->fraction ) },
	{ "idVec3", "endPos", offsetof( pathTrace_t, endPos ), sizeof( ((pathTrace_t *)0)->endPos ) },
	{ "idVec3", "normal", offsetof( pathTrace_t, normal ), sizeof( ((pathTrace_t *)0)->normal ) },
	{ "const idEntity *", "blockingEntity", offsetof( pathTrace_t, blockingEntity ), sizeof( ((pathTrace_t *)0)->blockingEntity ) },
	{ NULL, 0 }
};

static classVariableInfo_t ballistics_t_typeInfo[] = {
	{ "float", "angle", offsetof( ballistics_t, angle ), sizeof( ((ballistics_t *)0)->angle ) },
	{ "float", "time", offsetof( ballistics_t, time ), sizeof( ((ballistics_t *)0)->time ) },
	{ NULL, 0 }
};

static classVariableInfo_t idAI_Vagary_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t gameDebugLine_t_typeInfo[] = {
	{ "bool", "used", offsetof( gameDebugLine_t, used ), sizeof( ((gameDebugLine_t *)0)->used ) },
	{ "idVec3", "start", offsetof( gameDebugLine_t, start ), sizeof( ((gameDebugLine_t *)0)->start ) },
	{ "idVec3", "end", offsetof( gameDebugLine_t, end ), sizeof( ((gameDebugLine_t *)0)->end ) },
	{ "int", "color", offsetof( gameDebugLine_t, color ), sizeof( ((gameDebugLine_t *)0)->color ) },
	{ "bool", "blink", offsetof( gameDebugLine_t, blink ), sizeof( ((gameDebugLine_t *)0)->blink ) },
	{ "bool", "arrow", offsetof( gameDebugLine_t, arrow ), sizeof( ((gameDebugLine_t *)0)->arrow ) },
	{ NULL, 0 }
};

static classVariableInfo_t gameVersion_s_typeInfo[] = {
	{ "char[256]", "string", offsetof( gameVersion_s, string ), sizeof( ((gameVersion_s *)0)->string ) },
	{ NULL, 0 }
};

static classVariableInfo_t idTypeInfoTools_typeInfo[] = {
	{ NULL, 0 }
};

static classVariableInfo_t clipSector_t_typeInfo[] = {
	{ "int", "axis", offsetof( clipSector_t, axis ), sizeof( ((clipSector_t *)0)->axis ) },
	{ "float", "dist", offsetof( clipSector_t, dist ), sizeof( ((clipSector_t *)0)->dist ) },
	{ "clipSector_s *[2]", "children", offsetof( clipSector_t, children ), sizeof( ((clipSector_t *)0)->children ) },
	{ "clipLink_s *", "clipLinks", offsetof( clipSector_t, clipLinks ), sizeof( ((clipSector_t *)0)->clipLinks ) },
	{ NULL, 0 }
};

static classVariableInfo_t clipLink_t_typeInfo[] = {
	{ "idClipModel *", "clipModel", offsetof( clipLink_t, clipModel ), sizeof( ((clipLink_t *)0)->clipModel ) },
	{ "clipSector_s *", "sector", offsetof( clipLink_t, sector ), sizeof( ((clipLink_t *)0)->sector ) },
	{ "clipLink_s *", "prevInSector", offsetof( clipLink_t, prevInSector ), sizeof( ((clipLink_t *)0)->prevInSector ) },
	{ "clipLink_s *", "nextInSector", offsetof( clipLink_t, nextInSector ), sizeof( ((clipLink_t *)0)->nextInSector ) },
	{ "clipLink_s *", "nextLink", offsetof( clipLink_t, nextLink ), sizeof( ((clipLink_t *)0)->nextLink ) },
	{ NULL, 0 }
};

static classVariableInfo_t trmCache_t_typeInfo[] = {
	{ "idTraceModel", "trm", offsetof( trmCache_t, trm ), sizeof( ((trmCache_t *)0)->trm ) },
	{ "int", "refCount", offsetof( trmCache_t, refCount ), sizeof( ((trmCache_t *)0)->refCount ) },
	{ "float", "volume", offsetof( trmCache_t, volume ), sizeof( ((trmCache_t *)0)->volume ) },
	{ "idVec3", "centerOfMass", offsetof( trmCache_t, centerOfMass ), sizeof( ((trmCache_t *)0)->centerOfMass ) },
	{ "idMat3", "inertiaTensor", offsetof( trmCache_t, inertiaTensor ), sizeof( ((trmCache_t *)0)->inertiaTensor ) },
	{ NULL, 0 }
};

static classVariableInfo_t listParms_t_typeInfo[] = {
	{ "idBounds", "bounds", offsetof( listParms_t, bounds ), sizeof( ((listParms_t *)0)->bounds ) },
	{ "int", "contentMask", offsetof( listParms_t, contentMask ), sizeof( ((listParms_t *)0)->contentMask ) },
	{ "idClipModel * *", "list", offsetof( listParms_t, list ), sizeof( ((listParms_t *)0)->list ) },
	{ "int", "count", offsetof( listParms_t, count ), sizeof( ((listParms_t *)0)->count ) },
	{ "int", "maxCount", offsetof( listParms_t, maxCount ), sizeof( ((listParms_t *)0)->maxCount ) },
	{ NULL, 0 }
};

static classVariableInfo_t jointTransformData_t_typeInfo[] = {
	{ "renderEntity_t *", "ent", offsetof( jointTransformData_t, ent ), sizeof( ((jointTransformData_t *)0)->ent ) },
	{ "const idMD5Joint *", "joints", offsetof( jointTransformData_t, joints ), sizeof( ((jointTransformData_t *)0)->joints ) },
	{ NULL, 0 }
};

static classVariableInfo_t pvsPassage_t_typeInfo[] = {
	{ "byte *", "canSee", offsetof( pvsPassage_t, canSee ), sizeof( ((pvsPassage_t *)0)->canSee ) },
	{ NULL, 0 }
};

static classVariableInfo_t pvsPortal_t_typeInfo[] = {
	{ "int", "areaNum", offsetof( pvsPortal_t, areaNum ), sizeof( ((pvsPortal_t *)0)->areaNum ) },
	{ "idWinding *", "w", offsetof( pvsPortal_t, w ), sizeof( ((pvsPortal_t *)0)->w ) },
	{ "idBounds", "bounds", offsetof( pvsPortal_t, bounds ), sizeof( ((pvsPortal_t *)0)->bounds ) },
	{ "idPlane", "plane", offsetof( pvsPortal_t, plane ), sizeof( ((pvsPortal_t *)0)->plane ) },
	{ "pvsPassage_t *", "passages", offsetof( pvsPortal_t, passages ), sizeof( ((pvsPortal_t *)0)->passages ) },
	{ "bool", "done", offsetof( pvsPortal_t, done ), sizeof( ((pvsPortal_t *)0)->done ) },
	{ "byte *", "vis", offsetof( pvsPortal_t, vis ), sizeof( ((pvsPortal_t *)0)->vis ) },
	{ "byte *", "mightSee", offsetof( pvsPortal_t, mightSee ), sizeof( ((pvsPortal_t *)0)->mightSee ) },
	{ NULL, 0 }
};

static classVariableInfo_t pvsArea_t_typeInfo[] = {
	{ "int", "numPortals", offsetof( pvsArea_t, numPortals ), sizeof( ((pvsArea_t *)0)->numPortals ) },
	{ "idBounds", "bounds", offsetof( pvsArea_t, bounds ), sizeof( ((pvsArea_t *)0)->bounds ) },
	{ "pvsPortal_t * *", "portals", offsetof( pvsArea_t, portals ), sizeof( ((pvsArea_t *)0)->portals ) },
	{ NULL, 0 }
};

static classVariableInfo_t pvsStack_t_typeInfo[] = {
	{ "pvsStack_s *", "next", offsetof( pvsStack_t, next ), sizeof( ((pvsStack_t *)0)->next ) },
	{ "byte *", "mightSee", offsetof( pvsStack_t, mightSee ), sizeof( ((pvsStack_t *)0)->mightSee ) },
	{ NULL, 0 }
};

static classTypeInfo_t classTypeInfo[] = {
	{ "wallEdge_t", "", sizeof(wallEdge_t), wallEdge_t_typeInfo },
	{ "obstacle_t", "", sizeof(obstacle_t), obstacle_t_typeInfo },
	{ "pathNode_t", "", sizeof(pathNode_t), pathNode_t_typeInfo },
	{ "pathTrace_t", "", sizeof(pathTrace_t), pathTrace_t_typeInfo },
	{ "ballistics_t", "", sizeof(ballistics_t), ballistics_t_typeInfo },
	{ "idAI_Vagary", "idAI", sizeof(idAI_Vagary), idAI_Vagary_typeInfo },
	{ "gameDebugLine_t", "", sizeof(gameDebugLine_t), gameDebugLine_t_typeInfo },
	{ "gameVersion_s", "", sizeof(gameVersion_s), gameVersion_s_typeInfo },
	{ "idTypeInfoTools", "", sizeof(idTypeInfoTools), idTypeInfoTools_typeInfo },
	{ "clipSector_t", "", sizeof(clipSector_t), clipSector_t_typeInfo },
	{ "clipLink_t", "", sizeof(clipLink_t), clipLink_t_typeInfo },
	{ "trmCache_t", "", sizeof(trmCache_t), trmCache_t_typeInfo },
	{ "listParms_t", "", sizeof(listParms_t), listParms_t_typeInfo },
	{ "jointTransformData_t", "", sizeof(jointTransformData_t), jointTransformData_t_typeInfo },
	{ "pvsPassage_t", "", sizeof(pvsPassage_t), pvsPassage_t_typeInfo },
	{ "pvsPortal_t", "", sizeof(pvsPortal_t), pvsPortal_t_typeInfo },
	{ "pvsArea_t", "", sizeof(pvsArea_t), pvsArea_t_typeInfo },
	{ "pvsStack_t", "", sizeof(pvsStack_t), pvsStack_t_typeInfo },
	{ NULL, NULL, 0, NULL }
};

#endif /* !__GAMETYPEINFO_H__ */
