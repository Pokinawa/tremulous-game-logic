/*
===========================================================================
Copyright (C) 2004-2006 Tony J. White

This file is part of Tremulous.

This shrubbot implementation is the original work of Tony J. White.

Contains contributions from Wesley van Beelen, Chris Bajumpaa, Josh Menke,
and Travis Maurer.

The functionality of this code mimics the behaviour of the currently
inactive project shrubet (http://www.etstats.com/shrubet/index.php?ver=2)
by Ryan Mannion.   However, shrubet was a closed-source project and
none of it's code has been copied, only it's functionality.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

// big ugly global buffer for use with buffered printing of long outputs
static char g_bfb[ 32000 ];

// note: list ordered alphabetically
g_admin_cmd_t g_admin_cmds[ ] =
  {
    {"adjustban", G_admin_adjustban, "ban",
      "change the duration or reason of a ban.  duration is specified as "
      "numbers followed by units 'w' (weeks), 'd' (days), 'h' (hours) or "
      "'m' (minutes), or seconds if no units are specified.  if the duration is"
      " preceded by a + or -, the ban duration will be extended or shortened by"
      " the specified amount",
      "[^3ban#^7] (^5/mask^7) (^5duration^7) (^5reason^7)"
    },

    {"admintest", G_admin_admintest, "admintest",
      "display your current admin level",
      ""
    },

    {"allowbuild", G_admin_denybuild, "denybuild",
      "restore a player's ability to build",
      "[^3name|slot#^7]"
    },

    {"allready", G_admin_allready, "allready",
      "makes everyone ready in intermission",
      ""
    },

    {"ban", G_admin_ban, "ban",
      "ban a player by IP and GUID with an optional expiration time and reason."
      " duration is specified as numbers followed by units 'w' (weeks), 'd' "
      "(days), 'h' (hours) or 'm' (minutes), or seconds if no units are "
      "specified",
      "[^3name|slot#|IP(/mask)^7] (^5duration^7) (^5reason^7)"
    },

    {"cancelvote", G_admin_endvote, "cancelvote",
      "cancel a vote taking place",
      "(^5a|h^7)"
    },

    {"denybuild", G_admin_denybuild, "denybuild",
      "take away a player's ability to build",
      "[^3name|slot#^7]"
    },

    {"help", G_admin_help, "help",
      "display commands available to you or help on a specific command",
      "(^5command^7)"
    },

    {"kick", G_admin_kick, "kick",
      "kick a player with an optional reason",
      "[^3name|slot#^7] (^5reason^7)"
    },

    {"listadmins", G_admin_listadmins, "listadmins",
      "display a list of all server admins and their levels",
      "(^5name|start admin#^7)"
    },

    {"listlayouts", G_admin_listlayouts, "listlayouts",
      "display a list of all available layouts for a map",
      "(^5mapname^7)"
    },

    {"listplayers", G_admin_listplayers, "listplayers",
      "display a list of players, their client numbers and their levels",
      ""
    },

    {"lock", G_admin_lock, "lock",
      "lock a team to prevent anyone from joining it",
      "[^3a|h^7]"
    },

    {"map", G_admin_map, "map",
      "load a map (and optionally force layout)",
      "[^3mapname^7] (^5layout^7)"
    },

    {"mute", G_admin_mute, "mute",
      "mute a player",
      "[^3name|slot#^7]"
    },

    {"namelog", G_admin_namelog, "namelog",
      "display a list of names used by recently connected players",
      "(^5name|IP(/mask)^7)"
    },

    {"nextmap", G_admin_nextmap, "nextmap",
      "go to the next map in the cycle",
      ""
    },

    {"passvote", G_admin_endvote, "passvote",
      "pass a vote currently taking place",
      "(^5a|h^7)"
    },

    {"putteam", G_admin_putteam, "putteam",
      "move a player to a specified team",
      "[^3name|slot#^7] [^3h|a|s^7]"
    },

    {"readconfig", G_admin_readconfig, "readconfig",
      "reloads the admin config file and refreshes permission flags",
      ""
    },

    {"rename", G_admin_rename, "rename",
      "rename a player",
      "[^3name|slot#^7] [^3new name^7]"
    },

    {"restart", G_admin_restart, "restart",
      "restart the current map (optionally using named layout or keeping/switching teams)",
      "(^5layout^7) (^5keepteams|switchteams|keepteamslock|switchteamslock^7)"
    },

    {"setlevel", G_admin_setlevel, "setlevel",
      "sets the admin level of a player",
      "[^3name|slot#|admin#^7] [^3level^7]"
    },

    {"showbans", G_admin_showbans, "showbans",
      "display a (partial) list of active bans",
      "(^5start at ban#^7) (^5name|IP(/mask)^7)"
    },

    {"spec999", G_admin_spec999, "spec999",
      "move 999 pingers to the spectator team",
      ""},

    {"time", G_admin_time, "time",
      "show the current local server time",
      ""},

    {"unban", G_admin_unban, "ban",
      "unbans a player specified by the slot as seen in showbans",
      "[^3ban#^7]"
    },

    {"unlock", G_admin_lock, "lock",
      "unlock a locked team",
      "[^3a|h^7]"
    },

    {"unmute", G_admin_mute, "mute",
      "unmute a muted player",
      "[^3name|slot#^7]"
    }
  };

static int adminNumCmds = sizeof( g_admin_cmds ) / sizeof( g_admin_cmds[ 0 ] );

static int admin_level_maxname = 0;
g_admin_level_t *g_admin_levels[ MAX_ADMIN_LEVELS ];
g_admin_admin_t *g_admin_admins[ MAX_ADMIN_ADMINS ];
g_admin_ban_t *g_admin_bans[ MAX_ADMIN_BANS ];
g_admin_command_t *g_admin_commands[ MAX_ADMIN_COMMANDS ];
g_admin_namelog_t *g_admin_namelog[ MAX_ADMIN_NAMELOGS ];

// match a certain flag within these flags
static qboolean admin_permission( char *flags, const char *flag, qboolean *perm )
{
  char *token, *token_p = flags;
  qboolean allflags = qfalse;
  qboolean p = qfalse;
  *perm = qfalse;
  while( *( token = COM_Parse( &token_p ) ) )
  {
    *perm = qtrue;
    if( *token == '-' || *token == '+' )
      *perm = *token++ == '+';
    if( !strcmp( token, flag ) )
      return qtrue;
    if( !strcmp( token, ADMF_ALLFLAGS ) )
    {
      allflags = qtrue;
      p = *perm;
    }
  }
  if( allflags )
    *perm = p;
  return allflags;
}

g_admin_cmd_t *G_admin_cmd( const char *cmd )
{
  return bsearch( cmd, g_admin_cmds, adminNumCmds, sizeof( g_admin_cmd_t ),
    cmdcmp );
}

g_admin_level_t *G_admin_level( const int l )
{
  int i;

  for( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
  {
    if( g_admin_levels[ i ]->level == l )
      return g_admin_levels[ i ];
  }

  return NULL;
}

g_admin_admin_t *G_admin_admin( const char *guid )
{
  int i;

  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
  {
    if( !Q_stricmp( g_admin_admins[ i ]->guid, guid ) )
      return g_admin_admins[ i ];
  }

  return NULL;
}

g_admin_command_t *G_admin_command( const char *cmd )
{
  int i;

  for( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
  {
    if( !Q_stricmp( g_admin_commands[ i ]->command, cmd ) )
      return g_admin_commands[ i ];
  }

  return NULL;
}

qboolean G_admin_permission( gentity_t *ent, const char *flag )
{
  qboolean perm;
  g_admin_admin_t *a;
  g_admin_level_t *l;

  // console always wins
  if( !ent )
    return qtrue;

  if( ( a = ent->client->pers.admin ) )
  {
    if( admin_permission( a->flags, flag, &perm ) )
      return perm;

    l = G_admin_level( a->level );
  }
  else
    l = G_admin_level( 0 );

  if( l )
    return admin_permission( l->flags, flag, &perm ) && perm;

  return qfalse;
}

qboolean G_admin_name_check( gentity_t *ent, char *name, char *err, int len )
{
  int i;
  gclient_t *client;
  char testName[ MAX_NAME_LENGTH ] = {""};
  char name2[ MAX_NAME_LENGTH ] = {""};
  int alphaCount = 0;

  G_SanitiseString( name, name2, sizeof( name2 ) );

  if( !strcmp( name2, "unnamedplayer" ) )
    return qtrue;

  if( !strcmp( name2, "console" ) )
  {
    Q_strncpyz( err, "The name 'console' is not allowed.", len );
    return qfalse;
  }

  G_DecolorString( name, testName, sizeof( testName ) );
  if( isdigit( testName[ 0 ] ) )
  {
    Q_strncpyz( err, "Names cannot begin with numbers", len );
    return qfalse;
  }

  for( i = 0; testName[ i ]; i++)
  {
    if( isalpha( testName[ i ] ) )
     alphaCount++;
  }

  if( alphaCount == 0 )
  {
    Q_strncpyz( err, "Names must contain letters", len );
    return qfalse;
  }

  for( i = 0; i < level.maxclients; i++ )
  {
    client = &level.clients[ i ];
    if( client->pers.connected == CON_DISCONNECTED )
      continue;

    // can rename ones self to the same name using different colors
    if( i == ( ent - g_entities ) )
      continue;

    G_SanitiseString( client->pers.netname, testName, sizeof( testName ) );
    if( !strcmp( name2, testName ) )
    {
      Com_sprintf( err, len, "The name '%s^7' is already in use", name );
      return qfalse;
    }
  }

  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
  {
    if( g_admin_admins[ i ]->level < 1 )
      continue;
    G_SanitiseString( g_admin_admins[ i ]->name, testName, sizeof( testName ) );
    if( !strcmp( name2, testName ) &&
      ent->client->pers.admin != g_admin_admins[ i ] )
    {
      Com_sprintf( err, len, "The name '%s^7' belongs to an admin, "
        "please use another name", name );
      return qfalse;
    }
  }
  return qtrue;
}

static qboolean admin_higher_admin( g_admin_admin_t *a, g_admin_admin_t *b )
{
  qboolean perm;

  if( !b )
    return qtrue;

  if( admin_permission( b->flags, ADMF_IMMUTABLE, &perm ) )
    return !perm;

  return b->level <= ( a ? a->level : 0 );
}

static qboolean admin_higher_guid( char *admin_guid, char *victim_guid )
{
  return admin_higher_admin( G_admin_admin( admin_guid ),
    G_admin_admin( victim_guid ) );
}

static qboolean admin_higher( gentity_t *admin, gentity_t *victim )
{

  // console always wins
  if( !admin )
    return qtrue;

  return admin_higher_admin( admin->client->pers.admin,
    victim->client->pers.admin );
}

static void admin_writeconfig_string( char *s, fileHandle_t f )
{
  if( s[ 0 ] )
    trap_FS_Write( s, strlen( s ), f );
  trap_FS_Write( "\n", 1, f );
}

static void admin_writeconfig_int( int v, fileHandle_t f )
{
  char buf[ 32 ];

  Com_sprintf( buf, sizeof( buf ), "%d\n", v );
  trap_FS_Write( buf, strlen( buf ), f );
}

static void admin_writeconfig( void )
{
  fileHandle_t f;
  int i;
  int t;
  char levels[ MAX_STRING_CHARS ] = {""};

  if( !g_admin.string[ 0 ] )
  {
    G_Printf( S_COLOR_YELLOW "WARNING: g_admin is not set. "
      " configuration will not be saved to a file.\n" );
    return;
  }
  t = trap_RealTime( NULL );
  if( trap_FS_FOpenFile( g_admin.string, &f, FS_WRITE ) < 0 )
  {
    G_Printf( "admin_writeconfig: could not open g_admin file \"%s\"\n",
              g_admin.string );
    return;
  }
  for( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
  {
    trap_FS_Write( "[level]\n", 8, f );
    trap_FS_Write( "level   = ", 10, f );
    admin_writeconfig_int( g_admin_levels[ i ]->level, f );
    trap_FS_Write( "name    = ", 10, f );
    admin_writeconfig_string( g_admin_levels[ i ]->name, f );
    trap_FS_Write( "flags   = ", 10, f );
    admin_writeconfig_string( g_admin_levels[ i ]->flags, f );
    trap_FS_Write( "\n", 1, f );
  }
  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
  {
    // don't write level 0 users
    if( g_admin_admins[ i ]->level == 0 )
      continue;

    trap_FS_Write( "[admin]\n", 8, f );
    trap_FS_Write( "name    = ", 10, f );
    admin_writeconfig_string( g_admin_admins[ i ]->name, f );
    trap_FS_Write( "guid    = ", 10, f );
    admin_writeconfig_string( g_admin_admins[ i ]->guid, f );
    trap_FS_Write( "level   = ", 10, f );
    admin_writeconfig_int( g_admin_admins[ i ]->level, f );
    trap_FS_Write( "flags   = ", 10, f );
    admin_writeconfig_string( g_admin_admins[ i ]->flags, f );
    trap_FS_Write( "\n", 1, f );
  }
  for( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
  {
    // don't write expired bans
    // if expires is 0, then it's a perm ban
    if( g_admin_bans[ i ]->expires != 0 && g_admin_bans[ i ]->expires <= t )
      continue;

    trap_FS_Write( "[ban]\n", 6, f );
    trap_FS_Write( "name    = ", 10, f );
    admin_writeconfig_string( g_admin_bans[ i ]->name, f );
    trap_FS_Write( "guid    = ", 10, f );
    admin_writeconfig_string( g_admin_bans[ i ]->guid, f );
    trap_FS_Write( "ip      = ", 10, f );
    admin_writeconfig_string( g_admin_bans[ i ]->ip, f );
    trap_FS_Write( "reason  = ", 10, f );
    admin_writeconfig_string( g_admin_bans[ i ]->reason, f );
    trap_FS_Write( "made    = ", 10, f );
    admin_writeconfig_string( g_admin_bans[ i ]->made, f );
    trap_FS_Write( "expires = ", 10, f );
    admin_writeconfig_int( g_admin_bans[ i ]->expires, f );
    trap_FS_Write( "banner  = ", 10, f );
    admin_writeconfig_string( g_admin_bans[ i ]->banner, f );
    trap_FS_Write( "\n", 1, f );
  }
  for( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
  {
    levels[ 0 ] = '\0';
    trap_FS_Write( "[command]\n", 10, f );
    trap_FS_Write( "command = ", 10, f );
    admin_writeconfig_string( g_admin_commands[ i ]->command, f );
    trap_FS_Write( "exec    = ", 10, f );
    admin_writeconfig_string( g_admin_commands[ i ]->exec, f );
    trap_FS_Write( "desc    = ", 10, f );
    admin_writeconfig_string( g_admin_commands[ i ]->desc, f );
    trap_FS_Write( "flag    = ", 10, f );
    admin_writeconfig_string( g_admin_commands[ i ]->flag, f );
    trap_FS_Write( "\n", 1, f );
  }
  trap_FS_FCloseFile( f );
}

static void admin_readconfig_string( char **cnf, char *s, int size )
{
  char *t;

  //COM_MatchToken(cnf, "=");
  s[ 0 ] = '\0';
  t = COM_ParseExt( cnf, qfalse );
  if( strcmp( t, "=" ) )
  {
    COM_ParseWarning( "expected '=' before \"%s\"", t );
    Q_strncpyz( s, t, size );
  }
  while( 1 )
  {
    t = COM_ParseExt( cnf, qfalse );
    if( !*t )
      break;
    if( strlen( t ) + strlen( s ) >= size )
      break;
    if( *s )
      Q_strcat( s, size, " " );
    Q_strcat( s, size, t );
  }
}

static void admin_readconfig_int( char **cnf, int *v )
{
  char *t;

  //COM_MatchToken(cnf, "=");
  t = COM_ParseExt( cnf, qfalse );
  if( !strcmp( t, "=" ) )
  {
    t = COM_ParseExt( cnf, qfalse );
  }
  else
  {
    COM_ParseWarning( "expected '=' before \"%s\"", t );
  }
  *v = atoi( t );
}

// if we can't parse any levels from readconfig, set up default
// ones to make new installs easier for admins
static void admin_default_levels( void )
{
  g_admin_level_t *l;
  int i;

  for( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
  {
    BG_Free( g_admin_levels[ i ] );
    g_admin_levels[ i ] = NULL;
  }
  for( i = 0; i <= 5; i++ )
  {
    l = BG_Alloc( sizeof( g_admin_level_t ) );
    l->level = i;
    g_admin_levels[ i ] = l;
  }
  Q_strncpyz( g_admin_levels[ 0 ]->name, "^4Unknown Player",
    sizeof( l->name ) );
  Q_strncpyz( g_admin_levels[ 0 ]->flags,
    "listplayers admintest help time",
    sizeof( l->flags ) );

  Q_strncpyz( g_admin_levels[ 1 ]->name, "^5Server Regular",
    sizeof( l->name ) );
  Q_strncpyz( g_admin_levels[ 1 ]->flags,
    "listplayers admintest help time",
    sizeof( l->flags ) );

  Q_strncpyz( g_admin_levels[ 2 ]->name, "^6Team Manager",
    sizeof( l->name ) );
  Q_strncpyz( g_admin_levels[ 2 ]->flags,
    "listplayers admintest help time putteam spec999",
    sizeof( l->flags ) );

  Q_strncpyz( g_admin_levels[ 3 ]->name, "^2Junior Admin",
    sizeof( l->name ) );
  Q_strncpyz( g_admin_levels[ 3 ]->flags,
    "listplayers admintest help time putteam spec999 kick mute ADMINCHAT",
    sizeof( l->flags ) );

  Q_strncpyz( g_admin_levels[ 4 ]->name, "^3Senior Admin",
    sizeof( l->name ) );
  Q_strncpyz( g_admin_levels[ 4 ]->flags,
    "listplayers admintest help time putteam spec99 kick mute showbans ban "
    "namelog ADMINCHAT",
    sizeof( l->flags ) );

  Q_strncpyz( g_admin_levels[ 5 ]->name, "^1Server Operator",
    sizeof( l->name ) );
  Q_strncpyz( g_admin_levels[ 5 ]->flags,
    "ALLFLAGS -IMMUTABLE -INCOGNITO",
    sizeof( l->flags ) );
  admin_level_maxname = 15;
}

void G_admin_authlog( gentity_t *ent )
{
  char            aflags[ MAX_ADMIN_FLAGS * 2 ];
  g_admin_level_t *level;
  int             levelNum = 0;

  if( !ent )
    return;

  if( ent->client->pers.admin )
    levelNum = ent->client->pers.admin->level;

  level = G_admin_level( levelNum );

  Com_sprintf( aflags, sizeof( aflags ), "%s %s",
               ent->client->pers.admin->flags,
               ( level ) ? level->flags : "" );

  G_LogPrintf( "AdminAuth: %i \"%s" S_COLOR_WHITE "\": \"%s" S_COLOR_WHITE 
               "\" [%d] (%s): %s\n", 
               ent - g_entities, ent->client->pers.netname, 
               ent->client->pers.admin->name, ent->client->pers.admin->level,
               ent->client->pers.guid, aflags );
}

static void admin_log( gentity_t *admin, char *cmd, int skiparg )
{
  char *name;

  name = ( admin ) ? admin->client->pers.netname : "console";

  G_LogPrintf( "AdminCmd: %i \"%s" S_COLOR_WHITE "\" "
                          "(\"%s" S_COLOR_WHITE "\") [%d]: %s %s\n", 
               ( admin ) ? admin->s.clientNum : -1,
               name,
               ( admin && admin->client->pers.admin ) ?
                          admin->client->pers.admin->name : name,
               ( admin && admin->client->pers.admin ) ? 
                          admin->client->pers.admin->level : 0,
               cmd,
               G_SayConcatArgs( 1 + skiparg ) );
}

static int admin_listadmins( gentity_t *ent, int start, char *search )
{
  int drawn = 0;
  char name[ MAX_NAME_LENGTH ] = {""};
  char name2[ MAX_NAME_LENGTH ] = {""};
  char lname[ MAX_NAME_LENGTH ] = {""};
  int i, j;
  gentity_t *vic;
  int colorlen;
  g_admin_admin_t *a;
  g_admin_level_t *l;

  if( search[ 0 ] )
    start = 0;

  ADMBP_begin();

  // print out all connected players regardless of level if name searching
  for( i = 0; i < level.maxclients && search[ 0 ]; i++ )
  {
    vic = &g_entities[ i ];

    if( vic->client->pers.connected == CON_DISCONNECTED )
      continue;

    G_SanitiseString( vic->client->pers.netname, name, sizeof( name ) );
    if( !strstr( name, search ) )
      continue;

    lname[ 0 ] = '\0';
    a = vic->client->pers.admin;
    if( ( l = G_admin_level( a ? a->level : 0 ) ) )
      Q_strncpyz( lname, l->name, sizeof( l->name ) );

    for( colorlen = j = 0; lname[ j ]; j++ )
    {
      if( Q_IsColorString( &lname[ j ] ) )
        colorlen += 2;
    }

    ADMBP( va( "%4i %4i %*s^7 %s\n",
      i,
      l ? l->level : 0,
      admin_level_maxname + colorlen,
      lname,
      vic->client->pers.netname ) );
    drawn++;
  }

  for( i = start; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ] &&
    ( search[ 0 ] || i - start < MAX_ADMIN_LISTITEMS ); i++ )
  {
    a = g_admin_admins[ i ];

    if( search[ 0 ] )
    {
      G_SanitiseString( a->name, name, sizeof( name ) );
      if( !strstr( name, search ) )
        continue;

      // we don't want to draw the same player twice
      for( j = 0; j < level.maxclients; j++ )
      {
        vic = &g_entities[ j ];
        if( vic->client->pers.connected == CON_DISCONNECTED )
          continue;
        G_SanitiseString( vic->client->pers.netname, name2, sizeof( name2 ) );
        if( vic->client->pers.admin == a && strstr( name2, search ) )
        {
          break;
        }
      }
      if( j < level.maxclients )
        continue;
    }

    lname[ 0 ] = '\0';
    if( ( l = G_admin_level( a->level ) ) )
      Q_strncpyz( lname, l->name, sizeof( lname ) );

    for( colorlen = j = 0; lname[ j ]; j++ )
    {
      if( Q_IsColorString( &lname[ j ] ) )
        colorlen += 2;
    }

    ADMBP( va( "%4i %4i %*s^7 %s\n",
      ( i + MAX_CLIENTS ),
      a->level,
      admin_level_maxname + colorlen,
      lname,
      a->name ) );
    drawn++;
  }
  ADMBP_end();
  return drawn;
}

void G_admin_duration( int secs, char *duration, int dursize )
{
  // sizeof("12.5 minutes") == 13
  if( secs > ( 60 * 60 * 24 * 365 * 50 ) || secs < 0 )
    Q_strncpyz( duration, "PERMANENT", dursize );
  else if( secs >= ( 60 * 60 * 24 * 365 ) )
    Com_sprintf( duration, dursize, "%1.1f years",
      ( secs / ( 60 * 60 * 24 * 365.0f ) ) );
  else if( secs >= ( 60 * 60 * 24 * 90 ) )
    Com_sprintf( duration, dursize, "%1.1f weeks",
      ( secs / ( 60 * 60 * 24 * 7.0f ) ) );
  else if( secs >= ( 60 * 60 * 24 ) )
    Com_sprintf( duration, dursize, "%1.1f days",
      ( secs / ( 60 * 60 * 24.0f ) ) );
  else if( secs >= ( 60 * 60 ) )
    Com_sprintf( duration, dursize, "%1.1f hours",
      ( secs / ( 60 * 60.0f ) ) );
  else if( secs >= 60 )
    Com_sprintf( duration, dursize, "%1.1f minutes",
      ( secs / 60.0f ) );
  else
    Com_sprintf( duration, dursize, "%i seconds", secs );
}

qboolean G_admin_ban_check( gentity_t *ent, char *reason, int rlen )
{
  int i;
  int t;
  addr_t ip, test;
  int mask = -1;

  t = trap_RealTime( NULL );
  // this happens when ip = localhost
  if( !G_AddressParse( ent->client->pers.ip, &ip, NULL ) )
    return qfalse;
  for( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
  {
    // 0 is for perm ban
    if( g_admin_bans[ i ]->expires != 0 && g_admin_bans[ i ]->expires <= t )
      continue;
    
    if( !Q_stricmp( g_admin_bans[ i ]->guid, ent->client->pers.guid ) ||
      ( !G_admin_permission( ent, ADMF_IMMUNITY ) &&
        G_AddressParse( g_admin_bans[ i ]->ip, &test, &mask ) &&
        G_AddressCompare( &ip, &test, mask ) ) )
    {
      char duration[ 13 ];
      G_admin_duration( g_admin_bans[ i ]->expires - t,
        duration, sizeof( duration ) );
      if( reason )
        Com_sprintf(
          reason,
          rlen,
          "You have been banned by %s^7 reason: %s^7 expires: %s",
          g_admin_bans[ i ]->banner,
          g_admin_bans[ i ]->reason,
          duration
        );
      G_Printf( S_COLOR_YELLOW "%s" S_COLOR_YELLOW " tried to connect from %s "
        "(ban #%d)\n",
        g_admin_bans[ i ]->name,
        ent->client->pers.ip,
        i + 1 );
      return qtrue;
    }
  }
  return qfalse;
}

qboolean G_admin_cmd_check( gentity_t *ent, qboolean say )
{
  char command[ MAX_ADMIN_CMD_LEN ];
  char *cmd;
  int skip = 0;
  g_admin_cmd_t *admincmd;
  g_admin_command_t *c;

  command[ 0 ] = '\0';
  G_SayArgv( 0, command, sizeof( command ) );
  if( !command[ 0 ] )
    return qfalse;
  if( !Q_stricmp( command, "say" ) ||
    ( !Q_stricmp( command, "say_team" ) &&
      G_admin_permission( ent, ADMF_TEAMCHAT_CMD ) ) )
  {
    skip = 1;
    G_SayArgv( 1, command, sizeof( command ) );
  }

  cmd = command;
  if( *cmd == '!' )
    cmd++;
  else
    return qfalse;

  if( ( c = G_admin_command( cmd ) ) )
  {
    if( G_admin_permission( ent, c->flag ) )
    {
      // flooding say will have already been accounted for in ClientCommand
      if( !say && G_FloodLimited( ent ) )
        return qtrue;
      admin_log( ent, cmd, skip );
      trap_SendConsoleCommand( EXEC_APPEND, c->exec );
    }
    else
    {
      admin_log( ent, S_COLOR_RED "attempted" S_COLOR_WHITE, skip - 1 );
      ADMP( va( "^3!%s: ^7permission denied\n", c->command ) );
    }
    return qtrue;
  }

  if( ( admincmd = G_admin_cmd( cmd ) ) )
  {
    if( G_admin_permission( ent, admincmd->flag ) )
    {
      // flooding say will have already been accounted for in ClientCommand
      if( !say && G_FloodLimited( ent ) )
        return qtrue;
      admin_log( ent, cmd, skip );
      admincmd->handler( ent, skip );
    }
    else
    {
      ADMP( va( "^3!%s: ^7permission denied\n", admincmd->keyword ) );
      admin_log( ent, "attempted", skip - 1 );
    }
    return qtrue;
  }
  return qfalse;
}

void G_admin_namelog_cleanup( )
{
  int i;

  for( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
  {
    BG_Free( g_admin_namelog[ i ] );
    g_admin_namelog[ i ] = NULL;
  }
}

void G_admin_namelog_update( gclient_t *client, qboolean disconnect )
{
  int i, j;
  g_admin_namelog_t *namelog;
  char n1[ MAX_NAME_LENGTH ];
  char n2[ MAX_NAME_LENGTH ];
  int clientNum = ( client - level.clients );

  G_SanitiseString( client->pers.netname, n1, sizeof( n1 ) );
  for( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
  {
    if( disconnect && g_admin_namelog[ i ]->slot != clientNum )
      continue;

    if( !disconnect && g_admin_namelog[ i ]->slot != clientNum &&
                       g_admin_namelog[ i ]->slot != -1 )
    {
      continue;
    }

    if( !Q_stricmp( client->pers.ip, g_admin_namelog[ i ]->ip ) &&
      !Q_stricmp( client->pers.guid, g_admin_namelog[ i ]->guid ) )
    {
      for( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
         g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
      {
        G_SanitiseString( g_admin_namelog[ i ]->name[ j ], n2, sizeof( n2 ) );
        if( !strcmp( n1, n2 ) )
          break;
      }
      if( j == MAX_ADMIN_NAMELOG_NAMES )
        j = client->pers.nameChanges % MAX_ADMIN_NAMELOG_NAMES;
      Q_strncpyz( g_admin_namelog[ i ]->name[ j ], client->pers.netname,
        sizeof( g_admin_namelog[ i ]->name[ j ] ) );
      g_admin_namelog[ i ]->slot = ( disconnect ) ? -1 : clientNum;

      // if this player is connecting, they are no longer banned
      if( !disconnect )
        g_admin_namelog[ i ]->banned = qfalse;

      return;
    }
  }
  if( i == MAX_ADMIN_NAMELOGS )
  {
    G_Printf( "G_admin_namelog_update: warning, g_admin_namelogs overflow\n" );
    return;
  }
  namelog = BG_Alloc( sizeof( g_admin_namelog_t ) );
  memset( namelog, 0, sizeof( namelog ) );
  for( j = 0; j < MAX_ADMIN_NAMELOG_NAMES; j++ )
    namelog->name[ j ][ 0 ] = '\0';
  Q_strncpyz( namelog->ip, client->pers.ip, sizeof( namelog->ip ) );
  Q_strncpyz( namelog->guid, client->pers.guid, sizeof( namelog->guid ) );
  Q_strncpyz( namelog->name[ 0 ], client->pers.netname,
    sizeof( namelog->name[ 0 ] ) );
  namelog->slot = ( disconnect ) ? -1 : clientNum;
  g_admin_namelog[ i ] = namelog;
}

qboolean G_admin_readconfig( gentity_t *ent, int skiparg )
{
  g_admin_level_t *l = NULL;
  g_admin_admin_t *a = NULL;
  g_admin_ban_t *b = NULL;
  g_admin_command_t *c = NULL;
  int lc = 0, ac = 0, bc = 0, cc = 0;
  fileHandle_t f;
  int len;
  char *cnf, *cnf2;
  char *t;
  qboolean level_open, admin_open, ban_open, command_open;
  int i;

  G_admin_cleanup();

  if( !g_admin.string[ 0 ] )
  {
    ADMP( "^3!readconfig: g_admin is not set, not loading configuration "
      "from a file\n" );
    admin_default_levels();
    return qfalse;
  }

  len = trap_FS_FOpenFile( g_admin.string, &f, FS_READ );
  if( len < 0 )
  {
    G_Printf( "^3!readconfig: ^7could not open admin config file %s\n",
            g_admin.string );
    admin_default_levels();
    return qfalse;
  }
  cnf = BG_Alloc( len + 1 );
  cnf2 = cnf;
  trap_FS_Read( cnf, len, f );
  *( cnf + len ) = '\0';
  trap_FS_FCloseFile( f );

  admin_level_maxname = 0;

  level_open = admin_open = ban_open = command_open = qfalse;
  COM_BeginParseSession( g_admin.string );
  while( 1 )
  {
    t = COM_Parse( &cnf );
    if( !*t )
      break;

    if( !Q_stricmp( t, "[level]" ) )
    {
      if( lc >= MAX_ADMIN_LEVELS )
        return qfalse;
      l = BG_Alloc( sizeof( g_admin_level_t ) );
      g_admin_levels[ lc++ ] = l;
      level_open = qtrue;
      admin_open = ban_open = command_open = qfalse;
    }
    else if( !Q_stricmp( t, "[admin]" ) )
    {
      if( ac >= MAX_ADMIN_ADMINS )
        return qfalse;
      a = BG_Alloc( sizeof( g_admin_admin_t ) );
      g_admin_admins[ ac++ ] = a;
      admin_open = qtrue;
      level_open = ban_open = command_open = qfalse;
    }
    else if( !Q_stricmp( t, "[ban]" ) )
    {
      if( bc >= MAX_ADMIN_BANS )
        return qfalse;
      b = BG_Alloc( sizeof( g_admin_ban_t ) );
      g_admin_bans[ bc++ ] = b;
      ban_open = qtrue;
      level_open = admin_open = command_open = qfalse;
    }
    else if( !Q_stricmp( t, "[command]" ) )
    {
      if( cc >= MAX_ADMIN_COMMANDS )
        return qfalse;
      c = BG_Alloc( sizeof( g_admin_command_t ) );
      g_admin_commands[ cc++ ] = c;
      command_open = qtrue;
      level_open = admin_open = ban_open = qfalse;
    }
    else if( level_open )
    {
      if( !Q_stricmp( t, "level" ) )
      {
        admin_readconfig_int( &cnf, &l->level );
      }
      else if( !Q_stricmp( t, "name" ) )
      {
        admin_readconfig_string( &cnf, l->name, sizeof( l->name ) );
        // max printable name length for formatting
        len = Q_PrintStrlen( l->name );
        if( len > admin_level_maxname )
          admin_level_maxname = len;
      }
      else if( !Q_stricmp( t, "flags" ) )
      {
        admin_readconfig_string( &cnf, l->flags, sizeof( l->flags ) );
      }
      else
      {
        COM_ParseError( "[level] unrecognized token \"%s\"", t );
      }
    }
    else if( admin_open )
    {
      if( !Q_stricmp( t, "name" ) )
      {
        admin_readconfig_string( &cnf, a->name, sizeof( a->name ) );
      }
      else if( !Q_stricmp( t, "guid" ) )
      {
        admin_readconfig_string( &cnf, a->guid, sizeof( a->guid ) );
      }
      else if( !Q_stricmp( t, "level" ) )
      {
        admin_readconfig_int( &cnf, &a->level );
      }
      else if( !Q_stricmp( t, "flags" ) )
      {
        admin_readconfig_string( &cnf, a->flags, sizeof( a->flags ) );
      }
      else
      {
        COM_ParseError( "[admin] unrecognized token \"%s\"", t );
      }

    }
    else if( ban_open )
    {
      if( !Q_stricmp( t, "name" ) )
      {
        admin_readconfig_string( &cnf, b->name, sizeof( b->name ) );
      }
      else if( !Q_stricmp( t, "guid" ) )
      {
        admin_readconfig_string( &cnf, b->guid, sizeof( b->guid ) );
      }
      else if( !Q_stricmp( t, "ip" ) )
      {
        admin_readconfig_string( &cnf, b->ip, sizeof( b->ip ) );
      }
      else if( !Q_stricmp( t, "reason" ) )
      {
        admin_readconfig_string( &cnf, b->reason, sizeof( b->reason ) );
      }
      else if( !Q_stricmp( t, "made" ) )
      {
        admin_readconfig_string( &cnf, b->made, sizeof( b->made ) );
      }
      else if( !Q_stricmp( t, "expires" ) )
      {
        admin_readconfig_int( &cnf, &b->expires );
      }
      else if( !Q_stricmp( t, "banner" ) )
      {
        admin_readconfig_string( &cnf, b->banner, sizeof( b->banner ) );
      }
      else
      {
        COM_ParseError( "[ban] unrecognized token \"%s\"", t );
      }
    }
    else if( command_open )
    {
      if( !Q_stricmp( t, "command" ) )
      {
        admin_readconfig_string( &cnf, c->command, sizeof( c->command ) );
      }
      else if( !Q_stricmp( t, "exec" ) )
      {
        admin_readconfig_string( &cnf, c->exec, sizeof( c->exec ) );
      }
      else if( !Q_stricmp( t, "desc" ) )
      {
        admin_readconfig_string( &cnf, c->desc, sizeof( c->desc ) );
      }
      else if( !Q_stricmp( t, "flag" ) )
      {
        admin_readconfig_string( &cnf, c->flag, sizeof( c->flag ) );
      }
      else
      {
        COM_ParseError( "[command] unrecognized token \"%s\"", t );
      }
    }
    else
    {
      COM_ParseError( "unexpected token \"%s\"", t );
    }
  }
  BG_Free( cnf2 );
  ADMP( va( "^3!readconfig: ^7loaded %d levels, %d admins, %d bans, %d commands\n",
          lc, ac, bc, cc ) );
  if( lc == 0 )
    admin_default_levels();

  // restore admin mapping
  for( i = 0; i < level.maxclients; i++ )
  {
    if( level.clients[ i ].pers.connected != CON_DISCONNECTED )
    {
      level.clients[ i ].pers.admin =
        G_admin_admin( level.clients[ i ].pers.guid );
      G_admin_authlog( &g_entities[ i ] );
    }
  }

  return qtrue;
}

qboolean G_admin_time( gentity_t *ent, int skiparg )
{
  qtime_t qt;

  trap_RealTime( &qt );
  ADMP( va( "^3!time: ^7local time is %02i:%02i:%02i\n",
    qt.tm_hour, qt.tm_min, qt.tm_sec ) );
  return qtrue;
}

qboolean G_admin_setlevel( gentity_t *ent, int skiparg )
{
  char name[ MAX_NAME_LENGTH ] = {""};
  char lstr[ 12 ]; // 11 is max strlen() for 32-bit (signed) int
  char testname[ MAX_NAME_LENGTH ] = {""};
  int i;
  gentity_t *vic = NULL;
  g_admin_admin_t *a = NULL;
  g_admin_level_t *l;

  if( G_SayArgc() < 3 + skiparg )
  {
    ADMP( "^3!setlevel: ^7usage: !setlevel [name|slot#] [level]\n" );
    return qfalse;
  }

  G_SayArgv( 1 + skiparg, testname, sizeof( testname ) );
  G_SayArgv( 2 + skiparg, lstr, sizeof( lstr ) );

  if( !( l = G_admin_level( atoi( lstr ) ) ) )
  {
    ADMP( "^3!setlevel: ^7level is not defined\n" );
    return qfalse;
  }

  if( ent && l->level >
    ( ent->client->pers.admin ? ent->client->pers.admin->level : 0 ) )
  {
    ADMP( "^3!setlevel: ^7you may not use !setlevel to set a level higher "
      "than your current level\n" );
    return qfalse;
  }

  for( i = 0; testname[ i ] && isdigit( testname[ i ] ); i++ );
  if( !testname[ i ] )
  {
    int id = atoi( testname );
    if( id < MAX_CLIENTS )
      vic = &g_entities[ id ];
    else if( id < MAX_CLIENTS + MAX_ADMIN_ADMINS &&
      g_admin_admins[ id - MAX_CLIENTS ] )
      a = g_admin_admins[ id - MAX_CLIENTS ];
    else
    {
      ADMP( va( "^3!setlevel: ^7%s not in range 1-%d\n", testname,
        MAX_CLIENTS + MAX_ADMIN_ADMINS ) );
      return qfalse;
    }
  }
  else
    G_SanitiseString( testname, name, sizeof( name ) );

  if( vic && vic->client && vic->client->pers.connected != CON_DISCONNECTED )
    a = vic->client->pers.admin;
  else if( !a )
  {
    int matches = 0;
    for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ] && matches < 2; i++ )
    {
      G_SanitiseString( g_admin_admins[ i ]->name, testname, sizeof( testname ) );
      if( strstr( testname, name ) )
      {
        a = g_admin_admins[ i ];
        matches++;
      }
    }

    for( i = 0; i < level.maxclients && matches < 2; i++ )
    {
      if( level.clients[ i ].pers.connected == CON_DISCONNECTED )
        continue;

      if( matches && level.clients[ i ].pers.admin == a )
      {
        vic = &g_entities[ i ];
        continue;
      }

      G_SanitiseString( level.clients[ i ].pers.netname, testname,
        sizeof( testname ) );
      if( strstr( testname, name ) )
      {
        vic = &g_entities[ i ];
        a = vic->client->pers.admin;
        matches++;
      }
    }

    if( matches == 0 )
    {
      ADMP( "^3!setlevel:^7 no match.  use !listplayers or !listadmins to "
        "find an appropriate number to use instead of name.\n" );
      return qfalse;
    }
    if( matches > 1 )
    {
      ADMP( "^3!setlevel:^7 more than one match.  Use the admin number "
        "instead:\n" );
      admin_listadmins( ent, 0, name );
      return qfalse;
    }
  }

  if( ent && !admin_higher_admin( ent->client->pers.admin, a ) )
  {
    ADMP( "^3!setlevel: ^7sorry, but your intended victim has a higher"
        " admin level than you\n" );
    return qfalse;
  }

  if( !a && vic )
  {
    for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
    {
      if( g_admin_admins[ i ]->level == 0 )
        break;
    }
    if( i == MAX_ADMIN_ADMINS )
    {
      ADMP( "^3!setlevel: ^7too many admins\n" );
      return qfalse;
    }
    a = BG_Alloc( sizeof( g_admin_admin_t ) );
    g_admin_admins[ i ] = a;
    vic->client->pers.admin = a;
    Q_strncpyz( a->guid, vic->client->pers.guid, sizeof( a->guid ) );
  }

  a->level = l->level;
  if( vic )
    Q_strncpyz( a->name, vic->client->pers.netname, sizeof( a->name ) );

  AP( va(
    "print \"^3!setlevel: ^7%s^7 was given level %d admin rights by %s\n\"",
    a->name, a->level, ( ent ) ? ent->client->pers.netname : "console" ) );

  admin_writeconfig();
  if( vic ) 
    G_admin_authlog( vic );
  return qtrue;
}

static qboolean admin_create_ban( gentity_t *ent,
  char *netname,
  char *guid,
  char *ip,
  int seconds,
  char *reason )
{
  g_admin_ban_t *b = NULL;
  qtime_t       qt;
  int           t;
  int           i;
  char          *name;

  t = trap_RealTime( &qt );

  for( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
  {
    if( g_admin_bans[ i ]->expires != 0 && g_admin_bans[ i ]->expires <= t )
    {
      b = g_admin_bans[ i ];
      break;
    }
  }

  if( !b )
  {
    if( i == MAX_ADMIN_BANS || g_admin_bans[ i ] )
    {
      ADMP( "^3!ban: ^7too many bans\n" );
      return qfalse;
    }
    b = BG_Alloc( sizeof( g_admin_ban_t ) );
    g_admin_bans[ i ] = b;
  }

  Q_strncpyz( b->name, netname, sizeof( b->name ) );
  Q_strncpyz( b->guid, guid, sizeof( b->guid ) );
  Q_strncpyz( b->ip, ip, sizeof( b->ip ) );

  Com_sprintf( b->made, sizeof( b->made ), "%02i/%02i/%02i %02i:%02i:%02i",
    qt.tm_mon + 1, qt.tm_mday, qt.tm_year % 100,
    qt.tm_hour, qt.tm_min, qt.tm_sec );

  if( ent && ent->client->pers.admin )
    name = ent->client->pers.admin->name;
  else if( ent )
    name = ent->client->pers.netname;
  else
    name = "console";

  Q_strncpyz( b->banner, name, sizeof( b->banner ) );
  if( !seconds )
    b->expires = 0;
  else
    b->expires = t + seconds;
  if( !*reason )
    Q_strncpyz( b->reason, "banned by admin", sizeof( b->reason ) );
  else
    Q_strncpyz( b->reason, reason, sizeof( b->reason ) );

  for( i = 0; i < level.maxclients; i++ )
  {
    if( level.clients[ i ].pers.connected == CON_DISCONNECTED )
      continue;
    if( G_admin_ban_check( &g_entities[ i ], NULL, 0 ) )
    {
      trap_SendServerCommand( i,
        va( "disconnect \"You have been kicked by %s\nreason:\n%s\"",
          b->banner, b->reason ) );

      trap_DropClient( i, va( "has been kicked by %s^7. reason: %s",
        b->banner, b->reason ) );
    }
  }
  return qtrue;
}

int G_admin_parse_time( const char *time )
{
  int seconds = 0, num = 0;
  while( *time )
  {
    if( !isdigit( *time ) )
      return -1;
    while( isdigit( *time ) )
      num = num * 10 + *time++ - '0';

    if( !*time )
      break;
    switch( *time++ )
    {
      case 'w': num *= 7;
      case 'd': num *= 24;
      case 'h': num *= 60;
      case 'm': num *= 60;
      case 's': break;
      default:  return -1;
    }
    seconds += num;
    num = 0;
  }
  if( num )
    seconds += num;
  return seconds;
}

qboolean G_admin_kick( gentity_t *ent, int skiparg )
{
  int pids[ MAX_CLIENTS ], found;
  char name[ MAX_NAME_LENGTH ], *reason, err[ MAX_STRING_CHARS ];
  int minargc;
  gentity_t *vic;

  minargc = 3 + skiparg;
  if( G_admin_permission( ent, ADMF_UNACCOUNTABLE ) )
    minargc = 2 + skiparg;

  if( G_SayArgc() < minargc )
  {
    ADMP( "^3!kick: ^7usage: !kick [name] [reason]\n" );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, name, sizeof( name ) );
  reason = G_SayConcatArgs( 2 + skiparg );
  if( ( found = G_ClientNumbersFromString( name, pids, MAX_CLIENTS ) ) != 1 )
  {
    G_MatchOnePlayer( pids, found, err, sizeof( err ) );
    ADMP( va( "^3!kick: ^7%s\n", err ) );
    return qfalse;
  }
  vic = &g_entities[ pids[ 0 ] ];
  if( !admin_higher( ent, vic ) )
  {
    ADMP( "^3!kick: ^7sorry, but your intended victim has a higher admin"
        " level than you\n" );
    return qfalse;
  }
  if( vic->client->pers.localClient )
  {
    ADMP( "^3!kick: ^7disconnecting the host would end the game\n" );
    return qfalse;
  }
  admin_create_ban( ent,
    vic->client->pers.netname,
    vic->client->pers.guid,
    vic->client->pers.ip,
    MAX( 1, G_admin_parse_time( g_adminTempBan.string ) ),
    ( *reason ) ? reason : "kicked by admin" );
  admin_writeconfig();

  return qtrue;
}

qboolean G_admin_ban( gentity_t *ent, int skiparg )
{
  int seconds;
  char search[ MAX_NAME_LENGTH ];
  char secs[ MAX_TOKEN_CHARS ];
  char *reason;
  char duration[ 13 ];
  int logmatch = -1, logmatches = 0;
  int i, j;
  qboolean exactmatch = qfalse;
  char n2[ MAX_NAME_LENGTH ];
  char s2[ MAX_NAME_LENGTH ];
  int netmask = -1;
  addr_t ip, cmp;
  qboolean ipmatch = qfalse;

  if( G_SayArgc() < 2 + skiparg )
  {
    ADMP( "^3!ban: ^7usage: !ban [name|slot|IP(/mask)] [duration] [reason]\n" );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, search, sizeof( search ) );
  G_SanitiseString( search, s2, sizeof( s2 ) );
  G_SayArgv( 2 + skiparg, secs, sizeof( secs ) );

  seconds = G_admin_parse_time( secs );
  if( seconds <= 0 )
  {
    seconds = 0;
    reason = G_SayConcatArgs( 2 + skiparg );
  }
  else
  {
    reason = G_SayConcatArgs( 3 + skiparg );
  }
  if( !*reason && !G_admin_permission( ent, ADMF_UNACCOUNTABLE ) )
  {
    ADMP( "^3!ban: ^7you must specify a reason\n" );
    return qfalse;
  }
  if( !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) )
  {
    int maximum = MAX( 1, G_admin_parse_time( g_adminMaxBan.string ) );
    if( seconds == 0 || seconds > maximum )
    {
      ADMP( "^3!ban: ^7you may not issue permanent bans\n" );
      seconds = maximum;
    }
  }

  // ban by clientnum
  for( i = 0; search[ i ] && isdigit( search[ i ] ); i++ );
  if( !search[ i ] )
  {
    i = atoi( search );
    if( i < MAX_CLIENTS &&
      level.clients[ i ].pers.connected != CON_DISCONNECTED )
    {
      logmatches = 1;
      exactmatch = qtrue;
      for( logmatch = 0; g_admin_namelog[ logmatch ]->slot != i; logmatch++ );
    }
  }
  else if( G_AddressParse( search, &ip, &netmask ) )
  {
    int max = ip.type == IPv4 ? 32 : 128;
    int min = ent ? max / 2 : 1;
    if( netmask < min || netmask > max )
    {
      ADMP( va( "^3!ban: ^7invalid netmask (%d is not one of %d-%d)\n",
        netmask, min, max ) );
      return qfalse;
    }
    ipmatch = qtrue;
  }

  for( i = 0;
       !exactmatch && i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ];
       i++ )
  {
    // skip players in the namelog who have already been banned
    if( g_admin_namelog[ i ]->banned )
      continue;

    if( ipmatch )
    {
      if( G_AddressParse( g_admin_namelog[ i ]->ip, &cmp, NULL ) &&
          G_AddressCompare( &ip, &cmp, netmask ) )
      {
        logmatches++;
        logmatch = i;
        if( ( ip.type == IPv4 && netmask == 32 ) ||
          ( ip.type == IPv6 && netmask == 128 ) )
        {
          exactmatch = qtrue;
          break;
        }
      }
      continue;
    }
    for( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
       g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
    {
      G_SanitiseString( g_admin_namelog[ i ]->name[ j ], n2, sizeof( n2 ) );
      if( strstr( n2, s2 ) )
      {
        if( logmatch != i )
          logmatches++;
        logmatch = i;
      }
    }
  }

  if( !logmatches )
  {
    ADMP( "^3!ban: ^7no player found by that name, IP, or slot number\n" );
    return qfalse;
  }
  if( !ipmatch && logmatches > 1 )
  {
    ADMBP_begin();
    ADMBP( "^3!ban: ^7multiple recent clients match name, use IP or slot#:\n" );
    for( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
    {
      for( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
         g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
      {
        G_SanitiseString( g_admin_namelog[ i ]->name[ j ], n2, sizeof( n2 ) );
        if( strstr( n2, s2 ) )
        {
          ADMBP( va( "%s %15s ^7'%s^7'\n",
           ( g_admin_namelog[ i ]->slot > -1 ) ?
             va( "^3%-2d", g_admin_namelog[ i ]->slot ) : "- ",
           g_admin_namelog[ i ]->ip,
           g_admin_namelog[ i ]->name[ j ] ) );
        }
      }
    }
    ADMBP_end();
    return qfalse;
  }

  if( ent && !admin_higher_guid( ent->client->pers.guid,
    g_admin_namelog[ logmatch ]->guid ) )
  {

    ADMP( "^3!ban: ^7sorry, but your intended victim has a higher admin"
      " level than you\n" );
    return qfalse;
  }
  if( !strcmp( g_admin_namelog[ logmatch ]->ip, "localhost" ) )
  {
    ADMP( "^3!ban: ^7disconnecting the host would end the game\n" );
    return qfalse;
  }

  G_admin_duration( ( seconds ) ? seconds : -1,
    duration, sizeof( duration ) );

  admin_create_ban( ent,
    g_admin_namelog[ logmatch ]->name[ 0 ],
    g_admin_namelog[ logmatch ]->guid,
    ipmatch ? search : g_admin_namelog[ logmatch ]->ip,
    seconds, reason );

  g_admin_namelog[ logmatch ]->banned = qtrue;

  if( !g_admin.string[ 0 ] )
    ADMP( "^3!ban: ^7WARNING g_admin not set, not saving ban to a file\n" );
  else
    admin_writeconfig();

  return qtrue;
}

qboolean G_admin_unban( gentity_t *ent, int skiparg )
{
  int bnum;
  int time = trap_RealTime( NULL );
  char bs[ 5 ];

  if( G_SayArgc() < 2 + skiparg )
  {
    ADMP( "^3!unban: ^7usage: !unban [ban#]\n" );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, bs, sizeof( bs ) );
  bnum = atoi( bs );
  if( bnum < 1 || bnum > MAX_ADMIN_BANS || !g_admin_bans[ bnum - 1 ] )
  {
    ADMP( "^3!unban: ^7invalid ban#\n" );
    return qfalse;
  }
  if( !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) &&
    ( g_admin_bans[ bnum - 1 ]->expires == 0 ||
    ( g_admin_bans[ bnum - 1 ]->expires - time > MAX( 1,
      G_admin_parse_time( g_adminMaxBan.string ) ) ) ) )
  {
    ADMP( "^3!unban: ^7you cannot remove permanent bans\n" );
    return qfalse;
  }
  g_admin_bans[ bnum - 1 ]->expires = time;
  AP( va( "print \"^3!unban: ^7ban #%d for %s^7 has been removed by %s\n\"",
          bnum,
          g_admin_bans[ bnum - 1 ]->name,
          ( ent ) ? ent->client->pers.netname : "console" ) );
  admin_writeconfig();
  return qtrue;
}

qboolean G_admin_adjustban( gentity_t *ent, int skiparg )
{
  int bnum;
  int length, maximum;
  int expires;
  int time = trap_RealTime( NULL );
  char duration[ 13 ] = {""};
  char *reason;
  char bs[ 5 ];
  char secs[ MAX_TOKEN_CHARS ];
  char mode = '\0';
  g_admin_ban_t *ban;
  int mask = 0;

  if( G_SayArgc() < 3 + skiparg )
  {
    ADMP( "^3!adjustban: ^7usage: !adjustban [ban#] [/mask] [duration] [reason]"
      "\n" );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, bs, sizeof( bs ) );
  bnum = atoi( bs );
  if( bnum < 1 || bnum > MAX_ADMIN_BANS || !g_admin_bans[ bnum - 1 ] )
  {
    ADMP( "^3!adjustban: ^7invalid ban#\n" );
    return qfalse;
  }
  ban = g_admin_bans[ bnum - 1 ];
  maximum = MAX( 1, G_admin_parse_time( g_adminMaxBan.string ) );
  if( !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) &&
    ( ban->expires == 0 || ban->expires - time > maximum ) )
  {
    ADMP( "^3!adjustban: ^7you cannot modify permanent bans\n" );
    return qfalse;
  }
  G_SayArgv( 2 + skiparg, secs, sizeof( secs ) );
  if( secs[ 0 ] == '/' )
  {
    int max = strchr( ban->ip, ':' ) ? 128 : 32;
    int min = ent ? max / 2 : 1;
    mask = atoi( secs + 1 );
    if( mask < min || mask > max )
    {
      ADMP( va( "^3!adjustban: ^7invalid netmask (%d is not one of %d-%d)\n",
        mask, min, max ) );
      return qfalse;
    }
    G_SayArgv( 3 + skiparg++, secs, sizeof( secs ) );
  }
  if( secs[ 0 ] == '+' || secs[ 0 ] == '-' )
    mode = secs[ 0 ];
  length = G_admin_parse_time( &secs[ mode ? 1 : 0 ] );
  if( length < 0 )
    skiparg--;
  else
  {
    if( length )
    {
      if( ban->expires == 0 && mode )
      {
        ADMP( "^3!adjustban: ^7new duration must be explicit\n" );
        return qfalse;
      }
      if( mode == '+' )
        expires = ban->expires + length;
      else if( mode == '-' )
        expires = ban->expires - length;
      else
        expires = time + length;
      if( expires <= time )
      {
        ADMP( "^3!adjustban: ^7ban duration must be positive\n" );
        return qfalse;
      }
    }
    else
      length = expires = 0;
    if( !G_admin_permission( ent, ADMF_CAN_PERM_BAN ) &&
      ( length == 0 || length > maximum ) )
    {
      ADMP( "^3!adjustban: ^7you may not issue permanent bans\n" );
      expires = time + maximum;
    }

    ban->expires = expires;
    G_admin_duration( ( expires ) ? expires - time : -1, duration,
      sizeof( duration ) );
  }
  if( mask )
  {
    char *p = strchr( ban->ip, '/' );
    if( !p )
      p = ban->ip + strlen( ban->ip );
    Com_sprintf( p, sizeof( ban->ip ) - ( p - ban->ip ), "/%d", mask );
  }
  reason = G_SayConcatArgs( 3 + skiparg );
  if( *reason )
    Q_strncpyz( ban->reason, reason, sizeof( ban->reason ) );
  AP( va( "print \"^3!adjustban: ^7ban #%d for %s^7 has been updated by %s^7 "
    "%s%s%s%s%s%s\n\"",
    bnum,
    ban->name,
    ( ent ) ? ent->client->pers.netname : "console",
    ( mask ) ?
      va( "netmask: /%d%s", mask,
        ( length >= 0 || *reason ) ? ", " : "" ) : "",
    ( length >= 0 ) ? "duration: " : "",
    duration,
    ( length >= 0 && *reason ) ? ", " : "",
    ( *reason ) ? "reason: " : "",
    reason ) );
  if( ent )
    Q_strncpyz( ban->banner, ent->client->pers.netname, sizeof( ban->banner ) );
  admin_writeconfig();
  return qtrue;
}

qboolean G_admin_putteam( gentity_t *ent, int skiparg )
{
  int pids[ MAX_CLIENTS ], found;
  char name[ MAX_NAME_LENGTH ], team[ sizeof( "spectators" ) ],
       err[ MAX_STRING_CHARS ];
  gentity_t *vic;
  team_t teamnum = TEAM_NONE;

  G_SayArgv( 1 + skiparg, name, sizeof( name ) );
  G_SayArgv( 2 + skiparg, team, sizeof( team ) );
  if( G_SayArgc() < 3 + skiparg )
  {
    ADMP( "^3!putteam: ^7usage: !putteam [name] [h|a|s]\n" );
    return qfalse;
  }

  if( ( found = G_ClientNumbersFromString( name, pids, MAX_CLIENTS ) ) != 1 )
  {
    G_MatchOnePlayer( pids, found, err, sizeof( err ) );
    ADMP( va( "^3!putteam: ^7%s\n", err ) );
    return qfalse;
  }
  if( !admin_higher( ent, &g_entities[ pids[ 0 ] ] ) )
  {
    ADMP( "^3!putteam: ^7sorry, but your intended victim has a higher "
        " admin level than you\n" );
    return qfalse;
  }
  vic = &g_entities[ pids[ 0 ] ];
  teamnum = G_TeamFromString( team );
  if( teamnum == NUM_TEAMS )
  {
    ADMP( va( "^3!putteam: ^7unknown team %s\n", team ) );
    return qfalse;
  }
  if( vic->client->pers.teamSelection == teamnum )
    return qfalse;
  G_ChangeTeam( vic, teamnum );

  AP( va( "print \"^3!putteam: ^7%s^7 put %s^7 on to the %s team\n\"",
          ( ent ) ? ent->client->pers.netname : "console",
          vic->client->pers.netname, BG_TeamName( teamnum ) ) );
  return qtrue;
}

qboolean G_admin_map( gentity_t *ent, int skiparg )
{
  char map[ MAX_QPATH ];
  char layout[ MAX_QPATH ] = { "" };

  if( G_SayArgc( ) < 2 + skiparg )
  {
    ADMP( "^3!map: ^7usage: !map [map] (layout)\n" );
    return qfalse;
  }

  G_SayArgv( skiparg + 1, map, sizeof( map ) );

  if( !trap_FS_FOpenFile( va( "maps/%s.bsp", map ), NULL, FS_READ ) )
  {
    ADMP( va( "^3!map: ^7invalid map name '%s'\n", map ) );
    return qfalse;
  }

  if( G_SayArgc( ) > 2 + skiparg )
  {
    G_SayArgv( skiparg + 2, layout, sizeof( layout ) );
    if( !Q_stricmp( layout, "*BUILTIN*" ) ||
      trap_FS_FOpenFile( va( "layouts/%s/%s.dat", map, layout ),
        NULL, FS_READ ) > 0 )
    {
      trap_Cvar_Set( "g_layouts", layout );
    }
    else
    {
      ADMP( va( "^3!map: ^7invalid layout name '%s'\n", layout ) );
      return qfalse;
    }
  }

  trap_SendConsoleCommand( EXEC_APPEND, va( "map %s", map ) );
  level.restarted = qtrue;
  AP( va( "print \"^3!map: ^7map '%s' started by %s %s\n\"", map,
          ( ent ) ? ent->client->pers.netname : "console",
          ( layout[ 0 ] ) ? va( "(forcing layout '%s')", layout ) : "" ) );
  return qtrue;
}

qboolean G_admin_mute( gentity_t *ent, int skiparg )
{
  int pids[ MAX_CLIENTS ], found;
  char name[ MAX_NAME_LENGTH ], err[ MAX_STRING_CHARS ];
  char command[ MAX_ADMIN_CMD_LEN ], *cmd;
  gentity_t *vic;

  G_SayArgv( skiparg, command, sizeof( command ) );
  cmd = command;
  if( cmd && *cmd == '!' )
    cmd++;
  if( G_SayArgc() < 2 + skiparg )
  {
    ADMP( va( "^3!%s: ^7usage: !%s [name|slot#]\n", cmd, cmd ) );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, name, sizeof( name ) );
  if( ( found = G_ClientNumbersFromString( name, pids, MAX_CLIENTS ) ) != 1 )
  {
    G_MatchOnePlayer( pids, found, err, sizeof( err ) );
    ADMP( va( "^3!%s: ^7%s\n", cmd, err ) );
    return qfalse;
  }
  if( !admin_higher( ent, &g_entities[ pids[ 0 ] ] ) )
  {
    ADMP( va( "^3!%s: ^7sorry, but your intended victim has a higher admin"
        " level than you\n", cmd ) );
    return qfalse;
  }
  vic = &g_entities[ pids[ 0 ] ];
  if( vic->client->pers.muted == qtrue )
  {
    if( !Q_stricmp( cmd, "mute" ) )
    {
      ADMP( "^3!mute: ^7player is already muted\n" );
      return qtrue;
    }
    vic->client->pers.muted = qfalse;
    CPx( pids[ 0 ], "cp \"^1You have been unmuted\"" );
    AP( va( "print \"^3!unmute: ^7%s^7 has been unmuted by %s\n\"",
            vic->client->pers.netname,
            ( ent ) ? ent->client->pers.netname : "console" ) );
  }
  else
  {
    if( !Q_stricmp( cmd, "unmute" ) )
    {
      ADMP( "^3!unmute: ^7player is not currently muted\n" );
      return qtrue;
    }
    vic->client->pers.muted = qtrue;
    CPx( pids[ 0 ], "cp \"^1You've been muted\"" );
    AP( va( "print \"^3!mute: ^7%s^7 has been muted by ^7%s\n\"",
            vic->client->pers.netname,
            ( ent ) ? ent->client->pers.netname : "console" ) );
  }
  return qtrue;
}

qboolean G_admin_denybuild( gentity_t *ent, int skiparg )
{
  int pids[ MAX_CLIENTS ], found;
  char name[ MAX_NAME_LENGTH ], err[ MAX_STRING_CHARS ];
  char command[ MAX_ADMIN_CMD_LEN ], *cmd;
  gentity_t *vic;

  G_SayArgv( skiparg, command, sizeof( command ) );
  cmd = command;
  if( cmd && *cmd == '!' )
    cmd++;
  if( G_SayArgc() < 2 + skiparg )
  {
    ADMP( va( "^3!%s: ^7usage: !%s [name|slot#]\n", cmd, cmd ) );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, name, sizeof( name ) );
  if( ( found = G_ClientNumbersFromString( name, pids, MAX_CLIENTS ) ) != 1 )
  {
    G_MatchOnePlayer( pids, found, err, sizeof( err ) );
    ADMP( va( "^3!%s: ^7%s\n", cmd, err ) );
    return qfalse;
  }
  if( !admin_higher( ent, &g_entities[ pids[ 0 ] ] ) )
  {
    ADMP( va( "^3!%s: ^7sorry, but your intended victim has a higher admin"
              " level than you\n", cmd ) );
    return qfalse;
  }
  vic = &g_entities[ pids[ 0 ] ];
  if( vic->client->pers.denyBuild )
  {
    if( !Q_stricmp( cmd, "denybuild" ) )
    {
      ADMP( "^3!denybuild: ^7player already has no building rights\n" );
      return qtrue;
    }
    vic->client->pers.denyBuild = qfalse;
    CPx( pids[ 0 ], "cp \"^1You've regained your building rights\"" );
    AP( va(
      "print \"^3!allowbuild: ^7building rights for ^7%s^7 restored by %s\n\"",
      vic->client->pers.netname,
      ( ent ) ? ent->client->pers.netname : "console" ) );
  }
  else
  {
    if( !Q_stricmp( cmd, "allowbuild" ) )
    {
      ADMP( "^3!allowbuild: ^7player already has building rights\n" );
      return qtrue;
    }
    vic->client->pers.denyBuild = qtrue;
    vic->client->ps.stats[ STAT_BUILDABLE ] = BA_NONE;
    CPx( pids[ 0 ], "cp \"^1You've lost your building rights\"" );
    AP( va(
      "print \"^3!denybuild: ^7building rights for ^7%s^7 revoked by ^7%s\n\"",
      vic->client->pers.netname,
      ( ent ) ? ent->client->pers.netname : "console" ) );
  }
  return qtrue;
}

qboolean G_admin_listadmins( gentity_t *ent, int skiparg )
{
  int i, found = 0;
  char search[ MAX_NAME_LENGTH ] = {""};
  char s[ MAX_NAME_LENGTH ] = {""};
  int start = 0;
  int drawn = 0;

  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
  {
    if( g_admin_admins[ i ]->level == 0 )
      continue;
    found++;
  }
  if( !found )
  {
    ADMP( "^3!listadmins: ^7no admins defined\n" );
    return qfalse;
  }

  if( G_SayArgc() == 2 + skiparg )
  {
    G_SayArgv( 1 + skiparg, s, sizeof( s ) );
    i = s[ 0 ] == '-' && s[ 1 ] ? 1 : 0;
    for( ; s[ i ] && isdigit( s[ i ] ); i++ );
    if( !s[ i ] )
    {
      start = atoi( s );
      if( start > 0 )
        start--;
      else if( start < 0 )
        start = found + start;
    }
    else
      G_SanitiseString( s, search, sizeof( search ) );
  }

  if( start >= found || start < 0 )
    start = 0;

  drawn = admin_listadmins( ent, start, search );

  if( search[ 0 ] )
  {
    ADMP( va( "^3!listadmins:^7 found %d admins matching '%s^7'\n",
      drawn, search ) );
  }
  else
  {
    ADMBP_begin();
    ADMBP( va( "^3!listadmins:^7 showing admin %d - %d of %d.  ",
      found ? start + 1 : 0,
      start + MAX_ADMIN_LISTITEMS > found ? found : start + MAX_ADMIN_LISTITEMS,
      found ) );
    if( start + MAX_ADMIN_LISTITEMS < found )
    {
      ADMBP( va( "run '!listadmins %d' to see more",
        ( start + MAX_ADMIN_LISTITEMS + 1 ) ) );
    }
    ADMBP( "\n" );
    ADMBP_end();
  }
  return qtrue;
}

qboolean G_admin_listlayouts( gentity_t *ent, int skiparg )
{
  char list[ MAX_CVAR_VALUE_STRING ];
  char map[ MAX_QPATH ];
  int count = 0;
  char *s;
  char layout[ MAX_QPATH ] = { "" };
  int i = 0;

  if( G_SayArgc( ) == 2 + skiparg )
    G_SayArgv( 1 + skiparg, map, sizeof( map ) );
  else
    trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );

  count = G_LayoutList( map, list, sizeof( list ) );
  ADMBP_begin( );
  ADMBP( va( "^3!listlayouts:^7 %d layouts found for '%s':\n", count, map ) );
  s = &list[ 0 ];
  while( *s )
  {
    if( *s == ' ' )
    {
      ADMBP( va ( " %s\n", layout ) );
      layout[ 0 ] = '\0';
      i = 0;
    }
    else if( i < sizeof( layout ) - 2 )
    {
      layout[ i++ ] = *s;
      layout[ i ] = '\0';
    }
    s++;
  }
  if( layout[ 0 ] )
    ADMBP( va ( " %s\n", layout ) );
  ADMBP_end( );
  return qtrue;
}

qboolean G_admin_listplayers( gentity_t *ent, int skiparg )
{
  int             i, j;
  gclient_t       *p;
  char            c, t; // color and team letter
  char            *registeredname;
  char            lname[ MAX_NAME_LENGTH ];
  char            muted, denied;
  int             colorlen;
  char            namecleaned[ MAX_NAME_LENGTH ];
  char            name2cleaned[ MAX_NAME_LENGTH ];
  g_admin_level_t *l;
  g_admin_level_t *d = G_admin_level( 0 );
  qboolean        hint;
  qboolean        canset = G_admin_permission( ent, "setlevel" );

  ADMBP_begin();
  ADMBP( va( "^3!listplayers: ^7%d players connected:\n",
    level.numConnectedClients ) );
  for( i = 0; i < level.maxclients; i++ )
  {
    p = &level.clients[ i ];
    if( p->pers.connected == CON_DISCONNECTED )
      continue;
    if( p->pers.connected == CON_CONNECTING )
    {
      t = 'C';
      c = COLOR_YELLOW;
    }
    else
    {
      t = toupper( *( BG_TeamName( p->pers.teamSelection ) ) );
      if( p->pers.teamSelection == TEAM_HUMANS )
        c = COLOR_CYAN;
      else if( p->pers.teamSelection == TEAM_ALIENS )
        c = COLOR_RED;
      else
        c = COLOR_WHITE;
    }

    muted = p->pers.muted ? 'M' : ' ';
    denied = p->pers.denyBuild ? 'B' : ' ';

    l = d;
    registeredname = NULL;
    hint = canset;
    if( p->pers.admin )
    {
      if( hint )
        hint = admin_higher( ent, &g_entities[ i ] );
      if( hint || !G_admin_permission( &g_entities[ i ], ADMF_INCOGNITO ) )
      {
        l = G_admin_level( p->pers.admin->level );
        G_SanitiseString( p->pers.netname, namecleaned,
                          sizeof( namecleaned ) );
        G_SanitiseString( p->pers.admin->name,
                          name2cleaned, sizeof( name2cleaned ) );
        if( Q_stricmp( namecleaned, name2cleaned ) )
          registeredname = p->pers.admin->name;
      }
    }

    if( l )
      Q_strncpyz( lname, l->name, sizeof( lname ) );

    for( colorlen = j = 0; lname[ j ]; j++ )
    {
      if( Q_IsColorString( &lname[ j ] ) )
        colorlen += 2;
    }

    ADMBP( va( "%2i ^%c%c^7 %-2i^2%c^7 %*s^7 ^1%c%c^7 %s^7 %s%s%s\n",
              i,
              c,
              t,
              l ? l->level : 0,
              hint ? '*' : ' ',
              admin_level_maxname + colorlen,
              lname,
              muted,
              denied,
              p->pers.netname,
              ( registeredname ) ? "(a.k.a. " : "",
              ( registeredname ) ? registeredname : "",
              ( registeredname ) ? S_COLOR_WHITE ")" : "" ) );

  }
  ADMBP_end();
  return qtrue;
}

qboolean G_admin_showbans( gentity_t *ent, int skiparg )
{
  int i, j;
  int found = 0;
  int max = -1, count;
  int t;
  char duration[ 13 ];
  int max_name = 1, max_banner = 1;
  int colorlen1, colorlen2;
  int len;
  int secs;
  int start = 0;
  char filter[ MAX_NAME_LENGTH ] = {""};
  char date[ 11 ];
  char *made;
  char n1[ MAX_NAME_LENGTH ] = {""};
  qboolean ipmatch = qfalse;
  addr_t ipa, ipb;
  int neta, netb;
  char name_match[ MAX_NAME_LENGTH ] = {""};

  t = trap_RealTime( NULL );

  for( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
  {
    if( g_admin_bans[ i ]->expires != 0 && g_admin_bans[ i ]->expires <= t )
      continue;
    found++;
    max = i;
  }

  if( max < 0 )
  {
    ADMP( "^3!showbans: ^7no bans to display\n" );
    return qfalse;
  }

  if( G_SayArgc() >= 2 + skiparg )
  {
    G_SayArgv( 1 + skiparg, filter, sizeof( filter ) );
    if( G_SayArgc() >= 3 + skiparg )
    {
      start = atoi( filter );
      G_SayArgv( 2 + skiparg, filter, sizeof( filter ) );
    }
    else
    {
      i = filter[ 0 ] == '-' && filter[ 1 ] ? 1 : 0;
      for( ; filter[ i ] && isdigit( filter[ i ] ); i++ );
      if( filter[ i ] )
        G_SanitiseString( filter, name_match, sizeof( name_match ) );
      else
        start = atoi( filter );
    }
    // showbans 1 means start with ban 0
    if( start > 0 )
      start--;
    else if( start < 0 )
    {
      for( i = max, count = 0; i >= 0 && count < -start; i-- )
      {
        if( g_admin_bans[ i ]->expires == 0 || g_admin_bans[ i ]->expires > t )
          count++;
      }

      start = i + 1;
    }
    else
      ipmatch = G_AddressParse( filter, &ipa, &neta );
  }

  if( start > max )
  {
    ADMP( va( "^3!showbans: ^7%d is the last valid ban\n", max + 1 ) );
    return qfalse;
  }

  for( i = start, count = 0; i <= max && count < MAX_ADMIN_SHOWBANS; i++ )
  {
    if( g_admin_bans[ i ]->expires != 0 && g_admin_bans[ i ]->expires <= t )
      continue;

    if( ipmatch )
    {
      if( !G_AddressParse( g_admin_bans[ i ]->ip, &ipb, &netb ) )
        continue;
      if( !G_AddressCompare( &ipa, &ipb, neta ) &&
          !G_AddressCompare( &ipa, &ipb, netb ) )
        continue;
    }
    else if( name_match[ 0 ] )
    {
      G_SanitiseString( g_admin_bans[ i ]->name, n1, sizeof( n1 ) );
      if( !strstr( n1, name_match) )
        continue;
    }

    count++;

    len = Q_PrintStrlen( g_admin_bans[ i ]->name );
    if( len > max_name )
      max_name = len;

    len = Q_PrintStrlen( g_admin_bans[ i ]->banner );
    if( len > max_banner )
      max_banner = len;
  }

  ADMBP_begin();
  for( i = start, count = 0; i <= max && count < MAX_ADMIN_SHOWBANS; i++ )
  {
    if( g_admin_bans[ i ]->expires != 0 && g_admin_bans[ i ]->expires <= t )
      continue;

    if( ipmatch )
    {
      if( !G_AddressParse( g_admin_bans[ i ]->ip, &ipb, &netb ) )
        continue;
      if( !G_AddressCompare( &ipa, &ipb, neta ) &&
          !G_AddressCompare( &ipa, &ipb, netb ) )
        continue;
    }
    else if( name_match[ 0 ] )
    {
      G_SanitiseString( g_admin_bans[ i ]->name, n1, sizeof( n1 ) );
      if( !strstr( n1, name_match) )
        continue;
    }

    count++;

    // only print out the the date part of made
    date[ 0 ] = '\0';
    made = g_admin_bans[ i ]->made;
    for( j = 0; *made && *made != ' ' && j < sizeof( date ) - 1; j++ )
      date[ j ] = *made++;
    date[ j ] = 0;

    secs = g_admin_bans[ i ]->expires - t;
    G_admin_duration( secs, duration, sizeof( duration ) );

    for( colorlen1 = j = 0; g_admin_bans[ i ]->name[ j ]; j++ )
    {
      if( Q_IsColorString( &g_admin_bans[ i ]->name[ j ] ) )
        colorlen1 += 2;
    }

    for( colorlen2 = j = 0; g_admin_bans[ i ]->banner[ j ]; j++ )
    {
      if( Q_IsColorString( &g_admin_bans[ i ]->banner[ j ] ) )
        colorlen2 += 2;
    }


    ADMBP( va( "%4i %*s^7 %-15s %-8s %*s^7 %-10s\n     \\__ %s\n",
             ( i + 1 ),
             max_name + colorlen1,
             g_admin_bans[ i ]->name,
             g_admin_bans[ i ]->ip,
             date,
             max_banner + colorlen2,
             g_admin_bans[ i ]->banner,
             duration,
             g_admin_bans[ i ]->reason ) );
  }

  if( name_match[ 0 ] || ipmatch )
  {
    ADMBP( va( "^3!showbans:^7 found %d matching bans by %s.  ",
             count,
             ( ipmatch ) ? "IP" : "name" ) );
  }
  else
  {
    ADMBP( va( "^3!showbans:^7 showing bans %d - %d of %d (%d total).",
             ( found ) ? ( start + 1 ) : 0,
             i,
             max + 1,
             found ) );
  }

  if( i <= max )
    ADMBP( va( "  run !showbans %d%s%s to see more",
             i + 1,
             ( name_match[ 0 ] ) ? " " : "",
             ( name_match[ 0 ] ) ? filter : "" ) );
  ADMBP( "\n" );
  ADMBP_end();
  return qtrue;
}

qboolean G_admin_help( gentity_t *ent, int skiparg )
{
  if( G_SayArgc() < 2 + skiparg )
  {
    int i;
    int count = 0;

    ADMBP_begin();
    for( i = 0; i < adminNumCmds; i++ )
    {
      if( G_admin_permission( ent, g_admin_cmds[ i ].flag ) )
      {
        ADMBP( va( "^3!%-12s", g_admin_cmds[ i ].keyword ) );
        count++;
        // show 6 commands per line
        if( count % 6 == 0 )
          ADMBP( "\n" );
      }
    }
    for( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
    {
      if( !G_admin_permission( ent, g_admin_commands[ i ]->flag ) )
        continue;
      ADMBP( va( "^3!%-12s", g_admin_commands[ i ]->command ) );
      count++;
      // show 6 commands per line
      if( count % 6 == 0 )
        ADMBP( "\n" );
    }
    if( count % 6 )
      ADMBP( "\n" );
    ADMBP( va( "^3!help: ^7%i available commands\n", count ) );
    ADMBP( "run !help [^3command^7] for help with a specific command.\n" );
    ADMBP_end();

    return qtrue;
  }
  else
  {
    //!help param
    char param[ MAX_ADMIN_CMD_LEN ];
    char *cmd;
    g_admin_cmd_t *admincmd;
    g_admin_command_t *c;
    qboolean denied = qfalse;

    G_SayArgv( 1 + skiparg, param, sizeof( param ) );
    cmd = ( param[0] == '!' ) ? &param[1] : &param[0];
    ADMBP_begin();
    if( ( c = G_admin_command( cmd ) ) )
    {
      if( !G_admin_permission( ent, c->flag ) )
      {
        ADMBP( va( "^3!help: ^7help for '!%s':\n", c->command ) );
        ADMBP( va( " ^3Description: ^7%s\n", c->desc ) );
        ADMBP( va( " ^3Syntax: ^7!%s\n", c->command ) );
        ADMBP( va( " ^3Flag: ^7'%s'\n", c->flag ) );
        ADMBP_end( );
        return qtrue;
      }
      denied = qtrue;
    }
    if( ( admincmd = G_admin_cmd( cmd ) ) )
    {
      if( G_admin_permission( ent, admincmd->flag ) )
      {
        ADMBP( va( "^3!help: ^7help for '!%s':\n", admincmd->keyword ) );
        ADMBP( va( " ^3Description: ^7%s\n", admincmd->function ) );
        ADMBP( va( " ^3Syntax: ^7!%s %s\n", admincmd->keyword,
                 admincmd->syntax ) );
        ADMBP( va( " ^3Flag: ^7'%s'\n", admincmd->flag ) );
        ADMBP_end();
        return qtrue;
      }
      denied = qtrue;
    }
    ADMBP( va( "^3!help: ^7%s '%s'\n",
      denied ? "you do not have permission to use" : "no help found for",
      cmd ) );
    ADMBP_end( );
    return qfalse;
  }
}

qboolean G_admin_admintest( gentity_t *ent, int skiparg )
{
  g_admin_level_t *l;

  if( !ent )
  {
    ADMP( "^3!admintest: ^7you are on the console.\n" );
    return qtrue;
  }

  l = G_admin_level( ent->client->pers.admin ? ent->client->pers.admin->level : 0 );

  AP( va( "print \"^3!admintest: ^7%s^7 is a level %d admin %s%s^7%s\n\"",
          ent->client->pers.netname,
          l ? l->level : 0,
          l ? "(" : "",
          l ? l->name : "",
          l ? ")" : "" ) );
  return qtrue;
}

qboolean G_admin_allready( gentity_t *ent, int skiparg )
{
  int i = 0;
  gclient_t *cl;

  if( !level.intermissiontime )
  {
    ADMP( "^3!allready: ^7this command is only valid during intermission\n" );
    return qfalse;
  }

  for( i = 0; i < g_maxclients.integer; i++ )
  {
    cl = level.clients + i;
    if( cl->pers.connected != CON_CONNECTED )
      continue;

    if( cl->pers.teamSelection == TEAM_NONE )
      continue;

    cl->readyToExit = qtrue;
  }
  AP( va( "print \"^3!allready:^7 %s^7 says everyone is READY now\n\"",
     ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}

qboolean G_admin_endvote( gentity_t *ent, int skiparg )
{
  char teamName[ sizeof( "spectators" ) ] = {"s"};
  char command[ MAX_ADMIN_CMD_LEN ], *cmd;
  team_t team;
  qboolean cancel;
  char *msg;

  G_SayArgv( skiparg, command, sizeof( command ) );
  cmd = command;
  if( *cmd == '!' )
    cmd++;
  cancel = !Q_stricmp( cmd, "cancelvote" );
  if( G_SayArgc() == 2 + skiparg )
    G_SayArgv( 1 + skiparg, teamName, sizeof( teamName ) );
  team = G_TeamFromString( teamName );
  if( team == NUM_TEAMS )
  {
    ADMP( va( "^3!%s: ^7invalid team '%s'\n", cmd, teamName ) );
    return qfalse;
  }
  msg = va( "print \"^3!%s: ^7%s^7 decided that everyone voted %s\n\"",
    cmd, ( ent ) ? ent->client->pers.netname : "console",
    cancel ? "No" : "Yes" );
  if( !level.voteTime[ team ] )
  {
    ADMP( va( "^3!%s: ^7no vote in progress\n", cmd ) );
    return qfalse;
  }
  level.voteNo[ team ] = cancel ? level.numVotingClients[ team ] : 0;
  level.voteYes[ team ] = cancel ? 0 : level.numVotingClients[ team ];
  CheckVote( team );
  if( team == TEAM_NONE )
    AP( msg );
  else
    G_TeamCommand( team, msg );
  return qtrue;
}

qboolean G_admin_spec999( gentity_t *ent, int skiparg )
{
  int i;
  gentity_t *vic;

  for( i = 0; i < level.maxclients; i++ )
  {
    vic = &g_entities[ i ];
    if( !vic->client )
      continue;
    if( vic->client->pers.connected != CON_CONNECTED )
      continue;
    if( vic->client->pers.teamSelection == TEAM_NONE )
      continue;
    if( vic->client->ps.ping == 999 )
    {
      G_ChangeTeam( vic, TEAM_NONE );
      AP( va( "print \"^3!spec999: ^7%s^7 moved %s^7 to spectators\n\"",
        ( ent ) ? ent->client->pers.netname : "console",
        vic->client->pers.netname ) );
    }
  }
  return qtrue;
}

qboolean G_admin_rename( gentity_t *ent, int skiparg )
{
  int pids[ MAX_CLIENTS ], found;
  char name[ MAX_NAME_LENGTH ];
  char newname[ MAX_NAME_LENGTH ];
  char err[ MAX_STRING_CHARS ];
  char userinfo[ MAX_INFO_STRING ];
  gentity_t *victim = NULL;

  if( G_SayArgc() < 3 + skiparg )
  {
    ADMP( "^3!rename: ^7usage: !rename [name] [newname]\n" );
    return qfalse;
  }
  G_SayArgv( 1 + skiparg, name, sizeof( name ) );
  Q_strncpyz( newname, G_SayConcatArgs( 2 + skiparg ), sizeof( newname ) );
  if( ( found = G_ClientNumbersFromString( name, pids, MAX_CLIENTS ) ) != 1 )
  {
    G_MatchOnePlayer( pids, found, err, sizeof( err ) );
    ADMP( va( "^3!rename: ^7%s\n", err ) );
    return qfalse;
  }
  victim = &g_entities[ pids[ 0 ] ];
  if( !admin_higher( ent, victim ) )
  {
    ADMP( "^3!rename: ^7sorry, but your intended victim has a higher admin"
        " level than you\n" );
    return qfalse;
  }
  if( !G_admin_name_check( victim, newname, err, sizeof( err ) ) )
  {
    ADMP( va( "^3!rename: ^7%s\n", err ) );
    return qfalse;
  }
  victim->client->pers.nameChanges--;
  victim->client->pers.nameChangeTime = 0;
  trap_GetUserinfo( pids[ 0 ], userinfo, sizeof( userinfo ) );
  AP( va( "print \"^3!rename: ^7%s^7 has been renamed to %s^7 by %s\n\"",
          victim->client->pers.netname,
          newname,
          ( ent ) ? ent->client->pers.netname : "console" ) );
  Info_SetValueForKey( userinfo, "name", newname );
  trap_SetUserinfo( pids[ 0 ], userinfo );
  ClientUserinfoChanged( pids[ 0 ] );
  return qtrue;
}

qboolean G_admin_restart( gentity_t *ent, int skiparg )
{
  char      layout[ MAX_CVAR_VALUE_STRING ] = { "" };
  char      teampref[ MAX_STRING_CHARS ] = { "" };
  int       i;
  gclient_t *cl;

  if( G_SayArgc( ) > 1 + skiparg )
  {
    char map[ MAX_QPATH ];

    trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
    G_SayArgv( skiparg + 1, layout, sizeof( layout ) );

    // Figure out which argument is which
    if( Q_stricmp( layout, "keepteams" ) && 
        Q_stricmp( layout, "keepteamslock" ) && 
        Q_stricmp( layout, "switchteams" ) && 
        Q_stricmp( layout, "switchteamslock" ) )
    {
      if( !Q_stricmp( layout, "*BUILTIN*" ) ||
          trap_FS_FOpenFile( va( "layouts/%s/%s.dat", map, layout ),
                             NULL, FS_READ ) > 0 )
      {
        trap_Cvar_Set( "g_layouts", layout );
      }
      else
      {
        ADMP( va( "^3!restart: ^7layout '%s' does not exist\n", layout ) );
        return qfalse;
      }
    }
    else 
    {
      layout[ 0 ] = '\0';
      G_SayArgv( skiparg + 1, teampref, sizeof( teampref ) );    
    }
  }
  
  if( G_SayArgc( ) > 2 + skiparg ) 
    G_SayArgv( skiparg + 2, teampref, sizeof( teampref ) );      
  
  if( !Q_stricmpn( teampref, "keepteams", 9 ) )
  {
    for( i = 0; i < g_maxclients.integer; i++ )
    {
      cl = level.clients + i;
      if( cl->pers.connected != CON_CONNECTED )
        continue;

      if( cl->pers.teamSelection == TEAM_NONE )
        continue;

      cl->sess.restartTeam = cl->pers.teamSelection;
    }
  } 
  else if( !Q_stricmpn( teampref, "switchteams", 11 ) )
  {
    for( i = 0; i < g_maxclients.integer; i++ )
    {
      cl = level.clients + i;

      if( cl->pers.connected != CON_CONNECTED )
        continue;

      if( cl->pers.teamSelection == TEAM_HUMANS )
        cl->sess.restartTeam = TEAM_ALIENS;
      else if(cl->pers.teamSelection == TEAM_ALIENS )
	    cl->sess.restartTeam = TEAM_HUMANS;
    }  	  
  }
  
  if( !Q_stricmp( teampref, "switchteamslock" ) || 
      !Q_stricmp( teampref, "keepteamslock" ) )
    trap_Cvar_Set( "g_lockTeamsAtStart", "1" );

  trap_SendConsoleCommand( EXEC_APPEND, "map_restart" );
  
  AP( va( "print \"^3!restart: ^7map restarted by %s %s %s\n\"",
          ( ent ) ? ent->client->pers.netname : "console",
          ( layout[ 0 ] ) ? va( "^7(forcing layout '%s^7')", layout ) : "",
          ( teampref[ 0 ] ) ? va( "^7(with teams option: '%s^7')", teampref ) : "" ) );
  return qtrue;
}

qboolean G_admin_nextmap( gentity_t *ent, int skiparg )
{
  AP( va( "print \"^3!nextmap: ^7%s^7 decided to load the next map\n\"",
    ( ent ) ? ent->client->pers.netname : "console" ) );
  level.lastWin = TEAM_NONE;
  trap_SetConfigstring( CS_WINNER, "Evacuation" );
  LogExit( va( "nextmap was run by %s",
    ( ent ) ? ent->client->pers.netname : "console" ) );
  return qtrue;
}

qboolean G_admin_namelog( gentity_t *ent, int skiparg )
{
  int i, j;
  char search[ MAX_NAME_LENGTH ] = {""};
  char s2[ MAX_NAME_LENGTH ] = {""};
  char n2[ MAX_NAME_LENGTH ] = {""};
  int printed = 0;
  addr_t a, b;
  int mask = -1;
  qboolean ipmatch = qfalse;

  if( G_SayArgc() > 1 + skiparg )
  {
    G_SayArgv( 1 + skiparg, search, sizeof( search ) );
    ipmatch = G_AddressParse( search, &a, &mask );
    if( !ipmatch )
      G_SanitiseString( search, s2, sizeof( s2 ) );
  }
  ADMBP_begin();
  for( i = 0; i < MAX_ADMIN_NAMELOGS && g_admin_namelog[ i ]; i++ )
  {
    if( search[ 0 ] )
    {
      if( ipmatch )
      {
        if( !G_AddressParse( g_admin_namelog[ i ]->ip, &b, NULL ) ||
            !G_AddressCompare( &a, &b, mask ) )
          continue;
      }
      else
      {
        for( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
          g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
        {
          G_SanitiseString( g_admin_namelog[ i ]->name[ j ], n2, sizeof( n2 ) );
          if( strstr( n2, s2 ) )
          {
            break;
          }
        }
        if( j == MAX_ADMIN_NAMELOG_NAMES || !g_admin_namelog[ i ]->name[ j ][ 0 ] )
          continue;
      }
    }
    printed++;
    ADMBP( va( "%s (*%s) %15s^7",
      ( g_admin_namelog[ i ]->slot > -1 ) ?
        va( "^3%-2d", g_admin_namelog[ i ]->slot ) : "- ",
      g_admin_namelog[ i ]->guid + 24, g_admin_namelog[ i ]->ip ) );
    for( j = 0; j < MAX_ADMIN_NAMELOG_NAMES &&
      g_admin_namelog[ i ]->name[ j ][ 0 ]; j++ )
    {
      ADMBP( va( " '%s^7'", g_admin_namelog[ i ]->name[ j ] ) );
    }
    ADMBP( "\n" );
  }
  ADMBP( va( "^3!namelog:^7 %d recent clients found\n", printed ) );
  ADMBP_end();
  return qtrue;
}

qboolean G_admin_lock( gentity_t *ent, int skiparg )
{
  char command[ MAX_ADMIN_CMD_LEN ], *cmd;
  char teamName[ sizeof( "aliens" ) ];
  team_t team;
  qboolean lock, fail = qfalse;

  G_SayArgv( skiparg, command, sizeof( command ) );
  cmd = command;
  if( *cmd == '!' )
    cmd++;
  if( G_SayArgc() < 2 + skiparg )
  {
    ADMP( va( "^3!%s: ^7usage: !%s [a|h]\n", cmd, cmd ) );
    return qfalse;
  }
  lock = !Q_stricmp( cmd, "lock" );
  G_SayArgv( 1 + skiparg, teamName, sizeof( teamName ) );
  team = G_TeamFromString( teamName );

  if( team == TEAM_ALIENS )
  {
    if( level.alienTeamLocked == lock )
      fail = qtrue;
    else
      level.alienTeamLocked = lock;
  }
  else if( team == TEAM_HUMANS )
  {
    if( level.humanTeamLocked == lock )
      fail = qtrue;
    else
      level.humanTeamLocked = lock;
  }
  else
  {
    ADMP( va( "^3!%s: ^7invalid team: '%s'\n", cmd, teamName ) );
    return qfalse;
  }

  if( fail )
  {
    ADMP( va( "^3!%s: ^7the %s team is %s locked\n",
      cmd, BG_TeamName( team ), lock ? "already" : "not currently" ) );

    return qfalse;
  }

  AP( va( "print \"^3!%s: ^7the %s team has been %slocked by %s\n\"",
    cmd, BG_TeamName( team ), lock ? "" : "un",
    ent ? ent->client->pers.netname : "console" ) );

  return qtrue;
}

/*
================
 G_admin_print

 This function facilitates the ADMP define.  ADMP() is similar to CP except
 that it prints the message to the server console if ent is not defined.
================
*/
void G_admin_print( gentity_t *ent, char *m )
{
  if( ent )
    trap_SendServerCommand( ent - level.gentities, va( "print \"%s\"", m ) );
  else
  {
    char m2[ MAX_STRING_CHARS ];
    if( !trap_Cvar_VariableIntegerValue( "com_ansiColor" ) )
    {
      G_DecolorString( m, m2, sizeof( m2 ) );
      trap_Print( m2 );
    }
    else
      trap_Print( m );
  }
}

