function SA_debug(daCOM_h, build_path, build)

    % Check arguments
    elf_filename = [build_path '\support\specAn\loaders\testApp\build\smake\' build '\SPECAN_t0.elf'];
    
    % Need to load symbols from program file to get EvaluateSymbol to work
    daCOM_h.LoadProgramFileEx(elf_filename, 0, 2)
    
    % Read SCP output buffer contents
    SCPbuffA_addr = daCOM_h.EvaluateSymbol('((((SPECAN_INSTANCE_CTX_T *)(SPECAN_topCtx.SA_ctxPtr))->MCP_ptrs).SCPoutBufferA).cpxAddr');
    SCPdata = readComplexData(daCOM_h, SCPbuffA_addr, 1, 512);
    
    % Calculate spectrum of SCP i/p data
    figure(1)
    plot(fftshift(abs(fft(SCPdata)))); grid; title('Spectrum of SCP data')
    
    % Read window function buffer
    windowFunc_addr = daCOM_h.EvaluateSymbol('((((SPECAN_INSTANCE_CTX_T *)(SPECAN_topCtx.SA_ctxPtr))->MCP_ptrs).windowFunc).cpxAddr');
    windowFunc = readComplexData(daCOM_h, windowFunc_addr, 0, 8192);
    
    % Plot window function
    figure(2)
    ax(1) = subplot(2, 1, 1);
    plot(real(windowFunc)); grid; title('Window function - real')
    ax(2) = subplot(2, 1, 2);
    plot(imag(windowFunc)); grid; title('Window function - imag')
    linkaxes(ax, 'x');
    
end


function data = readComplexData(DA_h, addr, int_bits, len)
    % Data is 32-bit values with imag part on the LSBs.  Read in as 16-bit values
    % so we get interspersed real & imag values.
    a = DA_h.ReadMemoryBlock(addr, len * 2, -2, 1);
    b = double(cell2mat(a));
    data = b(2:2:end) + (j * b(1:2:end));
    % right-shift 4 places because we have 12-bit quantities on MSBs of 16-bit words
    data = data / 16;
    % Convert to floating point in the desired Q format
    frac_bits = 11 - int_bits;
    data = data / 2^frac_bits;
end

