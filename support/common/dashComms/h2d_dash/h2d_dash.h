#ifndef __H2D_DASH_H__
#define __H2D_DASH_H__

/*============================================================================
====	D E F I N E S
=============================================================================*/
#define MDTV_ERROR_NO_COMMS	(-3)

/*============================================================================
====	D A T A
=============================================================================*/
static CDash dash;

/*============================================================================
	D A S H   F U N C T I O N S
=============================================================================*/

/*
* Support defs & functions for Dash Script (used for all DASH/target communications)
*/

class ComInitializer
{
public:
	ComInitializer() { CoInitialize(NULL); }
	~ComInitializer() { CoUninitialize(); }
} InitializeIt;

CLSID BuildCLSID(const char* name)
{
	const CString progID = name;
	const BSTR id = progID.AllocSysString();
	CLSID clsid;
	CLSIDFromProgID(id, &clsid);
	::SysFreeString(id);
	return clsid;
}

CDash InitializeDashScript()
{
	CLSID clsid = BuildCLSID("DashScript.Dash");
	IUnknown* pUnknown = NULL;
	if (CoCreateInstance(clsid, NULL, CLSCTX_SERVER, IID_IUnknown, reinterpret_cast<void**>(&pUnknown)) == S_OK)
	{
		IDispatch* pDispatch = NULL;
		if (pUnknown->QueryInterface(IID_IDispatch, reinterpret_cast<void**>(&pDispatch)) == S_OK)
		{
			return CDash(pDispatch);
		}
		pUnknown->Release();
	}
	return CDash();
}

void DashScriptInitialize()
{
	CoInitialize(NULL);
	dash = InitializeDashScript();
}

void DashScriptUninitialize()
{
	dash = CDash();
	CoUninitialize();
}

void WriteByte(CDash dash, long address, byte value)
{
	/* WARNING: this does not appear to work, it crashes */
	dash.WriteByte(COleVariant(address), COleVariant(value));
}
void WriteWord(CDash dash, long address, short value)
{
	/* WARNING: this does not appear to work, it crashes */
	dash.WriteWord(COleVariant(address), COleVariant(value));
}

void WriteLong(CDash dash, long address, long value)
{
	dash.WriteLong(COleVariant(address), COleVariant(value));
}

byte ReadByte (CDash dash, long address)
{
	/* WARNING: this does not appear to work, it crashes */
	COleVariant result(dash.ReadByte(COleVariant(address)));

	return result.bVal;
}

short ReadWord(CDash dash, long address)
{
	/* WARNING: this does not appear to work, it crashes */
	COleVariant result(dash.ReadWord(COleVariant(address)));
	return result.uintVal;
}

long ReadLong(CDash dash, long address)
{
	COleVariant result(dash.ReadLong(COleVariant(address)));
	return result.lVal;
}

long InitMemory(CDash dash, long address, long size, long num, long initial, long increment)// return 0 for error, non 0 for success
{
	return dash.InitMemory(COleVariant(address), COleVariant(size), COleVariant(num), COleVariant(initial), COleVariant(increment));
}

long DashScriptLoadProgramFile(LPCTSTR filenname, unsigned long showProgress)
{
	return(dash.LoadProgramFile(filenname, showProgress));
}

void DashScriptHardReset(void)
{
	dash.HardReset();
}

void DashScriptSoftReset(void)
{
	dash.SoftReset();
}

long DashScriptRun(void)
{
	return(dash.Run());
}

long DashScriptStop(void)
{
	return(dash.Stop());
}

long DashScriptIsRunning(void)
{
	return(dash.IsRunning());
}

void BeforeInit (void)
{
	#ifdef MTX
	/*****************************
	Code to be copied from ".js" for MTX.
	Currently there is no code in the BeforeInit in the MTX app js file.
	NB: if/when there is code to be added here, then it will probably need to detect when there
		has been an exception due to no comms with DASH/target & return error status
	******************************/
	#endif

    WriteLong(dash, 0x04803614, 0x14000);
    WriteLong(dash, 0x04803608, 0x14000);
    WriteLong(dash, 0x04803610, 0x2A000);
}
void PreLoadConfig (void)
{
	#ifdef MTX
	/*****************************
	Code to be copied from ".js" for MTX
	Currently there is no code in the PreLoadConfig in the MTX app js file.
	NB: if/when there is code to be added here, then it will probably need to detect when there
		has been an exception due to no comms with DASH/target & return error status
	******************************/
	#endif
}

