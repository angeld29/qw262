/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 */
// vm.c -- virtual machine
/*


intermix code and data
symbol table

a dll has one imported function: VM_SystemCall
and one exported function: Perform


*/
#ifdef USE_PR2a
#include "vm_local.h"

opcode_info_t ops[ OP_MAX ] = 
{
	{ 0, 0, 0, 0 }, // undef
	{ 0, 0, 0, 0 }, // ignore
	{ 0, 0, 0, 0 }, // break

	{ 4, 0, 0, 0 }, // enter
	{ 4,-4, 0, 0 }, // leave
	{ 0, 0, 1, 0 }, // call
	{ 0, 4, 0, 0 }, // push
	{ 0,-4, 1, 0 }, // pop

	{ 4, 4, 0, 0 }, // const
	{ 4, 4, 0, 0 }, // local
	{ 0,-4, 1, 0 }, // jump

	{ 4,-8, 2, JUMP }, // eq
	{ 4,-8, 2, JUMP }, // ne

	{ 4,-8, 2, JUMP }, // lti
	{ 4,-8, 2, JUMP }, // lei
	{ 4,-8, 2, JUMP }, // gti
	{ 4,-8, 2, JUMP }, // gei

	{ 4,-8, 2, JUMP }, // ltu
	{ 4,-8, 2, JUMP }, // leu
	{ 4,-8, 2, JUMP }, // gtu
	{ 4,-8, 2, JUMP }, // geu

	{ 4,-8, 2, JUMP }, // eqf
	{ 4,-8, 2, JUMP }, // nef

	{ 4,-8, 2, JUMP }, // ltf
	{ 4,-8, 2, JUMP }, // lef
	{ 4,-8, 2, JUMP }, // gtf
	{ 4,-8, 2, JUMP }, // gef

	{ 0, 0, 1, 0 }, // load1
	{ 0, 0, 1, 0 }, // load2
	{ 0, 0, 1, 0 }, // load4
	{ 0,-8, 2, 0 }, // store1
	{ 0,-8, 2, 0 }, // store2
	{ 0,-8, 2, 0 }, // store4
	{ 1,-4, 1, 0 }, // arg
	{ 4,-8, 2, 0 }, // bcopy

	{ 0, 0, 1, 0 }, // sex8
	{ 0, 0, 1, 0 }, // sex16

	{ 0, 0, 1, 0 }, // negi
	{ 0,-4, 3, 0 }, // add
	{ 0,-4, 3, 0 }, // sub
	{ 0,-4, 3, 0 }, // divi
	{ 0,-4, 3, 0 }, // divu
	{ 0,-4, 3, 0 }, // modi
	{ 0,-4, 3, 0 }, // modu
	{ 0,-4, 3, 0 }, // muli
	{ 0,-4, 3, 0 }, // mulu

	{ 0,-4, 3, 0 }, // band
	{ 0,-4, 3, 0 }, // bor
	{ 0,-4, 3, 0 }, // bxor
	{ 0, 0, 1, 0 }, // bcom

	{ 0,-4, 3, 0 }, // lsh
	{ 0,-4, 3, 0 }, // rshi
	{ 0,-4, 3, 0 }, // rshu

	{ 0, 0, 1, 0 }, // negf
	{ 0,-4, 3, 0 }, // addf
	{ 0,-4, 3, 0 }, // subf
	{ 0,-4, 3, 0 }, // divf
	{ 0,-4, 3, 0 }, // mulf

	{ 0, 0, 1, 0 }, // cvif
	{ 0, 0, 1, 0 } // cvfi
};

