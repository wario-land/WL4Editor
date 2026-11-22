@ Clean-room MPlay symbolic constants for WL4Editor.
@
@ This file contains only instruction names and operand constants. It is not
@ Nintendo's MPlayDef.s and contains no copied implementation or comments.
@
@ The .equ values below are the bytes/operands consumed by the MPlay engine.
@ Comments explain the meaning; changing a value changes the instruction set.


@ Wait instructions: Wxx waits xx MPlay ticks. The suffix is the
@ duration, not the constant value; the constants are consecutive bytes.
@ Only the engine's supported duration table is named, so 25-27, 29,
@ 31, 33-35, ... are absent; the corresponding byte still exists.
@ The same duration table is reused by Nxx note commands.

	.equ	W00,	0x80 @ wait 00 ticks
	.equ	W01,	W00+1 @ wait 01 ticks
	.equ	W02,	W00+2 @ wait 02 ticks
	.equ	W03,	W00+3 @ wait 03 ticks
	.equ	W04,	W00+4 @ wait 04 ticks
	.equ	W05,	W00+5 @ wait 05 ticks
	.equ	W06,	W00+6 @ wait 06 ticks
	.equ	W07,	W00+7 @ wait 07 ticks
	.equ	W08,	W00+8 @ wait 08 ticks
	.equ	W09,	W00+9 @ wait 09 ticks
	.equ	W10,	W00+10 @ wait 10 ticks
	.equ	W11,	W00+11 @ wait 11 ticks
	.equ	W12,	W00+12 @ wait 12 ticks
	.equ	W13,	W00+13 @ wait 13 ticks
	.equ	W14,	W00+14 @ wait 14 ticks
	.equ	W15,	W00+15 @ wait 15 ticks
	.equ	W16,	W00+16 @ wait 16 ticks
	.equ	W17,	W00+17 @ wait 17 ticks
	.equ	W18,	W00+18 @ wait 18 ticks
	.equ	W19,	W00+19 @ wait 19 ticks
	.equ	W20,	W00+20 @ wait 20 ticks
	.equ	W21,	W00+21 @ wait 21 ticks
	.equ	W22,	W00+22 @ wait 22 ticks
	.equ	W23,	W00+23 @ wait 23 ticks
	.equ	W24,	W00+24 @ wait 24 ticks
	.equ	W28,	W00+25 @ wait 28 ticks
	.equ	W30,	W00+26 @ wait 30 ticks
	.equ	W32,	W00+27 @ wait 32 ticks
	.equ	W36,	W00+28 @ wait 36 ticks
	.equ	W40,	W00+29 @ wait 40 ticks
	.equ	W42,	W00+30 @ wait 42 ticks
	.equ	W44,	W00+31 @ wait 44 ticks
	.equ	W48,	W00+32 @ wait 48 ticks
	.equ	W52,	W00+33 @ wait 52 ticks
	.equ	W54,	W00+34 @ wait 54 ticks
	.equ	W56,	W00+35 @ wait 56 ticks
	.equ	W60,	W00+36 @ wait 60 ticks
	.equ	W64,	W00+37 @ wait 64 ticks
	.equ	W66,	W00+38 @ wait 66 ticks
	.equ	W68,	W00+39 @ wait 68 ticks
	.equ	W72,	W00+40 @ wait 72 ticks
	.equ	W76,	W00+41 @ wait 76 ticks
	.equ	W78,	W00+42 @ wait 78 ticks
	.equ	W80,	W00+43 @ wait 80 ticks
	.equ	W84,	W00+44 @ wait 84 ticks
	.equ	W88,	W00+45 @ wait 88 ticks
	.equ	W90,	W00+46 @ wait 90 ticks
	.equ	W92,	W00+47 @ wait 92 ticks
	.equ	W96,	W00+48 @ wait 96 ticks

@ FINE: Terminate the track. The engine does not continue past this point.
	.equ	FINE,	0xb1 @ FINE

