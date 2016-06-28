#include "cbase.h"
#include "tf_weapon_stickbomb.h"

#ifdef CLIENT_DLL
#include "c_tf_player.h"
#else
#include "tf_player.h"
#include "tf_fx.h"
#include "takedamageinfo.h"
#include "tf_gamerules.h"
#endif


IMPLEMENT_NETWORKCLASS_ALIASED(TFStickBomb, DT_TFWeaponStickBomb);

BEGIN_NETWORK_TABLE(CTFStickBomb, DT_TFWeaponStickBomb)
#ifdef CLIENT_DLL
	RecvPropInt(RECVINFO(m_iDetonated))
#else
	SendPropInt(SENDINFO(m_iDetonated))
#endif
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CTFStickBomb)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(tf_weapon_stickbomb, CTFStickBomb);
PRECACHE_WEAPON_REGISTER(tf_weapon_stickbomb);


#define MODEL_NORMAL   "models/weapons/c_models/c_caber/c_caber.mdl"
#define MODEL_EXPLODED "models/weapons/c_models/c_caber/c_caber_exploded.mdl"

#define BODYGROUP_NORMAL 0
#define BODYGROUP_BROKEN 1

#define TF_STICKBOMB_NORMAL    0
#define TF_STICKBOMB_DETONATED 1


CTFStickBomb::CTFStickBomb()
{
	this->m_iDetonated = TF_STICKBOMB_NORMAL;
}


const char *CTFStickBomb::GetWorldModel() const
{
	if (this->m_iDetonated == TF_STICKBOMB_DETONATED) {
		return MODEL_EXPLODED;
	}
	
	return BaseClass::GetWorldModel();
}

void CTFStickBomb::Precache()
{
	BaseClass::Precache();
	
	PrecacheModel(MODEL_NORMAL);
	PrecacheModel(MODEL_EXPLODED);
}

void CTFStickBomb::Smack()
{
	BaseClass::Smack();
	
	if (this->m_iDetonated == TF_STICKBOMB_NORMAL && this->ConnectedHit()) {
		this->m_iDetonated = TF_STICKBOMB_DETONATED;
		this->m_bBroken = true;
		
		this->SwitchBodyGroups();
		
#ifdef GAME_DLL
		CTFPlayer *owner = ToTFPlayer(this->GetOwner());
		if (owner != nullptr) {
			// TF2 does these things and doesn't use the results:
			// calls owner->EyeAngles() and then AngleVectors()
			// calls this->GetSwingRange()
			
			// my bet: they meant to multiply the fwd vector by the swing range
			// and then use that for the damage force, but they typo'd it and
			// just reused the shoot position instead
			
			Vector where = owner->Weapon_ShootPosition();
			
			CPVSFilter filter(where);
			TE_TFExplosion(filter, 0.0f, where, Vector(0.0f, 0.0f, 1.0f),
				TF_WEAPON_GRENADELAUNCHER, ENTINDEX(owner));
			
			/* why is the damage force vector set to Weapon_ShootPosition()?
			 * I dunno! */
			CTakeDamageInfo dmginfo(owner, owner, this, where, where, 75.0f,
				DMG_BLAST | DMG_CRITICAL | (this->IsCurrentAttackACrit() ? DMG_USEDISTANCEMOD : 0),
				TF_DMG_CUSTOM_NONE, &where);
			/* should be TF_DMG_CUSTOM_STICKBOMB_EXPLOSION or some such */
			
			CTFRadiusDamageInfo radius;
			radius.info       = dmginfo;
			radius.m_vecSrc   = where;
			radius.m_flRadius = 100.0f;
			TFGameRules()->RadiusDamage(radius);
		}
#endif
	}
}

void CTFStickBomb::SwitchBodyGroups()
{
	DevMsg("CTFStickBomb::SwitchBodyGroups\n");
	
#ifdef CLIENT_DLL
	
	// this is problematic for some reason
#if 0
//	int value = (this->m_iDetonated == TF_STICKBOMB_DETONATED ? BODYGROUP_BROKEN : BODYGROUP_NORMAL);
	
	DevMsg("GetNumBodyGroups: %d\n", this->GetNumBodyGroups());
	for (int i = 0; i < this->GetNumBodyGroups(); ++i) {
		DevMsg("Bodygroup %d: \"%s\"\n", i, this->GetBodygroupName(i));
	}
	
//	int group = this->FindBodygroupByName("broken");
//	if (group != -1) {
//		this->SetBodygroup(group, value);
//	} else {
//		Warning("CTFStickBomb::SwitchBodyGroups: can't find bodygroup \"broken\"!\n");
//	}
	
	CTFPlayer *owner = ToTFPlayer(this->GetOwner());
	if (owner != nullptr && owner->GetActiveWeapon() == this && owner->GetViewModel() != nullptr) {
		DevMsg("VM GetNumBodyGroups: %d\n", owner->GetViewModel()->GetNumBodyGroups());
		for (int i = 0; i < owner->GetViewModel()->GetNumBodyGroups(); ++i) {
			DevMsg("Bodygroup %d: \"%s\"\n", i, owner->GetViewModel()->GetBodygroupName(i));
		}
		
//		owner->GetViewModel()->SetBodygroup(group, value);
	}
	
	DevMsg("ModelName: \"%s\"\n", this->GetModelName());
	DevMsg("VM ModelName: \"%s\"\n", owner->GetViewModel()->GetModelName());
#endif
#endif
}

void CTFStickBomb::WeaponRegenerate()
{
	BaseClass::WeaponRegenerate();
	
	this->m_iDetonated = TF_STICKBOMB_NORMAL;
	
	SetContextThink(&CTFStickBomb::SwitchBodyGroups, gpGlobals->curtime + 0.001f, "SwitchBodyGroups");
}

void CTFStickBomb::WeaponReset()
{
	BaseClass::WeaponReset();
	
	this->m_iDetonated = TF_STICKBOMB_NORMAL;
	
	this->SwitchBodyGroups();
}


#ifdef CLIENT_DLL
int CTFStickBomb::GetWorldModelIndex()
{
	if (modelinfo == nullptr) {
		return BaseClass::GetWorldModelIndex();
	}
	
	int index = modelinfo->GetModelIndex(this->m_iDetonated == TF_STICKBOMB_DETONATED ? MODEL_EXPLODED : MODEL_NORMAL);
	this->m_iWorldModelIndex = index;
	return index;
}
#endif