const char *opname[ 256 ] = {
	"OP_UNDEF", 

	"OP_IGNORE", 

	"OP_BREAK",

	"OP_ENTER",
	"OP_LEAVE",
	"OP_CALL",
	"OP_PUSH",
	"OP_POP",

	"OP_CONST",

	"OP_LOCAL",

	"OP_JUMP",

	//-------------------

	"OP_EQ",
	"OP_NE",

	"OP_LTI",
	"OP_LEI",
	"OP_GTI",
	"OP_GEI",

	"OP_LTU",
	"OP_LEU",
	"OP_GTU",
	"OP_GEU",

	"OP_EQF",
	"OP_NEF",

	"OP_LTF",
	"OP_LEF",
	"OP_GTF",
	"OP_GEF",

	//-------------------

	"OP_LOAD1",
	"OP_LOAD2",
	"OP_LOAD4",
	"OP_STORE1",
	"OP_STORE2",
	"OP_STORE4",
	"OP_ARG",

	"OP_BLOCK_COPY",

	//-------------------

	"OP_SEX8",
	"OP_SEX16",

	"OP_NEGI",
	"OP_ADD",
	"OP_SUB",
	"OP_DIVI",
	"OP_DIVU",
	"OP_MODI",
	"OP_MODU",
	"OP_MULI",
	"OP_MULU",

	"OP_BAND",
	"OP_BOR",
	"OP_BXOR",
	"OP_BCOM",

	"OP_LSH",
	"OP_RSHI",
	"OP_RSHU",

	"OP_NEGF",
	"OP_ADDF",
	"OP_SUBF",
	"OP_DIVF",
	"OP_MULF",

	"OP_CVIF",
	"OP_CVFI"
};

cvar_t	*vm_rtChecks;

int		vm_debugLevel;

// used by Com_Error to get rid of running vm's before longjmp
static int forced_unload;

//struct vm_s	vmTable[ VM_COUNT ];
void VM_VmInfo_f( void );
void VM_VmProfile_f( void );


void VM_Debug( int level ) {
	vm_debugLevel = level;
}


/*
==============
VM_CheckBounds
==============
*/
void VM_CheckBounds( const vm_t *vm, unsigned int address, unsigned int length )
{
	//if ( !vm->entryPoint )
	{
		if ( (address | length) > vm->dataMask || (address + length) > vm->dataMask )
		{
			SV_Error( "program tried to bypass data segment bounds" );
		}
	}
}

/*
==============
VM_CheckBounds2
==============
*/
void VM_CheckBounds2( const vm_t *vm, unsigned int addr1, unsigned int addr2, unsigned int length )
{
	//if ( !vm->entryPoint )
	{
		if ( (addr1 | addr2 | length) > vm->dataMask || (addr1 + length) > vm->dataMask || (addr2+length) > vm->dataMask )
		{
			SV_Error( "program tried to bypass data segment bounds" );
		}
	}
}

/*
==============
VM_Init
==============
*/
cvar_t	sv_progtype = {"sv_progtype","0"};	// bound the size of the
cvar_t	sv_enableprofile = {"sv_enableprofile","0"};	
cvar_t	sv_progsname = {"sv_progsname", "qwprogs"};

void ED2_PrintEdicts (void);
void PR2_Profile_f (void);
void ED2_PrintEdict_f (void);
void ED_Count (void);
void PR_CleanLogText_Init(); 

void VM_Init( void ) {
	int p;
	int usedll;
	Cvar_RegisterVariable(&sv_progtype);
	Cvar_RegisterVariable(&sv_progsname);
	Cvar_RegisterVariable(&sv_enableprofile);

	p = COM_CheckParm ("-progtype");

	if (p && p < com_argc)
	{
		usedll = atoi(com_argv[p + 1]);

		if (usedll > 2)
			usedll = VM_NONE;
		Cvar_SetValue(&sv_progtype,usedll);
	}


	Cmd_AddCommand ("edict", ED2_PrintEdict_f);
	Cmd_AddCommand ("edicts", ED2_PrintEdicts);
	Cmd_AddCommand ("edictcount", ED_Count);
    
	Cmd_AddCommand( "vmprofile", VM_VmProfile_f );
	Cmd_AddCommand( "vminfo", VM_VmInfo_f );

	//Cmd_AddCommand ("mod", PR2_GameConsoleCommand);

	PR_CleanLogText_Init();
}