@ GOTO: Unconditional absolute pointer jump; the .word after it is the target.
	.equ	GOTO,	0xb2 @ GOTO

@ PATT: Call a pattern/subroutine. It runs until the matching PEND.
	.equ	PATT,	0xb3 @ PATT

@ PEND: Return from a PATT call.
	.equ	PEND,	0xb4 @ PEND

@ REPT: Repeat a block: one count byte followed by an absolute pointer.
	.equ	REPT,	0xb5 @ REPT

@ MEMACC: Memory arithmetic/branch operation: op, address and data bytes.
	.equ	MEMACC,	0xb9 @ MEMACC

@ PRIO: Track priority (0..127).
	.equ	PRIO,	0xba @ PRIO

@ TEMPO: Tempo in BPM/2 (engine tempo units).
	.equ	TEMPO,	0xbb @ TEMPO

@ KEYSH: Key shift / transpose in semitones.
	.equ	KEYSH,	0xbc @ KEYSH

@ VOICE: Select a Program (instrument) number.
	.equ	VOICE,	0xbd @ VOICE

@ VOL: Track volume.
	.equ	VOL,	0xbe @ VOL

@ PAN: Stereo panpot; c_v is center.
	.equ	PAN,	0xbf @ PAN

@ BEND: Pitch bend; c_v is center.
	.equ	BEND,	0xc0 @ BEND

@ BENDR: Pitch-bend range.
	.equ	BENDR,	0xc1 @ BENDR

@ LFOS: LFO speed.
	.equ	LFOS,	0xc2 @ LFOS

@ LFODL: LFO delay.
	.equ	LFODL,	0xc3 @ LFODL

@ MOD: Modulation depth.
	.equ	MOD,	0xc4 @ MOD

@ MODT: Modulation type: mod_vib, mod_tre or mod_pan.
	.equ	MODT,	0xc5 @ MODT

@ TUNE: Micro tuning; c_v is center.
	.equ	TUNE,	0xc8 @ TUNE

@ XCMD: Extended command; followed by a subcommand and operand.
	.equ	XCMD,	0xcd @ XCMD

@ xIECV: XCMD subcommand: echo volume.
	.equ	xIECV,	0x08 @ xIECV

@ xIECL: XCMD subcommand: echo length.
	.equ	xIECL,	0x09 @ xIECL

@ EOT: End a tied note.
	.equ	EOT,	0xce @ EOT

@ TIE: Start a tied note (the next note continues the tie).
	.equ	TIE,	0xcf @ TIE

