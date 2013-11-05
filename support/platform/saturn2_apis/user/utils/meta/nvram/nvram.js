var path = ".\\";
var elf_files = new ElfFiles ("nvram_t0.elf",  0 ,  0 ,  0 );
var max_thread = 0;
var oly_files = new Array ();
function BeforeInit()
{
	// Flush MMU
	WriteLong(0x04700000, 0x00000000);
}
// Main Sequence

// the following two lines are possible form for the generated lines from
//   ldlk to tell this script where and which elfs are to be loaded
// ++++ var path = "j:\\tests\\";
// ++++ var elf_files = new ElfFiles("thread0.elf", "thread1.elf",0,0);
// ++++ var max_thread = 2;

var program_file_load_option = 0;   // load binary and symbols
for (var n = 0; n < GetParamCount(); ++n)
{    
	if (GetParam(n).toLowerCase() == "-load_binary_only")
	{
		program_file_load_option = 1;   // load binary
	}
	else if (GetParam(n).toLowerCase() == "-load_source_only")
	{
		program_file_load_option = 2;   // load symbols
	}
}


// threads hold the information of which thread has an elf file loaded
var threads = new Threads (0, 0, 0, 0); 

if (program_file_load_option != 2)
{
	HardReset ();
	BeforeInit ();
	PreLoadConfig ();
	LoadAllOverlay ();
	LoadAllProgram ();
	PostLoadConfig ();
}
else
{
	LoadAllOverlay ();
	LoadAllProgram ();
}

// Functions

function Threads(t0,t1,t2,t3)
{
	this.t0 = t0;
	this.t1 = t1;
	this.t2 = t2;
	this.t3 = t3;
}

function ElfFiles(e0,e1,e2,e3)
{
	this.e0 = e0;
	this.e1 = e1;
	this.e2 = e2;
	this.e3 = e3;
}

function LoadAllProgram()
{
	var current;
	var next;
	var num = 0;
	var entry_point;
	var exec_name;
			
	current = GetFirstThread();
	
	do
	{
		threads ["t" + num] = 0;
		exec_name = elf_files["e" + num];
		if (exec_name != 0)
		{
			if (SelectTarget (current))
			{
				Write ("Loading " + exec_name);
				if (LoadProgramFileEx (path + exec_name, 0, program_file_load_option) != 0)
				{
					if (program_file_load_option != 2)
					{
						entry_point = EvaluateSymbol ("CodeScapeStart");
						WriteRegister ("pc", entry_point);
						Write ("PC = 0x" + toHex (ReadRegister ("pc")));
					}
				}
				else
				{
					Write ("Failed to LoadProgramFile()");
					break;
				}
			}
			else
			{
				Write ("Failed to SelectTarget()");
				break;
			}
		}
		
		num +=1;
		next = GetNextThread (current);
		current = next;
	} while (num <= max_thread);
}

function LoadAllOverlay()
{
	var current;
			
	current = GetFirstThread();
	SelectTarget (current);

	for (var index = 0; index < oly_files.length; index++)
	{
		Write ("Loading overlay " + oly_files[index]);

		if (LoadProgramFileEx (path + oly_files[index], 0, 4) == 0)
		{
			Write ("Failed to LoadProgramFile()");
			break;
		}
	}
}

// converts to hex - Displays leading zeros and copes with minus numbers
function toHex( value )
{
	hexTab = "0123456789ABCDEF";
	var result = "";
	for( ToHexi = 0; ToHexi < 8; ++ToHexi )
	{
		var temp = value & 0x0f;
		result = hexTab.substr( temp, 1 ) + result;
		value = value >>> 4;
	}
	return result;
}

function GetMilliTime()
{
	var t = new Date();
	return t.getTime();
}

function Sleep( milliseconds )
{
    for (var target_time = GetMilliTime() + milliseconds; GetMilliTime() <=
target_time; )
        ;
}

