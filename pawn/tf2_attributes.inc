#if defined _tf2attrib_included
#endinput
#endif
#define _tf2attrib_included

#define OVERRIDE_CLASSNAME		(1 << 0)
#define OVERRIDE_ITEM_DEF		(1 << 1)
#define OVERRIDE_ITEM_LEVEL		(1 << 2)
#define OVERRIDE_ITEM_QUALITY	(1 << 3)
#define OVERRIDE_ATTRIBUTES		(1 << 4)

native Handle:CreateScriptedItemOverride(iFlags);
native SetOverrideFlags(Handle:hItemOverride, iFlags);
native GetOverrideFlags(Handle:hItemOverride);
native SetOverrideClassname(Handle:hItemOverride, String:strClassName[]);
native GetOverrideClassname(Handle:hItemOverride, String:strDest, iDestSize);
native SetOverrideItemDefinitionIndex(Handle:hItemOverride, iItemDefinitionIndex);
native GetOverrideItemDefinitionIndex(Handle:hItemOverride);
native SetOverrideQuality(Handle:hItemOverride, iEntityQuality); 
native GetOverrideQuality(Handle:hItemOverride);
native SetOverrideLevel(Handle:hItemOverride, iEntityLevel); 
native GetOverrideLevel(Handle:hItemOverride);
native SetOverrideNumAttributes(Handle:hItemOverride, iNumAttributes);
native GetOverrideNumAttributes(Handle:hItemOverride);
native SetOverrideAttribute(Handle:hItemOverride, iSlotIndex, iAttributeDefinitionIndex, Float:flValue);
native GetOverrideAttributeId(Handle:hItemOverride, iSlotIndex);
native Float:GetOverrideAttributeValue(Handle:hItemOverride, iSlotIndex); 

/**
* Called when an item is about to be given to a client. Return Plugin_Changed
* to override the item atributes to the one passed at the hAttributes parameter.
* Return Plugin_Continue to keep them intact.
*
* @param iClient				Client Index.
* @param strClassName			.
* @param iItemDefinitionIndex	Item definition index.
* @param hItemOverride			.	
*/
forward Action:OnGiveNamedItem(iClient, String:strClassName[], iItemDefinitionIndex, &Handle:hItemOverride);

/**
* Do not edit below this line!
*/
public Extension:__ext_tf2items = 
{
	name = "TF2Items",
	file = "tf2items.ext.2.ep2v",
	autoload = 0,
	#if defined REQUIRE_EXTENSIONS
		required = 1,
	#else
		required = 0,
	#endif
};
