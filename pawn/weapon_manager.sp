#pragma semicolon 1 // Force strict semicolon mode.

// ====[ INCLUDES ]============================================================
#include <sourcemod>
#include <tf2items>

// ====[ CONSTANTS ]===========================================================
#define PLUGIN_NAME		"[TF2Items] Weapon Manager"
#define PLUGIN_AUTHOR		"Damizean & Asherkin"
#define PLUGIN_VERSION		"1.3.0"
#define PLUGIN_CONTACT		"http://limetech.org/"

#define DEBUG

// ====[ VARIABLES ]===========================================================
new Handle:g_hPlayerInfo;
new Handle:g_hPlayerArray;
new Handle:g_hGlobalSettings;

// ====[ PLUGIN ]==============================================================
public Plugin:myinfo =
{
	name			= PLUGIN_NAME,
	author			= PLUGIN_AUTHOR,
	description	= PLUGIN_NAME,
	version		= PLUGIN_VERSION,
	url				= PLUGIN_CONTACT
};

// ====[ FUNCTIONS ]===========================================================

/* OnPluginStart()
 *
 * When the plugin starts up.
 */
public OnPluginStart()
{
	ParseItems();
}

/* TF2Items_OnGiveNamedItem()
 *
 * When an item is about to be given to a client.
 */
public Action:TF2Items_OnGiveNamedItem(iClient, String:strClassName[], iItemDefinitionIndex, &Handle:hItemOverride)
{
	// Find item. If any is found, override the attributes with these.
	new Handle:hItem = FindItem(iClient, iItemDefinitionIndex);
	if (hItem != INVALID_HANDLE)
	{
		hItemOverride = hItem;
		return Plugin_Changed;
	}

	// None found, use default values.
	return Plugin_Continue;
}

/* FindItem()
 *
 * 
 */
Handle:FindItem(iClient, iItemDefinitionIndex)
{
	// Check if the player is valid
	if (!IsValidClient(iClient)) return INVALID_HANDLE;

	// Retrieve the STEAM auth string
	decl String:strAuth[64]; GetClientAuthString(iClient, strAuth, sizeof(strAuth));

	// Check if it's on the list. If not, try with the global settings.
	new Handle:hItemArray = INVALID_HANDLE; 
	GetTrieValue(g_hPlayerInfo, strAuth, hItemArray);
	
	// Check for each.
	new Handle:hOutput;
	hOutput = FindItemOnArray(hItemArray, iItemDefinitionIndex);
	if (hOutput == INVALID_HANDLE) hOutput = FindItemOnArray(g_hGlobalSettings, iItemDefinitionIndex);

	// Done
	return hOutput;
}

/* FindItemOnArray()
 *
 * 
 */
Handle:FindItemOnArray(Handle:hArray, iItemDefinitionIndex)
{
	// Check if the array is valid.
	if (hArray == INVALID_HANDLE) return INVALID_HANDLE;
	new Handle:hWildcardItem = INVALID_HANDLE;

	// Iterate through each item entry and close the handle.
	for (new iItem = 0; iItem < GetArraySize(hArray); iItem++)
	{
		// Retrieve item
		new Handle:hItem = GetArrayCell(hArray, iItem);
		if (hItem == INVALID_HANDLE) continue;

		// Is the item we're looking for? If so return item.
		if (TF2Items_GetItemIndex(hItem) == iItemDefinitionIndex)
			return hItem;

		// Is a wildcard item? If so, store it.
		if (TF2Items_GetItemIndex(hItem) == -1)
			hWildcardItem = hItem;
	}

	// Done, returns wildcard item if it exists.
	return hWildcardItem;
}

/* ParseItems()
 *
 * Reads up the items information from the Key-Values.
 */
ParseItems()
{
	decl String:strBuffer[256];

	// Destroy the current items data.
	DestroyItems();

	// Create key values object and parse file.
	BuildPath(Path_SM, strBuffer, sizeof(strBuffer), "data/customweps_pawn.txt");
	new Handle:hKeyValues = CreateKeyValues("TF2Items");
	if (FileToKeyValues(hKeyValues, strBuffer) == false)
	{
		SetFailState("Error, can't read file containing the item list : %s", strBuffer);
	}

	// Check the version
	KvGetSectionName(hKeyValues, strBuffer, sizeof(strBuffer));
	if (StrEqual("custom_weapons_v3", strBuffer) == false)
	{
		SetFailState("customweps.txt structure corrupt or incorrect version: \"%s\"", strBuffer);
	}

	// Create the array and trie to store & access the item information.
	g_hPlayerArray = CreateArray();
	g_hPlayerInfo = CreateTrie();

	#if defined DEBUG
		LogMessage("Parsing items");
		LogMessage("{");
	#endif 

	// Jump into the first subkey and go on.
	if (KvGotoFirstSubKey(hKeyValues))
	{
		do
		{
			// Retrieve player information.
			KvGetSectionName(hKeyValues, strBuffer, sizeof(strBuffer));

			#if defined DEBUG
				LogMessage("	SteamID: %s", strBuffer);
				LogMessage("	{");
			#endif

			// Create new array entry and upload to the array and the trie.
			new Handle:hEntry = CreateArray();
			SetTrieValue(g_hPlayerInfo, strBuffer, hEntry);
			PushArrayCell(g_hPlayerArray, hEntry);

			// Read all the item entries
			ParseItemsEntry(hKeyValues, hEntry);

			#if defined DEBUG
				LogMessage("	}");
			#endif
		}
		while (KvGotoNextKey(hKeyValues));
		KvGoBack(hKeyValues);
	}

	// Close key values
	CloseHandle(hKeyValues);

	// Try to find the global item settings.
	GetTrieValue(g_hPlayerInfo, "*", g_hGlobalSettings);

	// Done.
	#if defined DEBUG
		LogMessage("}");
	#endif
}