@ Note instructions: Nxx starts a note whose duration is xx MPlay
@ ticks, followed by key/velocity (and optional gate) bytes. N01..N24
@ are consecutive; after 24 only the duration table values are named.
@ Therefore N90, N92 and N96 exist while N91, N93, N94, N95 do not.
@ The missing names are not missing opcodes: their bytes are still in
@ the sequence, but the engine's duration table does not name them.

	.equ	N01,	TIE+1 @ note duration 01 ticks
	.equ	N02,	N01+1 @ note duration 02 ticks
	.equ	N03,	N01+2 @ note duration 03 ticks
	.equ	N04,	N01+3 @ note duration 04 ticks
	.equ	N05,	N01+4 @ note duration 05 ticks
	.equ	N06,	N01+5 @ note duration 06 ticks
	.equ	N07,	N01+6 @ note duration 07 ticks
	.equ	N08,	N01+7 @ note duration 08 ticks
	.equ	N09,	N01+8 @ note duration 09 ticks
	.equ	N10,	N01+9 @ note duration 10 ticks
	.equ	N11,	N01+10 @ note duration 11 ticks
	.equ	N12,	N01+11 @ note duration 12 ticks
	.equ	N13,	N01+12 @ note duration 13 ticks
	.equ	N14,	N01+13 @ note duration 14 ticks
	.equ	N15,	N01+14 @ note duration 15 ticks
	.equ	N16,	N01+15 @ note duration 16 ticks
	.equ	N17,	N01+16 @ note duration 17 ticks
	.equ	N18,	N01+17 @ note duration 18 ticks
	.equ	N19,	N01+18 @ note duration 19 ticks
	.equ	N20,	N01+19 @ note duration 20 ticks
	.equ	N21,	N01+20 @ note duration 21 ticks
	.equ	N22,	N01+21 @ note duration 22 ticks
	.equ	N23,	N01+22 @ note duration 23 ticks
	.equ	N24,	N01+23 @ note duration 24 ticks
	.equ	N28,	N01+24 @ note duration 28 ticks
	.equ	N30,	N01+25 @ note duration 30 ticks
	.equ	N32,	N01+26 @ note duration 32 ticks
	.equ	N36,	N01+27 @ note duration 36 ticks
	.equ	N40,	N01+28 @ note duration 40 ticks
	.equ	N42,	N01+29 @ note duration 42 ticks
	.equ	N44,	N01+30 @ note duration 44 ticks
	.equ	N48,	N01+31 @ note duration 48 ticks
	.equ	N52,	N01+32 @ note duration 52 ticks
	.equ	N54,	N01+33 @ note duration 54 ticks
	.equ	N56,	N01+34 @ note duration 56 ticks
	.equ	N60,	N01+35 @ note duration 60 ticks
	.equ	N64,	N01+36 @ note duration 64 ticks
	.equ	N66,	N01+37 @ note duration 66 ticks
	.equ	N68,	N01+38 @ note duration 68 ticks
	.equ	N72,	N01+39 @ note duration 72 ticks
	.equ	N76,	N01+40 @ note duration 76 ticks
	.equ	N78,	N01+41 @ note duration 78 ticks
	.equ	N80,	N01+42 @ note duration 80 ticks
	.equ	N84,	N01+43 @ note duration 84 ticks
	.equ	N88,	N01+44 @ note duration 88 ticks
	.equ	N90,	N01+45 @ note duration 90 ticks
	.equ	N92,	N01+46 @ note duration 92 ticks
	.equ	N96,	N01+47 @ note duration 96 ticks

@ Operand limits and musical constants.
	.equ	mxv,	0x7F
@ c_v: center value for PAN, BEND and TUNE (signed range -64..+63).
	.equ	c_v,	0x40

