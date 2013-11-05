var path = ".\\";
var elf_files = new ElfFiles ("__TEST_ELF_NAME__");
var oly_files = new Array ();

//
// This is the Comet set up
//

function BeforeInit()
{
	// Map 374KB of system SRAM MTX's bulk memory space, as we don't use the first 8k of SRAM
	// saving it instead for the USB boot loader
	WriteLong(0x04803010, (0xB0000000 >>> 2) );
	WriteLong(0x04803014, (0xB005E000 >>> 2) );
	WriteLong(0x04803018, (0xE0202000 >>> 2) );
	WriteLong(0x0480301C, 0x00000002);

	// Write all ones to unused mapping entry in MC_REQ
	WriteLong(0x04803020, 0xFFFFFFFF);
	WriteLong(0x04803024, 0xFFFFFFFF);
	WriteLong(0x04803028, 0xFFFFFFFF);
	WriteLong(0x0480302c, 0xFFFFFFFF);

	// Write all ones to unused mapping entry in MC_REQ
	WriteLong(0x04803030, 0xFFFFFFFF);
	WriteLong(0x04803034, 0xFFFFFFFF);
	WriteLong(0x04803038, 0xFFFFFFFF);
	WriteLong(0x0480303c, 0xFFFFFFFF);

	// Write all ones to unused mapping entry in MC_REQ
	WriteLong(0x04803040, 0xFFFFFFFF);
	WriteLong(0x04803044, 0xFFFFFFFF);
	WriteLong(0x04803048, 0xFFFFFFFF);
	WriteLong(0x0480304c, 0xFFFFFFFF);

	// Write all ones to unused mapping entry in MC_REQ
	WriteLong(0x04803050, 0xFFFFFFFF);
	WriteLong(0x04803054, 0xFFFFFFFF);
	WriteLong(0x04803058, 0xFFFFFFFF);
	WriteLong(0x0480305c, 0xFFFFFFFF);

	// Write all ones to unused mapping entry in MC_REQ
	WriteLong(0x04803060, 0xFFFFFFFF);
	WriteLong(0x04803064, 0xFFFFFFFF);
	WriteLong(0x04803068, 0xFFFFFFFF);
	WriteLong(0x0480306c, 0xFFFFFFFF);

	// Write all ones to unused mapping entry in MC_REQ
	WriteLong(0x04803070, 0xFFFFFFFF);
	WriteLong(0x04803074, 0xFFFFFFFF);
	WriteLong(0x04803078, 0xFFFFFFFF);
	WriteLong(0x0480307c, 0xFFFFFFFF);

	// Write all ones to unused mapping entry in MC_REQ
	WriteLong(0x04803080, 0xFFFFFFFF);
	WriteLong(0x04803084, 0xFFFFFFFF);
	WriteLong(0x04803088, 0xFFFFFFFF);
	WriteLong(0x0480308c, 0xFFFFFFFF);

}
