-- /*******************************************************************************
--  * Copyright 2018 -- 2023 IBM Corporation
--  *
--  * Licensed under the Apache License, Version 2.0 (the "License");
--  * you may not use this file except in compliance with the License.
--  * You may obtain a copy of the License at
--  *
--  *     http://www.apache.org/licenses/LICENSE-2.0
--  *
--  * Unless required by applicable law or agreed to in writing, software
--  * distributed under the License is distributed on an "AS IS" BASIS,
--  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--  * See the License for the specific language governing permissions and
--  * limitations under the License.
-- *******************************************************************************/

--  *
--  *                       cloudFPGA
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
         soROLE_Nrc_Udp_Meta_TDATA   : out   std_ulogic_vector( 63 downto 0);
         soROLE_Nrc_Udp_Meta_TVALID  : out   std_ulogic;
         soROLE_Nrc_Udp_Meta_TREADY  : in    std_ulogic;
         soROLE_Nrc_Udp_Meta_TKEEP   : out   std_ulogic_vector(  7 downto 0);
         soROLE_Nrc_Udp_Meta_TLAST   : out   std_ulogic;
         siNRC_Role_Udp_Meta_TDATA   : in    std_ulogic_vector( 63 downto 0);
         siNRC_Role_Udp_Meta_TVALID  : in    std_ulogic;
         siNRC_Role_Udp_Meta_TREADY  : out   std_ulogic;
         siNRC_Role_Udp_Meta_TKEEP   : in    std_ulogic_vector(  7 downto 0);
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
         soROLE_Nrc_Tcp_Meta_TDATA   : out   std_ulogic_vector( 63 downto 0);
         soROLE_Nrc_Tcp_Meta_TVALID  : out   std_ulogic;
         soROLE_Nrc_Tcp_Meta_TREADY  : in    std_ulogic;
         soROLE_Nrc_Tcp_Meta_TKEEP   : out   std_ulogic_vector(  7 downto 0);
         soROLE_Nrc_Tcp_Meta_TLAST   : out   std_ulogic;
         siNRC_Role_Tcp_Meta_TDATA   : in    std_ulogic_vector( 63 downto 0);
         siNRC_Role_Tcp_Meta_TVALID  : in    std_ulogic;
         siNRC_Role_Tcp_Meta_TREADY  : out   std_ulogic;
         siNRC_Role_Tcp_Meta_TKEEP   : in    std_ulogic_vector(  7 downto 0);
         siNRC_Role_Tcp_Meta_TLAST   : in    std_ulogic;


    --------------------------------------------------------
    -- SHELL / Mem / Mp0 Interface
    --------------------------------------------------------
    ---- Memory Port #0 / S2MM-AXIS ----------------   
    ------ Stream Read Command ---------
         soMEM_Mp0_RdCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
         soMEM_Mp0_RdCmd_tvalid          : out   std_ulogic;
         soMEM_Mp0_RdCmd_tready          : in    std_ulogic;
    ------ Stream Read Status ----------
         siMEM_Mp0_RdSts_tdata           : in    std_ulogic_vector(  7 downto 0);
         siMEM_Mp0_RdSts_tvalid          : in    std_ulogic;
         siMEM_Mp0_RdSts_tready          : out   std_ulogic;
    ------ Stream Data Input Channel ---
         siMEM_Mp0_Read_tdata            : in    std_ulogic_vector(511 downto 0);
         siMEM_Mp0_Read_tkeep            : in    std_ulogic_vector( 63 downto 0);
         siMEM_Mp0_Read_tlast            : in    std_ulogic;
         siMEM_Mp0_Read_tvalid           : in    std_ulogic;
         siMEM_Mp0_Read_tready           : out   std_ulogic;
    ------ Stream Write Command --------
         soMEM_Mp0_WrCmd_tdata           : out   std_ulogic_vector( 79 downto 0);
         soMEM_Mp0_WrCmd_tvalid          : out   std_ulogic;
         soMEM_Mp0_WrCmd_tready          : in    std_ulogic;
    ------ Stream Write Status ---------
         siMEM_Mp0_WrSts_tdata           : in    std_ulogic_vector(  7 downto 0);
         siMEM_Mp0_WrSts_tvalid          : in    std_ulogic;
         siMEM_Mp0_WrSts_tready          : out   std_ulogic;
    ------ Stream Data Output Channel --
         soMEM_Mp0_Write_tdata           : out   std_ulogic_vector(511 downto 0);
         soMEM_Mp0_Write_tkeep           : out   std_ulogic_vector( 63 downto 0);
         soMEM_Mp0_Write_tlast           : out   std_ulogic;
         soMEM_Mp0_Write_tvalid          : out   std_ulogic;
         soMEM_Mp0_Write_tready          : in    std_ulogic; 

    --------------------------------------------------------
    -- SHELL / Mem / Mp1 Interface
    --------------------------------------------------------
         moMEM_Mp1_AWID                  : out   std_ulogic_vector(3 downto 0);
         moMEM_Mp1_AWADDR                : out   std_ulogic_vector(32 downto 0);
         moMEM_Mp1_AWLEN                 : out   std_ulogic_vector(7 downto 0);
         moMEM_Mp1_AWSIZE                : out   std_ulogic_vector(2 downto 0);
         moMEM_Mp1_AWBURST               : out   std_ulogic_vector(1 downto 0);
         moMEM_Mp1_AWVALID               : out   std_ulogic;
         moMEM_Mp1_AWREADY               : in    std_ulogic;
         moMEM_Mp1_WDATA                 : out   std_ulogic_vector(511 downto 0);
         moMEM_Mp1_WSTRB                 : out   std_ulogic_vector(63 downto 0);
         moMEM_Mp1_WLAST                 : out   std_ulogic;
         moMEM_Mp1_WVALID                : out   std_ulogic;
         moMEM_Mp1_WREADY                : in    std_ulogic;
         moMEM_Mp1_BID                   : in    std_ulogic_vector(3 downto 0);
         moMEM_Mp1_BRESP                 : in    std_ulogic_vector(1 downto 0);
         moMEM_Mp1_BVALID                : in    std_ulogic;
         moMEM_Mp1_BREADY                : out   std_ulogic;
         moMEM_Mp1_ARID                  : out   std_ulogic_vector(3 downto 0);
         moMEM_Mp1_ARADDR                : out   std_ulogic_vector(32 downto 0);
         moMEM_Mp1_ARLEN                 : out   std_ulogic_vector(7 downto 0);
         moMEM_Mp1_ARSIZE                : out   std_ulogic_vector(2 downto 0);
         moMEM_Mp1_ARBURST               : out   std_ulogic_vector(1 downto 0);
         moMEM_Mp1_ARVALID               : out   std_ulogic;
         moMEM_Mp1_ARREADY               : in    std_ulogic;
         moMEM_Mp1_RID                   : in    std_ulogic_vector(3 downto 0);
         moMEM_Mp1_RDATA                 : in    std_ulogic_vector(511 downto 0);
         moMEM_Mp1_RRESP                 : in    std_ulogic_vector(1 downto 0);
         moMEM_Mp1_RLAST                 : in    std_ulogic;
         moMEM_Mp1_RVALID                : in    std_ulogic;
         moMEM_Mp1_RREADY                : out   std_ulogic;

    ---- [APP_RDROL] -------------------
    -- to be use as ROLE VERSION IDENTIFICATION --
         poSHL_Mmio_RdReg                    : out   std_ulogic_vector( 15 downto 0);

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

  constant cUSE_DEPRECATED_DIRECTIVES       : boolean := false;

  --============================================================================
  --  SIGNAL DECLARATIONS
  --============================================================================  


  ------------------------------------------------------
  -- MPI Interface
  ------------------------------------------------------

  signal sAPP_Fifo_MPIif_din        : std_ulogic_vector(71 downto 0);
  signal sAPP_Fifo_MPIif_full_n     : std_ulogic;
  signal sAPP_Fifo_MPIif_full       : std_ulogic;
  signal sAPP_Fifo_MPIif_write      : std_ulogic;
  signal sAPP_Fifo_MPIdata_din      : std_ulogic_vector(72 downto 0);
  signal sAPP_Fifo_MPIdata_full_n   : std_ulogic;
  signal sAPP_Fifo_MPIdata_full     : std_ulogic;
  signal sAPP_Fifo_MPIdata_write    : std_ulogic;
  signal sFifo_APP_MPIdata_dout     : std_ulogic_vector(72 downto 0);
  signal sFifo_APP_MPIdata_empty_n  : std_ulogic;
  signal sFifo_APP_MPIdata_empty    : std_ulogic;
  signal sFifo_APP_MPIdata_read     : std_ulogic;

  signal sFifo_MPE_MPIif_dout       : std_ulogic_vector(71 downto 0);
  signal sFifo_MPE_MPIif_empty_n    : std_ulogic;
  signal sFifo_MPE_MPIif_empty      : std_ulogic;
  signal sFifo_MPE_MPIif_read       : std_ulogic;
  signal sFifo_MPE_MPIdata_dout     : std_ulogic_vector(72 downto 0);
  signal sFifo_MPE_MPIdata_empty_n  : std_ulogic;
  signal sFifo_MPE_MPIdata_empty    : std_ulogic;
  signal sFifo_MPE_MPIdata_read     : std_ulogic;
  signal sMPE_Fifo_MPIdata_din      : std_ulogic_vector(72 downto 0);
  signal sMPE_Fifo_MPIdata_full_n   : std_ulogic;
  signal sMPE_Fifo_MPIdata_full     : std_ulogic;
  signal sMPE_Fifo_MPIdata_write    : std_ulogic;

  signal sFifo_APP_MPIFeB_dout       : std_ulogic_vector(7 downto 0);
  signal sFifo_APP_MPIFeB_empty_n    : std_ulogic;
  signal sFifo_APP_MPIFeB_empty      : std_ulogic;
  signal sFifo_APP_MPIFeB_read       : std_ulogic;
  signal sMPE_Fifo_MPIFeB_din        : std_ulogic_vector(7 downto 0);
  signal sMPE_Fifo_MPIFeB_full_n     : std_ulogic;
  signal sMPE_Fifo_MPIFeB_full       : std_ulogic;
  signal sMPE_Fifo_MPIFeB_write      : std_ulogic;

  signal active_low_reset  : std_logic;

  signal sAPP_Debug : std_logic_vector(15 downto 0);
  signal sMPE_Debug : std_logic_vector(31 downto 0);


  signal sMetaOutTlastAsVector_Udp : std_logic_vector(0 downto 0);
  signal sMetaInTlastAsVector_Udp  : std_logic_vector(0 downto 0);
  signal sMetaOutTlastAsVector_Tcp : std_logic_vector(0 downto 0);
  signal sMetaInTlastAsVector_Tcp  : std_logic_vector(0 downto 0);
  signal sDataOutTlastAsVector_Udp : std_logic_vector(0 downto 0);
  signal sDataInTlastAsVector_Udp  : std_logic_vector(0 downto 0);

  signal sUdpPostCnt : std_ulogic_vector(9 downto 0);
  signal sTcpPostCnt : std_ulogic_vector(9 downto 0);

  signal sDramAWADDR_64bit : std_ulogic_vector(63 downto 0);
  signal sDramARADDR_64bit : std_ulogic_vector(63 downto 0);

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

           -- rank and size
           piFMC_ROL_rank_V        : in std_logic_vector (31 downto 0);
           piFMC_ROL_rank_V_ap_vld : in std_logic;
           piFMC_ROL_size_V        : in std_logic_vector (31 downto 0);
           piFMC_ROL_size_V_ap_vld : in std_logic;
           --------------------------------------------------------
           -- From SHELL / Udp Data Interfaces
           --------------------------------------------------------
           siNrc_data_TDATA     : in  std_logic_vector( 63 downto 0);
           siNrc_data_TKEEP     : in  std_logic_vector(  7 downto 0);
           siNrc_data_TLAST     : in  std_logic;
           siNrc_data_TVALID    : in  std_logic;
           siNrc_data_TREADY    : out std_logic;
           --------------------------------------------------------
           -- To SHELL / Udp Data Interfaces
           --------------------------------------------------------
           soNrc_data_TDATA     : out std_logic_vector( 63 downto 0);
           soNrc_data_TKEEP     : out std_logic_vector(  7 downto 0);
           soNrc_data_TLAST     : out std_logic;
           soNrc_data_TVALID    : out std_logic;
           soNrc_data_TREADY    : in  std_logic;
           -- NRC Meta and Ports
           siNrc_meta_TDATA     : in std_logic_vector ( 63 downto 0);
           siNrc_meta_TVALID    : in std_logic;
           siNrc_meta_TREADY    : out std_logic;
           siNrc_meta_TKEEP     : in std_logic_vector ( 7 downto 0);
           siNrc_meta_TLAST     : in std_logic_vector (0 downto 0);

           soNrc_meta_TDATA     : out std_logic_vector ( 63 downto 0);
           soNrc_meta_TVALID    : out std_logic;
           soNrc_meta_TREADY    : in std_logic;
           soNrc_meta_TKEEP     : out std_logic_vector ( 7 downto 0);
           soNrc_meta_TLAST     : out std_logic_vector (0 downto 0);

           poROL_NRC_Rx_ports_V        : out std_logic_vector (31 downto 0);
           poROL_NRC_Rx_ports_V_ap_vld : out std_logic
         );
  end component TriangleApplication;


  --FIXME: there are different m_axi definitions in 
  -- /hls/mpi_wrapperv2/mpi_wrapperv2_prj/solution1/impl/vhdl/mpi_wrapperv2.vhd
  -- and
  -- /ip/mpi_wrapperv2/synth/mpi_wrapperv2.vhd
  component mpi_wrapperv2 is
    --generic (
    --          C_M_AXI_BOAPP_DRAM_ADDR_WIDTH : INTEGER := 64;
    --          C_M_AXI_BOAPP_DRAM_ID_WIDTH : INTEGER := 1;
    --          C_M_AXI_BOAPP_DRAM_AWUSER_WIDTH : INTEGER := 1;
    --          C_M_AXI_BOAPP_DRAM_DATA_WIDTH : INTEGER := 512;
    --          C_M_AXI_BOAPP_DRAM_WUSER_WIDTH : INTEGER := 1;
    --          C_M_AXI_BOAPP_DRAM_ARUSER_WIDTH : INTEGER := 1;
    --          C_M_AXI_BOAPP_DRAM_RUSER_WIDTH : INTEGER := 1;
    --          C_M_AXI_BOAPP_DRAM_BUSER_WIDTH : INTEGER := 1;
    --          C_M_AXI_BOAPP_DRAM_TARGET_ADDR : INTEGER := 0;
    --          C_M_AXI_BOAPP_DRAM_USER_VALUE : INTEGER := 0;
    --          C_M_AXI_BOAPP_DRAM_PROT_VALUE : INTEGER := 0;
    --          C_M_AXI_BOAPP_DRAM_CACHE_VALUE : INTEGER := 3 );
    port (
           ap_clk : IN STD_LOGIC;
           ap_rst_n : IN STD_LOGIC;
           ap_start : IN STD_LOGIC;
           ap_done : OUT STD_LOGIC;
           ap_idle : OUT STD_LOGIC;
           ap_ready : OUT STD_LOGIC;
           piFMC_to_ROLE_rank_V : IN STD_LOGIC_VECTOR (31 downto 0);
           piFMC_to_ROLE_rank_V_ap_vld : IN STD_LOGIC;
           piFMC_to_ROLE_size_V : IN STD_LOGIC_VECTOR (31 downto 0);
           piFMC_to_ROLE_size_V_ap_vld : IN STD_LOGIC;
           poMMIO_V : OUT STD_LOGIC_VECTOR (15 downto 0);
           poMMIO_V_ap_vld : OUT STD_LOGIC;
           soMPIif_V_din : OUT STD_LOGIC_VECTOR (71 downto 0);
           soMPIif_V_full_n : IN STD_LOGIC;
           soMPIif_V_write : OUT STD_LOGIC;
           siMPIFeB_V_dout : IN STD_LOGIC_VECTOR (7 downto 0);
           siMPIFeB_V_empty_n : IN STD_LOGIC;
           siMPIFeB_V_read : OUT STD_LOGIC;
           soMPI_data_V_din : OUT STD_LOGIC_VECTOR (72 downto 0);
           soMPI_data_V_full_n : IN STD_LOGIC;
           soMPI_data_V_write : OUT STD_LOGIC;
           siMPI_data_V_dout : IN STD_LOGIC_VECTOR (72 downto 0);
           siMPI_data_V_empty_n : IN STD_LOGIC;
           siMPI_data_V_read : OUT STD_LOGIC;
           --m_axi_boAPP_DRAM_AWVALID : OUT STD_LOGIC;
           --m_axi_boAPP_DRAM_AWREADY : IN STD_LOGIC;
           --m_axi_boAPP_DRAM_AWADDR : OUT STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_ADDR_WIDTH-1 downto 0);
           --m_axi_boAPP_DRAM_AWID : OUT STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_ID_WIDTH-1 downto 0);
           --m_axi_boAPP_DRAM_AWLEN : OUT STD_LOGIC_VECTOR (7 downto 0);
           --m_axi_boAPP_DRAM_AWSIZE : OUT STD_LOGIC_VECTOR (2 downto 0);
           --m_axi_boAPP_DRAM_AWBURST : OUT STD_LOGIC_VECTOR (1 downto 0);
           --m_axi_boAPP_DRAM_AWLOCK : OUT STD_LOGIC_VECTOR (1 downto 0);
           --m_axi_boAPP_DRAM_AWCACHE : OUT STD_LOGIC_VECTOR (3 downto 0);
           --m_axi_boAPP_DRAM_AWPROT : OUT STD_LOGIC_VECTOR (2 downto 0);
           --m_axi_boAPP_DRAM_AWQOS : OUT STD_LOGIC_VECTOR (3 downto 0);
           --m_axi_boAPP_DRAM_AWREGION : OUT STD_LOGIC_VECTOR (3 downto 0);
           --m_axi_boAPP_DRAM_AWUSER : OUT STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_AWUSER_WIDTH-1 downto 0);
           --m_axi_boAPP_DRAM_WVALID : OUT STD_LOGIC;
           --m_axi_boAPP_DRAM_WREADY : IN STD_LOGIC;
           --m_axi_boAPP_DRAM_WDATA : OUT STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_DATA_WIDTH-1 downto 0);
           --m_axi_boAPP_DRAM_WSTRB : OUT STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_DATA_WIDTH/8-1 downto 0);
           --m_axi_boAPP_DRAM_WLAST : OUT STD_LOGIC;
           --m_axi_boAPP_DRAM_WID : OUT STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_ID_WIDTH-1 downto 0);
           --m_axi_boAPP_DRAM_WUSER : OUT STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_WUSER_WIDTH-1 downto 0);
           --m_axi_boAPP_DRAM_ARVALID : OUT STD_LOGIC;
           --m_axi_boAPP_DRAM_ARREADY : IN STD_LOGIC;
           --m_axi_boAPP_DRAM_ARADDR : OUT STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_ADDR_WIDTH-1 downto 0);
           --m_axi_boAPP_DRAM_ARID : OUT STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_ID_WIDTH-1 downto 0);
           --m_axi_boAPP_DRAM_ARLEN : OUT STD_LOGIC_VECTOR (7 downto 0);
           --m_axi_boAPP_DRAM_ARSIZE : OUT STD_LOGIC_VECTOR (2 downto 0);
           --m_axi_boAPP_DRAM_ARBURST : OUT STD_LOGIC_VECTOR (1 downto 0);
           --m_axi_boAPP_DRAM_ARLOCK : OUT STD_LOGIC_VECTOR (1 downto 0);
           --m_axi_boAPP_DRAM_ARCACHE : OUT STD_LOGIC_VECTOR (3 downto 0);
           --m_axi_boAPP_DRAM_ARPROT : OUT STD_LOGIC_VECTOR (2 downto 0);
           --m_axi_boAPP_DRAM_ARQOS : OUT STD_LOGIC_VECTOR (3 downto 0);
           --m_axi_boAPP_DRAM_ARREGION : OUT STD_LOGIC_VECTOR (3 downto 0);
           --m_axi_boAPP_DRAM_ARUSER : OUT STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_ARUSER_WIDTH-1 downto 0);
           --m_axi_boAPP_DRAM_RVALID : IN STD_LOGIC;
           --m_axi_boAPP_DRAM_RREADY : OUT STD_LOGIC;
           --m_axi_boAPP_DRAM_RDATA : IN STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_DATA_WIDTH-1 downto 0);
           --m_axi_boAPP_DRAM_RLAST : IN STD_LOGIC;
           --m_axi_boAPP_DRAM_RID : IN STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_ID_WIDTH-1 downto 0);
           --m_axi_boAPP_DRAM_RUSER : IN STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_RUSER_WIDTH-1 downto 0);
           --m_axi_boAPP_DRAM_RRESP : IN STD_LOGIC_VECTOR (1 downto 0);
           --m_axi_boAPP_DRAM_BVALID : IN STD_LOGIC;
           --m_axi_boAPP_DRAM_BREADY : OUT STD_LOGIC;
           --m_axi_boAPP_DRAM_BRESP : IN STD_LOGIC_VECTOR (1 downto 0);
           --m_axi_boAPP_DRAM_BID : IN STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_ID_WIDTH-1 downto 0);
           --m_axi_boAPP_DRAM_BUSER : IN STD_LOGIC_VECTOR (C_M_AXI_BOAPP_DRAM_BUSER_WIDTH-1 downto 0) );
           m_axi_boAPP_DRAM_AWADDR : OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
           m_axi_boAPP_DRAM_AWLEN : OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
           m_axi_boAPP_DRAM_AWSIZE : OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
           m_axi_boAPP_DRAM_AWBURST : OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
           m_axi_boAPP_DRAM_AWLOCK : OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
           m_axi_boAPP_DRAM_AWREGION : OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
           m_axi_boAPP_DRAM_AWCACHE : OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
           m_axi_boAPP_DRAM_AWPROT : OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
           m_axi_boAPP_DRAM_AWQOS : OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
           m_axi_boAPP_DRAM_AWVALID : OUT STD_LOGIC;
           m_axi_boAPP_DRAM_AWREADY : IN STD_LOGIC;
           m_axi_boAPP_DRAM_WDATA : OUT STD_LOGIC_VECTOR(511 DOWNTO 0);
           m_axi_boAPP_DRAM_WSTRB : OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
           m_axi_boAPP_DRAM_WLAST : OUT STD_LOGIC;
           m_axi_boAPP_DRAM_WVALID : OUT STD_LOGIC;
           m_axi_boAPP_DRAM_WREADY : IN STD_LOGIC;
           m_axi_boAPP_DRAM_BRESP : IN STD_LOGIC_VECTOR(1 DOWNTO 0);
           m_axi_boAPP_DRAM_BVALID : IN STD_LOGIC;
           m_axi_boAPP_DRAM_BREADY : OUT STD_LOGIC;
           m_axi_boAPP_DRAM_ARADDR : OUT STD_LOGIC_VECTOR(63 DOWNTO 0);
           m_axi_boAPP_DRAM_ARLEN : OUT STD_LOGIC_VECTOR(7 DOWNTO 0);
           m_axi_boAPP_DRAM_ARSIZE : OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
           m_axi_boAPP_DRAM_ARBURST : OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
           m_axi_boAPP_DRAM_ARLOCK : OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
           m_axi_boAPP_DRAM_ARREGION : OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
           m_axi_boAPP_DRAM_ARCACHE : OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
           m_axi_boAPP_DRAM_ARPROT : OUT STD_LOGIC_VECTOR(2 DOWNTO 0);
           m_axi_boAPP_DRAM_ARQOS : OUT STD_LOGIC_VECTOR(3 DOWNTO 0);
           m_axi_boAPP_DRAM_ARVALID : OUT STD_LOGIC;
           m_axi_boAPP_DRAM_ARREADY : IN STD_LOGIC;
           m_axi_boAPP_DRAM_RDATA : IN STD_LOGIC_VECTOR(511 DOWNTO 0);
           m_axi_boAPP_DRAM_RRESP : IN STD_LOGIC_VECTOR(1 DOWNTO 0);
           m_axi_boAPP_DRAM_RLAST : IN STD_LOGIC;
           m_axi_boAPP_DRAM_RVALID : IN STD_LOGIC;
           m_axi_boAPP_DRAM_RREADY : OUT STD_LOGIC;
    boFdram_V : IN STD_LOGIC_VECTOR (63 downto 0)
    -- ZRLMPI_COMPONENT_INSERT below
   --
         );
  end component;


  component MessagePassingEngine is
    port (
           siTcp_data_TDATA : IN STD_LOGIC_VECTOR (63 downto 0);
           siTcp_data_TKEEP : IN STD_LOGIC_VECTOR (7 downto 0);
           siTcp_data_TLAST : IN STD_LOGIC_VECTOR (0 downto 0);
           siTcp_meta_TDATA : IN STD_LOGIC_VECTOR (63 downto 0);
           siTcp_meta_TKEEP : IN STD_LOGIC_VECTOR (7 downto 0);
           siTcp_meta_TLAST : IN STD_LOGIC_VECTOR (0 downto 0);
           soTcp_data_TDATA : OUT STD_LOGIC_VECTOR (63 downto 0);
           soTcp_data_TKEEP : OUT STD_LOGIC_VECTOR (7 downto 0);
           soTcp_data_TLAST : OUT STD_LOGIC_VECTOR (0 downto 0);
           soTcp_meta_TDATA : OUT STD_LOGIC_VECTOR (63 downto 0);
           soTcp_meta_TKEEP : OUT STD_LOGIC_VECTOR (7 downto 0);
           soTcp_meta_TLAST : OUT STD_LOGIC_VECTOR (0 downto 0);
           poROL_NRC_Rx_ports_V : OUT STD_LOGIC_VECTOR (31 downto 0);
           piFMC_rank_V : IN STD_LOGIC_VECTOR (31 downto 0);
           -- poMMIO_V : OUT STD_LOGIC_VECTOR (31 downto 0);
           siMPIif_V_dout : IN STD_LOGIC_VECTOR (71 downto 0);
           siMPIif_V_empty_n : IN STD_LOGIC;
           siMPIif_V_read : OUT STD_LOGIC;
           soMPIFeB_V_din : OUT STD_LOGIC_VECTOR (7 downto 0);
           soMPIFeB_V_full_n : IN STD_LOGIC;
           soMPIFeB_V_write : OUT STD_LOGIC;
           siMPI_data_V_dout : IN STD_LOGIC_VECTOR (72 downto 0);
           siMPI_data_V_empty_n : IN STD_LOGIC;
           siMPI_data_V_read : OUT STD_LOGIC;
           soMPI_data_V_din : OUT STD_LOGIC_VECTOR (72 downto 0);
           soMPI_data_V_full_n : IN STD_LOGIC;
           soMPI_data_V_write : OUT STD_LOGIC;
           ap_clk : IN STD_LOGIC;
           ap_rst_n : IN STD_LOGIC;
           poROL_NRC_Rx_ports_V_ap_vld : OUT STD_LOGIC;
           -- poMMIO_V_ap_vld : OUT STD_LOGIC;
           soTcp_meta_TVALID : OUT STD_LOGIC;
           soTcp_meta_TREADY : IN STD_LOGIC;
           soTcp_data_TVALID : OUT STD_LOGIC;
           soTcp_data_TREADY : IN STD_LOGIC;
           siTcp_data_TVALID : IN STD_LOGIC;
           siTcp_data_TREADY : OUT STD_LOGIC;
           siTcp_meta_TVALID : IN STD_LOGIC;
           siTcp_meta_TREADY : OUT STD_LOGIC;
           piFMC_rank_V_ap_vld : IN STD_LOGIC );
  end component;


  component FifoMpiData is
    port (
           clk    : in std_logic;
           srst   : in std_logic;
           din    : in std_logic_vector(72 downto 0);
           full   : out std_logic;
           wr_en  : in std_logic;
           dout   : out std_logic_vector(72 downto 0);
           empty  : out std_logic;
           rd_en  : in std_logic );
  end component;

  component FifoMpiInfo is
    port (
           clk    : in std_logic;
           srst   : in std_logic;
           din    : in std_logic_vector(71 downto 0);
           full   : out std_logic;
           wr_en  : in std_logic;
           dout   : out std_logic_vector(71 downto 0);
           empty  : out std_logic;
           rd_en  : in std_logic );
  end component;

  component FifoMpiFeedback is
    port (
           clk    : in std_logic;
           srst   : in std_logic;
           din    : in std_logic_vector(7 downto 0);
           full   : out std_logic;
           wr_en  : in std_logic;
           dout   : out std_logic_vector(7 downto 0);
           empty  : out std_logic;
           rd_en  : in std_logic );
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

  -- to be use as ROLE VERSION IDENTIFICATION --
  poSHL_Mmio_RdReg <= x"C002";


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

  active_low_reset <= (not piMMIO_Ly7_Rst) and (piMMIO_Ly7_En);

  sAPP_Fifo_MPIif_full_n <=  not sAPP_Fifo_MPIif_full;
  sAPP_Fifo_MPIdata_full_n <= not sAPP_Fifo_MPIdata_full;
  sFifo_APP_MPIdata_empty_n <= not sFifo_APP_MPIdata_empty;

  sMPE_Fifo_MPIFeB_full_n <= not sMPE_Fifo_MPIFeB_full;
  sFifo_APP_MPIFeB_empty_n <= not sFifo_APP_MPIFeB_empty;

  moMEM_Mp1_AWADDR <= sDramAWADDR_64bit(32 downto 0);
  moMEM_Mp1_ARADDR <=  sDramARADDR_64bit(32 downto 0);

  MPI_APP: mpi_wrapperv2
  port map (
             ap_clk                       => piSHL_156_25Clk ,
             ap_rst_n                     => active_low_reset,
         --ap_rst                       => piMMIO_Ly7_Rst,
             ap_start                     => piMMIO_Ly7_En,
             piFMC_to_ROLE_rank_V         => piFMC_ROLE_rank,
             piFMC_to_ROLE_rank_V_ap_vld  => '1',
             piFMC_to_ROLE_size_V         => piFMC_ROLE_size,
             piFMC_to_ROLE_size_V_ap_vld  => '1',
             poMMIO_V                     => sAPP_Debug,
             soMPIif_V_din                => sAPP_Fifo_MPIif_din       ,
             soMPIif_V_full_n             => sAPP_Fifo_MPIif_full_n    ,
             soMPIif_V_write              => sAPP_Fifo_MPIif_write     ,
             siMPIFeB_V_dout              => sFifo_APP_MPIFeB_dout     ,
             siMPIFeB_V_empty_n           => sFifo_APP_MPIFeB_empty_n  ,
             siMPIFeB_V_read              => sFifo_APP_MPIFeB_read     ,
             soMPI_data_V_din             => sAPP_Fifo_MPIdata_din     ,
             soMPI_data_V_full_n          => sAPP_Fifo_MPIdata_full_n  ,
             soMPI_data_V_write           => sAPP_Fifo_MPIdata_write   ,
             siMPI_data_V_dout            => sFifo_APP_MPIdata_dout    ,
             siMPI_data_V_empty_n         => sFifo_APP_MPIdata_empty_n ,
             siMPI_data_V_read            => sFifo_APP_MPIdata_read    ,
         -- m_axi_boAPP_DRAM_AWID        => moMEM_Mp1_AWID      ,
             m_axi_boAPP_DRAM_AWADDR      => sDramAWADDR_64bit   ,
             m_axi_boAPP_DRAM_AWLEN       => moMEM_Mp1_AWLEN     ,
             m_axi_boAPP_DRAM_AWSIZE      => moMEM_Mp1_AWSIZE    ,
             m_axi_boAPP_DRAM_AWBURST     => moMEM_Mp1_AWBURST   ,
         --m_axi_boAPP_DRAM_AWLOCK, AWREGION, AWCACHE, AWPROT, AWQOS
             m_axi_boAPP_DRAM_AWVALID     => moMEM_Mp1_AWVALID   ,
             m_axi_boAPP_DRAM_AWREADY     => moMEM_Mp1_AWREADY   ,
             m_axi_boAPP_DRAM_WDATA       => moMEM_Mp1_WDATA     ,
             m_axi_boAPP_DRAM_WSTRB       => moMEM_Mp1_WSTRB     ,
             m_axi_boAPP_DRAM_WLAST       => moMEM_Mp1_WLAST     ,
             m_axi_boAPP_DRAM_WVALID      => moMEM_Mp1_WVALID    ,
             m_axi_boAPP_DRAM_WREADY      => moMEM_Mp1_WREADY    ,
         --m_axi_boAPP_DRAM_BID         => moMEM_Mp1_BID       ,
             m_axi_boAPP_DRAM_BRESP       => moMEM_Mp1_BRESP     ,
             m_axi_boAPP_DRAM_BVALID      => moMEM_Mp1_BVALID    ,
             m_axi_boAPP_DRAM_BREADY      => moMEM_Mp1_BREADY    ,
         --m_axi_boAPP_DRAM_ARID        => moMEM_Mp1_ARID      ,
             m_axi_boAPP_DRAM_ARADDR      => sDramARADDR_64bit   ,
             m_axi_boAPP_DRAM_ARLEN       => moMEM_Mp1_ARLEN     ,
             m_axi_boAPP_DRAM_ARSIZE      => moMEM_Mp1_ARSIZE    ,
             m_axi_boAPP_DRAM_ARBURST     => moMEM_Mp1_ARBURST   ,
         --ARLOCK, ARREGION, ARCACHE,A ARROT, ARQOS
             m_axi_boAPP_DRAM_ARVALID     => moMEM_Mp1_ARVALID   ,
             m_axi_boAPP_DRAM_ARREADY     => moMEM_Mp1_ARREADY   ,
         -- m_axi_boAPP_DRAM_RID         => moMEM_Mp1_RID       ,
             m_axi_boAPP_DRAM_RDATA       => moMEM_Mp1_RDATA     ,
             m_axi_boAPP_DRAM_RRESP       => moMEM_Mp1_RRESP     ,
             m_axi_boAPP_DRAM_RLAST       => moMEM_Mp1_RLAST     ,
             m_axi_boAPP_DRAM_RVALID      => moMEM_Mp1_RVALID    ,
             m_axi_boAPP_DRAM_RREADY      => moMEM_Mp1_RREADY    ,
             boFdram_V                    => x"0000000000000000" 
             -- ZRLMPI_PORT_MAP_INSERT below
             --
           );

  FIFO_IF_APP_MPE: FifoMpiInfo
  port map (
             clk     => piSHL_156_25Clk,
             srst    => piMMIO_Ly7_Rst,
             din     => sAPP_Fifo_MPIif_din    ,
             full    => sAPP_Fifo_MPIif_full   ,
             wr_en   => sAPP_Fifo_MPIif_write  ,
             dout    => sFifo_MPE_MPIif_dout   ,
             empty   => sFifo_MPE_MPIif_empty  ,
             rd_en   => sFifo_MPE_MPIif_read    
           );

  FIFO_DATA_APP_MPE: FifoMpiData
  port map (
             clk     => piSHL_156_25Clk,
             srst    => piMMIO_Ly7_Rst,
             din     => sAPP_Fifo_MPIdata_din    ,
             full    => sAPP_Fifo_MPIdata_full   ,
             wr_en   => sAPP_Fifo_MPIdata_write  ,
             dout    => sFifo_MPE_MPIdata_dout   ,
             empty   => sFifo_MPE_MPIdata_empty  ,
             rd_en   => sFifo_MPE_MPIdata_read    
           );

  FIFO_DATA_MPE_APP: FifoMpiData
  port map (
             clk     => piSHL_156_25Clk,
             srst    => piMMIO_Ly7_Rst,
             din     => sMPE_Fifo_MPIdata_din     ,
             full    => sMPE_Fifo_MPIdata_full    ,
             wr_en   => sMPE_Fifo_MPIdata_write   ,
             dout    => sFifo_APP_MPIdata_dout    ,
             empty   => sFifo_APP_MPIdata_empty   ,
             rd_en   => sFifo_APP_MPIdata_read    
           );

  FIFO_FEB_MPE_APP: FifoMpiFeedback
  port map (
             clk     => piSHL_156_25Clk,
             srst    => piMMIO_Ly7_Rst,
             din     => sMPE_Fifo_MPIFeB_din     ,
             full    => sMPE_Fifo_MPIFeB_full    ,
             wr_en   => sMPE_Fifo_MPIFeB_write   ,
             dout    => sFifo_APP_MPIFeB_dout    ,
             empty   => sFifo_APP_MPIFeB_empty   ,
             rd_en   => sFifo_APP_MPIFeB_read    
           );

  sFifo_MPE_MPIif_empty_n <=  not sFifo_MPE_MPIif_empty;
  sFifo_MPE_MPIdata_empty_n <= not sFifo_MPE_MPIdata_empty;
  sMPE_Fifo_MPIdata_full_n <= not sMPE_Fifo_MPIdata_full;

  MPE: MessagePassingEngine
  port map (
             ap_clk              => piSHL_156_25Clk,
             ap_rst_n            => active_low_reset, 
        --ap_start            => piMMIO_Ly7_En,
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
             soTcp_meta_TLAST    =>  sMetaOutTlastAsVector_Udp  ,
             poROL_NRC_Rx_ports_V => poROL_Nrc_Udp_Rx_ports,
             piFMC_rank_V        =>  piFMC_ROLE_rank,
             piFMC_rank_V_ap_vld =>  '1',
        -- poMMIO_V            =>  sMPE_Debug,
             siMPIif_V_dout       => sFifo_MPE_MPIif_dout      ,
             siMPIif_V_empty_n    => sFifo_MPE_MPIif_empty_n   ,
             siMPIif_V_read       => sFifo_MPE_MPIif_read      ,
             soMPIFeB_V_din       => sMPE_Fifo_MPIFeB_din     ,
             soMPIFeB_V_full_n    => sMPE_Fifo_MPIFeB_full_n  ,
             soMPIFeB_V_write     => sMPE_Fifo_MPIFeB_write   ,
             siMPI_data_V_dout    => sFifo_MPE_MPIdata_dout    ,
             siMPI_data_V_empty_n => sFifo_MPE_MPIdata_empty_n ,
             siMPI_data_V_read    => sFifo_MPE_MPIdata_read    ,
             soMPI_data_V_din     => sMPE_Fifo_MPIdata_din     ,
             soMPI_data_V_full_n  => sMPE_Fifo_MPIdata_full_n  ,
             soMPI_data_V_write   => sMPE_Fifo_MPIdata_write   
           );

  --debug swith 
  --poSHL_Mmio_RdReg <= sAPP_Debug  when (unsigned(piSHL_Mmio_WrReg) = 0) else 
  --                    sMPE_Debug(15 downto 0) when (unsigned(piSHL_Mmio_WrReg) = 1) else 
  --                    sMPE_Debug(31 downto 16);

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
             --ap_rst_n                    => (not piMMIO_Ly7_Rst),
             ap_rst_n            => active_low_reset,

             piFMC_ROL_rank_V         => piFMC_ROLE_rank,
             piFMC_ROL_rank_V_ap_vld  => '1',
             piFMC_ROL_size_V         => piFMC_ROLE_size,
             piFMC_ROL_size_V_ap_vld  => '1',
             --------------------------------------------------------
             -- From SHELL / Udp Data Interfaces
             --------------------------------------------------------
             siNrc_data_TDATA     => siNRC_Tcp_Data_tdata,
             siNrc_data_TKEEP     => siNRC_Tcp_Data_tkeep,
             siNrc_data_TLAST     => siNRC_Tcp_Data_tlast,
             siNrc_data_TVALID    => siNRC_Tcp_Data_tvalid,
             siNrc_data_TREADY    => siNRC_Tcp_Data_tready,
             --------------------------------------------------------
             -- To SHELL / Udp Data Interfaces
             --------------------------------------------------------
             soNrc_data_TDATA     => soNRC_Tcp_Data_tdata,
             soNrc_data_TKEEP     => soNRC_Tcp_Data_tkeep,
             soNrc_data_TLAST     => soNRC_Tcp_Data_tlast,
             soNrc_data_TVALID    => soNRC_Tcp_Data_tvalid,
             soNrc_data_TREADY    => soNRC_Tcp_Data_tready, 

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
    --  1st Memory Port dummy connections
    --################################################################################
  soMEM_Mp0_RdCmd_tdata   <= (others => '0');
  soMEM_Mp0_RdCmd_tvalid  <= '0';
  siMEM_Mp0_RdSts_tready  <= '0';
  siMEM_Mp0_Read_tready   <= '0';
  soMEM_Mp0_WrCmd_tdata   <= (others => '0');
  soMEM_Mp0_WrCmd_tvalid  <= '0';
  siMEM_Mp0_WrSts_tready  <= '0';
  soMEM_Mp0_Write_tdata   <= (others => '0');
  soMEM_Mp0_Write_tkeep   <= (others => '0');
  soMEM_Mp0_Write_tlast   <= '0';
  soMEM_Mp0_Write_tvalid  <= '0';


end architecture Flash;

