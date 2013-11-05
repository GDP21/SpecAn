///////////////////////////////////////////////////////////////////////////////
//	runwithargs.js
///////////////////////////////////////////////////////////////////////////////
var fso = new ActiveXObject( "Scripting.FileSystemObject" );
var file = fso.OpenTextFile( GetParam(0) );
var script = file.ReadAll();
file.close();

// execute the existing ldlk generated script
eval( script );

SelectTarget(GetFirstThread());
var argc = GetParamCount();

if (SymbolExists("metag_argv"))
{    
    	WriteMessage("Patching " + (argc - 1) + " arguments ");

	
	var addr = EvaluateSymbol ("&metag_argv");
	
	for (var x = 1; x < argc; x++)
	{
		var arga = ReadLong( addr + (x*4) );
				
		WriteMessage("patching arg " + GetParam(x) + " to address 0x" + toHex(arga));
		WriteString( arga, GetParam(x) );
	}
}
else
{
	WriteMessage("ERROR: metag_argc/metag_argv not found. Arguments not patched.");
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
// End of runwithargs.js
