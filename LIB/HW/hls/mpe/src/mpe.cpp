
#include "mpe.hpp"
#include <stdint.h>


using namespace hls;

// TODO: removed for now, FIXME: add to Management Companion Core in Middleware
//ap_uint<32> localMRT[MAX_MRT_SIZE];
//ap_uint<32> config[NUMBER_CONFIG_WORDS];
//ap_uint<32> status[NUMBER_STATUS_WORDS];

//sendState fsmSendState = WRITE_STANDBY;
stream<Axis<32> > sFifoDataTX("sFifoDataTX");
//static stream<IPMeta> sFifoIPdstTX("sFifoIPdstTX");
int enqueueCnt = 0;
bool tlast_occured_TX = false;
uint32_t expected_recv_count = 0;
uint32_t expected_send_count = 0;
uint32_t recv_total_cnt = 0;
uint32_t send_total_cnt = 0;

uint32_t exp_recv_count_enqueue = 0;
uint32_t enqueue_recv_total_cnt = 0;
NodeId current_data_src_node_id = 0xFFF;
NrcPort current_data_src_port = 0x0;
NrcPort current_data_dst_port = 0x0;
bool expect_more_data = false;

//receiveState fsmReceiveState = READ_STANDBY;
//stream<Axis<8> > sFifoDataRX("sFifoDataRX");
stream<uint32_t> sFifoDataRX("sFifoDataRX");
stream<uint32_t> rx_overflow_fifo("rx_overflow_fifo");

mpeState fsmMpeState = IDLE;
deqState sendDeqFsm = DEQ_IDLE;
deqState recvDeqFsm = DEQ_IDLE;
MPI_Interface currentInfo = MPI_Interface();
//packetType currentPacketType = ERROR;
mpiType currentDataType = MPI_INT;
int handshakeLinesCnt = 0;

//stream<ap_uint<MPIF_HEADER_LENGTH*8> > sFifoHeaderCache("sFifoHeaderCache");
stream<ap_uint<32> > sFifoHeaderCache("sFifoHeaderCache");
bool checked_cache = false;
uint32_t expected_src_rank = 0;
uint16_t current_cache_data_cnt = 0;

//bool tablesInitialized = false;

//ap_uint<32> read_timeout_cnt = 0;

/*
ap_uint<32> littleEndianToInteger(ap_uint<8> *buffer, int lsb)
{
  ap_uint<32> tmp = 0;
  tmp  = ((ap_uint<32>) buffer[lsb + 3]); 
  tmp |= ((ap_uint<32>) buffer[lsb + 2]) << 8; 
  tmp |= ((ap_uint<32>) buffer[lsb + 1]) << 16; 
  tmp |= ((ap_uint<32>) buffer[lsb + 0]) << 24; 

  //printf("LSB: %#1x, return: %#04x\n",(uint8_t) buffer[lsb + 3], (uint32_t) tmp);

  return tmp;
}

void integerToLittleEndian(ap_uint<32> n, ap_uint<8> *bytes)
{
  bytes[0] = (n >> 24) & 0xFF;
  bytes[1] = (n >> 16) & 0xFF;
  bytes[2] = (n >> 8) & 0xFF;
  bytes[3] = n & 0xFF;
}
*/

void convertAxisToNtsWidth(stream<Axis<32> > &small, NetworkWord &out)
{
#pragma HLS inline

  out.tdata = 0;
  out.tlast = 0;
  out.tkeep = 0;

  for(int i = 0; i < 2; i++)
  //for(int i = 7; i >=0 ; i--)
  {
//#pragma HLS unroll
    Axis<32> tmpl = Axis<32>();
    //if(!small.empty())
    if(small.read_nb(tmpl))
    {
     // Axis<8> tmp = small.read();
      //printf("read from fifo: %#02x\n", (unsigned int) tmp.tdata);
      out.tdata |= ((ap_uint<64>) (tmpl.tdata) )<< (i*32);
      out.tkeep |= (ap_uint<8>) 0x0F << i*4;
      //TODO? NO latch, because last read from small is still last read
      if(out.tlast == 0)
      {
        out.tlast = tmpl.tlast;
      }

    } else {
      printf("\ttried to read empty small stream!\n");
      //now, we set tlast just to be sure...TODO?
      out.tlast = 1;
      break;
    }
  }

}

void convertAxisToMpiWidth(NetworkWord big, stream<Axis<32> > &out)
{
//#pragma HLS inline

  int positionOfTlast = 2;
  ap_uint<8> tkeep = big.tkeep;
  for(int i = 0; i<2; i++) //no reverse order!
  {
//#pragma HLS unroll
    //tkeep = (tkeep >> 1);
    tkeep = (tkeep >> 4);
    //printf("\tshifted tkeep %#02x\n",(uint8_t) tkeep);
    //if((tkeep & 0x01) == 0)
    if((tkeep & 0x0F) == 0)
    {
      positionOfTlast = i;
      break;
    }
  }

  //for(int i = 7; i >=0 ; i--)
  for(int i = 0; i < 2; i++)
  {
//#pragma HLS unroll
    //out.full? 
    Axis<32> tmp = Axis<32>();
    if(i == positionOfTlast)
    //if(i == 0)
    {
      //only possible position...
      tmp.tlast = big.tlast;
      printf("tlast set.\n");
    } else {
      tmp.tlast = 0;
    }
    tmp.tdata = (ap_uint<32>) (big.tdata >> i*32);
    //tmp.tdata = (ap_uint<8>) (big.tdata >> (7-i)*8);
    tmp.tkeep = (ap_uint<4>) (big.tkeep >> i*4);
    //tmp.tkeep = (ap_uint<1>) (big.tkeep >> (7-i));

    if(tmp.tkeep == 0)
    {
      continue;
    }

    out.write(tmp); 
  }

}