@ Note-name aliases: CnM2 = C at octave -2, ... Gn8 = G at octave 8.
@ Values are MIDI key numbers (0..127), in chromatic order.
@ These are operand aliases, not separate MPlay opcodes.
	.equ	CnM2,	0
	.equ	CsM2,	1
	.equ	DnM2,	2
	.equ	DsM2,	3
	.equ	EnM2,	4
	.equ	FnM2,	5
	.equ	FsM2,	6
	.equ	GnM2,	7
	.equ	GsM2,	8
	.equ	AnM2,	9
	.equ	AsM2,	10
	.equ	BnM2,	11
	.equ	CnM1,	12
	.equ	CsM1,	13
	.equ	DnM1,	14
	.equ	DsM1,	15
	.equ	EnM1,	16
	.equ	FnM1,	17
	.equ	FsM1,	18
	.equ	GnM1,	19
	.equ	GsM1,	20
	.equ	AnM1,	21
	.equ	AsM1,	22
	.equ	BnM1,	23
	.equ	Cn0,	24
	.equ	Cs0,	25
	.equ	Dn0,	26
	.equ	Ds0,	27
	.equ	En0,	28
	.equ	Fn0,	29
	.equ	Fs0,	30
	.equ	Gn0,	31
	.equ	Gs0,	32
	.equ	An0,	33
	.equ	As0,	34
	.equ	Bn0,	35
	.equ	Cn1,	36
	.equ	Cs1,	37
	.equ	Dn1,	38
	.equ	Ds1,	39
	.equ	En1,	40
	.equ	Fn1,	41
	.equ	Fs1,	42
	.equ	Gn1,	43
	.equ	Gs1,	44
	.equ	An1,	45
	.equ	As1,	46
	.equ	Bn1,	47
	.equ	Cn2,	48
	.equ	Cs2,	49
	.equ	Dn2,	50
	.equ	Ds2,	51
	.equ	En2,	52
	.equ	Fn2,	53
	.equ	Fs2,	54
	.equ	Gn2,	55
	.equ	Gs2,	56
	.equ	An2,	57
	.equ	As2,	58
	.equ	Bn2,	59
	.equ	Cn3,	60
	.equ	Cs3,	61
	.equ	Dn3,	62
	.equ	Ds3,	63
	.equ	En3,	64
	.equ	Fn3,	65
	.equ	Fs3,	66
	.equ	Gn3,	67
	.equ	Gs3,	68
	.equ	An3,	69
	.equ	As3,	70
	.equ	Bn3,	71
	.equ	Cn4,	72
	.equ	Cs4,	73
	.equ	Dn4,	74
	.equ	Ds4,	75
	.equ	En4,	76
	.equ	Fn4,	77
	.equ	Fs4,	78
	.equ	Gn4,	79
	.equ	Gs4,	80
	.equ	An4,	81
	.equ	As4,	82
	.equ	Bn4,	83
	.equ	Cn5,	84
	.equ	Cs5,	85
	.equ	Dn5,	86
	.equ	Ds5,	87
	.equ	En5,	88
	.equ	Fn5,	89
	.equ	Fs5,	90
	.equ	Gn5,	91
	.equ	Gs5,	92
	.equ	An5,	93
	.equ	As5,	94
	.equ	Bn5,	95
	.equ	Cn6,	96
	.equ	Cs6,	97
	.equ	Dn6,	98
	.equ	Ds6,	99
	.equ	En6,	100
	.equ	Fn6,	101
	.equ	Fs6,	102
	.equ	Gn6,	103
	.equ	Gs6,	104
	.equ	An6,	105
	.equ	As6,	106
	.equ	Bn6,	107
	.equ	Cn7,	108
	.equ	Cs7,	109
	.equ	Dn7,	110
	.equ	Ds7,	111
	.equ	En7,	112
	.equ	Fn7,	113
	.equ	Fs7,	114
	.equ	Gn7,	115
	.equ	Gs7,	116
	.equ	An7,	117
	.equ	As7,	118
	.equ	Bn7,	119
	.equ	Cn8,	120
	.equ	Cs8,	121
	.equ	Dn8,	122
	.equ	Ds8,	123
	.equ	En8,	124
	.equ	Fn8,	125
	.equ	Fs8,	126
	.equ	Gn8,	127