/*
===============
VM_ValueToSymbol

Assumes a program counter value
===============
*/
const char *VM_ValueToSymbol( vm_t *vm, int value ) {
	vmSymbol_t	*sym;
	static char		text[MAX_TOKEN_CHARS];

	sym = vm->symbols;
	if ( !sym ) {
		return "NO SYMBOLS";
	}

	// find the symbol
	while ( sym->next && sym->next->symValue <= value ) {
		sym = sym->next;
	}

	if ( value == sym->symValue ) {
		return sym->symName;
	}

	Q_snprintfz( text, sizeof( text ), "%s+%i", sym->symName, value - sym->symValue );

	return text;
}

/*
===============
VM_ValueToFunctionSymbol

For profiling, find the symbol behind this value
===============
*/
vmSymbol_t *VM_ValueToFunctionSymbol( vm_t *vm, int value ) {
	vmSymbol_t	*sym;
	static vmSymbol_t	nullSym;

	sym = vm->symbols;
	if ( !sym ) {
		return &nullSym;
	}

	while ( sym->next && sym->next->symValue <= value ) {
		sym = sym->next;
	}

	return sym;
}

/*
===============
VM_SymbolToValue
===============
*/
int VM_SymbolToValue( vm_t *vm, const char *symbol ) {
	vmSymbol_t	*sym;

	for ( sym = vm->symbols ; sym ; sym = sym->next ) {
		if ( !strcmp( symbol, sym->symName ) ) {
			return sym->symValue;
		}
	}
	return 0;
}

/*
===============
ParseHex
===============
*/
int	ParseHex( const char *text ) {
	int		value;
	int		c;

	value = 0;
	while ( ( c = *text++ ) != 0 ) {
		if ( c >= '0' && c <= '9' ) {
			value = value * 16 + c - '0';
			continue;
		}
		if ( c >= 'a' && c <= 'f' ) {
			value = value * 16 + 10 + c - 'a';
			continue;
		}
		if ( c >= 'A' && c <= 'F' ) {
			value = value * 16 + 10 + c - 'A';
			continue;
		}
	}

	return value;
}

/*
===============
VM_LoadSymbols
===============
*/
void VM_LoadSymbols( vm_t *vm ) {
    return;
	/*union {
		char	*c;
		void	*v;
	} mapfile;
	const char *text_p, *token;
	char	name[MAX_QPATH];
	char	symbols[MAX_QPATH];
	vmSymbol_t	**prev, *sym;
	int		count;
	int		value;
	int		chars;
	int		segment;
	int		numInstructions;

	// don't load symbols if not developer
	//if ( !com_developer->integer ) { return; }

	COM_StripExtension(vm->name, name);
    Q_snprintfz( symbols, sizeof( symbols ), "vm/%s.map", name );
	mapfile.v = COM_LoadTempFile( name );
	if ( !mapfile.c ) {
		Con_Printf( "Couldn't load symbol file: %s\n", symbols );
		return;
	}

	numInstructions = vm->instructionCount;

	// parse the symbols
	text_p = mapfile.c;
	prev = &vm->symbols;
	count = 0;

	while ( 1 ) {
		text_p = COM_Parse( text_p );
		if ( !text_p ) {
			break;
		}
		segment = ParseHex( com_token );
		if ( segment ) {
			COM_Parse( text_p );
			COM_Parse( text_p );
			continue;		// only load code segment values
		}

		text_p = COM_Parse( text_p );
		if ( !text_p ) {
			Con_Printf( "WARNING: incomplete line at end of file\n" );
			break;
		}
		value = ParseHex( com_token );

		text_p = COM_Parse( text_p );
		if ( !text_p ) {
			Con_Printf( "WARNING: incomplete line at end of file\n" );
			break;
		}
		chars = strlen( com_token );
		sym = Hunk_Alloc( sizeof( *sym ) + chars, h_high );
		*prev = sym;
		prev = &sym->next;
		sym->next = NULL;

		// convert value from an instruction number to a code offset
		if ( vm->instructionPointers && value >= 0 && value < numInstructions ) {
			value = vm->instructionPointers[value];
		}

		sym->symValue = value;
		strlcpy( sym->symName, com_token, chars + 1 );

		count++;
	}

	vm->numSymbols = count;
	Con_Printf( "%i symbols parsed from %s\n", count, symbols );
    */
}

