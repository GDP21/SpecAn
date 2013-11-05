// Machine generated IDispatch wrapper class(es) created with Add Class from Typelib Wizard

// CDash wrapper class

class CDash : public COleDispatchDriver
{
public:
	CDash(){} // Calls COleDispatchDriver default constructor
	CDash(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	CDash(const CDash& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

	// Attributes
public:

	// Operations
public:


	// IDash methods
public:
	long IsRunning()
	{
		long result;
		InvokeHelper(0x1, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long Run()
	{
		long result;
		InvokeHelper(0x2, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long Stop()
	{
		long result;
		InvokeHelper(0x3, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	VARIANT ReadRegister(LPCTSTR regName)
	{
		VARIANT result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x4, DISPATCH_METHOD, VT_VARIANT, (void*)&result, parms, regName);
		return result;
	}
	void WriteRegister(LPCTSTR regName, VARIANT value)
	{
		static BYTE parms[] = VTS_BSTR VTS_VARIANT ;
		InvokeHelper(0x5, DISPATCH_METHOD, VT_EMPTY, NULL, parms, regName, &value);
	}
	void WriteByte(VARIANT address, VARIANT value)
	{
		static BYTE parms[] = VTS_VARIANT VTS_VARIANT ;
		InvokeHelper(0x6, DISPATCH_METHOD, VT_EMPTY, NULL, parms, &address, &value);
	}
	void WriteWord(VARIANT address, VARIANT value)
	{
		static BYTE parms[] = VTS_VARIANT VTS_VARIANT ;
		InvokeHelper(0x7, DISPATCH_METHOD, VT_EMPTY, NULL, parms, &address, &value);
	}
	void WriteLong(VARIANT address, VARIANT value)
	{
		static BYTE parms[] = VTS_VARIANT VTS_VARIANT ;
		InvokeHelper(0x8, DISPATCH_METHOD, VT_EMPTY, NULL, parms, &address, &value);
	}
	VARIANT ReadByte(VARIANT address)
	{
		VARIANT result;
		static BYTE parms[] = VTS_VARIANT ;
		InvokeHelper(0x9, DISPATCH_METHOD, VT_VARIANT, (void*)&result, parms, &address);
		return result;
	}
	VARIANT ReadWord(VARIANT address)
	{
		VARIANT result;
		static BYTE parms[] = VTS_VARIANT ;
		InvokeHelper(0xa, DISPATCH_METHOD, VT_VARIANT, (void*)&result, parms, &address);
		return result;
	}
	VARIANT ReadLong(VARIANT address)
	{
		VARIANT result;
		static BYTE parms[] = VTS_VARIANT ;
		InvokeHelper(0xb, DISPATCH_METHOD, VT_VARIANT, (void*)&result, parms, &address);
		return result;
	}
	CString GetCurrentTarget()
	{
		CString result;
		InvokeHelper(0xc, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	CString GetFirstTarget()
	{
		CString result;
		InvokeHelper(0xd, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	CString GetNextTarget(LPCTSTR curTarget)
	{
		CString result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0xe, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, curTarget);
		return result;
	}
	long SelectTarget(LPCTSTR target)
	{
		long result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0xf, DISPATCH_METHOD, VT_I4, (void*)&result, parms, target);
		return result;
	}
	long RunTarget(LPCTSTR target)
	{
		long result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x10, DISPATCH_METHOD, VT_I4, (void*)&result, parms, target);
		return result;
	}
	long StopTarget(LPCTSTR target)
	{
		long result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x11, DISPATCH_METHOD, VT_I4, (void*)&result, parms, target);
		return result;
	}
	CString GetTargetInfo(LPCTSTR target)
	{
		CString result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x12, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, target);
		return result;
	}
	long RunAllTargets()
	{
		long result;
		InvokeHelper(0x13, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long StopAllTargets()
	{
		long result;
		InvokeHelper(0x14, DISPATCH_METHOD, VT_I4, (void*)&result, NULL);
		return result;
	}
	long IsTargetRunning(LPCTSTR target)
	{
		long result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x15, DISPATCH_METHOD, VT_I4, (void*)&result, parms, target);
		return result;
	}
	void HardReset()
	{
		InvokeHelper(0x16, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void SoftReset()
	{
		InvokeHelper(0x17, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	void CopyData(LPCTSTR srcID, VARIANT srcAddr, long size, long numElements, LPCTSTR destID, VARIANT destAddr)
	{
		static BYTE parms[] = VTS_BSTR VTS_VARIANT VTS_I4 VTS_I4 VTS_BSTR VTS_VARIANT ;
		InvokeHelper(0x18, DISPATCH_METHOD, VT_EMPTY, NULL, parms, srcID, &srcAddr, size, numElements, destID, &destAddr);
	}
	long SymbolExists(VARIANT symbol)
	{
		long result;
		static BYTE parms[] = VTS_VARIANT ;
		InvokeHelper(0x19, DISPATCH_METHOD, VT_I4, (void*)&result, parms, &symbol);
		return result;
	}
	VARIANT EvaluateSymbol(VARIANT symbol)
	{
		VARIANT result;
		static BYTE parms[] = VTS_VARIANT ;
		InvokeHelper(0x1a, DISPATCH_METHOD, VT_VARIANT, (void*)&result, parms, &symbol);
		return result;
	}
	CString SetBreakpoint(VARIANT address)
	{
		CString result;
		static BYTE parms[] = VTS_VARIANT ;
		InvokeHelper(0x1b, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, &address);
		return result;
	}
	CString CreateBreakpoint(long type, VARIANT address)
	{
		CString result;
		static BYTE parms[] = VTS_I4 VTS_VARIANT ;
		InvokeHelper(0x1c, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, type, &address);
		return result;
	}
	void ClearAllBreakpoints()
	{
		InvokeHelper(0x1d, DISPATCH_METHOD, VT_EMPTY, NULL, NULL);
	}
	long ClearBreakpoint(LPCTSTR id)
	{
		long result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x1e, DISPATCH_METHOD, VT_I4, (void*)&result, parms, id);
		return result;
	}
	long EnableBreakpoint(LPCTSTR id, long on)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_I4 ;
		InvokeHelper(0x1f, DISPATCH_METHOD, VT_I4, (void*)&result, parms, id, on);
		return result;
	}
	long SetBreakpointAction(LPCTSTR id, long action, long on)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_I4 VTS_I4 ;
		InvokeHelper(0x20, DISPATCH_METHOD, VT_I4, (void*)&result, parms, id, action, on);
		return result;
	}
	long SetBreakpointCondition(LPCTSTR id, LPCTSTR condExpr, long exprType, long triggerCnt, long incOnTrue, long breakOn)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I4 ;
		InvokeHelper(0x21, DISPATCH_METHOD, VT_I4, (void*)&result, parms, id, condExpr, exprType, triggerCnt, incOnTrue, breakOn);
		return result;
	}
	long SetWatchBreakpointParameters(LPCTSTR id, long incDataValue, LPCTSTR dataValue, long exprType, long accessSize, long accessType)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_I4 VTS_BSTR VTS_I4 VTS_I4 VTS_I4 ;
		InvokeHelper(0x22, DISPATCH_METHOD, VT_I4, (void*)&result, parms, id, incDataValue, dataValue, exprType, accessSize, accessType);
		return result;
	}
	long SetBreakpointDataMask(LPCTSTR id, VARIANT mask)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_VARIANT ;
		InvokeHelper(0x23, DISPATCH_METHOD, VT_I4, (void*)&result, parms, id, &mask);
		return result;
	}
	long SetBreakpointLocationMask(LPCTSTR id, long maskSelect)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_I4 ;
		InvokeHelper(0x24, DISPATCH_METHOD, VT_I4, (void*)&result, parms, id, maskSelect);
		return result;
	}
	long SetBreakpointLocationMaskEx(LPCTSTR id, long mask)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_I4 ;
		InvokeHelper(0x25, DISPATCH_METHOD, VT_I4, (void*)&result, parms, id, mask);
		return result;
	}
	long LoadExtendedStub(VARIANT address)
	{
		long result;
		static BYTE parms[] = VTS_VARIANT ;
		InvokeHelper(0x26, DISPATCH_METHOD, VT_I4, (void*)&result, parms, &address);
		return result;
	}
	long ConfigureTarget(LPCTSTR target, long runBootRom, long haltAfter, long ResetMethod, long JtagFreq)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_I4 VTS_I4 VTS_I4 VTS_I4 ;
		InvokeHelper(0x27, DISPATCH_METHOD, VT_I4, (void*)&result, parms, target, runBootRom, haltAfter, ResetMethod, JtagFreq);
		return result;
	}
	long ChannelReserve(long channel)
	{
		long result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x28, DISPATCH_METHOD, VT_I4, (void*)&result, parms, channel);
		return result;
	}
	long ChannelRelease(long channel)
	{
		long result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x29, DISPATCH_METHOD, VT_I4, (void*)&result, parms, channel);
		return result;
	}
	long ChannelValidate(long channel)
	{
		long result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x2a, DISPATCH_METHOD, VT_I4, (void*)&result, parms, channel);
		return result;
	}
	unsigned long ChannelDataReady(long channel)
	{
		unsigned long result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x2b, DISPATCH_METHOD, VT_UI4, (void*)&result, parms, channel);
		return result;
	}
	unsigned char ChannelRead(long channel)
	{
		unsigned char result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x2c, DISPATCH_METHOD, VT_UI1, (void*)&result, parms, channel);
		return result;
	}
	long ChannelWrite(long channel, unsigned char data)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_UI1 ;
		InvokeHelper(0x2d, DISPATCH_METHOD, VT_I4, (void*)&result, parms, channel, data);
		return result;
	}
	long LoadBinaryFile(LPCTSTR filename, VARIANT address, unsigned long hideProgress)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_VARIANT VTS_UI4 ;
		InvokeHelper(0x2e, DISPATCH_METHOD, VT_I4, (void*)&result, parms, filename, &address, hideProgress);
		return result;
	}
	long LoadProgramFile(LPCTSTR filenname, unsigned long showProgress)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_UI4 ;
		InvokeHelper(0x2f, DISPATCH_METHOD, VT_I4, (void*)&result, parms, filenname, showProgress);
		return result;
	}
	CString ReadString(VARIANT address, VARIANT maxSize)
	{
		CString result;
		static BYTE parms[] = VTS_VARIANT VTS_VARIANT ;
		InvokeHelper(0x30, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, &address, &maxSize);
		return result;
	}
	CString ReadWString(VARIANT address, VARIANT maxSize)
	{
		CString result;
		static BYTE parms[] = VTS_VARIANT VTS_VARIANT ;
		InvokeHelper(0x31, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, &address, &maxSize);
		return result;
	}
	long WriteString(VARIANT address, LPCTSTR str)
	{
		long result;
		static BYTE parms[] = VTS_VARIANT VTS_BSTR ;
		InvokeHelper(0x32, DISPATCH_METHOD, VT_I4, (void*)&result, parms, &address, str);
		return result;
	}
	long WriteWString(VARIANT address, LPCTSTR str)
	{
		long result;
		static BYTE parms[] = VTS_VARIANT VTS_BSTR ;
		InvokeHelper(0x33, DISPATCH_METHOD, VT_I4, (void*)&result, parms, &address, str);
		return result;
	}
	long Reflash(LPCTSTR target)
	{
		long result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x34, DISPATCH_METHOD, VT_I4, (void*)&result, parms, target);
		return result;
	}
	long SaveBinaryFile(LPCTSTR filename, VARIANT address, VARIANT length, unsigned long hideProgress)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_VARIANT VTS_VARIANT VTS_UI4 ;
		InvokeHelper(0x35, DISPATCH_METHOD, VT_I4, (void*)&result, parms, filename, &address, &length, hideProgress);
		return result;
	}
	long MemoryFill(VARIANT address, VARIANT size, VARIANT numElems, VARIANT value)
	{
		long result;
		static BYTE parms[] = VTS_VARIANT VTS_VARIANT VTS_VARIANT VTS_VARIANT ;
		InvokeHelper(0x36, DISPATCH_METHOD, VT_I4, (void*)&result, parms, &address, &size, &numElems, &value);
		return result;
	}
	long LoadProgramFileEx(LPCTSTR filename, long HardReset, unsigned long loadType, unsigned long showProgress)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_I4 VTS_UI4 VTS_UI4 ;
		InvokeHelper(0x37, DISPATCH_METHOD, VT_I4, (void*)&result, parms, filename, HardReset, loadType, showProgress);
		return result;
	}
	long InitMemory(VARIANT address, VARIANT size, VARIANT num, VARIANT initial, VARIANT increment)
	{
		long result;
		static BYTE parms[] = VTS_VARIANT VTS_VARIANT VTS_VARIANT VTS_VARIANT VTS_VARIANT ;
		InvokeHelper(0x38, DISPATCH_METHOD, VT_I4, (void*)&result, parms, &address, &size, &num, &initial, &increment);
		return result;
	}
	long CheckMemory(VARIANT address, VARIANT size, VARIANT num, VARIANT initial, VARIANT increment)
	{
		long result;
		static BYTE parms[] = VTS_VARIANT VTS_VARIANT VTS_VARIANT VTS_VARIANT VTS_VARIANT ;
		InvokeHelper(0x39, DISPATCH_METHOD, VT_I4, (void*)&result, parms, &address, &size, &num, &initial, &increment);
		return result;
	}
	void SetFileServerPath(LPCTSTR filename)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x3a, DISPATCH_METHOD, VT_EMPTY, NULL, parms, filename);
	}
	CString GetFirstThread()
	{
		CString result;
		InvokeHelper(0x3b, DISPATCH_METHOD, VT_BSTR, (void*)&result, NULL);
		return result;
	}
	CString GetNextThread(LPCTSTR current)
	{
		CString result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x3c, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, current);
		return result;
	}
	VARIANT ReadMemory(VARIANT address, VARIANT ElementCount, VARIANT ElementSize, VARIANT * data, VARIANT MemoryType)
	{
		VARIANT result;
		static BYTE parms[] = VTS_VARIANT VTS_VARIANT VTS_VARIANT VTS_PVARIANT VTS_VARIANT ;
		InvokeHelper(0x3d, DISPATCH_METHOD, VT_VARIANT, (void*)&result, parms, &address, &ElementCount, &ElementSize, data, &MemoryType);
		return result;
	}
	VARIANT WriteMemory(VARIANT address, VARIANT ElementCount, VARIANT ElementSize, VARIANT data, VARIANT MemoryType)
	{
		VARIANT result;
		static BYTE parms[] = VTS_VARIANT VTS_VARIANT VTS_VARIANT VTS_VARIANT VTS_VARIANT ;
		InvokeHelper(0x3e, DISPATCH_METHOD, VT_VARIANT, (void*)&result, parms, &address, &ElementCount, &ElementSize, &data, &MemoryType);
		return result;
	}
	void UseTarget(unsigned long serial)
	{
		static BYTE parms[] = VTS_UI4 ;
		InvokeHelper(0x3f, DISPATCH_METHOD, VT_EMPTY, NULL, parms, serial);
	}
	void UseTargetOn(unsigned long serial, LPCTSTR ip)
	{
		static BYTE parms[] = VTS_UI4 VTS_BSTR ;
		InvokeHelper(0x40, DISPATCH_METHOD, VT_EMPTY, NULL, parms, serial, ip);
	}
	long EnableAllBreakpointsOnTarget(LPCTSTR target, long on)
	{
		long result;
		static BYTE parms[] = VTS_BSTR VTS_I4 ;
		InvokeHelper(0x41, DISPATCH_METHOD, VT_I4, (void*)&result, parms, target, on);
		return result;
	}
	CString CreateBreakpointOnTarget(LPCTSTR target, long type, VARIANT address)
	{
		CString result;
		static BYTE parms[] = VTS_BSTR VTS_I4 VTS_VARIANT ;
		InvokeHelper(0x42, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, target, type, &address);
		return result;
	}
	void ClearAllBreakpointsOnTarget(LPCTSTR target)
	{
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x43, DISPATCH_METHOD, VT_EMPTY, NULL, parms, target);
	}
	CString DuplicateBreakpointToTarget(LPCTSTR id, LPCTSTR newTarget)
	{
		CString result;
		static BYTE parms[] = VTS_BSTR VTS_BSTR ;
		InvokeHelper(0x44, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, id, newTarget);
		return result;
	}
	long ChannelFlush(long channelID)
	{
		long result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x45, DISPATCH_METHOD, VT_I4, (void*)&result, parms, channelID);
		return result;
	}
	VARIANT ReadFloat(VARIANT address)
	{
		VARIANT result;
		static BYTE parms[] = VTS_VARIANT ;
		InvokeHelper(0x46, DISPATCH_METHOD, VT_VARIANT, (void*)&result, parms, &address);
		return result;
	}
	VARIANT ReadDouble(VARIANT address)
	{
		VARIANT result;
		static BYTE parms[] = VTS_VARIANT ;
		InvokeHelper(0x47, DISPATCH_METHOD, VT_VARIANT, (void*)&result, parms, &address);
		return result;
	}
	void WriteFloat(VARIANT address, VARIANT value)
	{
		static BYTE parms[] = VTS_VARIANT VTS_VARIANT ;
		InvokeHelper(0x48, DISPATCH_METHOD, VT_EMPTY, NULL, parms, &address, &value);
	}
	void WriteDouble(VARIANT address, VARIANT value)
	{
		static BYTE parms[] = VTS_VARIANT VTS_VARIANT ;
		InvokeHelper(0x49, DISPATCH_METHOD, VT_EMPTY, NULL, parms, &address, &value);
	}
	long LoadHex(LPCTSTR filename)
	{
		long result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x4a, DISPATCH_METHOD, VT_I4, (void*)&result, parms, filename);
		return result;
	}
	long LoadSREC(LPCTSTR filename)
	{
		long result;
		static BYTE parms[] = VTS_BSTR ;
		InvokeHelper(0x4b, DISPATCH_METHOD, VT_I4, (void*)&result, parms, filename);
		return result;
	}
	long RunAllThreads(VARIANT onlyLoaded)
	{
		long result;
		static BYTE parms[] = VTS_VARIANT ;
		InvokeHelper(0x4c, DISPATCH_METHOD, VT_I4, (void*)&result, parms, &onlyLoaded);
		return result;
	}
	long StopAllThreads(VARIANT onlyLoaded)
	{
		long result;
		static BYTE parms[] = VTS_VARIANT ;
		InvokeHelper(0x4d, DISPATCH_METHOD, VT_I4, (void*)&result, parms, &onlyLoaded);
		return result;
	}
	VARIANT ChannelBlockRead(long chan_id, long max_size, long blocking, long timeout)
	{
		VARIANT result;
		static BYTE parms[] = VTS_I4 VTS_I4 VTS_I4 VTS_I4 ;
		InvokeHelper(0x4e, DISPATCH_METHOD, VT_VARIANT, (void*)&result, parms, chan_id, max_size, blocking, timeout);
		return result;
	}
	long ChannelBlockWrite(long id, VARIANT data, long blocking, long timeout)
	{
		long result;
		static BYTE parms[] = VTS_I4 VTS_VARIANT VTS_I4 VTS_I4 ;
		InvokeHelper(0x4f, DISPATCH_METHOD, VT_I4, (void*)&result, parms, id, &data, blocking, timeout);
		return result;
	}
	CString LookUpIPAddress(long serial)
	{
		CString result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x50, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, serial);
		return result;
	}
	void ForceAssertionFailure(BOOL enable)
	{
		static BYTE parms[] = VTS_BOOL ;
		InvokeHelper(0x51, DISPATCH_METHOD, VT_EMPTY, NULL, parms, enable);
	}
	CString EvaluateExpression(VARIANT expr)
	{
		CString result;
		static BYTE parms[] = VTS_VARIANT ;
		InvokeHelper(0x52, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, &expr);
		return result;
	}
	BOOL SetABI(long abi)
	{
		BOOL result;
		static BYTE parms[] = VTS_I4 ;
		InvokeHelper(0x53, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, abi);
		return result;
	}
	BOOL StepInto()
	{
		BOOL result;
		InvokeHelper(0x54, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	BOOL StepOver()
	{
		BOOL result;
		InvokeHelper(0x55, DISPATCH_METHOD, VT_BOOL, (void*)&result, NULL);
		return result;
	}
	void AddTarget(unsigned long serial)
	{
		static BYTE parms[] = VTS_UI4 ;
		InvokeHelper(0x56, DISPATCH_METHOD, VT_EMPTY, NULL, parms, serial);
	}
	void AddTargetOn(unsigned long serial, LPCTSTR ipAddress)
	{
		static BYTE parms[] = VTS_UI4 VTS_BSTR ;
		InvokeHelper(0x57, DISPATCH_METHOD, VT_EMPTY, NULL, parms, serial, ipAddress);
	}

	// IDash properties
public:

};