@ Velocity aliases v000..v127: value is the MIDI-style velocity 0..127.
@ These are a dense block; no per-line comments are needed.
@ The engine consumes the numeric value, so the aliases are optional.
	.equ	v000,	0
	.equ	v001,	1
	.equ	v002,	2
	.equ	v003,	3
	.equ	v004,	4
	.equ	v005,	5
	.equ	v006,	6
	.equ	v007,	7
	.equ	v008,	8
	.equ	v009,	9
	.equ	v010,	10
	.equ	v011,	11
	.equ	v012,	12
	.equ	v013,	13
	.equ	v014,	14
	.equ	v015,	15
	.equ	v016,	16
	.equ	v017,	17
	.equ	v018,	18
	.equ	v019,	19
	.equ	v020,	20
	.equ	v021,	21
	.equ	v022,	22
	.equ	v023,	23
	.equ	v024,	24
	.equ	v025,	25
	.equ	v026,	26
	.equ	v027,	27
	.equ	v028,	28
	.equ	v029,	29
	.equ	v030,	30
	.equ	v031,	31
	.equ	v032,	32
	.equ	v033,	33
	.equ	v034,	34
	.equ	v035,	35
	.equ	v036,	36
	.equ	v037,	37
	.equ	v038,	38
	.equ	v039,	39
	.equ	v040,	40
	.equ	v041,	41
	.equ	v042,	42
	.equ	v043,	43
	.equ	v044,	44
	.equ	v045,	45
	.equ	v046,	46
	.equ	v047,	47
	.equ	v048,	48
	.equ	v049,	49
	.equ	v050,	50
	.equ	v051,	51
	.equ	v052,	52
	.equ	v053,	53
	.equ	v054,	54
	.equ	v055,	55
	.equ	v056,	56
	.equ	v057,	57
	.equ	v058,	58
	.equ	v059,	59
	.equ	v060,	60
	.equ	v061,	61
	.equ	v062,	62
	.equ	v063,	63
	.equ	v064,	64
	.equ	v065,	65
	.equ	v066,	66
	.equ	v067,	67
	.equ	v068,	68
	.equ	v069,	69
	.equ	v070,	70
	.equ	v071,	71
	.equ	v072,	72
	.equ	v073,	73
	.equ	v074,	74
	.equ	v075,	75
	.equ	v076,	76
	.equ	v077,	77
	.equ	v078,	78
	.equ	v079,	79
	.equ	v080,	80
	.equ	v081,	81
	.equ	v082,	82
	.equ	v083,	83
	.equ	v084,	84
	.equ	v085,	85
	.equ	v086,	86
	.equ	v087,	87
	.equ	v088,	88
	.equ	v089,	89
	.equ	v090,	90
	.equ	v091,	91
	.equ	v092,	92
	.equ	v093,	93
	.equ	v094,	94
	.equ	v095,	95
	.equ	v096,	96
	.equ	v097,	97
	.equ	v098,	98
	.equ	v099,	99
	.equ	v100,	100
	.equ	v101,	101
	.equ	v102,	102
	.equ	v103,	103
	.equ	v104,	104
	.equ	v105,	105
	.equ	v106,	106
	.equ	v107,	107
	.equ	v108,	108
	.equ	v109,	109
	.equ	v110,	110
	.equ	v111,	111
	.equ	v112,	112
	.equ	v113,	113
	.equ	v114,	114
	.equ	v115,	115
	.equ	v116,	116
	.equ	v117,	117
	.equ	v118,	118
	.equ	v119,	119
	.equ	v120,	120
	.equ	v121,	121
	.equ	v122,	122
	.equ	v123,	123
	.equ	v124,	124
	.equ	v125,	125
	.equ	v126,	126
	.equ	v127,	127

@ Gate-time percentage aliases for note commands.
	.equ	gtp1,	1
	.equ	gtp2,	2
	.equ	gtp3,	3

@ MODT modulation modes.
	.equ	mod_vib,0
	.equ	mod_tre,1
	.equ	mod_pan,2

@ MEMACC operation codes. The first six are arithmetic; the rest
@ are conditional branches or memory-to-memory variants.
	.equ	mem_set,0
	.equ	mem_add,1
	.equ	mem_sub,2
	.equ	mem_mem_set,3
	.equ	mem_mem_add,4
	.equ	mem_mem_sub,5
	.equ	mem_beq,6
	.equ	mem_bne,7
	.equ	mem_bhi,8
	.equ	mem_bhs,9
	.equ	mem_bls,10
	.equ	mem_blo,11
	.equ	mem_mem_beq,12
	.equ	mem_mem_bne,13
	.equ	mem_mem_bhi,14
	.equ	mem_mem_bhs,15
	.equ	mem_mem_bls,16
	.equ	mem_mem_blo,17

@ Song reverb flag value.
	.equ	reverb_set,0x80

@ PAM is an alias for PAN.
	.equ	PAM, PAN