/* ParseItemsEntry()
 *
 * Reads up a particular items entry.
 */
ParseItemsEntry(Handle:hKeyValues, Handle:hEntry)
{
	decl String:strBuffer[64];
	decl String:strBuffer2[64];

	// Jump into the first subkey.
	if (KvGotoFirstSubKey(hKeyValues))
	{
		do
		{
			new Handle:hItem = TF2Items_CreateItem(0);
			new iFlags = 0;

			// Retrieve item definition index and store.
			KvGetSectionName(hKeyValues, strBuffer, sizeof(strBuffer));
			if (strBuffer[0] == '*') TF2Items_SetItemIndex(hItem, -1);
			else					 TF2Items_SetItemIndex(hItem, StringToInt(strBuffer));

			#if defined DEBUG
				LogMessage("		Item: %i", TF2Items_GetItemIndex(hItem));
				LogMessage("		{");
			#endif

			// Retrieve entity level
			new iLevel = KvGetNum(hKeyValues, "level", -1);
			if (iLevel != -1)
			{
				TF2Items_SetLevel(hItem, iLevel);
				iFlags |= OVERRIDE_ITEM_LEVEL;
			}

			#if defined DEBUG
				if (iFlags & OVERRIDE_ITEM_LEVEL)
				LogMessage("			Level: %i", TF2Items_GetLevel(hItem));
			#endif

			// Retrieve entity quality
			new iQuality = KvGetNum(hKeyValues, "quality", -1);
			if (iQuality != -1)
			{
				TF2Items_SetQuality(hItem, iQuality);
				iFlags |= OVERRIDE_ITEM_QUALITY;
			}

			#if defined DEBUG
				if (iFlags & OVERRIDE_ITEM_QUALITY)
				LogMessage("			Quality: %i", TF2Items_GetQuality(hItem));
			#endif

			// Read all the attributes
			new iAttributeCount = 0;
			for (;;)
			{
				// Format the attribute entry name
				Format(strBuffer, sizeof(strBuffer), "%i", iAttributeCount+1);
				Format(strBuffer2, sizeof(strBuffer2), "%i-value", iAttributeCount+1);
	
				// Try to read the attribute
				new iAttributeIndex = KvGetNum(hKeyValues, strBuffer, -1);
				new Float:fAttributeValue = KvGetFloat(hKeyValues, strBuffer2, 0.0);
	
				// If not found, break.
				if (iAttributeIndex == -1) break;

				// Attribute found, set information.
				TF2Items_SetAttribute(hItem, iAttributeCount, iAttributeIndex, fAttributeValue);

				#if defined DEBUG
					LogMessage("			Attribute[%i] : %i / %f",
							   iAttributeCount,
							   TF2Items_GetAttributeId(hItem, iAttributeCount),
							   TF2Items_GetAttributeValue(hItem, iAttributeCount));
				#endif

				// Increase attribute count and continue.
				iAttributeCount++;
			}

			// Done, set attribute count and upload.
			if (iAttributeCount != 0)
			{
				TF2Items_SetNumAttributes(hItem, iAttributeCount);
				iFlags |= OVERRIDE_ATTRIBUTES;
			}

			// Set flags and upload.
			TF2Items_SetFlags(hItem, iFlags);
			PushArrayCell(hEntry, hItem);

			#if defined DEBUG
				LogMessage("			Flags: %05b", TF2Items_GetFlags(hItem));
				LogMessage("		}");
			#endif
		}
		while (KvGotoNextKey(hKeyValues));
		KvGoBack(hKeyValues);
	}
}

/* DestroyItems()
 *
 * Destroys the current list for items.
 */
DestroyItems()
{
	if (g_hPlayerArray != INVALID_HANDLE)
	{
		// Iterate through each player and retrieve the internal
		// weapon list.
		for (new iEntry = 0; iEntry < GetArraySize(g_hPlayerArray); iEntry++)
		{
			// Retrieve the item array.
			new Handle:hItemArray = GetArrayCell(g_hPlayerArray, iEntry);
			if (hItemArray == INVALID_HANDLE) continue;

			// Iterate through each item entry and close the handle.
			for (new iItem = 0; iItem < GetArraySize(hItemArray); iItem++)
			{
				// Retrieve item
				new Handle:hItem = GetArrayCell(hItemArray, iItem);
				if (hItem == INVALID_HANDLE) continue;

				// Close handle
				CloseHandle(hItem);
			}
		}

		// Done, free array
		CloseHandle(g_hPlayerArray);
	}

	// Free player trie
	if (g_hPlayerInfo != INVALID_HANDLE)
	{
		CloseHandle(g_hPlayerInfo);
	}

	// Done
	g_hPlayerInfo = INVALID_HANDLE;
	g_hPlayerArray = INVALID_HANDLE;
	g_hGlobalSettings = INVALID_HANDLE;
}

/* IsValidClient()
 *
 * Checks if a client is valid.
 */
bool:IsValidClient(iClient)
{
	if (iClient < 1 || iClient > MaxClients) return false;
	if (!IsClientConnected(iClient)) return false;
	return IsClientInGame(iClient);
}