UNIT_TYPE=MOD
UNIT_NAME=SPECAN_core

C_SRC=SPECAN_core.c \
      SPECAN_init.c \
      SPECAN_coreAPI.c \
      SPECAN_controlTask.c \
      SPECAN_frontEnd.c \
      SPECAN_mcpCtrl.c \
      SPECAN_configure.c \
      SPECAN_scpData.c \
      SPECAN_isr.c \
      SPECAN_scanCtrl.c \
      SPECAN_HammingWindow.c \
      SPECAN_HanningWindow.c \
      SPECAN_logging.c \
      SPECAN_timestamp.c \
      SPECAN_dcOffsetHandling.c \
      SPECAN_compositeMgr.c

# List of directories where register config files may be found
# (UCC_DRIVERS_DIRS is set up by the top level uccDrivers makefile fragment)
REG_DIRL+=$(UCC_DRIVERS_DIRS)

# include *.xml files from SCP driver
REG_SRC=scpDmaConfigs

DCP_SRC=SPECAN_mcposDriver.dcps

DCP_MAPFILES=SPECAN_pipeline.dcpsys

# DISABLE_DEFAULT_MCPFLAGS is currently needed when building MCP code in "non-legacy" mode
UNIT_OPTIONS+=DISABLE_DEFAULT_MCPFLAGS
# Debugging option, gives verbose output from MCP build:
#GLOBAL_MCPFLAGS+=-l # -v
# Re-name the MCP output from the default name of mcp_source (not strictly necessary)
MCP_IMAGE=mcpprogs
MCP_IMAGES=mcpImageDemod
$(MCP_IMAGE)_MCP_FORMAT_CMD=mcpImageDemod.elf -sp mcp_src_

MCP_DIRL+=$(UCCLIB_ROOT)/mcpOS/source/mcp \
		  $(UCC_INST_ROOT)/config/uccps/uccp$(UCCP) \
		  $(UNIT_DIR)/source/mcp \
		  $(UCC_INST_ROOT)/lib

MCP_SRC=mcpos.mcp \
        uccp$(UCCP)_registers.mcp \
        SPECAN_accumPower.mcp \
        SPECAN_outerLoopProc.mcp \
        SPECAN_SCPoutBufferProc.mcp \
        SPECAN_sharedData.mcp \
        SPECAN_symbols.mcp \
        SPECAN_loopCtrl.mcp \
        SPECAN_processSCPqueues.mcp \
        SPECAN_fftProc.mcp \
        SPECAN_windowProc.mcp \
        MCPLib.mcp        

MCP_MAPFILE=SPECAN.map

include $(SMAKE_ROOT)/smake.mk
