#if defined _LH_INCLUDED_
#endinput

#define _LH_INCLUDED_

/**
 * Called every time a player establishes a connection to the server.
 * Return Plugin_Continue to let the original logic run through or return a higher
 * action to hide the player from the log.
 *
 * @param client               User ID of the client connecting.
 * @return                     Plugin_Continue to show the client, anything higher
   to hide them.
*/
forward Action:OnClientConnectMessage(client);

/**
 * Called once a previously connecting client has fully entered the game.
 * Return Plugin_Continue to let the original logic run through or return a higher
 * action to hide the player from the log.
 *
 * @param client               User ID of the client connected.
 * @return                     Plugin_Continue to show the client, anything higher
   to hide them.
*/
forward Action:OnClientEntergameMessage(client);

/**
 * Called as soon as a client disconnected from the server.
 * Return Plugin_Continue to let the original logic run through or return a higher
 * action to hide the player from the log.
 *
 * @param client               User ID of the client disconnected.
 * @return                     Plugin_Continue to show the client, anything higher
   to hide them.
*/
forward Action:OnClientDisconnectMessage(client);