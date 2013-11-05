var path = ".\\";
var elf_files = new ElfFiles ("%1", 0,  0 ,  0 );
var max_thread = 0;

function BeforeInit()
{
}
// Main Sequence

// the following two lines are possible form for the generated lines from
//   ldlk to tell this script where and which elfs are to be loaded
// ++++ var path = "j:\\tests\\";
// ++++ var elf_files = new ElfFiles("thread0.elf", "thread1.elf",0,0);
// ++++ var max_thread = 2;

// threads hold the information of which thread has an elf file loaded
var threads = new Threads (0, 0, 0, 0); 

HardReset ();
BeforeInit ();
PreLoadConfig ();
LoadAllProgram ();
PostLoadConfig ();

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
				if (LoadProgramFileEx (path + exec_name, 0, 0) != 0)
				{
					entry_point = EvaluateSymbol ("CodeScapeStart");
					WriteRegister ("pc", entry_point);
					Write ("PC = 0x" + toHex (ReadRegister ("pc")));
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
target_time; IsRunning())
        ;
}

