function SA_buildSpectrum(da_h, build_path, build)
%function SA_buildSpectrum(da_h, build_path, build)
%
% Read the power spectra from MCP and assemble a complete spectra. Will
% pause after reading the spectra, set a breakpoint in Codescape, run to
% tune after each iteration, then run the script, or run past pause.
%
    daCOM_h = da_h.h;

    da_h.daFindTarget('MTP');

    contextString = '((SPECAN_INSTANCE_CTX_T *)(SPECAN_topCtx.SA_ctxPtr))';

    % Check arguments
    elf_filename = [build_path '\support\specAn\loaders\testApp\build\smake\' build '\SPECAN_t0.elf'];
    %mcpelf_filename = [build_path '\support\specAn\application\testApp\build\smake\obj\SPECAN_core_' build '\mcp\mcpImageDemod.elf'];

    % Need to load symbols from program file to get EvaluateSymbol to work
    daCOM_h.LoadProgramFileEx(elf_filename, 0, 2)

    sampleRate = daCOM_h.EvaluateSymbol([contextString '->q27p4_effectiveSampleRate']);
    sampleRateMHz = double(sampleRate / (2^4)) / 1e6;    % in Q27.4
    %sampleRateMHz = 80;

    %tunerBW = daCOM_h.EvaluateSymbol([contextString '->currentBW']);
    tunerStep = daCOM_h.EvaluateSymbol([contextString '->tuningStep']);
    tunerStepMHz = double(tunerStep) / 1e6;

    dcCompOn = daCOM_h.EvaluateSymbol([contextString '->dcOffsetCtx.enabled']);

    dcBin = daCOM_h.EvaluateSymbol([contextString '->dcOffsetCtx.offsetBin']);
    dcBin = double(dcBin) + (-1:1);

    fftLen = daCOM_h.EvaluateSymbol([contextString '->FFTsize']);
    fftLen = double(fftLen);


    xax = (-fftLen/2:fftLen/2-1) * sampleRateMHz / fftLen;
    % calculate passband start and stop points

    tuneStepBins = (tunerStepMHz / sampleRateMHz) * fftLen;

    % overlapping bin is halfway between tune step.
    olapBin = (tuneStepBins / 2);

    startPt = fftLen/2 - olapBin;
    endPt   = fftLen/2 + olapBin + 1; % this gives correct overlap

    xax2 = xax(startPt+1:endPt); % axis for passband only


    h=figure;

    olapSpecOld = 0;
    %offset =0;

    while (1)


        da_h.daFindTarget('MTP');

        centreFreq = daCOM_h.EvaluateSymbol([contextString '->currentCentreFreq']);
        offset = double(centreFreq) / 1e6;

        figure(h);

        ax(1) = subplot(211);
        title('Resultant Power FFT');
        grid on;
        hold on;

        xlabel('Frequency (MHz)');        

        if dcCompOn
            primaryBuffer = readDoubleMemory(daCOM_h, [contextString '->dcOffsetCtx.pTmpBuf'], fftLen);
            
            da_h.daFindTarget('MCP');
            secondaryBuffer = da_h.daReadMCPMemory('outputPowBuf', fftLen, 'd', 'w');
            
            ax(1) = subplot(211);
            plot(xax+offset, 10*log10(fftshift(primaryBuffer)), 'b', ...
                xax+offset, 10*log10(fftshift(secondaryBuffer)), 'r');
            
            
            % Matlab model of DC compensation
            plotBuffer = dcCompensation(primaryBuffer, secondaryBuffer, dcBin, fftLen);

            % The equivalent DC processed buffer
            da_h.daFindTarget('MTP');
            dcProcessed = readDoubleMemory(daCOM_h, [contextString '->dcOffsetCtx.pTmpBuf'], fftLen);

            ax(2) = subplot(212);
            plot(xax+offset, 10*log10(fftshift(dcProcessed)), 'b:', ...
                xax+offset, 10*log10(fftshift(plotBuffer)), 'r');
            hold on;
            
        else
            da_h.daFindTarget('MCP');

            plotBuffer = da_h.daReadMCPMemory('outputPowBuf', fftLen, 'd', 'w');
            
            plot(xax+offset, 10*log10(fftshift(plotBuffer)), 'b');
        end                

        plotBuffer = 10*log10(fftshift(plotBuffer)); % convert to dB and shift FFTs

        plotBuffer = plotBuffer(startPt+1:endPt); % restrict to composite band

        % knit buffer in with previous plot. How?
        olapSpecLo = plotBuffer(1);
        olapSpecHi = plotBuffer(end);

        if (olapSpecOld ~= 0)
            offsetdB = olapSpecOld -olapSpecLo;
        else
            offsetdB = 0;
        end
        % save off the overlapping bins in dB
        olapSpecOld = olapSpecHi+offsetdB;

        ax(2) = subplot(212);
        plot(xax2(1)+offset, olapSpecLo, '.r', ...
            xax2(end)+offset, olapSpecHi, '.b',...
            xax2+offset, plotBuffer+offsetdB, 'k--');

        title('Composite spectrum');
        hold on;
        grid on;
        axis auto;        
        linkaxes(ax, 'x');     
        
        disp('Hit any key to run again');
        pause;

    end
end

% average the ffts and remove DC.
function outBuff = dcCompensation(buff1, buff2, dcBin, fftLen)

A1 = buff1(fftLen-dcBin+1); % corrupted bin from first buffer
A2 = buff2(fftLen-dcBin+1); % uncorrupted from second buffer

B1 = buff1(dcBin+1);        % uncorrupted bin from first buffer
B2 = buff2(dcBin+1); % corrupted bin from second buffer

dcEst = ((A1-A2) + (B2-B1)) / 2;

% compensate in each bin
buff1(fftLen-dcBin+1) = A1 - dcEst;
buff2(dcBin+1) = B2 - dcEst;

% average to create plotting buffer
outBuff = (buff1 + buff2) / 2;
outBuff = abs(outBuff);

end

function buff = readDoubleMemory(daCOM_h, symbolicAddress, len)

% Data is 32-bit values, read in as 16-bit values
addr = daCOM_h.EvaluateSymbol(symbolicAddress);
a = daCOM_h.ReadMemoryBlock(addr, len*2, -2, 1);
buff = double(cell2mat(a));
buff = buff + ((buff < 0) * 2^16); % convert to unsigned
buff = buff(1:2:len*2-1) + (buff(2:2:len*2)*2^16);
buff = buff / (2^23);
end