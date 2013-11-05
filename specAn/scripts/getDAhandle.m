function [da_h] = getDAhandle()
%function [da_h] = getDAhandle()
%
% Return the UniSim Dash object, a wrapper to the COM object.
    % Add paths
    fpath = mfilename('fullpath'); % Get full path to this file including filename 
    [pathstr, ~, ~, ~] = fileparts(fpath); % pathstr contains the path to this file 

    UniSim_dir = [pathstr '\..\..\common\Matlab\simulation\Frameworks\UniSim'];

    start_dir = pwd;
    cd([UniSim_dir '\build'])
    UniSim_addPaths(UniSim_dir);
    addpath([pathstr '\..\..\common\Matlab\simulation\DA']);
    cd(start_dir)
    
    % connect to DA
    da_h = da();
    da_h.daStart();
    %daCOM_h = da_h.h;
end