void PostLoadConfig(void)
{
	#ifdef MTX
	/*****************************
	Code to be copied from ".js" for MTX
	Currently there is no code in the PostLoadConfig in the MTX app js file.
	NB: if/when there is code to be added here, then it will probably need to detect when there
		has been an exception due to no comms with DASH/target & return error status
	******************************/
	#endif
}

int MDTV_SUPPORT_IsCommsWorking(void)
{
	/*
	* This is a simple support function which will check we can communicate with the DASH/target.
	* Basically we try a read PC register (because all targets should have the PC register), but
	* we also setup the exception handler to catch the case when it fails.
	* If it succeeds then we assume we have communications with the target & return success.
	* If we hit the exception handler, then we can assume that we are not communicating with the target
	* & return error
	* Note that we don't care what the  value of PC actually is.
	*/
	DashScriptInitialize();
	if (AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		try
		{
			/* Should succeed unless no comms with target*/
			dash.ReadRegister("PC");
			DashScriptUninitialize();
			return(1);
		}
		catch (COleDispatchException* e)
		{
			e->Delete();
			DashScriptUninitialize();
			return (MDTV_ERROR_NO_COMMS);
		}
	}
	DashScriptUninitialize();
	return (MDTV_ERROR_NO_COMMS);
}

int MDTV_SUPPORT_IsRunning(void)
{
	long status = 0;

	/* This is a simple support function which stops the target */
	DashScriptInitialize();
	if (AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		try
		{
			/* Should succeed unless no comms with target*/
			status = DashScriptIsRunning();
			DashScriptUninitialize();
			return(status);
		}
		catch (COleDispatchException* e)
		{
			e->Delete();
			DashScriptUninitialize();
			return (MDTV_ERROR_NO_COMMS);
		}
	}
	DashScriptUninitialize();
	return (MDTV_ERROR_NO_COMMS);
}

int MDTV_SUPPORT_Stop(void)
{
	long status = 0;

	/* This is a simple support function which stops the target */
	DashScriptInitialize();
	if (AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		try
		{
			/* Should succeed unless no comms with target*/
			status = DashScriptStop();
			DashScriptUninitialize();
			return(status);
		}
		catch (COleDispatchException* e)
		{
			e->Delete();
			DashScriptUninitialize();
			return (MDTV_ERROR_NO_COMMS);
		}
	}
	DashScriptUninitialize();
	return (MDTV_ERROR_NO_COMMS);
}

int MDTV_SUPPORT_HwReset(void)
{
	/* This is a simple support function which does a hardware reset */
	DashScriptInitialize();
	if (AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		try
		{
			/* Should succeed unless no comms with target*/
			DashScriptHardReset();
			DashScriptUninitialize();
			return(1);
		}
		catch (COleDispatchException* e)
		{
			e->Delete();
			DashScriptUninitialize();
			return (MDTV_ERROR_NO_COMMS);
		}
	}
	DashScriptUninitialize();
	return (MDTV_ERROR_NO_COMMS);
}

int MDTV_SUPPORT_SwReset(void)
{
	/* This is a simple support function which does a hardware reset */
	DashScriptInitialize();
	if (AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		try
		{
			/* Should succeed unless no comms with target*/
			DashScriptSoftReset();
			DashScriptUninitialize();
			return(1);
		}
		catch (COleDispatchException* e)
		{
			e->Delete();
			DashScriptUninitialize();
			return (MDTV_ERROR_NO_COMMS);
		}
	}
	DashScriptUninitialize();
	return (MDTV_ERROR_NO_COMMS);
}

#endif /* __H2D_DASH_H__   */

















