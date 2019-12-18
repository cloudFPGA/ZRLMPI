--  *
--  *                       cloudFPGA
--  *     Copyright IBM Research, All Rights Reserved
--  *    =============================================
--  *     Created: Apr 2019
--  *     Authors: FAB, WEI, NGL
--  *
--  *     Description:
--  *       ROLE template for Themisto SRA including ZRLMPI cores
--  *

--******************************************************************************
--**  CONTEXT CLAUSE  **  FMKU60 ROLE(Flash)
--******************************************************************************
library IEEE;
use     IEEE.std_logic_1164.all;
use     IEEE.numeric_std.all;

library UNISIM; 
use     UNISIM.vcomponents.all;

-- library XIL_DEFAULTLIB;
-- use     XIL_DEFAULTLIB.all;


--******************************************************************************
--**  ENTITY  **  FMKU60 ROLE
--******************************************************************************

entity Role_Themisto is
  port (

    --------------------------------------------------------
    -- SHELL / Global Input Clock and Reset Interface
    --------------------------------------------------------
    piSHL_156_25Clk                     : in    std_ulogic;
    piSHL_156_25Rst                     : in    std_ulogic;
    -- LY7 Enable and Reset
    piMMIO_Ly7_Rst                      : in    std_ulogic;
    piMMIO_Ly7_En                       : in    std_ulogic;

    ------------------------------------------------------
    -- SHELL / Role / Nts0 / Udp Interface
    ------------------------------------------------------
    ---- Input AXI-Write Stream Interface ----------
    siNRC_Udp_Data_tdata       : in    std_ulogic_vector( 63 downto 0);
    siNRC_Udp_Data_tkeep       : in    std_ulogic_vector(  7 downto 0);
    siNRC_Udp_Data_tvalid      : in    std_ulogic;
    siNRC_Udp_Data_tlast       : in    std_ulogic;
    siNRC_Udp_Data_tready      : out   std_ulogic;
    ---- Output AXI-Write Stream Interface ---------
    soNRC_Udp_Data_tdata       : out   std_ulogic_vector( 63 downto 0);
    soNRC_Udp_Data_tkeep       : out   std_ulogic_vector(  7 downto 0);
    soNRC_Udp_Data_tvalid      : out   std_ulogic;
    soNRC_Udp_Data_tlast       : out   std_ulogic;
    soNRC_Udp_Data_tready      : in    std_ulogic;
    -- Open Port vector
    poROL_Nrc_Udp_Rx_ports     : out    std_ulogic_vector( 31 downto 0);
    -- ROLE <-> NRC Meta Interface
    soROLE_Nrc_Udp_Meta_TDATA   : out   std_ulogic_vector( 79 downto 0);
    soROLE_Nrc_Udp_Meta_TVALID  : out   std_ulogic;
    soROLE_Nrc_Udp_Meta_TREADY  : in    std_ulogic;
    soROLE_Nrc_Udp_Meta_TKEEP   : out   std_ulogic_vector(  9 downto 0);
    soROLE_Nrc_Udp_Meta_TLAST   : out   std_ulogic;
    siNRC_Role_Udp_Meta_TDATA   : in    std_ulogic_vector( 79 downto 0);
    siNRC_Role_Udp_Meta_TVALID  : in    std_ulogic;
    siNRC_Role_Udp_Meta_TREADY  : out   std_ulogic;
    siNRC_Role_Udp_Meta_TKEEP   : in    std_ulogic_vector(  9 downto 0);
    siNRC_Role_Udp_Meta_TLAST   : in    std_ulogic;
      
    ------------------------------------------------------
    -- SHELL / Role / Nts0 / Tcp Interface
    ------------------------------------------------------
    ---- Input AXI-Write Stream Interface ----------
    siNRC_Tcp_Data_tdata       : in    std_ulogic_vector( 63 downto 0);
    siNRC_Tcp_Data_tkeep       : in    std_ulogic_vector(  7 downto 0);
    siNRC_Tcp_Data_tvalid      : in    std_ulogic;
    siNRC_Tcp_Data_tlast       : in    std_ulogic;
    siNRC_Tcp_Data_tready      : out   std_ulogic;
    ---- Output AXI-Write Stream Interface ---------
    soNRC_Tcp_Data_tdata       : out   std_ulogic_vector( 63 downto 0);
    soNRC_Tcp_Data_tkeep       : out   std_ulogic_vector(  7 downto 0);
    soNRC_Tcp_Data_tvalid      : out   std_ulogic;
    soNRC_Tcp_Data_tlast       : out   std_ulogic;
    soNRC_Tcp_Data_tready      : in    std_ulogic;
    -- Open Port vector
    poROL_Nrc_Tcp_Rx_ports     : out    std_ulogic_vector( 31 downto 0);
    -- ROLE <-> NRC Meta Interface
    soROLE_Nrc_Tcp_Meta_TDATA   : out   std_ulogic_vector( 79 downto 0);
    soROLE_Nrc_Tcp_Meta_TVALID  : out   std_ulogic;
    soROLE_Nrc_Tcp_Meta_TREADY  : in    std_ulogic;
    soROLE_Nrc_Tcp_Meta_TKEEP   : out   std_ulogic_vector(  9 downto 0);
    soROLE_Nrc_Tcp_Meta_TLAST   : out   std_ulogic;
    siNRC_Role_Tcp_Meta_TDATA   : in    std_ulogic_vector( 79 downto 0);
    siNRC_Role_Tcp_Meta_TVALID  : in    std_ulogic;
    siNRC_Role_Tcp_Meta_TREADY  : out   std_ulogic;
    siNRC_Role_Tcp_Meta_TKEEP   : in    std_ulogic_vector(  9 downto 0);
    siNRC_Role_Tcp_Meta_TLAST   : in    std_ulogic;
    
    
    --------------------------------------------------------
    -- SHELL / Mem / Mp0 Interface
    --------------------------------------------------------
    ---- Memory Port #0 / S2MM-AXIS ----------------   
    ------ Stream Read Command ---------
    soSHL_Mem_Mp0_RdCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp0_RdCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp0_RdCmd_tready          : in    std_ulogic;
    ------ Stream Read Status ----------
    siSHL_Mem_Mp0_RdSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp0_RdSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp0_RdSts_tready          : out   std_ulogic;
    ------ Stream Data Input Channel ---
    siSHL_Mem_Mp0_Read_tdata            : in    std_ulogic_vector(511 downto 0);
    siSHL_Mem_Mp0_Read_tkeep            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Mem_Mp0_Read_tlast            : in    std_ulogic;
    siSHL_Mem_Mp0_Read_tvalid           : in    std_ulogic;
    siSHL_Mem_Mp0_Read_tready           : out   std_ulogic;
    ------ Stream Write Command --------
    soSHL_Mem_Mp0_WrCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp0_WrCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp0_WrCmd_tready          : in    std_ulogic;
    ------ Stream Write Status ---------
    siSHL_Mem_Mp0_WrSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp0_WrSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp0_WrSts_tready          : out   std_ulogic;
    ------ Stream Data Output Channel --
    soSHL_Mem_Mp0_Write_tdata           : out   std_ulogic_vector(511 downto 0);
    soSHL_Mem_Mp0_Write_tkeep           : out   std_ulogic_vector( 63 downto 0);
    soSHL_Mem_Mp0_Write_tlast           : out   std_ulogic;
    soSHL_Mem_Mp0_Write_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp0_Write_tready          : in    std_ulogic; 
    
    --------------------------------------------------------
    -- SHELL / Mem / Mp1 Interface
    --------------------------------------------------------
    ---- Memory Port #1 / S2MM-AXIS ------------------   
    ------ Stream Read Command ---------
    soSHL_Mem_Mp1_RdCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp1_RdCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp1_RdCmd_tready          : in    std_ulogic;
    ------ Stream Read Status ----------
    siSHL_Mem_Mp1_RdSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp1_RdSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp1_RdSts_tready          : out   std_ulogic;
    ------ Stream Data Input Channel ---
    siSHL_Mem_Mp1_Read_tdata            : in    std_ulogic_vector(511 downto 0);
    siSHL_Mem_Mp1_Read_tkeep            : in    std_ulogic_vector( 63 downto 0);
    siSHL_Mem_Mp1_Read_tlast            : in    std_ulogic;
    siSHL_Mem_Mp1_Read_tvalid           : in    std_ulogic;
    siSHL_Mem_Mp1_Read_tready           : out   std_ulogic;
    ------ Stream Write Command --------
    soSHL_Mem_Mp1_WrCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
    soSHL_Mem_Mp1_WrCmd_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp1_WrCmd_tready          : in    std_ulogic;
    ------ Stream Write Status ---------
    siSHL_Mem_Mp1_WrSts_tvalid          : in    std_ulogic;
    siSHL_Mem_Mp1_WrSts_tdata           : in    std_ulogic_vector(  7 downto 0);
    siSHL_Mem_Mp1_WrSts_tready          : out   std_ulogic;
    ------ Stream Data Output Channel --
    soSHL_Mem_Mp1_Write_tdata           : out   std_ulogic_vector(511 downto 0);
    soSHL_Mem_Mp1_Write_tkeep           : out   std_ulogic_vector( 63 downto 0);
    soSHL_Mem_Mp1_Write_tlast           : out   std_ulogic;
    soSHL_Mem_Mp1_Write_tvalid          : out   std_ulogic;
    soSHL_Mem_Mp1_Write_tready          : in    std_ulogic; 
    
    --------------------------------------------------------
    -- SHELL / Mmio / AppFlash Interface
    --------------------------------------------------------
    ---- [DIAG_CTRL_1] -----------------
    piSHL_Mmio_Mc1_MemTestCtrl          : in    std_ulogic_vector(1 downto 0);
    ---- [DIAG_STAT_1] -----------------
    poSHL_Mmio_Mc1_MemTestStat          : out   std_ulogic_vector(1 downto 0);
    ---- [DIAG_CTRL_2] -----------------
    piSHL_Mmio_UdpEchoCtrl              : in    std_ulogic_vector(  1 downto 0);
    piSHL_Mmio_UdpPostDgmEn             : in    std_ulogic;
    piSHL_Mmio_UdpCaptDgmEn             : in    std_ulogic;
    piSHL_Mmio_TcpEchoCtrl              : in    std_ulogic_vector(  1 downto 0);
    piSHL_Mmio_TcpPostSegEn             : in    std_ulogic;
    piSHL_Mmio_TcpCaptSegEn             : in    std_ulogic;
    ---- [APP_RDROL] -------------------
    poSHL_Mmio_RdReg                    : out  std_logic_vector( 15 downto 0);
    --- [APP_WRROL] --------------------
    piSHL_Mmio_WrReg                    : in   std_logic_vector( 15 downto 0);

    --------------------------------------------------------
    -- TOP : Secondary Clock (Asynchronous)
    --------------------------------------------------------
    piTOP_250_00Clk                     : in    std_ulogic;  -- Freerunning
    
    ------------------------------------------------
    -- SMC Interface
    ------------------------------------------------ 
    piFMC_ROLE_rank                      : in    std_logic_vector(31 downto 0);
    piFMC_ROLE_size                      : in    std_logic_vector(31 downto 0);
    
    poVoid                              : out   std_ulogic

  );
  