static void VM_SwapLongs( void *data, int length )
{
	int i, *ptr;
	ptr = (int *) data;
	length /= sizeof( int );
	for ( i = 0; i < length; i++ ) {
		ptr[ i ] = LittleLong( ptr[ i ] );
	}
}

/*
=================
VM_ValidateHeader
=================
*/
static char *VM_ValidateHeader( vmHeader_t *header, int fileSize ) 
{
	static char errMsg[128];
	int n;

	// truncated
	if ( fileSize < ( sizeof( vmHeader_t ) - sizeof( int ) ) ) {
		sprintf( errMsg, "truncated image header (%i bytes long)", fileSize );
		return errMsg;
	}

	// bad magic
	if ( LittleLong( header->vmMagic ) != VM_MAGIC && LittleLong( header->vmMagic ) != VM_MAGIC_VER2 ) {
		sprintf( errMsg, "bad file magic %08x", LittleLong( header->vmMagic ) );
		return errMsg;
	}
	
	// truncated
	if ( fileSize < sizeof( vmHeader_t ) && LittleLong( header->vmMagic ) != VM_MAGIC_VER2 ) {
		sprintf( errMsg, "truncated image header (%i bytes long)", fileSize );
		return errMsg;
	}

	if ( LittleLong( header->vmMagic ) == VM_MAGIC_VER2 )
		n = sizeof( vmHeader_t );
	else
		n = ( sizeof( vmHeader_t ) - sizeof( int ) );

	// byte swap the header
	VM_SwapLongs( header, n );

	// bad code offset
	if ( header->codeOffset >= fileSize ) {
		sprintf( errMsg, "bad code segment offset %i", header->codeOffset );
		return errMsg;
	}

	// bad code length
	if ( header->codeLength <= 0 || header->codeOffset + header->codeLength > fileSize ) {
		sprintf( errMsg, "bad code segment length %i", header->codeLength );
		return errMsg;
	}

	// bad data offset
	if ( header->dataOffset >= fileSize || header->dataOffset != header->codeOffset + header->codeLength ) {
		sprintf( errMsg, "bad data segment offset %i", header->dataOffset );
		return errMsg;
	}

	// bad data length
	if ( header->dataOffset + header->dataLength > fileSize )  {
		sprintf( errMsg, "bad data segment length %i", header->dataLength );
		return errMsg;
	}

	if ( header->vmMagic == VM_MAGIC_VER2 ) 
	{
		// bad lit/jtrg length
		if ( header->dataOffset + header->dataLength + header->litLength + header->jtrgLength != fileSize ) {
			sprintf( errMsg, "bad lit/jtrg segment length" );
			return errMsg;
		}
	} 
	// bad lit length
	else if ( header->dataOffset + header->dataLength + header->litLength != fileSize ) 
	{
		sprintf( errMsg, "bad lit segment length %i", header->litLength );
		return errMsg;
	}

	return NULL;	
}