//returns: 0 ok, 1 not ok
uint8_t checkHeader(ap_uint<8> bytes[MPIF_HEADER_LENGTH], MPI_Header &header, NetworkMeta &metaSrc,
                     packetType expected_type, mpiCall expected_call, bool skip_meta, uint32_t expected_src_rank)
{
#pragma HLS inline

        int ret = bytesToHeader(bytes, header);
        bool unexpected_header = false;

        if(ret != 0)
        {
          printf("invalid header.\n");
          //fsmMpeState = RECV_DATA_ERROR;
          //fsmReceiveState = READ_ERROR; //to clear streams?
         //status[MPE_STATUS_READ_ERROR_CNT]++;
          //status[MPE_STATUS_LAST_READ_ERROR] = RX_INVALID_HEADER;
          //status[MPE_STATUS_LAST_READ_ERROR] = ret;
         //status[MPE_STATUS_LAST_READ_ERROR] = 10 - ret;
         //status[MPE_STATUS_READOUT] = 0;
         //status[MPE_STATUS_READOUT + 1] = 0;
         //status[MPE_STATUS_READOUT + 2] = 0;
        //  for(int i = 0; i< 4; i++)
        //  {
        //   //status[MPE_STATUS_READOUT] |= ((ap_uint<32>) bytes[i]) << i*8;
        //  }
        //  for(int i = 0; i< 4; i++)
        //  {
        //   //status[MPE_STATUS_READOUT + 1] |= ((ap_uint<32>) bytes[4 + i]) << i*8;
        //  }
        //  for(int i = 0; i< 4; i++)
        //  {
        //   //status[MPE_STATUS_READOUT + 2] |= ((ap_uint<32>) bytes[8 + i]) << i*8;
        //  }
          unexpected_header = true;
        }
        
        else if(!skip_meta && (header.src_rank != metaSrc.src_rank))
        {
          printf("header does not match network meta. header rank %d, got meta %d;\n", (int) header.src_rank, (int) metaSrc.src_rank);
          //fsmMpeState = RECV_DATA_ERROR; //to clear streams?
         //status[MPE_STATUS_READ_ERROR_CNT]++;
         //status[MPE_STATUS_LAST_READ_ERROR] = MPE_RX_IP_MISSMATCH;
          unexpected_header = true;
        }

        else if(header.src_rank != expected_src_rank)
        {
          printf("header does not match rank. expected rank %d, got %d;\n", expected_src_rank, (int) header.src_rank);
          unexpected_header = true;
        }

        else if(header.type != expected_type)
        {
          printf("Header type missmatch! Expected %d, got %d!\n",(int) expected_type, (int) header.type);
          //fsmMpeState = RECV_DATA_ERROR; //to clear streams?
         //status[MPE_STATUS_READ_ERROR_CNT]++;
         //status[MPE_STATUS_LAST_READ_ERROR] = MPE_RX_WRONG_DATA_TYPE;
          unexpected_header = true;
        }

        //check data type 
        //if((currentDataType == MPI_INT && header.call != MPI_RECV_INT) || (currentDataType == MPI_FLOAT && header.call != MPI_RECV_FLOAT))
        else if(header.call != expected_call)
        {
          printf("receiver expects different data type: %d.\n", (int) header.call);
          //fsmMpeState = RECV_DATA_ERROR; //to clear streams?
         //status[MPE_STATUS_READ_ERROR_CNT]++;
         //status[MPE_STATUS_LAST_READ_ERROR] = MPE_RX_WRONG_DST_RANK;
          unexpected_header = true;
        }
        

        if(unexpected_header)
        {
          //put to cache
          if(current_cache_data_cnt >= HEADER_CACHE_LENTH -1)
          {
            printf("header cache is full, drop the header...\n");
          } else {
            //ap_uint<MPIF_HEADER_LENGTH*8> new_cache_line = 0x0;
            printf("wrong header; put to cache: \n\t");
            for(int j = 0; j<MPIF_HEADER_LENGTH/4; j++)
            {
              //#pragma HLS unroll
              //new_cache_line |= ((ap_uint<MPIF_HEADER_LENGTH*8>) bytes[j]) << (31-j);
              //new_cache_line |= ap_uint<MPIF_HEADER_LENGTH*8>(bytes[j]) << (31-j);
              //blocking...
              //sFifoHeaderCache.write(bytes[j]);
              ap_uint<32> cache_tmp = 0x0;
              for(int k = 0; k<4; k++)
              {
                //cache_tmp |= (ap_uint<32>) (bytes[j*4+k] << (3-k)*8 );
                cache_tmp |= ((ap_uint<32>) bytes[j*4+k]) << (3-k)*8;
                printf("%02x", (int) bytes[j*4+k]);
              }
              //blocking...
              sFifoHeaderCache.write(cache_tmp);
            }
            printf("\n");
            //blocking...
            //sFifoHeaderCache.write(new_cache_line);
            current_cache_data_cnt++;
            //ap_uint<MPIF_HEADER_LENGTH*8> cache_line = sFifoHeaderCache.read();
          //for(int k = 0; k<32; k++)
          //{
          //  printf(" %02X", (uint8_t) (cache_line >> 31-k));
          //}
          //printf("\n");
          //sFifoHeaderCache.write(cache_line);
          }
          return 1;
        }
        return 0;
}