end Role_Themisto;


-- *****************************************************************************
-- **  ARCHITECTURE  **  FLASH of ROLE 
-- *****************************************************************************

architecture Flash of Role_Themisto is

  constant cUSE_DEPRECATED_DIRECTIVES       : boolean := true;

  --============================================================================
  --  SIGNAL DECLARATIONS
  --============================================================================  


  ----============================================================================
  ---- TEMPORARY PROC: ROLE / Nts0 / Tcp Interface to AVOID UNDEFINED CONTENT
  ----============================================================================
  -------- Input AXI-Write Stream Interface --------
  --signal sROL_Shl_Nts0_Tcp_Axis_tready      : std_ulogic;
  --signal sSHL_Rol_Nts0_Tcp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
  --signal sSHL_Rol_Nts0_Tcp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
  --signal sSHL_Rol_Nts0_Tcp_Axis_tlast       : std_ulogic;
  --signal sSHL_Rol_Nts0_Tcp_Axis_tvalid      : std_ulogic;
  -------- Output AXI-Write Stream Interface -------
  --signal sROL_Shl_Nts0_Tcp_Axis_tdata       : std_ulogic_vector( 63 downto 0);
  --signal sROL_Shl_Nts0_Tcp_Axis_tkeep       : std_ulogic_vector(  7 downto 0);
  --signal sROL_Shl_Nts0_Tcp_Axis_tlast       : std_ulogic;
  --signal sROL_Shl_Nts0_Tcp_Axis_tvalid      : std_ulogic;
  --signal sSHL_Rol_Nts0_Tcp_Axis_tready      : std_ulogic;

  --============================================================================
  -- TEMPORARY PROC: ROLE / Mem / Mp0 Interface to AVOID UNDEFINED CONTENT
  --============================================================================
  ------  Stream Read Command --------------
  signal sROL_Shl_Mem_Mp0_Axis_RdCmd_tdata  : std_ulogic_vector( 71 downto 0);
  signal sROL_Shl_Mem_Mp0_Axis_RdCmd_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_RdCmd_tready : std_ulogic;
  ------ Stream Read Status ----------------
  signal sROL_Shl_Mem_Mp0_Axis_RdSts_tready : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_RdSts_tdata  : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid : std_ulogic;
  ------ Stream Data Input Channel ---------
  signal sROL_Shl_Mem_Mp0_Axis_Read_tready  : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_Read_tdata   : std_ulogic_vector(511 downto 0);
  signal sSHL_Rol_Mem_Mp0_Axis_Read_tkeep   : std_ulogic_vector( 63 downto 0);
  signal sSHL_Rol_Mem_Mp0_Axis_Read_tlast   : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_Read_tvalid  : std_ulogic;
  ------ Stream Write Command --------------
  signal sROL_Shl_Mem_Mp0_Axis_WrCmd_tdata  : std_ulogic_vector( 71 downto 0);
  signal sROL_Shl_Mem_Mp0_Axis_WrCmd_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_WrCmd_tready : std_ulogic;
  ------ Stream Write Status ---------------
  signal sROL_Shl_Mem_Mp0_Axis_WrSts_tready : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_WrSts_tdata  : std_ulogic_vector(  7 downto 0);
  signal sSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid : std_ulogic;
  ------ Stream Data Output Channel --------
  signal sROL_Shl_Mem_Mp0_Axis_Write_tdata  : std_ulogic_vector(511 downto 0);
  signal sROL_Shl_Mem_Mp0_Axis_Write_tkeep  : std_ulogic_vector( 63 downto 0);
  signal sROL_Shl_Mem_Mp0_Axis_Write_tlast  : std_ulogic;
  signal sROL_Shl_Mem_Mp0_Axis_Write_tvalid : std_ulogic;
  signal sSHL_Rol_Mem_Mp0_Axis_Write_tready : std_ulogic;
    
  ------------------------------------------------------
  -- MPI Interface
  ------------------------------------------------------
  signal sAPP_MPE_MPIif_mpi_call_TDATA        : std_ulogic_vector(7 downto 0);
  signal sAPP_MPE_MPIif_mpi_call_TVALID       : std_ulogic;
  signal sAPP_MPE_MPIif_mpi_call_TREADY       : std_ulogic;
  signal sAPP_MPE_MPIif_count_TDATA           : std_ulogic_vector(31 downto 0);
  signal sAPP_MPE_MPIif_count_TVALID          : std_ulogic;
  signal sAPP_MPE_MPIif_count_TREADY          : std_ulogic;
  signal sAPP_MPE_MPIif_rank_TDATA            : std_ulogic_vector(31 downto 0);
  signal sAPP_MPE_MPIif_rank_TVALID           : std_ulogic;
  signal sAPP_MPE_MPIif_rank_TREADY           : std_ulogic;
  signal sMPE_APP_MPI_data_TDATA              : std_ulogic_vector(7 downto 0);
  signal sMPE_APP_MPI_data_TVALID             : std_ulogic;
  signal sMPE_APP_MPI_data_TREADY             : std_ulogic;
  signal sMPE_APP_MPI_data_TKEEP              : std_logic_vector(0 downto 0);
  signal sMPE_APP_MPI_data_TLAST              : std_logic_vector(0 downto 0);
  signal sAPP_MPE_MPI_data_TDATA              : std_ulogic_vector(7 downto 0);
  signal sAPP_MPE_MPI_data_TVALID             : std_ulogic;
  signal sAPP_MPE_MPI_data_TREADY             : std_ulogic;
  signal sAPP_MPE_MPI_data_TKEEP              : std_logic_vector(0 downto 0);
  signal sAPP_MPE_MPI_data_TLAST              : std_logic_vector(0 downto 0);


  signal active_low_reset  : std_logic;

  -- I hate Vivado HLS 
  signal sReadTlastAsVector : std_logic_vector(0 downto 0);
  signal sWriteTlastAsVector : std_logic_vector(0 downto 0);
  signal sResetAsVector : std_logic_vector(0 downto 0);

  signal sMetaOutTlastAsVector_Udp : std_logic_vector(0 downto 0);
  signal sMetaInTlastAsVector_Udp  : std_logic_vector(0 downto 0);
  signal sMetaOutTlastAsVector_Tcp : std_logic_vector(0 downto 0);
  signal sMetaInTlastAsVector_Tcp  : std_logic_vector(0 downto 0);
  signal sDataOutTlastAsVector_Udp : std_logic_vector(0 downto 0);
  signal sDataInTlastAsVector_Udp  : std_logic_vector(0 downto 0);

  signal sUdpPostCnt : std_ulogic_vector(9 downto 0);
  signal sTcpPostCnt : std_ulogic_vector(9 downto 0);


  --============================================================================
  --  VARIABLE DECLARATIONS
  --============================================================================  

  --===========================================================================
  --== COMPONENT DECLARATIONS
  --===========================================================================
  component TriangleApplication is
    port (
      ------------------------------------------------------
      -- From SHELL / Clock and Reset
      ------------------------------------------------------
           ap_clk                      : in  std_logic;
           ap_rst_n                    : in  std_logic;
           ap_start                    : in  std_logic;

      -- rank and size
           piFMC_ROL_rank_V        : in std_logic_vector (31 downto 0);
      --piSMC_ROL_rank_V_ap_vld : in std_logic;
           piFMC_ROL_size_V        : in std_logic_vector (31 downto 0);
      --piSMC_ROL_size_V_ap_vld : in std_logic;
      --------------------------------------------------------
      -- From SHELL / Udp Data Interfaces
      --------------------------------------------------------
           siSHL_This_Data_tdata     : in  std_logic_vector( 63 downto 0);
           siSHL_This_Data_tkeep     : in  std_logic_vector(  7 downto 0);
           siSHL_This_Data_tlast     : in  std_logic;
           siSHL_This_Data_tvalid    : in  std_logic;
           siSHL_This_Data_tready    : out std_logic;
      --------------------------------------------------------
      -- To SHELL / Udp Data Interfaces
      --------------------------------------------------------
           soTHIS_Shl_Data_tdata     : out std_logic_vector( 63 downto 0);
           soTHIS_Shl_Data_tkeep     : out std_logic_vector(  7 downto 0);
           soTHIS_Shl_Data_tlast     : out std_logic;
           soTHIS_Shl_Data_tvalid    : out std_logic;
           soTHIS_Shl_Data_tready    : in  std_logic;
      -- NRC Meta and Ports
           siNrc_meta_TDATA          : in std_logic_vector (79 downto 0);
           siNrc_meta_TVALID         : in std_logic;
           siNrc_meta_TREADY         : out std_logic;
           siNrc_meta_TKEEP          : in std_logic_vector (9 downto 0);
           siNrc_meta_TLAST          : in std_logic_vector (0 downto 0);

           soNrc_meta_TDATA          : out std_logic_vector (79 downto 0);
           soNrc_meta_TVALID         : out std_logic;
           soNrc_meta_TREADY         : in std_logic;
           soNrc_meta_TKEEP          : out std_logic_vector (9 downto 0);
           soNrc_meta_TLAST          : out std_logic_vector (0 downto 0);

           poROL_NRC_Rx_ports_V        : out std_logic_vector (31 downto 0);
           poROL_NRC_Rx_ports_V_ap_vld : out std_logic
         );
  end component TriangleApplication;
  
  component mpi_wrapperv1 is
    port (
    ap_clk : IN STD_LOGIC;
    ap_rst_n : IN STD_LOGIC;
    ap_start : IN STD_LOGIC;
    ap_done : OUT STD_LOGIC;
    ap_idle : OUT STD_LOGIC;
    ap_ready : OUT STD_LOGIC;
    piSMC_to_ROLE_rank_V : IN STD_LOGIC_VECTOR (31 downto 0);
    piSMC_to_ROLE_rank_V_ap_vld : IN STD_LOGIC;
    piSMC_to_ROLE_size_V : IN STD_LOGIC_VECTOR (31 downto 0);
    piSMC_to_ROLE_size_V_ap_vld : IN STD_LOGIC;
    poMMIO_V : OUT STD_LOGIC_VECTOR (15 downto 0);
    poMMIO_V_ap_vld : OUT STD_LOGIC;
    soMPIif_V_mpi_call_V_TDATA : OUT STD_LOGIC_VECTOR (7 downto 0);
    soMPIif_V_mpi_call_V_TVALID : OUT STD_LOGIC;
    soMPIif_V_mpi_call_V_TREADY : IN STD_LOGIC;
    soMPIif_V_count_V_TDATA : OUT STD_LOGIC_VECTOR (31 downto 0);
    soMPIif_V_count_V_TVALID : OUT STD_LOGIC;
    soMPIif_V_count_V_TREADY : IN STD_LOGIC;
    soMPIif_V_rank_V_TDATA : OUT STD_LOGIC_VECTOR (31 downto 0);
    soMPIif_V_rank_V_TVALID : OUT STD_LOGIC;
    soMPIif_V_rank_V_TREADY : IN STD_LOGIC;
    soMPI_data_TDATA : OUT STD_LOGIC_VECTOR (7 downto 0);
    soMPI_data_TVALID : OUT STD_LOGIC;
    soMPI_data_TREADY : IN STD_LOGIC;
    soMPI_data_TKEEP : OUT STD_LOGIC_VECTOR (0 downto 0);
    soMPI_data_TLAST : OUT STD_LOGIC_VECTOR (0 downto 0);
    siMPI_data_TDATA : IN STD_LOGIC_VECTOR (7 downto 0);
    siMPI_data_TVALID : IN STD_LOGIC;
    siMPI_data_TREADY : OUT STD_LOGIC;
    siMPI_data_TKEEP : IN STD_LOGIC_VECTOR (0 downto 0);
    siMPI_data_TLAST : IN STD_LOGIC_VECTOR (0 downto 0) );
  end component;

  component MessagePassingEngine is
  port (
      siTcp_data_TDATA : IN STD_LOGIC_VECTOR (63 downto 0);
      siTcp_data_TKEEP : IN STD_LOGIC_VECTOR (7 downto 0);
      siTcp_data_TLAST : IN STD_LOGIC_VECTOR (0 downto 0);
      siTcp_meta_TDATA : IN STD_LOGIC_VECTOR (79 downto 0);
      siTcp_meta_TKEEP : IN STD_LOGIC_VECTOR (9 downto 0);
      siTcp_meta_TLAST : IN STD_LOGIC_VECTOR (0 downto 0);
      soTcp_data_TDATA : OUT STD_LOGIC_VECTOR (63 downto 0);
      soTcp_data_TKEEP : OUT STD_LOGIC_VECTOR (7 downto 0);
      soTcp_data_TLAST : OUT STD_LOGIC_VECTOR (0 downto 0);
      soTcp_meta_TDATA : OUT STD_LOGIC_VECTOR (79 downto 0);
      soTcp_meta_TKEEP : OUT STD_LOGIC_VECTOR (9 downto 0);
      soTcp_meta_TLAST : OUT STD_LOGIC_VECTOR (0 downto 0);
      piFMC_rank_V : IN STD_LOGIC_VECTOR (31 downto 0);
      siMPIif_V_mpi_call_V_TDATA : IN STD_LOGIC_VECTOR (7 downto 0);
      siMPIif_V_count_V_TDATA : IN STD_LOGIC_VECTOR (31 downto 0);
      siMPIif_V_rank_V_TDATA : IN STD_LOGIC_VECTOR (31 downto 0);
      siMPI_data_TDATA : IN STD_LOGIC_VECTOR (7 downto 0);
      siMPI_data_TKEEP : IN STD_LOGIC_VECTOR (0 downto 0);
      siMPI_data_TLAST : IN STD_LOGIC_VECTOR (0 downto 0);
      soMPI_data_TDATA : OUT STD_LOGIC_VECTOR (7 downto 0);
      soMPI_data_TKEEP : OUT STD_LOGIC_VECTOR (0 downto 0);
      soMPI_data_TLAST : OUT STD_LOGIC_VECTOR (0 downto 0);
      ap_clk : IN STD_LOGIC;
      ap_rst_n : IN STD_LOGIC;
      siMPIif_V_mpi_call_V_TVALID : IN STD_LOGIC;
      siMPIif_V_mpi_call_V_TREADY : OUT STD_LOGIC;
      siMPIif_V_count_V_TVALID : IN STD_LOGIC;
      siMPIif_V_count_V_TREADY : OUT STD_LOGIC;
      siMPIif_V_rank_V_TVALID : IN STD_LOGIC;
      siMPIif_V_rank_V_TREADY : OUT STD_LOGIC;
      soTcp_meta_TVALID : OUT STD_LOGIC;
      soTcp_meta_TREADY : IN STD_LOGIC;
      soTcp_data_TVALID : OUT STD_LOGIC;
      soTcp_data_TREADY : IN STD_LOGIC;
      siTcp_data_TVALID : IN STD_LOGIC;
      siTcp_data_TREADY : OUT STD_LOGIC;
      siMPI_data_TVALID : IN STD_LOGIC;
      siMPI_data_TREADY : OUT STD_LOGIC;
      siTcp_meta_TVALID : IN STD_LOGIC;
      siTcp_meta_TREADY : OUT STD_LOGIC;
      piFMC_rank_V_ap_vld : IN STD_LOGIC;
      soMPI_data_TVALID : OUT STD_LOGIC;
      soMPI_data_TREADY : IN STD_LOGIC;
      ap_start : IN STD_LOGIC);
  end component;



  --===========================================================================
  --== FUNCTION DECLARATIONS  [TODO-Move to a package]
  --===========================================================================
  function fVectorize(s: std_logic) return std_logic_vector is
    variable v: std_logic_vector(0 downto 0);
  begin
    v(0) := s;
    return v;
  end fVectorize;

  function fScalarize(v: in std_logic_vector) return std_ulogic is
  begin
    assert v'length = 1
    report "scalarize: output port must be single bit!"
    severity FAILURE;
    return v(v'LEFT);
  end;


--################################################################################
--#                                                                              #
--#                          #####   ####  ####  #     #                         #
--#                          #    # #    # #   #  #   #                          #
--#                          #    # #    # #    #  ###                           #
--#                          #####  #    # #    #   #                            #
--#                          #    # #    # #    #   #                            #
--#                          #    # #    # #   #    #                            #
--#                          #####   ####  ####     #                            #
--#                                                                              #
--################################################################################

begin

  --  -- write constant to EMIF Register to test read out 
  --  --poROL_SHL_EMIF_2B_Reg <= x"EF" & EMIF_inv; 
  --  poROL_SHL_EMIF_2B_Reg( 7 downto 0)  <= EMIF_inv; 
  --  poSHL_Mmio_RdReg(11 downto 8) <= piFMC_ROLE_rank(3 downto 0) when (unsigned(piFMC_ROLE_rank) /= 0) else 
  --  x"F"; 
  --  poSHL_Mmio_RdReg(15 downto 12) <= piFMC_ROLE_size(3 downto 0) when (unsigned(piFMC_ROLE_size) /= 0) else 
  --  x"E"; 

  --  EMIF_inv <= (not piSHL_ROL_EMIF_2B_Reg(7 downto 0)) when piSHL_ROL_EMIF_2B_Reg(15) = '1' else 
  --              x"BE" ;

  -- poSHL_Mmio_RdReg <= sMemTestDebugOut when (unsigned(piSHL_Mmio_WrReg) /= 0) else 
  --  x"EFBE"; 

  --################################################################################
  --#                                                                              #
  --#    #     #  #####    ######     #####                                        #
  --#    #     #  #    #   #     #   #     # #####   #####                         #
  --#    #     #  #     #  #     #   #     # #    #  #    #                        #
  --#    #     #  #     #  ######    ####### #####   #####                         #
  --#    #     #  #    #   #         #     # #       #                             #
  --#    #######  #####    #         #     # #       #                             #
  --#                                                                              #
  --################################################################################

  -- gUdpAppFlashDepre : if cUSE_DEPRECATED_DIRECTIVES generate --TODO

  --  begin 

  sMetaInTlastAsVector_Udp(0) <= siNRC_Role_Udp_Meta_TLAST;
  soROLE_Nrc_Udp_Meta_TLAST <=  sMetaOutTlastAsVector_Udp(0);
  sDataInTlastAsVector_Udp(0) <= siNRC_Udp_Data_tlast;
  soNRC_Udp_Data_tlast <= sDataOutTlastAsVector_Udp(0);
  
  active_low_reset <= not (piMMIO_Ly7_Rst);
  
  MPI_APP: mpi_wrapperv1
    port map (
         ap_clk                       => piSHL_156_25Clk ,
         ap_rst_n                     => active_low_reset,
         ap_start                     => piMMIO_Ly7_En,
         piSMC_to_ROLE_rank_V         => piFMC_ROLE_rank,
         piSMC_to_ROLE_rank_V_ap_vld  => '1',
         piSMC_to_ROLE_size_V         => piFMC_ROLE_size,
         piSMC_to_ROLE_size_V_ap_vld  => '1',
         poMMIO_V                     => poSHL_Mmio_RdReg,
         soMPIif_V_mpi_call_V_TDATA   =>  sAPP_MPE_MPIif_mpi_call_TDATA ,
         soMPIif_V_mpi_call_V_TVALID  =>  sAPP_MPE_MPIif_mpi_call_TVALID,
         soMPIif_V_mpi_call_V_TREADY  =>  sAPP_MPE_MPIif_mpi_call_TREADY,
         soMPIif_V_count_V_TDATA      =>  sAPP_MPE_MPIif_count_TDATA    ,
         soMPIif_V_count_V_TVALID     =>  sAPP_MPE_MPIif_count_TVALID   ,
         soMPIif_V_count_V_TREADY     =>  sAPP_MPE_MPIif_count_TREADY   ,
         soMPIif_V_rank_V_TDATA       =>  sAPP_MPE_MPIif_rank_TDATA     ,
         soMPIif_V_rank_V_TVALID      =>  sAPP_MPE_MPIif_rank_TVALID    ,
         soMPIif_V_rank_V_TREADY      =>  sAPP_MPE_MPIif_rank_TREADY    ,
         soMPI_data_TDATA   =>  sAPP_MPE_MPI_data_TDATA  ,
         soMPI_data_TVALID  =>  sAPP_MPE_MPI_data_TVALID ,
         soMPI_data_TREADY  =>  sAPP_MPE_MPI_data_TREADY ,
         soMPI_data_TKEEP   =>  sAPP_MPE_MPI_data_TKEEP  ,
         soMPI_data_TLAST   =>  sAPP_MPE_MPI_data_TLAST  ,
         siMPI_data_TDATA   =>  sMPE_APP_MPI_data_TDATA   ,
         siMPI_data_TVALID  =>  sMPE_APP_MPI_data_TVALID  ,
         siMPI_data_TREADY  =>  sMPE_APP_MPI_data_TREADY  ,
         siMPI_data_TKEEP   =>  sMPE_APP_MPI_data_TKEEP   ,
         siMPI_data_TLAST   =>  sMPE_APP_MPI_data_TLAST   
     );

    MPE: MessagePassingEngine
    port map (
        ap_clk              => piSHL_156_25Clk,
        ap_rst_n            => active_low_reset, 
        ap_start            => piMMIO_Ly7_En,
        siTcp_data_TDATA    =>  siNRC_Udp_Data_tdata ,
        siTcp_data_TKEEP    =>  siNRC_Udp_Data_tkeep ,
        siTcp_data_TVALID   =>  siNRC_Udp_Data_tvalid,
        siTcp_data_TLAST    =>  sDataInTlastAsVector_Udp,
        siTcp_data_TREADY   =>  siNRC_Udp_Data_tready,
        siTcp_meta_TDATA    =>  siNRC_Role_Udp_Meta_TDATA  ,
        siTcp_meta_TVALID   =>  siNRC_Role_Udp_Meta_TVALID ,
        siTcp_meta_TREADY   =>  siNRC_Role_Udp_Meta_TREADY ,
        siTcp_meta_TKEEP    =>  siNRC_Role_Udp_Meta_TKEEP  ,
        siTcp_meta_TLAST    =>  sMetaInTlastAsVector_Udp,
        soTcp_data_TDATA    =>  soNRC_Udp_Data_tdata   ,
        soTcp_data_TKEEP    =>  soNRC_Udp_Data_tkeep   ,
        soTcp_data_TVALID   =>  soNRC_Udp_Data_tvalid  ,
        soTcp_data_TLAST    =>  sDataOutTlastAsVector_Udp,
        soTcp_data_TREADY   =>  soNRC_Udp_Data_tready  ,
        soTcp_meta_TDATA    =>  soROLE_Nrc_Udp_Meta_TDATA  ,
        soTcp_meta_TVALID   =>  soROLE_Nrc_Udp_Meta_TVALID ,
        soTcp_meta_TREADY   =>  soROLE_Nrc_Udp_Meta_TREADY ,
        soTcp_meta_TKEEP    =>  soROLE_Nrc_Udp_Meta_TKEEP  ,
        soTcp_meta_TLAST    =>  sMetaOutTlastAsVector_Udp,
        piFMC_rank_V        =>  piFMC_ROLE_rank,
        piFMC_rank_V_ap_vld =>  '1',
        siMPIif_V_mpi_call_V_TDATA    => sAPP_MPE_MPIif_mpi_call_TDATA  ,
        siMPIif_V_mpi_call_V_TVALID   => sAPP_MPE_MPIif_mpi_call_TVALID ,
        siMPIif_V_mpi_call_V_TREADY   => sAPP_MPE_MPIif_mpi_call_TREADY ,
        siMPIif_V_count_V_TDATA       => sAPP_MPE_MPIif_count_TDATA   ,
        siMPIif_V_count_V_TVALID      => sAPP_MPE_MPIif_count_TVALID  ,
        siMPIif_V_count_V_TREADY      => sAPP_MPE_MPIif_count_TREADY  ,
        siMPIif_V_rank_V_TDATA        => sAPP_MPE_MPIif_rank_TDATA    ,
        siMPIif_V_rank_V_TVALID       => sAPP_MPE_MPIif_rank_TVALID   ,
        siMPIif_V_rank_V_TREADY       => sAPP_MPE_MPIif_rank_TREADY   ,
        siMPI_data_TDATA              => sAPP_MPE_MPI_data_TDATA   ,
        siMPI_data_TVALID             => sAPP_MPE_MPI_data_TVALID  ,
        siMPI_data_TREADY             => sAPP_MPE_MPI_data_TREADY  ,
        siMPI_data_TKEEP              => sAPP_MPE_MPI_data_TKEEP   ,
        siMPI_data_TLAST              => sAPP_MPE_MPI_data_TLAST   ,
        soMPI_data_TDATA              => sMPE_APP_MPI_data_TDATA   ,
        soMPI_data_TVALID             => sMPE_APP_MPI_data_TVALID  ,
        soMPI_data_TREADY             => sMPE_APP_MPI_data_TREADY  ,
        soMPI_data_TKEEP              => sMPE_APP_MPI_data_TKEEP   ,
        soMPI_data_TLAST              => sMPE_APP_MPI_data_TLAST   
     );
 
  
  --################################################################################
  --#                                                                              #
  --#    #######    ####   ######     #####                                        #
  --#       #      #       #     #   #     # #####   #####                         #
  --#       #     #        #     #   #     # #    #  #    #                        #
  --#       #     #        ######    ####### #####   #####                         #
  --#       #      #       #         #     # #       #                             #
  --#       #       ####   #         #     # #       #                             #
  --#                                                                              #
  --################################################################################

  -- gUdpAppFlashDepre : if cUSE_DEPRECATED_DIRECTIVES generate --TODO

  --  begin 

  sMetaInTlastAsVector_Tcp(0) <= siNRC_Role_Tcp_Meta_TLAST;
  soROLE_Nrc_Tcp_Meta_TLAST <=  sMetaOutTlastAsVector_Tcp(0);

  TAF: TriangleApplication
  port map (

             ------------------------------------------------------
             -- From SHELL / Clock and Reset
             ------------------------------------------------------
             ap_clk                      => piSHL_156_25Clk,
             ap_rst_n                    => (not piSHL_156_25Rst),
             --ap_start                    => '1',
             ap_start                    => piMMIO_Ly7_En,
          
             piFMC_ROL_rank_V         => piFMC_ROLE_rank,
             --piFMC_ROL_rank_V_ap_vld  => '1',
             piFMC_ROL_size_V         => piFMC_ROLE_size,
             --piFMC_ROL_size_V_ap_vld  => '1',
             --------------------------------------------------------
             -- From SHELL / Udp Data Interfaces
             --------------------------------------------------------
             siSHL_This_Data_tdata     => siNRC_Tcp_Data_tdata,
             siSHL_This_Data_tkeep     => siNRC_Tcp_Data_tkeep,
             siSHL_This_Data_tlast     => siNRC_Tcp_Data_tlast,
             siSHL_This_Data_tvalid    => siNRC_Tcp_Data_tvalid,
             siSHL_This_Data_tready    => siNRC_Tcp_Data_tready,
             --------------------------------------------------------
             -- To SHELL / Udp Data Interfaces
             --------------------------------------------------------
             soTHIS_Shl_Data_tdata     => soNRC_Tcp_Data_tdata,
             soTHIS_Shl_Data_tkeep     => soNRC_Tcp_Data_tkeep,
             soTHIS_Shl_Data_tlast     => soNRC_Tcp_Data_tlast,
             soTHIS_Shl_Data_tvalid    => soNRC_Tcp_Data_tvalid,
             soTHIS_Shl_Data_tready    => soNRC_Tcp_Data_tready, 

             siNrc_meta_TDATA          =>  siNRC_Role_Tcp_Meta_TDATA    ,
             siNrc_meta_TVALID         =>  siNRC_Role_Tcp_Meta_TVALID   ,
             siNrc_meta_TREADY         =>  siNRC_Role_Tcp_Meta_TREADY   ,
             siNrc_meta_TKEEP          =>  siNRC_Role_Tcp_Meta_TKEEP    ,
             siNrc_meta_TLAST          =>  sMetaInTlastAsVector_Tcp,

             soNrc_meta_TDATA          =>  soROLE_Nrc_Tcp_Meta_TDATA  ,
             soNrc_meta_TVALID         =>  soROLE_Nrc_Tcp_Meta_TVALID ,
             soNrc_meta_TREADY         =>  soROLE_Nrc_Tcp_Meta_TREADY ,
             soNrc_meta_TKEEP          =>  soROLE_Nrc_Tcp_Meta_TKEEP  ,
             soNrc_meta_TLAST          =>  sMetaOutTlastAsVector_Tcp,

             poROL_NRC_Rx_ports_V        => poROL_Nrc_Tcp_Rx_ports
           --poROL_NRC_Udp_Rx_ports_V_ap_vld => '1'
           );


  --################################################################################
  --#                                                                              #
  --#          MEMORY DUMMY CONNECTION                                             #
  --#                                                                              #
  --################################################################################


  pMp0RdCmd : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Mp0_Axis_RdCmd_tready  <= soSHL_Mem_Mp0_RdCmd_tready;
    end if;
    soSHL_Mem_Mp0_RdCmd_tdata  <= (others => '1');
    soSHL_Mem_Mp0_RdCmd_tvalid <= '0';
  end process pMp0RdCmd;
  
  pMp0RdSts : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Mp0_Axis_RdSts_tdata   <= siSHL_Mem_Mp0_RdSts_tdata;
      sSHL_Rol_Mem_Mp0_Axis_RdSts_tvalid  <= siSHL_Mem_Mp0_RdSts_tvalid;
    end if;
    siSHL_Mem_Mp0_RdSts_tready <= '1';
  end process pMp0RdSts;
  
  pMp0Read : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Mp0_Axis_Read_tdata   <= siSHL_Mem_Mp0_Read_tdata;
      sSHL_Rol_Mem_Mp0_Axis_Read_tkeep   <= siSHL_Mem_Mp0_Read_tkeep;
      sSHL_Rol_Mem_Mp0_Axis_Read_tlast   <= siSHL_Mem_Mp0_Read_tlast;
      sSHL_Rol_Mem_Mp0_Axis_Read_tvalid  <= siSHL_Mem_Mp0_Read_tvalid;
    end if;
    siSHL_Mem_Mp0_Read_tready <= '1';
  end process pMp0Read;    
  
  pMp0WrCmd : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Mp0_Axis_WrCmd_tready  <= soSHL_Mem_Mp0_WrCmd_tready;
    end if;
    soSHL_Mem_Mp0_WrCmd_tdata  <= (others => '0');
    soSHL_Mem_Mp0_WrCmd_tvalid <= '0';  
  end process pMp0WrCmd;
  
  pMp0WrSts : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Mp0_Axis_WrSts_tdata   <= siSHL_Mem_Mp0_WrSts_tdata;
      sSHL_Rol_Mem_Mp0_Axis_WrSts_tvalid  <= siSHL_Mem_Mp0_WrSts_tvalid;
    end if;
    siSHL_Mem_Mp0_WrSts_tready <= '1';
  end process pMp0WrSts;
  
  pMp0Write : process(piSHL_156_25Clk)
  begin
    if rising_edge(piSHL_156_25Clk) then
      sSHL_Rol_Mem_Mp0_Axis_Write_tready  <= soSHL_Mem_Mp0_Write_tready;
    end if;
    soSHL_Mem_Mp0_Write_tdata  <= (others => '0');
    soSHL_Mem_Mp0_Write_tkeep  <= (others => '0');
    soSHL_Mem_Mp0_Write_tlast  <= '0';
    soSHL_Mem_Mp0_Write_tvalid <= '0';
  end process pMp0Write;



end architecture Flash;