void G_admin_buffer_begin()
{
  g_bfb[ 0 ] = '\0';
}

void G_admin_buffer_end( gentity_t *ent )
{
  ADMP( g_bfb );
}

void G_admin_buffer_print( gentity_t *ent, char *m )
{
  // 1022 - strlen("print 64 \"\"") - 1
  if( strlen( m ) + strlen( g_bfb ) >= 1009 )
  {
    ADMP( g_bfb );
    g_bfb[ 0 ] = '\0';
  }
  Q_strcat( g_bfb, sizeof( g_bfb ), m );
}


void G_admin_cleanup()
{
  int i = 0;

  for( i = 0; i < MAX_ADMIN_LEVELS && g_admin_levels[ i ]; i++ )
  {
    BG_Free( g_admin_levels[ i ] );
    g_admin_levels[ i ] = NULL;
  }
  for( i = 0; i < MAX_ADMIN_ADMINS && g_admin_admins[ i ]; i++ )
  {
    BG_Free( g_admin_admins[ i ] );
    g_admin_admins[ i ] = NULL;
  }
  for( i = 0; i < MAX_ADMIN_BANS && g_admin_bans[ i ]; i++ )
  {
    BG_Free( g_admin_bans[ i ] );
    g_admin_bans[ i ] = NULL;
  }
  for( i = 0; i < MAX_ADMIN_COMMANDS && g_admin_commands[ i ]; i++ )
  {
    BG_Free( g_admin_commands[ i ] );
    g_admin_commands[ i ] = NULL;
  }
}
