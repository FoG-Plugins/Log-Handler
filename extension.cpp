/**
 * vim: set ts=4 :
 * =============================================================================
 * Log Handler
 * Copyright (C) 2015 www.f-o-g.eu.
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"

#include <iostream>
#include <regex>
#include <string>

/**
 * @file extension.cpp
 * @brief Implementation of the LH extension.
 */

LH g_LH;		/**< Global singleton for extension's main interface */
SMEXT_LINK(&g_LH);

// This holds our IGameConfig instance, for later destroying
IGameConfig *g_pGameConf = NULL;

// The actual detour & forwards
CDetour *g_pPrintDetour = NULL;
IForward *g_pConnectMessageForward = NULL;
IForward *g_pEntergameMessageForward = NULL;
IForward *g_pDisconnectMessageForward = NULL;

// Message strings we want to hook
const char *messages[3] = {"connected", "entered", "disconnected"};

DETOUR_DECL_MEMBER1(CLog_Print, void, char* const, message)
{
	if(!in_array(messages, message, 3))
	{
		DETOUR_MEMBER_CALL(CLog_Print)(message);
		return;
	}

	if(!g_pConnectMessageForward || !g_pEntergameMessageForward || !g_pDisconnectMessageForward)
	{
		g_pSM->LogMessage(myself, "Forwards failed to set up, calling original function.");

		// The Forward couldn't be set up for whatever reason, so just call the
		// original function and return.
		DETOUR_MEMBER_CALL(CLog_Print)(message);
		return;
	}

    // Fish out the player's unique id
    // Example: Player1<9999><STEAM_x:y:zz><> connected
	int pos1 = strpos(message, "<") + 1;
	int pos2 = strpos(message, ">") - pos1;

	std::string str(message);
	std::string userid_raw = str.substr(pos1, pos2);

    // Convert to an integer, for use in other plugins
	int userid = atoi(userid_raw.c_str());

    // Store the Forward result
	cell_t result = 0;

	if(strpos(message, "disconnected") > -1)
	{
		g_pSM->LogMessage(myself, "Client disconnected: %d", userid);

		g_pDisconnectMessageForward->PushCell(userid);
		g_pDisconnectMessageForward->Execute(&result);
	}
	else if(strpos(message, "entered") > -1)
	{
		g_pSM->LogMessage(myself, "Client entered game: %d", userid);

		g_pEntergameMessageForward->PushCell(userid);
		g_pEntergameMessageForward->Execute(&result);
	}
	else
	{
		g_pSM->LogMessage(myself, "Client connected: %d", userid);

		g_pConnectMessageForward->PushCell(userid);
		g_pConnectMessageForward->Execute(&result);
	}

	// We should block this message
	if(result > Pl_Continue)
	{
		return;
	}
	else
	{
		DETOUR_MEMBER_CALL(CLog_Print)(message);
		return;
	}
}

bool LH::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	char conf_error[255] = "";

	if (!gameconfs->LoadGameConfigFile("lh.games", &g_pGameConf, conf_error, sizeof(conf_error)))
	{
		if (conf_error[0])
		{
				snprintf(error, maxlength, "Could not read lh.games.txt: %s", conf_error);
		}
		return false;
	}

	// Prepare and setup detour
	CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);

	g_pPrintDetour = DETOUR_CREATE_MEMBER(CLog_Print, "CLog::Print");

	if(g_pPrintDetour != NULL)
	{
		// Now that the detour is working, enable the Forward too
		g_pPrintDetour->EnableDetour();
		g_pConnectMessageForward = forwards->CreateForward("OnClientConnectMessage", ET_Hook, 1, NULL, Param_Cell);
		g_pEntergameMessageForward = forwards->CreateForward("OnClientEntergameMessage", ET_Hook, 1, NULL, Param_Cell);
		g_pDisconnectMessageForward = forwards->CreateForward("OnClientDisconnectMessage", ET_Hook, 1, NULL, Param_Cell);

		return true;
	}

	snprintf(error, maxlength, "CLog::Print detour failed.");
	return false;
}

void LH::SDK_OnUnload()
{
	// Properly destroy everything
	g_pPrintDetour->Destroy();
	gameconfs->CloseGameConfigFile(g_pGameConf);

	forwards->ReleaseForward(g_pConnectMessageForward);
	forwards->ReleaseForward(g_pEntergameMessageForward);
	forwards->ReleaseForward(g_pDisconnectMessageForward);
}

int strpos(const char *haystack, const char *needle)
{
	char *sub = const_cast<char*>(strstr(haystack, needle));

	if(sub)
		return sub - haystack;

	return -1;
}

bool in_array(const char **array, const char *needle, int arr_size)
{
	for(int i = 0; i < arr_size; i++)
	{
		if(strpos(needle, array[i]) > -1) return true;
	}

	return false;
}