/*
=================
VM_LoadQVM

Load a .qvm file

if ( alloc )
 - Validate header, swap data
 - Alloc memory for data/instructions
 - Alloc memory for instructionPointers - NOT NEEDED
 - Load instructions
 - Clear/load data
else
 - Check for header changes
 - Clear/load data

=================
*/
static vmHeader_t *VM_LoadQVM( vm_t *vm, qboolean alloc ) {
	int					length;
	unsigned int		dataLength;
	unsigned int		dataAlloc;
	int					i;
	char				filename[MAX_QPATH], *errorMsg;
	unsigned int		crc32sum;
	//qboolean			tryjts;
	vmHeader_t			*header;
	char    num[32];

	// load the image
	Q_snprintfz( filename, sizeof(filename), "vm/%s.qvm", vm->name );
	Con_Printf( "Loading vm file %s...\n", filename );
	header = ( vmHeader_t*)COM_LoadTempFile( filename );
    length = com_filesize;
	if ( !header ) {
		Con_Printf( "Failed.\n" );
		VM_Free( vm );
		return NULL;
	}

	crc32sum = CRC_Block( ( byte * ) header, com_filesize );
	sprintf( num, "%i",  crc32sum );
	Info_SetValueForStarKey( svs.info, "*progs", num, MAX_SERVERINFO_STRING );

	// will also swap header
	errorMsg = VM_ValidateHeader( header, length );
	if ( errorMsg ) {
		VM_Free( vm );
		Con_Printf( "%s\n", errorMsg );
		return NULL;
	}

	vm->crc32sum = crc32sum;
	//tryjts = qfalse;

	if( header->vmMagic == VM_MAGIC_VER2 ) {
		Con_Printf( "...which has vmMagic VM_MAGIC_VER2\n" );
	} else {
	//	tryjts = qtrue;
	}

	vm->exactDataLength = header->dataLength + header->litLength + header->bssLength;
	dataLength = vm->exactDataLength + PROGRAM_STACK_EXTRA;
	vm->dataLength = dataLength;

	// round up to next power of 2 so all data operations can
	// be mask protected
	for ( i = 0 ; dataLength > ( 1 << i ) ; i++ ) 
		;
	dataLength = 1 << i;

	// reserve some space for effective LOCAL+LOAD* checks
	dataAlloc = dataLength + 1024;

	if ( dataLength >= (1U<<31) || dataAlloc >= (1U<<31) ) {
		VM_Free( vm );
		Con_Printf( "%s: data segment is too large\n", __func__ );
		return NULL;
	}

	if ( alloc ) {
		// allocate zero filled space for initialized and uninitialized data
		vm->dataBase = Hunk_Alloc( dataAlloc);
		vm->dataMask = dataLength - 1;
		vm->dataAlloc = dataAlloc;
	} else {
		// clear the data, but make sure we're not clearing more than allocated
		if ( vm->dataAlloc != dataAlloc ) {
			VM_Free( vm );
			Con_Printf( "Warning: Data region size of %s not matching after"
					"VM_Restart()\n", filename );
			return NULL;
		}
		memset( vm->dataBase, 0, vm->dataAlloc );
	}

	// copy the intialized data
	memcpy( vm->dataBase, (byte *)header + header->dataOffset, header->dataLength + header->litLength );

	// byte swap the longs
	VM_SwapLongs( vm->dataBase, header->dataLength );

	if( header->vmMagic == VM_MAGIC_VER2 ) {
		int previousNumJumpTableTargets = vm->numJumpTableTargets;

		header->jtrgLength &= ~0x03;

		vm->numJumpTableTargets = header->jtrgLength >> 2;
		Con_Printf( "Loading %d jump table targets\n", vm->numJumpTableTargets );

		if ( alloc ) {
			vm->jumpTableTargets = Hunk_Alloc( header->jtrgLength);
		} else {
			if ( vm->numJumpTableTargets != previousNumJumpTableTargets ) {
				VM_Free( vm );

				Con_Printf( "Warning: Jump table size of %s not matching after "
						"VM_Restart()\n", filename );
				return NULL;
			}

			memset( vm->jumpTableTargets, 0, header->jtrgLength );
		}

		memcpy( vm->jumpTableTargets, (byte *)header + header->dataOffset +
				header->dataLength + header->litLength, header->jtrgLength );

		// byte swap the longs
		VM_SwapLongs( vm->jumpTableTargets, header->jtrgLength );
	}

	/*if ( tryjts == qtrue && (length = Load_JTS( vm, crc32sum, NULL, vmPakIndex )) >= 0 ) {
		// we are trying to load newer file?
		if ( vm->jumpTableTargets && vm->numJumpTableTargets != length >> 2 ) {
			Con_Printf( "Reload jts file\n" );
			vm->jumpTableTargets = NULL;
			alloc = qtrue;
		}
		vm->numJumpTableTargets = length >> 2;
		Con_Printf( "Loading %d external jump table targets\n", vm->numJumpTableTargets );
		if ( alloc == qtrue ) {
			vm->jumpTableTargets = Hunk_Alloc( length);
		} else {
			memset( vm->jumpTableTargets, 0, length );
		}
		Load_JTS( vm, crc32sum, vm->jumpTableTargets, vmPakIndex );
	}*/

	return header;
}


#endif				/* USE_PR2 */
