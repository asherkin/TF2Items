#if defined _tf2items_included
#endinput
#endif
#define _tf2items_included

#define OVERRIDE_CLASSNAME		(1 << 0)
#define OVERRIDE_ITEM_DEF		(1 << 1)
#define OVERRIDE_ITEM_LEVEL		(1 << 2)
#define OVERRIDE_ITEM_QUALITY	(1 << 3)
#define OVERRIDE_ATTRIBUTES		(1 << 4)

native Handle:TF2Items_CreateItem(iFlags);
native TF2Items_SetFlags(Handle:hItemOverride, iFlags);
native TF2Items_GetFlags(Handle:hItemOverride);
native TF2Items_SetClassname(Handle:hItemOverride, String:strClassName[]);
native TF2Items_GetClassname(Handle:hItemOverride, String:strDest, iDestSize);
native TF2Items_SetItemIndex(Handle:hItemOverride, iItemDefinitionIndex);
native TF2Items_GetItemIndex(Handle:hItemOverride);
native TF2Items_SetQuality(Handle:hItemOverride, iEntityQuality); 
native TF2Items_GetQuality(Handle:hItemOverride);
native TF2Items_SetLevel(Handle:hItemOverride, iEntityLevel); 
native TF2Items_GetLevel(Handle:hItemOverride);
native TF2Items_SetNumAttributes(Handle:hItemOverride, iNumAttributes);
native TF2Items_GetNumAttributes(Handle:hItemOverride);
native TF2Items_SetAttribute(Handle:hItemOverride, iSlotIndex, iAttributeDefinitionIndex, Float:flValue);
native TF2Items_GetAttributeId(Handle:hItemOverride, iSlotIndex);
native Float:TF2Items_GetAttributeValue(Handle:hItemOverride, iSlotIndex); 

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
forward Action:TF2Items_OnGiveNamedItem(iClient, String:strClassName[], iItemDefinitionIndex, &Handle:hItemOverride);

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