void mpe_main(
    // ----- link to FMC -----
    // TODO: removed for now, FIXME: add to Management Companion Core in Middleware
    //ap_uint<32> ctrlLink[MAX_MRT_SIZE + NUMBER_CONFIG_WORDS + NUMBER_STATUS_WORDS],

    // ----- NRC Interface -----
    stream<NetworkWord>            &siTcp_data,
    stream<NetworkMetaStream>      &siTcp_meta,
    stream<NetworkWord>            &soTcp_data,
    stream<NetworkMetaStream>      &soTcp_meta,
    ap_uint<32>                   *po_rx_ports,
    
    ap_uint<32> *own_rank,

    // ----- Memory -----
    //ap_uint<8> *MEM, TODO: maybe later
    // ----- To Coaxium ------
    ap_uint<32> *MMIO_out,

    // ----- MPI_Interface -----
    stream<MPI_Interface> &siMPIif,
    //stream<MPI_Interface> &soMPIif,
    stream<Axis<32> > &siMPI_data,
    stream<Axis<32> > &soMPI_data
    )
{
#pragma HLS INTERFACE axis register both port=siTcp_data
#pragma HLS INTERFACE axis register both port=siTcp_meta
#pragma HLS INTERFACE axis register both port=soTcp_data
#pragma HLS INTERFACE axis register both port=soTcp_meta
#pragma HLS INTERFACE ap_ovld register port=po_rx_ports name=poROL_NRC_Rx_ports
// TODO: removed for now, FIXME: add to Management Companion Core in Middleware
//#pragma HLS INTERFACE s_axilite depth=512 port=ctrlLink bundle=piSMC_MPE_ctrlLink_AXI
//#pragma HLS INTERFACE s_axilite port=return bundle=piSMC_MPE_ctrlLink_AXI

#pragma HLS INTERFACE ap_vld register port=own_rank name=piFMC_rank

#pragma HLS INTERFACE ap_ovld register port=MMIO_out name=poMMIO

#pragma HLS INTERFACE ap_fifo port=siMPIif
#pragma HLS DATA_PACK     variable=siMPIif
#pragma HLS INTERFACE ap_fifo port=siMPI_data
#pragma HLS DATA_PACK     variable=siMPI_data
#pragma HLS INTERFACE ap_fifo port=soMPI_data
#pragma HLS DATA_PACK     variable=soMPI_data

//#pragma HLS RESOURCE variable=localMRT core=RAM_1P_BRAM //maybe better to decide automatic?


//===========================================================
// Core-wide pragmas

#pragma HLS DATAFLOW
#pragma HLS STREAM variable=sFifoDataTX depth=512
#pragma HLS STREAM variable=sFifoDataRX depth=512
#pragma HLS STREAM variable=rx_overflow_fifo depth=2
//#pragma HLS STREAM variable=sFifoHeaderCache depth=64
#pragma HLS STREAM variable=sFifoHeaderCache depth=2048 //HEADER_CACHE_LENTH*MPIF_HEADER_LENGTH
#pragma HLS INTERFACE ap_ctrl_none port=return

//===========================================================
// Core-wide variables

  ap_uint<8> bytes[MPIF_HEADER_LENGTH];
  MPI_Header header = MPI_Header(); 
  NetworkMeta metaDst = NetworkMeta();
  NetworkMeta metaSrc = NetworkMeta();
  uint8_t ret;
  bool found_cache = false;
  mpiCall expected_call = 0;
  packetType expected_type = 0;
  //ap_uint<MPIF_HEADER_LENGTH*8> cache_line = 0x0;


//===========================================================
// Reset global variables 

//#pragma HLS reset variable=tablesInitialized
//#pragma HLS reset variable=fsmSendState
//#pragma HLS reset variable=fsmReceiveState
#pragma HLS reset variable=fsmMpeState
#pragma HLS reset variable=sendDeqFsm
#pragma HLS reset variable=recvDeqFsm
//#pragma HLS reset variable=currentPacketType
#pragma HLS reset variable=currentDataType
#pragma HLS reset variable=handshakeLinesCnt

#pragma HLS reset variable=expected_recv_count
#pragma HLS reset variable=recv_total_cnt
#pragma HLS reset variable=expected_send_count
#pragma HLS reset variable=send_total_cnt
#pragma HLS reset variable=checked_cache
#pragma HLS reset variable=expected_src_rank
#pragma HLS reset variable=current_cache_data_cnt

#pragma HLS reset variable=exp_recv_count_enqueue
#pragma HLS reset variable=enqueue_recv_total_cnt
#pragma HLS reset variable=expect_more_data
#pragma HLS reset variable=current_data_src_node_id
#pragma HLS reset variable=current_data_src_port
#pragma HLS reset variable=current_data_dst_port


  *po_rx_ports = 0x1; //currently work only with default ports...


//  if(!tablesInitialized)
//  {
//    //for(int i = 0; i < MAX_MRT_SIZE; i++)
//    //{
//    //  localMRT[i] = 0;
//    //}
//    //for(int i = 0; i < NUMBER_CONFIG_WORDS; i++)
//    //{
//    //  config[i] = 0;
//    //}
//  //  for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
//  //  {
//  //   //status[i] = 0;
//  //  }
//    
//    //variables with constructors 
//    currentInfo = MPI_Interface(); //TODO: better to do this way?
//
//    tablesInitialized = true;
//  }

//
////===============

    uint32_t debug_out = 0;
    //debug_out = fsmReceiveState;
    //debug_out |= ((uint32_t) fsmSendState) << 8;
    debug_out |= ((uint32_t) fsmMpeState) << 16;
    debug_out |= 0xAC000000;

    *MMIO_out = (ap_uint<32>) debug_out;

////===========================================================
////  update//status TODO
//  for(int i = 0; i < NUMBER_STATUS_WORDS; i++)
//  {
//    ctrlLink[NUMBER_CONFIG_WORDS + i] =//status[i];
//  }
//

//===========================================================
// MPE GLOBAL STATE 

    bool word_tlast_occured = false;
    Axis<32> current_read_word = Axis<32>();
    uint8_t cnt = 0;

  switch(fsmMpeState) {
    case IDLE:
      sendDeqFsm = DEQ_IDLE;
      recvDeqFsm = DEQ_DONE;
      checked_cache = false;
      expected_src_rank = 0;
      if ( !siMPIif.empty() ) //TODO: try to fix combinatorial loop
      {
        currentInfo = siMPIif.read();
        switch(currentInfo.mpi_call)
        {
          case MPI_SEND_INT:
            currentDataType = MPI_INT;
            fsmMpeState = START_SEND;
            break;
          case MPI_SEND_FLOAT:
            currentDataType = MPI_FLOAT;
            fsmMpeState = START_SEND;
            break;
          case MPI_RECV_INT:
            currentDataType = MPI_INT;
            //fsmMpeState = START_RECEIVE;
            fsmMpeState = WAIT4REQ;
            expected_src_rank = currentInfo.rank;
            break;
          case MPI_RECV_FLOAT:
            currentDataType = MPI_FLOAT;
            //fsmMpeState = START_RECEIVE;
            fsmMpeState = WAIT4REQ;
            expected_src_rank = currentInfo.rank;
            break;
          case MPI_BARRIER: 
            //TODO not yet implemented 
            break;
        }
      }
      break;
    case START_SEND: 
      if ( !soTcp_meta.full() && !sFifoDataTX.full() )
      {
        header = MPI_Header();
        header.dst_rank = currentInfo.rank;
        header.src_rank = *own_rank;
        header.size = 0;
        header.call = static_cast<mpiCall>((int) currentInfo.mpi_call);
        header.type = SEND_REQUEST;

        expected_src_rank = header.dst_rank;

        headerToBytes(header, bytes);

        //in order not to block the URIF/TRIF core
        metaDst = NetworkMeta(header.dst_rank, ZRLMPI_DEFAULT_PORT, header.src_rank, ZRLMPI_DEFAULT_PORT, 0); //we set tlast
        soTcp_meta.write(NetworkMetaStream(metaDst));

        //write header
        for(int i = 0; i < MPIF_HEADER_LENGTH/4; i++)
        {
          //Axis<8> tmp = Axis<8>(bytes[i]);
          Axis<32> tmp = Axis<32>();
          tmp.tdata = 0x0;
          tmp.tkeep = 0x0F;
          for(int j = 0; j<4; j++)
          {
            //tmp.tdata |= ((ap_uint<32>) bytes[i*4+j]) << (3-j)*8;
            tmp.tdata |= ((ap_uint<32>) bytes[i*4+j]) << j*8;
          }
          printf("tdata32: %#08x\n",(uint32_t) tmp.tdata);
          tmp.tlast = 0;
          if ( i == (MPIF_HEADER_LENGTH/4) - 1)
          {
            tmp.tlast = 1;
          }
          sFifoDataTX.write(tmp);
          printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        }
        handshakeLinesCnt = (MPIF_HEADER_LENGTH + 7) /8;

        //dequeue
        if( !soTcp_data.full() && !sFifoDataTX.empty() )
        {
          NetworkWord word = NetworkWord();
          convertAxisToNtsWidth(sFifoDataTX, word);
          printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
          soTcp_data.write(word);
          handshakeLinesCnt--;

        }

        fsmMpeState = SEND_REQ;
      }
      break;
    case SEND_REQ:
      //dequeue
      if( !soTcp_data.full() && !sFifoDataTX.empty() )
      {
        NetworkWord word = NetworkWord();
        convertAxisToNtsWidth(sFifoDataTX, word);
        printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        soTcp_data.write(word);
        handshakeLinesCnt--;

      }
      //if( handshakeLinesCnt <= 0)
      if( handshakeLinesCnt <= 0 || sFifoDataTX.empty())
      {
        checked_cache = false;
        fsmMpeState = WAIT4CLEAR;
      }
      break;
    case WAIT4CLEAR:
      header = MPI_Header();
      expected_call = MPI_RECV_INT;
      if(currentDataType == MPI_FLOAT)
      {
        expected_call = MPI_RECV_FLOAT;
      }
      expected_type = CLEAR_TO_SEND;
      //check cache first
      found_cache = false;
      if(!checked_cache)
      {
        uint16_t checked_entries = 0;
        uint16_t static_data_cnt = current_cache_data_cnt;
        printf("check %d chache entries\n",static_data_cnt);
        //cache_line = 0x0;
        //while(!sFifoHeaderCache.empty())
        while(checked_entries < static_data_cnt)
        {
          //if(!sFifoHeaderCache.read_nb(cache_line))
          //{
          //  printf("Didn't found header in chache\n");
          //  break;
          //}
          printf("check cache entry: ");
          for(int j = 0; j < MPIF_HEADER_LENGTH/4; j++)
          {
            //bytes[j] = (ap_uint<8>) (cache_line >> (32-j));
            //bytes[j] = sFifoHeaderCache.read();
            ap_uint<32> cache_tmpr = sFifoHeaderCache.read();
            for(int k = 0; k<4; k++)
            {
              bytes[j*4+k] = (uint8_t) (cache_tmpr >> (3-k)*8);
              printf("%02x", (int) bytes[j*4+k]);
            }
          }
          printf("\n");
          current_cache_data_cnt--;
          ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, true, expected_src_rank);
          if(ret == 0)
          {//got CLEAR_TO_SEND 
            printf("Got CLEAR to SEND from the cache\n");
            fsmMpeState = SEND_DATA_START;
            found_cache = true;
            break;
          }
          //else, we continue
          //checkHeader puts it back to the cache
          checked_entries++; 
        }
        //in all cases
        checked_cache = true;
      }
      
      if( !found_cache && !siTcp_data.empty() && !siTcp_meta.empty() )
      {
        //read header
        for(int i = 0; i< (MPIF_HEADER_LENGTH+7)/8; i++)
        {
          NetworkWord tmp = siTcp_data.read();

          /*
          if(tmp.tkeep != 0xFF || tmp.tlast == 1)
          {
            printf("unexpected uncomplete read.\n");
            //TODO
            fsmMpeState = IDLE;
            fsmReceiveState = READ_ERROR; //to clear streams?
           //status[MPE_STATUS_READ_ERROR_CNT]++;
           //status[MPE_STATUS_LAST_READ_ERROR] = RX_INCOMPLETE_HEADER;
            break;
          }*/

          printf("Data read: tkeep %#03x, tdata %#016llx, tlast %d\n",(int) tmp.tkeep, (unsigned long long) tmp.tdata, (int) tmp.tlast);
          for(int j = 0; j<8; j++)
          {
//#pragma HLS unroll
            bytes[i*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
            //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          }
        }

        metaSrc = siTcp_meta.read().tdata;

        ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
        if(ret == 0)
        {//got CLEAR_TO_SEND 
          printf("Got CLEAR to SEND\n");
          fsmMpeState = SEND_DATA_START;
        }
        //else, we continue to wait
      }

      break;
    case SEND_DATA_START:
      if ( !sFifoDataTX.full() && !soTcp_meta.full() )
      {
        header = MPI_Header(); 
        //MPI_Interface info = siMPIif.read();
        MPI_Interface info = currentInfo;
        header.dst_rank = info.rank;
        header.src_rank = *own_rank;
        header.size = info.count;
        header.call = static_cast<mpiCall>((int) info.mpi_call);
        header.type = DATA;

        expected_src_rank = header.dst_rank;

        headerToBytes(header, bytes);

        //write header
        for(int i = 0; i < MPIF_HEADER_LENGTH/4; i++)
        {
          //Axis<8> tmp = Axis<8>(bytes[i]);
          Axis<32> tmp = Axis<32>();
          tmp.tdata = 0x0;
          tmp.tkeep = 0x0F;
          for(int j = 0; j<4; j++)
          {
            tmp.tdata |= ((ap_uint<32>) bytes[i*4+j]) << j*8;
          }
          tmp.tlast = 0; //in this case, always
          sFifoDataTX.write(tmp);
          printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        }

        metaDst = NetworkMeta(header.dst_rank, ZRLMPI_DEFAULT_PORT, header.src_rank, ZRLMPI_DEFAULT_PORT, 0); //we set tlast
        soTcp_meta.write(NetworkMetaStream(metaDst));
        expected_send_count = header.size;
        printf("[MPI_send] expect %d bytes.\n",expected_send_count);
        send_total_cnt = 0;

        tlast_occured_TX = false;
        enqueueCnt = MPIF_HEADER_LENGTH/4;
        fsmMpeState = SEND_DATA_RD;
        //start dequeue FSM
        sendDeqFsm = DEQ_WRITE;
      }
      break;
    case SEND_DATA_RD:
      //enqueue 
      cnt = 0;
      //while( !siMPI_data.empty() && !sFifoDataTX.full() && cnt<=8 && !tlast_occured_TX)
      //if( !siMPI_data.empty() && !sFifoDataTX.full() && !tlast_occured_TX)
      //while( cnt<=8 && !tlast_occured_TX)
      while( cnt<=2 && !tlast_occured_TX)
      {
        //current_read_byte = siMPI_data.read();
        if(!siMPI_data.read_nb(current_read_word))
        {
          break;
        }
        if(send_total_cnt >= (expected_send_count - 1))
        {// to be sure...
          current_read_word.tlast = 1;
        }
        //TODO: ? use "blocking" version!! better matches to MPI_Wrapper...
        sFifoDataTX.write(current_read_word);
        cnt++;
        send_total_cnt++;
        if(current_read_word.tlast == 1)
        {
          tlast_occured_TX = true;
          fsmMpeState = SEND_DATA_WRD;
          printf("tlast Occured.\n");
          printf("MPI read data: %#08x, tkeep: %d, tlast %d\n", (int) current_read_word.tdata, (int) current_read_word.tkeep, (int) current_read_word.tlast);
          //fsmMpeState = SEND_DATA_WRD;
        }
        enqueueCnt++;
      }
      //enqueueCnt += cnt;
      //printf("cnt: %d\n", cnt);

      break;
    case SEND_DATA_WRD:
      //wait for dequeue fsm
      if(sendDeqFsm == DEQ_DONE)
      {
        checked_cache = false;
        fsmMpeState = WAIT4ACK;
        sendDeqFsm = DEQ_IDLE;
      }
      break;
    case WAIT4ACK:
      header = MPI_Header();
      expected_call = MPI_RECV_INT;
      if(currentDataType == MPI_FLOAT)
      {
        expected_call = MPI_RECV_FLOAT;
      }
      expected_type = ACK;
      //check cache first
      found_cache = false;
      if(!checked_cache)
      {
        uint16_t checked_entries = 0;
        uint16_t static_data_cnt = current_cache_data_cnt;
        printf("check %d chache entries\n",static_data_cnt);
        //cache_line = 0x0;
        //while(!sFifoHeaderCache.empty())
        while(checked_entries < static_data_cnt)
        {
          //if(!sFifoHeaderCache.read_nb(cache_line))
          //{
          //  printf("Didn't found header in chache\n");
          //  break;
          //}
          printf("check cache entry: ");
          for(int j = 0; j < MPIF_HEADER_LENGTH/4; j++)
          {
            //bytes[j] = (ap_uint<8>) (cache_line >> (31-j));
            //bytes[j] = sFifoHeaderCache.read();
            //printf("%02x", (int) bytes[j]);
            ap_uint<32> cache_tmpr = sFifoHeaderCache.read();
            for(int k = 0; k<4; k++)
            {
              bytes[j*4+k] = (uint8_t) (cache_tmpr >> (3-k)*8);
              printf("%02x", (int) bytes[j*4+k]);
            }
          }
          printf("\n");
          current_cache_data_cnt--;
          ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, true, expected_src_rank);
          if(ret == 0)
          {//got CLEAR_TO_SEND 
            printf("Got ACK from the cache\n");
            fsmMpeState = IDLE;
            found_cache = true;
            break;
          }
          //else, we continue
          //checkHeader puts it back to the cache
          checked_entries++;
        }
        //in all cases
        checked_cache = true;
      }
      if( !found_cache && !siTcp_data.empty() && !siTcp_meta.empty() )
      {
        //read header
        for(int i = 0; i< (MPIF_HEADER_LENGTH+7)/8; i++)
        {
          NetworkWord tmp = siTcp_data.read();

          /* TODO
             if(tmp.tkeep != 0xFF || tmp.tlast == 1)
             {
             printf("unexpected uncomplete read.\n");
          //TODO
          fsmMpeState = IDLE;
         //status[MPE_STATUS_READ_ERROR_CNT]++;
         //status[MPE_STATUS_LAST_READ_ERROR] = RX_INCOMPLETE_HEADER;
          break;
          }*/

          for(int j = 0; j<8; j++)
          {
//#pragma HLS unroll
            bytes[i*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
            //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          }
        }

        metaSrc = siTcp_meta.read().tdata;
        ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
        if(ret == 0)
        {
          printf("ACK received.\n");
          fsmMpeState = IDLE;
        }
        //else, we wait
      }
      break;
      //case START_RECEIVE: 
      //  break; 
    case WAIT4REQ:
      header = MPI_Header();
      expected_call = MPI_SEND_INT;
      if(currentDataType == MPI_FLOAT)
      {
        expected_call = MPI_SEND_FLOAT;
      }
      expected_type = SEND_REQUEST;
      //check cache first
      found_cache = false;
      if(!checked_cache)
      {
        uint16_t checked_entries = 0;
        uint16_t static_data_cnt = current_cache_data_cnt;
        printf("check %d chache entries\n",static_data_cnt);
        //cache_line = 0x0;
        //while(!sFifoHeaderCache.empty())
        while(checked_entries < static_data_cnt)
        {
          //if(!sFifoHeaderCache.read_nb(cache_line))
          //{
          //  printf("Didn't found header in chache\n");
          //  break;
          //}
          printf("check cache entry: ");
          for(int j = 0; j < MPIF_HEADER_LENGTH/4; j++)
          {
            //bytes[j] = (ap_uint<8>) (cache_line >> (31-j));
            //bytes[j] = sFifoHeaderCache.read();
            //printf("%02x", (int) bytes[j]);
            ap_uint<32> cache_tmpr = sFifoHeaderCache.read();
            for(int k = 0; k<4; k++)
            {
              bytes[j*4+k] = (uint8_t) (cache_tmpr >> (3-k)*8);
              printf("%02x", (int) bytes[j*4+k]);
            }
          }
          printf("\n");
          current_cache_data_cnt--;
          ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, true, expected_src_rank);
          if(ret == 0)
          {//got CLEAR_TO_SEND 
            printf("Got SEND_REQUEST from the cache\n");
            fsmMpeState = ASSEMBLE_CLEAR;
            found_cache = true;
            break;
          }
          //else, we continue
          //checkHeader puts it back to the cache
          checked_entries++;
        }
        //in all cases
        checked_cache = true;
      }
      if( !found_cache && !siTcp_data.empty() && !siTcp_meta.empty() )
      {
        //read header
        for(int i = 0; i< (MPIF_HEADER_LENGTH+7)/8; i++)
        {
          NetworkWord tmp = siTcp_data.read();

          /*
             if(tmp.tkeep != 0xFF || tmp.tlast == 1)
             {
             printf("unexpected uncomplete read.\n");
          //TODO
          fsmMpeState = IDLE;
          fsmReceiveState = READ_ERROR; //to clear streams?
         //status[MPE_STATUS_READ_ERROR_CNT]++;
         //status[MPE_STATUS_LAST_READ_ERROR] = RX_INCOMPLETE_HEADER;
          break;
          }*/

          //TODO 
          //header should have always full lines
          //if(tmp.tkeep != 0xff)
          //{
          //  i--;
          //  continue;
          //}

          for(int j = 0; j<8; j++)
          {
//#pragma HLS unroll
            bytes[i*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
            //bytes[i*8 + 7 -j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          }
        }

        metaSrc = siTcp_meta.read().tdata;
        ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
        if(ret == 0)
        {
          //got SEND_REQUEST 
          printf("Got SEND REQUEST\n");
          fsmMpeState = ASSEMBLE_CLEAR;
        }
        //else, we wait...
      }
      break;
    case ASSEMBLE_CLEAR:
      if(!soTcp_meta.full() && !sFifoDataTX.full() )
      {
        header = MPI_Header(); 
        header.dst_rank = currentInfo.rank;
        header.src_rank = *own_rank;
        header.size = 0;
        header.call = static_cast<mpiCall>((int) currentInfo.mpi_call);
        header.type = CLEAR_TO_SEND;

        expected_src_rank = header.dst_rank;

        headerToBytes(header, bytes);

        //in order not to block the URIF/TRIF core
        metaDst = NetworkMeta(header.dst_rank, ZRLMPI_DEFAULT_PORT, header.src_rank, ZRLMPI_DEFAULT_PORT, 0); //we set tlast
        soTcp_meta.write(NetworkMetaStream(metaDst));

        //write header
        for(int i = 0; i < MPIF_HEADER_LENGTH/4; i++)
        {
          //Axis<8> tmp = Axis<8>(bytes[i]);
          Axis<32> tmp = Axis<32>();
          tmp.tdata = 0x0;
          tmp.tkeep = 0x0F;
          for(int j = 0; j<4; j++)
          {
            tmp.tdata |= ((ap_uint<32>) bytes[i*4+j]) << j*8;
          }
          tmp.tlast = 0;
          if ( i == (MPIF_HEADER_LENGTH/4) - 1)
          {
            tmp.tlast = 1;
          }
          sFifoDataTX.write(tmp);
          printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        }
        handshakeLinesCnt = (MPIF_HEADER_LENGTH + 7) /8;

        //dequeue
        if( !soTcp_data.full() && !sFifoDataTX.empty() )
        {
          NetworkWord word = NetworkWord();
          convertAxisToNtsWidth(sFifoDataTX, word);
          printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
          soTcp_data.write(word);
          handshakeLinesCnt--;

        }

        fsmMpeState = SEND_CLEAR;
      }
      break;
    case SEND_CLEAR:
      //dequeue
      if( !soTcp_data.full() && !sFifoDataTX.empty() )
      {
        NetworkWord word = NetworkWord();
        convertAxisToNtsWidth(sFifoDataTX, word);
        printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        soTcp_data.write(word);
        handshakeLinesCnt--;

      }
      //if( handshakeLinesCnt <= 0)
      if( handshakeLinesCnt <= 0 || sFifoDataTX.empty())
      {
        fsmMpeState = RECV_DATA_START;
        expect_more_data = false;
        current_data_src_node_id = 0xFFF;
        current_data_src_port = 0x0;
        current_data_dst_port = 0x0;
        //start subFSM
        //fsmReceiveState = READ_IDLE;
        //read_timeout_cnt  = 0;
      }
      break;
    case RECV_DATA_START:
      header = MPI_Header();
      expected_call = MPI_SEND_INT;
      if(currentDataType == MPI_FLOAT)
      {
        expected_call = MPI_SEND_FLOAT;
      }
      expected_type = DATA;
      //for DATA no CACHE!
      //DATA arrives only, if we expect it

      //if( !siTcp.empty() && !siIP.empty() && !sFifoDataRX.full() && !soMPIif.full() )
      //if( !siTcp_data.empty() && !siTcp_meta.empty() && !sFifoDataRX.full() )
      if( !siTcp_data.empty() && !siTcp_meta.empty() )
      {
        //read meta
        metaSrc = siTcp_meta.read().tdata;
        
        if(metaSrc.src_rank == current_data_src_node_id && metaSrc.src_port == current_data_src_port
            && metaSrc.dst_port == current_data_dst_port && expect_more_data)
        {
          //continuation of data packet
          fsmMpeState = RECV_DATA_RD;
          printf("[MPI_Recv] new payload packet arrived.\n");
          break;
        }

        //read header
        for(int i = 0; i< (MPIF_HEADER_LENGTH+7)/8; i++)
        {
          NetworkWord tmp = siTcp_data.read();
/*
          if(tmp.tkeep != 0xFF || tmp.tlast == 1)
          {
            printf("unexpected uncomplete read.\n");
            fsmReceiveState = READ_ERROR;
           //status[MPE_STATUS_READ_ERROR_CNT]++;
           //status[MPE_STATUS_LAST_READ_ERROR] = RX_INCOMPLETE_HEADER;
            break;
          }*/

          for(int j = 0; j<8; j++)
          {
//#pragma HLS unroll
            bytes[i*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
            //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          }
        }

        ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
        if(ret == 0 
            && !expect_more_data) //we don't start a new data packet here
        {
          //valid header && valid source
          expected_recv_count = header.size;
          exp_recv_count_enqueue = header.size;
          printf("[MPI_Recv] expect %d bytes.\n",expected_recv_count);
          recv_total_cnt = 0;
          enqueue_recv_total_cnt = 0;
          current_data_src_node_id = metaSrc.src_rank;
          current_data_src_port = metaSrc.src_port;
          current_data_dst_port = metaSrc.dst_port;

          fsmMpeState = RECV_DATA_RD;
          recvDeqFsm = DEQ_WRITE;
          //read_timeout_cnt = 0;
        }
      }

      break;
    case RECV_DATA_RD:
      if(recvDeqFsm == DEQ_DONE)
      {
        fsmMpeState = RECV_DATA_DONE;
        recvDeqFsm = DEQ_IDLE;
        break;
      }
      if( !rx_overflow_fifo.empty() )
      {
       while(!rx_overflow_fifo.empty())
       {
         uint32_t current_word = rx_overflow_fifo.read();
         if(!sFifoDataRX.write_nb(current_word))
         {
           rx_overflow_fifo.write(current_word);
           break;
         }
       }
      }
      if( !siTcp_data.empty() && !sFifoDataRX.full() 
          && rx_overflow_fifo.empty()
          )
      {
        NetworkWord word = siTcp_data.read();
        printf("READ: tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        //convertAxisToMpiWidth(word, sFifoDataRX);
        for(int i = 0; i < 2; i++)
        {
//#pragma HLS unroll factor=2
          if((word.tkeep >> i*4) == 0)
          {
            continue;
          }
          //with swap
          //bufferIn[bufferInPtrWrite] = (ap_uint<8>) (big.tdata >> (7-i)*8);
          //default
          ap_uint<32> current_word = (ap_uint<32>) (word.tdata >> i*32);
          enqueue_recv_total_cnt++;
          if(!sFifoDataRX.write_nb(current_word))
          {
            rx_overflow_fifo.write(current_word);
          }
          //check if we have to receive a new packet meta
          if(word.tlast == 1)
          {
            if(enqueue_recv_total_cnt < exp_recv_count_enqueue) //not <=
            {//we expect more
              fsmMpeState = RECV_DATA_START;
              expect_more_data = true;
            } else {
              fsmMpeState = RECV_DATA_WRD;
            }
          }
        }
      }
      break;
    case RECV_DATA_WRD:
      //wait for dequeue FSM
      if(recvDeqFsm == DEQ_DONE)
      {
        fsmMpeState = RECV_DATA_DONE;
        recvDeqFsm = DEQ_IDLE;
      }
      break;
    case RECV_DATA_DONE:
      //if(fsmReceiveState == READ_STANDBY && !soTcp_meta.full() && !sFifoDataTX.full() )
      if( !soTcp_meta.full() && !sFifoDataTX.full() )
      {
        printf("Read completed.\n");

        header = MPI_Header(); 
        header.dst_rank = currentInfo.rank;
        header.src_rank = *own_rank;
        header.size = 0;
        header.call = static_cast<mpiCall>((int) currentInfo.mpi_call);
        header.type = ACK;

        headerToBytes(header, bytes);

        //in order not to block the URIF/TRIF core
        metaDst = NetworkMeta(header.dst_rank, ZRLMPI_DEFAULT_PORT, header.src_rank, ZRLMPI_DEFAULT_PORT, 0); //we set tlast
        soTcp_meta.write(NetworkMetaStream(metaDst));

        //write header
        for(int i = 0; i < MPIF_HEADER_LENGTH/4; i++)
        {
          //Axis<8> tmp = Axis<8>(bytes[i]);
          Axis<32> tmp = Axis<32>();
          tmp.tdata = 0x0;
          tmp.tkeep = 0x0F;
          for(int j = 0; j<4; j++)
          {
            tmp.tdata |= ((ap_uint<32>) bytes[i*4+j]) << j*8;
          }
          tmp.tlast = 0;
          if ( i == (MPIF_HEADER_LENGTH/4) - 1)
          {
            tmp.tlast = 1;
          }
          sFifoDataTX.write(tmp);
          printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        }
        handshakeLinesCnt = (MPIF_HEADER_LENGTH + 7) /8;

        //dequeue
        if( !soTcp_data.full() && !sFifoDataTX.empty() )
        {
          NetworkWord word = NetworkWord();
          convertAxisToNtsWidth(sFifoDataTX, word);
          printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
          soTcp_data.write(word);
          handshakeLinesCnt--;

        }

        fsmMpeState = SEND_ACK; 
      }
      break;
    case RECV_DATA_ERROR:
      //empty strings
      printf("Read error occured.\n");
      if( !siTcp_meta.empty())
      {
        siTcp_meta.read();
      }

      if( !siTcp_data.empty())
      {
        siTcp_data.read();
      } else {
        fsmMpeState = IDLE;
      }
      break;
    case SEND_ACK:
      //dequeue
      if( !soTcp_data.full() && !sFifoDataTX.empty() )
      {
        NetworkWord word = NetworkWord();
        convertAxisToNtsWidth(sFifoDataTX, word);
        printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        soTcp_data.write(word);
        handshakeLinesCnt--;

      }
      if( handshakeLinesCnt <= 0 || sFifoDataTX.empty())
      {
        fsmMpeState = IDLE;
      }
      break;
  }
  printf("fsmMpeState after FSM: %d\n", fsmMpeState);

//===========================================================
// DEQUEUE FSM SEND

  switch(sendDeqFsm)
  {
    default:
    case DEQ_IDLE:
      //NOP / "reset"
      break;
    
    case DEQ_WRITE:
      printf("enqueueCnt: %d\n", enqueueCnt);
      word_tlast_occured = false;
      if( !soTcp_data.full() && !sFifoDataTX.empty() && (enqueueCnt >= 2 || tlast_occured_TX))
      {
        NetworkWord word = NetworkWord();
        convertAxisToNtsWidth(sFifoDataTX, word);
        printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        soTcp_data.write(word);
        enqueueCnt -= 2;

        //split to packet sizes is done by UOE/TOE
        if(word.tlast == 1)
        {
          printf("SEND_DATA finished writing.\n");
          word_tlast_occured = true;
        }
      }
      
      if(word_tlast_occured)
      {
        sendDeqFsm = DEQ_DONE;
      }
      break;

    case DEQ_DONE:
      //NOP
      break;
  }

  printf("sendDeqFsm after FSM: %d\n", sendDeqFsm);

 //===========================================================
// DEQUEUE FSM RECV

  switch(recvDeqFsm)
  {
    default:
    case DEQ_IDLE:
      //NOP
      break;
    
    case DEQ_WRITE:

      word_tlast_occured = false;
      //cnt = 0;
      if( !sFifoDataRX.empty() && !soMPI_data.full() )
      //while( cnt < 8 && !word_tlast_occured)
      {
      //if( !sFifoDataRX.empty() )//&& !soMPI_data.full() ) try to solve combinatorial loops...
      //{
        //Axis<8> tmp = sFifoDataRX.read(); //USE "blocking" version!! better matches to MPI_Wrapper...
        Axis<32> tmp = Axis<32>();
        uint32_t new_data;
        if(!sFifoDataRX.read_nb(new_data))
        {
          break;
        }

        tmp.tdata = new_data;
        tmp.tkeep = 0xF; //TODO

        //if(tmp.tlast == 1)
        if(recv_total_cnt >= (expected_recv_count - 1))
        {
          printf("[MPI_Recv] expected byte count reached.\n");
          //fsmMpeState = RECV_DATA_DONE;
          word_tlast_occured = true;
          tmp.tlast = 1;
        } else {
          tmp.tlast = 0;
        }
        printf("toAPP: tkeep %#04x, tdata %#08x, tlast %d\n",(int) tmp.tkeep, (uint32_t) tmp.tdata, (int) tmp.tlast);
        soMPI_data.write(tmp);
        //cnt++;
        recv_total_cnt++;
      //}
      }
      //read_timeout_cnt++;
      //if(read_timeout_cnt >= MPE_READ_TIMEOUT)
      //{
      //    //fsmReceiveState = READ_ERROR; //to clear streams?
      //    fsmReceiveState = READ_STANDBY;
      //   //status[MPE_STATUS_READ_ERROR_CNT]++;
      //   //status[MPE_STATUS_LAST_READ_ERROR] = RX_TIMEOUT;

      //}

      if(word_tlast_occured)
      {
        recvDeqFsm = DEQ_DONE;
      }
      break;

    case DEQ_DONE:
      //NOP
      break;
  }

  printf("recvDeqFsm after FSM: %d\n", sendDeqFsm);
  

  return;
}



