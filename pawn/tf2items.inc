#if defined _tf2items_included
#endinput
#endif
#define _tf2items_included

#define OVERRIDE_CLASSNAME       (1 << 0)
#define OVERRIDE_ITEM_DEF        (1 << 1)
#define OVERRIDE_ITEM_LEVEL      (1 << 2)
#define OVERRIDE_ITEM_QUALITY    (1 << 3)
#define OVERRIDE_ATTRIBUTES      (1 << 4)

native Handle:TF2Items_CreateItem(iFlags);
native TF2Items_SetFlags(Handle:hItem, iFlags);
native TF2Items_GetFlags(Handle:hItem);
native TF2Items_SetClassname(Handle:hItemOverride, String:strClassName[]);
native TF2Items_GetClassname(Handle:hItem, String:strDest[], iDestSize);
native TF2Items_SetItemIndex(Handle:hItem, iItemDefinitionIndex);
native TF2Items_GetItemIndex(Handle:hItem);
native TF2Items_SetQuality(Handle:hItem, iEntityQuality); 
native TF2Items_GetQuality(Handle:hItem);
native TF2Items_SetLevel(Handle:hItem, iEntityLevel); 
native TF2Items_GetLevel(Handle:hItem);
native TF2Items_SetNumAttributes(Handle:hItem, iNumAttributes);
native TF2Items_GetNumAttributes(Handle:hItem);
native TF2Items_SetAttribute(Handle:hItem, iSlotIndex, iAttributeDefinitionIndex, Float:flValue);
native TF2Items_GetAttributeId(Handle:hItem, iSlotIndex);
native Float:TF2Items_GetAttributeValue(Handle:hItem, iSlotIndex); 
native TF2Items_GiveNamedItem(iClient, Handle:hItem);

/**
* Called when an item is about to be given to a client. Return Plugin_Changed
* to override the item atributes to the one passed at the hAttributes parameter.
* Return Plugin_Continue to keep them intact.
*
* @param iClient                Client Index.
* @param strClassName            .
* @param iItemDefinitionIndex    Item definition index.
* @param hItem            .    
*/
forward Action:TF2Items_OnGiveNamedItem(iClient, String:strClassName[], iItemDefinitionIndex, &Handle:hItem);

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