function PreLoadConfig()
{
	var val;

	// Configuration commands
	WriteLong(0x02015908, 0x00000000);
	Sleep(4);
	WriteLong(0x02015954, 0x1600000F);
	WriteLong(0x020159AC, 0x00000000);
	WriteLong(0x02015918, 0x00000001);
	WriteLong(0x02015950, 0x13702700);
	Sleep(4);
	WriteLong(0x02015954, 0x0600000F);
	WriteLong(0x02015954, 0x0400000F);
	Sleep(8);
	WriteLong(0x02015908, 0x00000002);
	WriteLong(0x02018E7C, 0x00000000);
	WriteLong(0x02018D08, 0x00000000);
	WriteLong(0x02018E44, 0x00000002);
	WriteLong(0x02018D10, 0x00000004);
	WriteLong(0x02018D54, 0x0000001F);
	WriteLong(0x02018D98, 0x00000001);
	WriteLong(0x02018DA0, 0x00000000);
	WriteLong(0x02018DA8, 0x00000003);
	WriteLong(0x02018E3C, 0x00000002);
	WriteLong(0x02018E40, 0x00000002);
	WriteLong(0x02018E84, 0x00000097);
	WriteLong(0x02018F6C, 0x00000002);
	WriteLong(0x02018D3C, 0x00000001);
	WriteLong(0x02018DE0, 0x00000008);
	WriteLong(0x02018DE4, 0x00000008);
	WriteLong(0x02018DE8, 0x0000000F);
	WriteLong(0x02018DEC, 0x00000000);
	WriteLong(0x02018DF0, 0x00000000);
	WriteLong(0x02018DF4, 0x00000000);
	WriteLong(0x02018DF8, 0x00000000);
	WriteLong(0x02018DFC, 0x00000000);
	WriteLong(0x02018E00, 0x00000000);
	WriteLong(0x02018E04, 0x0000000F);
	WriteLong(0x02018E08, 0x0000000F);
	WriteLong(0x02018E0C, 0x00000003);
	WriteLong(0x02018E10, 0x00000003);
	WriteLong(0x02018E14, 0x00000003);
	WriteLong(0x02018E18, 0x00000003);
	WriteLong(0x02018E1C, 0x0000000F);
	WriteLong(0x02018E20, 0x0000000F);
	WriteLong(0x02018E24, 0x0000000F);
	WriteLong(0x02018EAC, 0x00000010);
	WriteLong(0x02018E34, 0x00000005);
	WriteLong(0x02018E38, 0x00000005);
	WriteLong(0x02018E94, 0x00000001);
	WriteLong(0x02018E28, 0x00000002);
	WriteLong(0x02018E2C, 0x00000004);
	WriteLong(0x02018E30, 0x00000003);
	WriteLong(0x02018F7C, 0x00000002);
	WriteLong(0x02018F80, 0x00000000);
	WriteLong(0x02018F84, 0x00000000);
	WriteLong(0x02018DB0, 0x0000005F);
	WriteLong(0x02018D4C, 0x00000075);
	WriteLong(0x02018D84, 0x00000003);
	WriteLong(0x02018D40, 0x00000019);
	WriteLong(0x02018D68, 0x00000004);
	WriteLong(0x02018D58, 0x00000016);
	WriteLong(0x02018D5C, 0x00000020);
	WriteLong(0x02018D48, 0x00000008);
	WriteLong(0x02018D7C, 0x00000004);
	WriteLong(0x02018E88, 0x00000002);
	WriteLong(0x02018D60, 0x00000014);
	WriteLong(0x02018D94, 0x00000007);
	WriteLong(0x02018D88, 0x00000001);
	WriteLong(0x02018F60, 0x00000005);
	WriteLong(0x02018D50, 0x0000000F);
	WriteLong(0x02018DB4, 0x00000002);
	WriteLong(0x02018D70, 0x0000000B);
	WriteLong(0x02018D64, 0x00000006);
	WriteLong(0x02018F64, 0x00000006);
	WriteLong(0x02018D44, 0x0000003E);
	WriteLong(0x02018D8C, 0x00000005);
	WriteLong(0x02018D6C, 0x00000005);
	WriteLong(0x02018D74, 0x00000003);
	WriteLong(0x02018D80, 0x00000006);
	WriteLong(0x02018DB8, 0x00000F62);
	WriteLong(0x02018DBC, 0x00002042);
	WriteLong(0x02018E8C, 0x00004000);
	WriteLong(0x02018E90, 0x00006000);
	WriteLong(0x02018F94, 0x00000038);
	WriteLong(0x02018F98, 0x00000027);
	WriteLong(0x02018F9C, 0x00000030);
	WriteLong(0x02018FA0, 0x00000028);
	WriteLong(0x02018E7C, 0x00000001);
	WriteLong(0x02018C10, 0x2C000000);
	WriteLong(0x02018C14, 0x2D000000);
	WriteLong(0x02018C18, 0x00000000);
	WriteLong(0x02018C1C, 0x00000000);
	Sleep(28);
	WriteLong(0x04830140, 0x000001DF);
	WriteLong(0x04800070, 0x0000F0F0);
	WriteLong(0x04801070, 0x0000F0F0);
	WriteLong(0x04802070, 0x0000F0F0);
	WriteLong(0x04803070, 0x0000F0F0);

	// MMU Configuration commands
	WriteLong(0x04830028, 0x00000006);
	WriteLong(0x04830600, 0x0000C0C0);
	WriteLong(0x04830608, 0x0000C0C0);
	WriteLong(0x04830038, 0x00000001);
	WriteLong(0x04830040, 0x00000001);
	WriteLong(0x04830200, 0x88000703);
	WriteLong(0x04830208, 0x88040703);
	WriteLong(0x04830210, 0x80000000);
	WriteLong(0x04830218, 0x80000000);
	WriteLong(0x04830018, 0x00000001);
	WriteLong(0x04830220, 0x08000703);
	WriteLong(0x04830228, 0x08040703);
	WriteLong(0x04830230, 0x00000000);
	WriteLong(0x04830238, 0x00000000);
	WriteLong(0x04830020, 0x00000001);
	WriteLong(0x040000C0, 0x00000000);
}

function PostLoadConfig()
{
	var val;
	var thread;

	// Select Thread 0
	thread = GetFirstThread();
	SelectTarget(thread);
	// Zero section '.gheap'
	InitMemory(0xB7000000, 0x4, 0x6F0, 0x00000000, 0x00000000);
	// Zero section '.stack'
	InitMemory(0xB7002000, 0x4, 0x400, 0x00000000, 0x00000000);
	// Zero section '.lheap'
	InitMemory(0xB7003000, 0x4, 0x800, 0x00000000, 0x00000000);
}

// End of debug/nvram.js
