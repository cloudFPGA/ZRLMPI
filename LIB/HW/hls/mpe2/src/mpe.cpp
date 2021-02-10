
#include "mpe.hpp"
#include <stdint.h>


using namespace hls;



//void convertAxisToNtsWidth(stream<Axis<32> > &small, NetworkWord &out)
//{
//#pragma HLS inline
//
//  out.tdata = 0;
//  out.tlast = 0;
//  out.tkeep = 0;
//
//  for(int i = 0; i < 2; i++)
//    //for(int i = 7; i >=0 ; i--)
//  {
//    //#pragma HLS unroll
//    Axis<32> tmpl = Axis<32>();
//    //if(!small.empty())
//    if(small.read_nb(tmpl))
//    {
//      // Axis<8> tmp = small.read();
//      //printf("read from fifo: %#02x\n", (unsigned int) tmp.tdata);
//      out.tdata |= ((ap_uint<64>) (tmpl.tdata) )<< (i*32);
//      out.tkeep |= (ap_uint<8>) 0x0F << i*4;
//      //TODO? NO latch, because last read from small is still last read
//      if(out.tlast == 0)
//      {
//        out.tlast = tmpl.tlast;
//      }
//
//    } else {
//      printf("\ttried to read empty small stream!\n");
//      //now, we set tlast just to be sure...TODO?
//      out.tlast = 1;
//      break;
//    }
//  }
//
//}
//
//void convertAxisToMpiWidth(NetworkWord big, stream<Axis<32> > &out)
//{
//  //#pragma HLS inline
//
//  int positionOfTlast = 2;
//  ap_uint<8> tkeep = big.tkeep;
//  for(int i = 0; i<2; i++) //no reverse order!
//  {
//    //#pragma HLS unroll
//    //tkeep = (tkeep >> 1);
//    tkeep = (tkeep >> 4);
//    //printf("\tshifted tkeep %#02x\n",(uint8_t) tkeep);
//    //if((tkeep & 0x01) == 0)
//    if((tkeep & 0x0F) == 0)
//    {
//      positionOfTlast = i;
//      break;
//    }
//  }
//
//  //for(int i = 7; i >=0 ; i--)
//  for(int i = 0; i < 2; i++)
//  {
//    //#pragma HLS unroll
//    //out.full?
//    Axis<32> tmp = Axis<32>();
//    if(i == positionOfTlast)
//      //if(i == 0)
//    {
//      //only possible position...
//      tmp.tlast = big.tlast;
//      printf("tlast set.\n");
//    } else {
//      tmp.tlast = 0;
//    }
//    tmp.tdata = (ap_uint<32>) (big.tdata >> i*32);
//    //tmp.tdata = (ap_uint<8>) (big.tdata >> (7-i)*8);
//    tmp.tkeep = (ap_uint<4>) (big.tkeep >> i*4);
//    //tmp.tkeep = (ap_uint<1>) (big.tkeep >> (7-i));
//
//    if(tmp.tkeep == 0)
//    {
//      continue;
//    }
//
//    out.write(tmp);
//  }
//
//}


uint8_t extractByteCnt(Axis<64> currWord)
{
#pragma HLS INLINE

  uint8_t ret = 0;

  switch (currWord.tkeep) {
    case 0b11111111:
      ret = 8;
      break;
    case 0b01111111:
      ret = 7;
      break;
    case 0b00111111:
      ret = 6;
      break;
    case 0b00011111:
      ret = 5;
      break;
    case 0b00001111:
      ret = 4;
      break;
    case 0b00000111:
      ret = 3;
      break;
    case 0b00000011:
      ret = 2;
      break;
    default:
    case 0b00000001:
      ret = 1;
      break;
  }
  return ret;
}


//returns: 0 ok, 1 not ok
uint8_t checkHeader(stream<ap_uint<32> > &sFifoHeaderCache, uint16_t &current_cache_data_cnt, ap_uint<8> bytes[MPIF_HEADER_LENGTH], MPI_Header &header, NetworkMeta &metaSrc,
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


void pDeqRecv(
    stream<uint64_t> &sFifoDataRX,
    stream<Axis<64> > &soMPI_data,
    stream<uint16_t>  &sExpectedLength, //in LINES!
    stream<bool>      &sDeqRecvDone
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static deqState recvDeqFsm = DEQ_IDLE;
#pragma HLS reset variable=recvDeqFsm
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static uint16_t expected_recv_count = 0;
  static uint16_t recv_total_cnt = 0;

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------

  bool word_tlast_occured = false;

  switch(recvDeqFsm)
  {
    default:
    case DEQ_IDLE:
      if(!sExpectedLength.empty())
      {
        expected_recv_count = sExpectedLength.read();
        recv_total_cnt = 0;
        recvDeqFsm = DEQ_WRITE;
      }
      break;

    case DEQ_WRITE:

      //word_tlast_occured = false;
      if( !sFifoDataRX.empty() && !soMPI_data.full()
        )
      {
        Axis<64> tmp = Axis<64>();

        uint64_t new_data = sFifoDataRX.read();

        tmp.tdata = new_data;
        tmp.tkeep = 0xFFF;

        //if(tmp.tlast == 1)
        if(recv_total_cnt >= (expected_recv_count - 1))
        {
          printf("[MPI_Recv] expected byte count reached.\n");
          //word_tlast_occured = true;
          tmp.tlast = 1;
          recvDeqFsm = DEQ_DONE;
        } else {
        	//in ALL other cases
          tmp.tlast = 0;
        }
        printf("toAPP: tkeep %#04x, tdata %#08x, tlast %d\n",(int) tmp.tkeep, (uint32_t) tmp.tdata, (int) tmp.tlast);
        soMPI_data.write(tmp);
        //cnt++;
        recv_total_cnt++; //we are counting LINES!
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

//  if(word_tlast_occured)
//  {
//    recvDeqFsm = DEQ_DONE;
//  }
  break;

  case DEQ_DONE:
  if(!sDeqRecvDone.full())
  {
    sDeqRecvDone.write(true);
    recvDeqFsm = DEQ_IDLE;
  }
  break;
}

printf("recvDeqFsm after FSM: %d\n", recvDeqFsm);

}


void pDeqSend(
    stream<Axis<64> >  &sFifoDataTX,
    stream<NetworkWord>            &soTcp_data,
    stream<NetworkMetaStream>      &soTcp_meta,
    ap_uint<32> *own_rank,
    stream<NodeId>        &sDeqSendDestId,
    stream<bool>        &sDeqSendDone
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static deqState sendDeqFsm = DEQ_IDLE;
#pragma HLS reset variable=sendDeqFsm
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static uint32_t current_packet_line_cnt = 0x0;
  static NodeId current_send_dst_id = 0xFFF;
  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------
  bool word_tlast_occured = false;


  switch(sendDeqFsm)
  {
    default:
    case DEQ_IDLE:
      if(!sDeqSendDestId.empty())
      {
        current_send_dst_id = sDeqSendDestId.read();
        current_packet_line_cnt = 0;
        sendDeqFsm = DEQ_WRITE;
      }
      break;

    case DEQ_WRITE:
      //printf("enqueueCnt: %d\n", enqueueCnt);
      word_tlast_occured = false;
      if( !soTcp_data.full() && !sFifoDataTX.empty() //&& (enqueueCnt >= 2 || tlast_occured_TX))
        && !soTcp_meta.full() )
        {
          NetworkWord word = NetworkWord();
          //convertAxisToNtsWidth(sFifoDataTX, word);
          word.tdata = 0x0;
          word.tlast = 0x0;
          word.tkeep = 0x0;

          Axis<64> tmpl1 = Axis<64>();
          //        Axis<32> tmpl2 = Axis<32>();
          //        bool only_one_word_read = false;
          //        if(!sFifo_underflow_TX.empty())
          //        {
          //          //first word
          //          tmpl1 = sFifo_underflow_TX.read();
          //          //second word
          //          if(!sFifoDataTX.read_nb(tmpl2))
          //          {
          //            if(tlast_occured_TX)
          //            {
          //              only_one_word_read = true;
          //            } else {
          //              sFifo_underflow_TX.write(tmpl1);
          //              break;
          //            }
          //          }
          //        } else {
          //          //first word
          //          if(!sFifoDataTX.read_nb(tmpl1))
          //          {
          //            break;
          //          }
          //          //second word
          //          if(!sFifoDataTX.read_nb(tmpl2))
          //          {
          //            if(tlast_occured_TX)
          //            {
          //              only_one_word_read = true;
          //            } else {
          //              sFifo_underflow_TX.write(tmpl1);
          //              break;
          //            }
          //          }
          //        }
          //        //combine to NetworkWord
          tmpl1 = sFifoDataTX.read();
          word.tdata = tmpl1.tdata;
          word.tkeep = 0xFFF;
          word.tlast = tmpl1.tlast;
          //        if(!only_one_word_read)
          //        {
          //          word.tdata |= ((ap_uint<64>) tmpl2.tdata) << 32;
          //          word.tkeep |= ((ap_uint<8>) 0x0F) << 4;
          //          if(word.tlast == 0)
          //          {
          //            word.tlast = tmpl2.tlast;
          //          }
          //        } else {
          //          //just to be sure..
          //          word.tlast = 1;
          //        }
          //check before we split in parts
          if(word.tlast == 1)
          {
            printf("SEND_DATA finished writing.\n");
            word_tlast_occured = true;
          }

          //if(current_packet_line_cnt >= ZRLMPI_MAX_MESSAGE_SIZE_LINES)
          if(current_packet_line_cnt == 0)
          {//last write was last one or first one
            NetworkMeta metaDst = NetworkMeta(current_send_dst_id, ZRLMPI_DEFAULT_PORT, *own_rank, ZRLMPI_DEFAULT_PORT, 0);
            soTcp_meta.write(NetworkMetaStream(metaDst));
            printf("started new DATA part packet\n");
          }

          if(current_packet_line_cnt >= (ZRLMPI_MAX_MESSAGE_SIZE_LINES - 1))
          {//last one in this packet
            word.tlast = 1;
            current_packet_line_cnt = 0;
          } else {
            current_packet_line_cnt++;
          }

          printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
          soTcp_data.write(word);
          //enqueueCnt -= 2;
        }

      if(word_tlast_occured)
      {
        sendDeqFsm = DEQ_DONE;
      }
      break;

    case DEQ_DONE:
      if(!sDeqSendDone.full())
      {
        sDeqSendDone.write(true);
        sendDeqFsm = DEQ_IDLE;
      }
      break;
  }

  printf("sendDeqFsm after FSM: %d\n", sendDeqFsm);
}


void pMpeGlobal(
    stream<MPI_Interface> &siMPIif,
    ap_uint<32> *own_rank,
    stream<Axis<64> >     &sFifoDataTX,
    stream<NodeId>        &sDeqSendDestId,
    stream<bool>        &sDeqSendDone,
    stream<NetworkWord>            &siTcp_data,
    stream<NetworkMetaStream>      &siTcp_meta,
    stream<Axis<64> > &siMPI_data,
    stream<uint64_t> &sFifoDataRX,
    stream<uint16_t>  &sExpectedLength, //in LINES!
    stream<bool>      &sDeqRecvDone
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
  //  #pragma HLS pipeline II=1 //TODO
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  mpeState fsmMpeState = IDLE;

#pragma HLS reset variable=fsmMpeState
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static stream<ap_uint<32> > sFifoHeaderCache("sFifoHeaderCache");
  static bool checked_cache = false;
  static uint32_t expected_src_rank = 0;
  static uint16_t current_cache_data_cnt = 0;

  static MPI_Interface currentInfo = MPI_Interface();
  static mpiType currentDataType = MPI_INT;
  //static int handshakeLinesCnt = 0;

  static uint8_t header_i_cnt = 0;
  static ap_uint<8> bytes[MPIF_HEADER_LENGTH];

  static mpiCall expected_call = 0;
  static packetType expected_type = 0;
  static uint16_t checked_entries = 0;


  static NetworkMeta metaSrc = NetworkMeta();
  static uint32_t expected_send_count = 0;
  static uint32_t send_total_cnt = 0;

  static MPI_Header header = MPI_Header();
  //static NetworkMeta metaDst = NetworkMeta();

  static NodeId current_data_src_node_id = 0xFFF;
  static NrcPort current_data_src_port = 0x0;
  static NrcPort current_data_dst_port = 0x0;
  static bool expect_more_data = false;

  static uint16_t exp_recv_count_enqueue = 0;
  static uint16_t enqueue_recv_total_cnt = 0;



  //#pragma HLS STREAM variable=sFifoHeaderCache depth=64
#pragma HLS STREAM variable=sFifoHeaderCache depth=2048 //HEADER_CACHE_LENTH*MPIF_HEADER_LENGTH


  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------

  uint8_t ret;
  //bool found_cache = false;
  //ap_uint<MPIF_HEADER_LENGTH*8> cache_line = 0x0;

  Axis<64> current_read_word = Axis<64>();
  //int8_t cnt = 0;

  switch(fsmMpeState) 
  {
    case IDLE:
      checked_cache = false;
      expected_src_rank = 0xFFF;
      if ( !siMPIif.empty() ) //TODO: try to fix combinatorial loop
      {
        currentInfo = siMPIif.read();
        header_i_cnt = 0;
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
            fsmMpeState = START_RECV;
            //fsmMpeState = WAIT4REQ;
            expected_src_rank = currentInfo.rank;
            break;
          case MPI_RECV_FLOAT:
            currentDataType = MPI_FLOAT;
            fsmMpeState = START_RECV;
            //fsmMpeState = WAIT4REQ;
            expected_src_rank = currentInfo.rank;
            break;
          case MPI_BARRIER: 
            //TODO not yet implemented 
            break;
        }
      }
      break;
    case START_SEND: 
      if ( //!soTcp_meta.full() &&
          !sFifoDataTX.full()
          && !sDeqSendDestId.full()
         )
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
        //metaDst = NetworkMeta(header.dst_rank, ZRLMPI_DEFAULT_PORT, header.src_rank, ZRLMPI_DEFAULT_PORT, 0); //we set tlast
        //soTcp_meta.write(NetworkMetaStream(metaDst));
        sDeqSendDestId.write((NodeId) header.dst_rank);
        header_i_cnt = 0;

        //write header
        //for(int i = 0; i < MPIF_HEADER_LENGTH/4; i++)
        // {
        //Axis<8> tmp = Axis<8>(bytes[i]);
        Axis<64> tmp = Axis<64>();
        tmp.tdata = 0x0;
        tmp.tkeep = 0xFFF;
        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          //tmp.tdata |= ((ap_uint<32>) bytes[i*4+j]) << (3-j)*8;
          tmp.tdata |= ((ap_uint<64>) bytes[header_i_cnt*8+j]) << j*8;
        }
        //printf("tdata32: %#08x\n",(uint32_t) tmp.tdata);
        printf("tdata64: %#016x\n",(uint64_t) tmp.tdata);
        tmp.tlast = 0;
        //          if ( i == (MPIF_HEADER_LENGTH/4) - 1)
        //          {
        //            tmp.tlast = 1;
        //          }
        sFifoDataTX.write(tmp);
        //printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        header_i_cnt++;
        //}
        //handshakeLinesCnt = (MPIF_HEADER_LENGTH + 7) /8;

        //        //dequeue
        //        if( !soTcp_data.full() && !sFifoDataTX.empty() )
        //        {
        //          NetworkWord word = NetworkWord();
        //          convertAxisToNtsWidth(sFifoDataTX, word);
        //          printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        //          soTcp_data.write(word);
        //          handshakeLinesCnt--;
        //
        //        }

        //fsmMpeState = SEND_REQ;
        fsmMpeState = START_SEND_1;
      }
      break;
    case START_SEND_1:
      if(!sFifoDataTX.full())
      {
        Axis<64> tmp = Axis<64>();
        tmp.tdata = 0x0;
        tmp.tkeep = 0xFFF;
        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          //tmp.tdata |= ((ap_uint<32>) bytes[i*4+j]) << (3-j)*8;
          tmp.tdata |= ((ap_uint<64>) bytes[header_i_cnt*8+j]) << j*8;
        }
        printf("tdata64: %#016x\n",(uint64_t) tmp.tdata);
        tmp.tlast = 0;
        if ( header_i_cnt >= (MPIF_HEADER_LENGTH/8) - 1)
        {
          tmp.tlast = 1;
          fsmMpeState = SEND_REQ;
        }
        sFifoDataTX.write(tmp);
        header_i_cnt++;
      }
      break;
    case SEND_REQ:
      //      //dequeue
      //      if( !soTcp_data.full() && !sFifoDataTX.empty() )
      //      {
      //        NetworkWord word = NetworkWord();
      //        convertAxisToNtsWidth(sFifoDataTX, word);
      //        printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
      //        soTcp_data.write(word);
      //        handshakeLinesCnt--;
      //
      //      }
      //      //if( handshakeLinesCnt <= 0)
      //      if( handshakeLinesCnt <= 0 || sFifoDataTX.empty())
      //      {
      //        checked_cache = false;
      //        fsmMpeState = WAIT4CLEAR;
      //      }
      if(!sDeqSendDone.empty())
      {
        if(sDeqSendDone.read())
        {
          fsmMpeState = WAIT4CLEAR_CACHE;
          header = MPI_Header();
          expected_call = MPI_RECV_INT;
          if(currentDataType == MPI_FLOAT)
          {
            expected_call = MPI_RECV_FLOAT;
          }
          expected_type = CLEAR_TO_SEND;
          checked_cache = false;
          checked_entries = 0;
        }
      }
      break;
    case WAIT4CLEAR_CACHE:
      //check cache first
      //found_cache = false;
      //if(!checked_cache)
      //{
      //uint16_t static_data_cnt = current_cache_data_cnt;
      //printf("check %d chache entries\n",static_data_cnt);
      //cache_line = 0x0;
      //while(!sFifoHeaderCache.empty())
      //while(checked_entries < static_data_cnt)

      //if(checked_entries < current_cache_data_cnt)
      if(!sFifoHeaderCache.empty())
      {
        printf("check %d chache entries\n", (uint16_t) current_cache_data_cnt);
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
        ret = checkHeader(sFifoHeaderCache, current_cache_data_cnt, bytes, header, metaSrc, expected_type, expected_call, true, expected_src_rank);
        if(ret == 0)
        {//got CLEAR_TO_SEND 
          printf("Got CLEAR to SEND from the cache\n");
          fsmMpeState = SEND_DATA_START;
          //found_cache = true;
          break;
        }
        //else, we continue
        //checkHeader puts it back to the cache
        checked_entries++;
        if(checked_entries >= current_cache_data_cnt)
        {
          fsmMpeState = WAIT4CLEAR;
        }
      } else {
        fsmMpeState = WAIT4CLEAR;
      }
      //in all cases
      //  checked_cache = true;
      // }
      break;
    case WAIT4CLEAR:
      if( !siTcp_data.empty() && !siTcp_meta.empty()
        )
      {

        metaSrc = siTcp_meta.read().tdata;
        header_i_cnt = 0;
        fsmMpeState = WAIT4CLEAR_1;

        NetworkWord tmp = siTcp_data.read();
        printf("Data read: tkeep %#03x, tdata %#016llx, tlast %d\n",(int) tmp.tkeep, (unsigned long long) tmp.tdata, (int) tmp.tlast);
        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          bytes[header_i_cnt*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
        header_i_cnt = 1;
      }
      break;

    case WAIT4CLEAR_1:
      if( //!found_cache &&
          !siTcp_data.empty() //&& !siTcp_meta.empty()
        )
      {
        //read header
        //for(int i = 0; i< (MPIF_HEADER_LENGTH+7)/8; i++)
        //{
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
#pragma HLS unroll
          bytes[header_i_cnt*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
        header_i_cnt++;
        // }
        if(header_i_cnt >= (MPIF_HEADER_LENGTH+7)/8)
        {
          ret = checkHeader(sFifoHeaderCache, current_cache_data_cnt, bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
          if(ret == 0)
          {//got CLEAR_TO_SEND 
            printf("Got CLEAR to SEND\n");
            fsmMpeState = SEND_DATA_START;
          } else {
            //else, we continue to wait
            fsmMpeState = WAIT4CLEAR;
          }
        }
      }

      break;
    case SEND_DATA_START:
      if ( !sFifoDataTX.full() //&& !soTcp_meta.full()
          && !sDeqSendDestId.full()
         )
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


        header_i_cnt = 0;
        //write header
        //for(int i = 0; i < MPIF_HEADER_LENGTH/4; i++)
        // {
        //Axis<8> tmp = Axis<8>(bytes[i]);
        Axis<64> tmp = Axis<64>();
        tmp.tdata = 0x0;
        tmp.tkeep = 0xFFF;
        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          tmp.tdata |= ((ap_uint<64>) bytes[header_i_cnt*8+j]) << j*8;
        }
        tmp.tlast = 0; //in this case, always
        sFifoDataTX.write(tmp);
        //printf("Writing Header byte: %#02x\n", (int) bytes[header_i_cnt]);
        // }
        header_i_cnt = 1;

        //metaDst = NetworkMeta(header.dst_rank, ZRLMPI_DEFAULT_PORT, header.src_rank, ZRLMPI_DEFAULT_PORT, 0); //we set tlast
        //soTcp_meta.write(NetworkMetaStream(metaDst));

        sDeqSendDestId.write((NodeId) header.dst_rank);

        expected_send_count = header.size;
        printf("[MPI_send] expect %d bytes.\n",expected_send_count);
        send_total_cnt = 0;

        //enqueueCnt = MPIF_HEADER_LENGTH/4;
        //fsmMpeState = SEND_DATA_RD;
        fsmMpeState = SEND_DATA_START_1;
      }
      break;
    case SEND_DATA_START_1:
      if(!sFifoDataTX.full())
      {
        Axis<64> tmp = Axis<64>();
        tmp.tdata = 0x0;
        tmp.tkeep = 0xFFF;
        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          tmp.tdata |= ((ap_uint<64>) bytes[header_i_cnt*8+j]) << j*8;
        }
        tmp.tlast = 0; //in this case, always
        sFifoDataTX.write(tmp);
        //printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        header_i_cnt++;
        if(header_i_cnt >= MPIF_HEADER_LENGTH/8)
        {
          fsmMpeState = SEND_DATA_RD;
        }

      }
    case SEND_DATA_RD:
      //enqueue 
      //cnt = 0;
      ////while( !siMPI_data.empty() && !sFifoDataTX.full() && cnt<=8 && !tlast_occured_TX)
      ////if( !siMPI_data.empty() && !sFifoDataTX.full() && !tlast_occured_TX)
      ////while( cnt<=8 && !tlast_occured_TX)
      //while( cnt<=2 && !tlast_occured_TX)
      //{
      //  //current_read_byte = siMPI_data.read();
      //  if(!siMPI_data.read_nb(current_read_word))
      //  {
      //    break;
      //  }
      //  if(send_total_cnt >= (expected_send_count - 1))
      //  {// to be sure...
      //    current_read_word.tlast = 1;
      //  }
      //  //use "blocking" version
      //  sFifoDataTX.write(current_read_word);
      //  cnt++;
      //  send_total_cnt++;
      //  if(current_read_word.tlast == 1)
      //  {
      //    tlast_occured_TX = true;
      //    fsmMpeState = SEND_DATA_WRD;
      //    printf("tlast Occured.\n");
      //    printf("MPI read data: %#08x, tkeep: %d, tlast %d\n", (int) current_read_word.tdata, (int) current_read_word.tkeep, (int) current_read_word.tlast);
      //  }
      //  //enqueueCnt++;
      //}
      ////printf("cnt: %d\n", cnt);
      //      cnt = 0;
      //      while( cnt<=2 && !tlast_occured_TX)
      //      {
      //        if(!sFifoDataTX.full())
      //        {
      //          if(!tx_overflow_fifo.empty())
      //          {
      //            current_read_word = tx_overflow_fifo.read();
      //            cnt--;
      //          } else {
      //            if(!siMPI_data.empty())
      //            {
      //              if(!siMPI_data.read_nb(current_read_word))
      //              {
      //                break;
      //              }
      //            } else {
      //              break;
      //            }
      //          }
      //          if(send_total_cnt >= (expected_send_count - 1))
      //          {// to be sure...
      //            current_read_word.tlast = 1;
      //          }
      //          if(!sFifoDataTX.write_nb(current_read_word))
      //          {
      //            tx_overflow_fifo.write(current_read_word);
      //            break;
      //          }
      //          cnt++;
      //          send_total_cnt++;
      //          if(current_read_word.tlast == 1)
      //          {
      //            tlast_occured_TX = true;
      //            fsmMpeState = SEND_DATA_WRD;
      //            printf("tlast Occured.\n");
      //          }
      //          printf("MPI read data: %#08x, tkeep: %d, tlast %d\n", (int) current_read_word.tdata, (int) current_read_word.tkeep, (int) current_read_word.tlast);
      //        } else {
      //          break;
      //        }
      //      }
      if(!sFifoDataTX.full() && !siMPI_data.empty())
      {
        current_read_word = siMPI_data.read();
        if(send_total_cnt >= (expected_send_count - 2))
        {// to be sure...
          current_read_word.tlast = 1;
        }
        send_total_cnt += 2; //we read TWO words per line

        if(current_read_word.tlast == 1)
        {
          fsmMpeState = SEND_DATA_WRD;
          printf("tlast Occured.\n");
        }
        printf("MPI read data: %#08x, tkeep: %d, tlast %d\n", (int) current_read_word.tdata, (int) current_read_word.tkeep, (int) current_read_word.tlast);

      }
      break;
    case SEND_DATA_WRD:
      //wait for dequeue fsm
      if(!sDeqRecvDone.empty())
      {
        if(sDeqRecvDone.read())
        {
          checked_cache = false;
          fsmMpeState = WAIT4ACK_CACHE;
          header = MPI_Header();
          expected_call = MPI_RECV_INT;
          if(currentDataType == MPI_FLOAT)
          {
            expected_call = MPI_RECV_FLOAT;
          }
          expected_type = ACK;
        }
      }
      break;
    case WAIT4ACK_CACHE:
      //check cache first
      //found_cache = false;
      //if(!checked_cache)
      //{
      //uint16_t static_data_cnt = current_cache_data_cnt;
      //printf("check %d chache entries\n",static_data_cnt);
      //cache_line = 0x0;
      //while(!sFifoHeaderCache.empty())
      //while(checked_entries < static_data_cnt)

      //if(checked_entries < current_cache_data_cnt)
      if(!sFifoHeaderCache.empty())
      {
        printf("check %d chache entries\n", (uint16_t) current_cache_data_cnt);
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
        ret = checkHeader(sFifoHeaderCache, current_cache_data_cnt, bytes, header, metaSrc, expected_type, expected_call, true, expected_src_rank);
        if(ret == 0)
        {//got CLEAR_TO_SEND 
          printf("Got ACK from the cache\n");
          fsmMpeState = IDLE;
          //found_cache = true;
          break;
        }
        //else, we continue
        //checkHeader puts it back to the cache
        checked_entries++;
        if(checked_entries >= current_cache_data_cnt)
        {
          fsmMpeState = WAIT4ACK;
        }
      } else {
        fsmMpeState = WAIT4ACK;
      }
      //in all cases
      //  checked_cache = true;
      // }
      break;
    case WAIT4ACK:
      if( !siTcp_data.empty() && !siTcp_meta.empty() )
      {
        //read header
        //for(int i = 0; i< (MPIF_HEADER_LENGTH+7)/8; i++)
        // {
        NetworkWord tmp = siTcp_data.read();
        header_i_cnt = 0;

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
#pragma HLS unroll
          bytes[header_i_cnt*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
        // }
        header_i_cnt = 1;

        metaSrc = siTcp_meta.read().tdata;
        fsmMpeState = WAIT4ACK_1;
      }
      break;
    case WAIT4ACK_1:
      if( !siTcp_data.empty())
      {

        NetworkWord tmp = siTcp_data.read();

        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          bytes[header_i_cnt*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
        header_i_cnt++;

        if(header_i_cnt >= (MPIF_HEADER_LENGTH+7)/8)
        {
          ret = checkHeader(sFifoHeaderCache, current_cache_data_cnt, bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
          if(ret == 0)
          {
            printf("ACK received.\n");
            fsmMpeState = IDLE;
          } else {
            //else, we continue to wait
            fsmMpeState = WAIT4ACK;
          }
        }
      }
      break;
    case START_RECV:
      header = MPI_Header();
      expected_call = MPI_SEND_INT;
      if(currentDataType == MPI_FLOAT)
      {
        expected_call = MPI_SEND_FLOAT;
      }
      expected_type = SEND_REQUEST;
      fsmMpeState = WAIT4REQ_CACHE;
      break;
    case WAIT4REQ_CACHE:
      //check cache first
      //found_cache = false;
      //if(!checked_cache)
      //{
      //uint16_t static_data_cnt = current_cache_data_cnt;
      //printf("check %d chache entries\n",static_data_cnt);
      //cache_line = 0x0;
      //while(!sFifoHeaderCache.empty())
      //while(checked_entries < static_data_cnt)

      //if(checked_entries < current_cache_data_cnt)
      if(!sFifoHeaderCache.empty())
      {
        printf("check %d chache entries\n", (uint16_t) current_cache_data_cnt);
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
        ret = checkHeader(sFifoHeaderCache, current_cache_data_cnt, bytes, header, metaSrc, expected_type, expected_call, true, expected_src_rank);
        if(ret == 0)
        {//got CLEAR_TO_SEND 
          printf("Got SEND_REQUEST from the cache\n");
          fsmMpeState = ASSEMBLE_CLEAR;
          //found_cache = true;
          break;
        }

        //else, we continue
        //checkHeader puts it back to the cache
        checked_entries++;
        if(checked_entries >= current_cache_data_cnt)
        {
          fsmMpeState = WAIT4REQ;
        }
      } else {
        fsmMpeState = WAIT4REQ;
      }
      //in all cases
      //  checked_cache = true;
      // }
      break;
    case WAIT4REQ:
      if( !siTcp_data.empty() && !siTcp_meta.empty() )
      {
        //read header
        //for(int i = 0; i< (MPIF_HEADER_LENGTH+7)/8; i++)
        //{
        NetworkWord tmp = siTcp_data.read();
        header_i_cnt = 0;
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
#pragma HLS unroll
          bytes[header_i_cnt*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          //bytes[i*8 + 7 -j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
        header_i_cnt = 1;
        // }

        metaSrc = siTcp_meta.read().tdata;
        fsmMpeState = WAIT4REQ_1;
      }
      break;
    case WAIT4REQ_1:
      if( !siTcp_data.empty() )
      {
        NetworkWord tmp = siTcp_data.read();
        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          bytes[header_i_cnt*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          //bytes[i*8 + 7 -j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
        header_i_cnt++;

        if(header_i_cnt >= (MPIF_HEADER_LENGTH+7)/8)
        {
          ret = checkHeader(sFifoHeaderCache, current_cache_data_cnt, bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
          if(ret == 0)
          {
            //got SEND_REQUEST 
            printf("Got SEND REQUEST\n");
            fsmMpeState = ASSEMBLE_CLEAR;
          } else {
            //else, we wait...
            fsmMpeState = WAIT4REQ;
          }
        }
      }
      break;
    case ASSEMBLE_CLEAR:
      if(//!soTcp_meta.full() &&
          !sFifoDataTX.full()
          && sDeqSendDestId.full()
        )
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
        //metaDst = NetworkMeta(header.dst_rank, ZRLMPI_DEFAULT_PORT, header.src_rank, ZRLMPI_DEFAULT_PORT, 0); //we set tlast
        //soTcp_meta.write(NetworkMetaStream(metaDst));
        sDeqSendDestId.write((NodeId) header.dst_rank);

        header_i_cnt = 0;

        //write header
        //for(int i = 0; i < MPIF_HEADER_LENGTH/4; i++)
        //{
        //Axis<8> tmp = Axis<8>(bytes[i]);
        Axis<64> tmp = Axis<64>();
        tmp.tdata = 0x0;
        tmp.tkeep = 0xFFF;
        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          tmp.tdata |= ((ap_uint<64>) bytes[header_i_cnt*8+j]) << j*8;
        }
        tmp.tlast = 0;
        //          if ( i == (MPIF_HEADER_LENGTH/4) - 1)
        //          {
        //            tmp.tlast = 1;
        //          }
        sFifoDataTX.write(tmp);
        header_i_cnt = 1;

        //printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        //}
        //handshakeLinesCnt = (MPIF_HEADER_LENGTH + 7) /8;

        //dequeue
        //        if( !soTcp_data.full() && !sFifoDataTX.empty() )
        //        {
        //          NetworkWord word = NetworkWord();
        //          convertAxisToNtsWidth(sFifoDataTX, word);
        //          printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        //          soTcp_data.write(word);
        //          handshakeLinesCnt--;
        //
        //        }

        fsmMpeState = ASSEMBLE_CLEAR_1;
      }
      break;
    case ASSEMBLE_CLEAR_1:
      if(!sFifoDataTX.full())
      {
        Axis<64> tmp = Axis<64>();
        tmp.tdata = 0x0;
        tmp.tkeep = 0xFFF;
        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          tmp.tdata |= ((ap_uint<64>) bytes[header_i_cnt*8+j]) << j*8;
        }
        tmp.tlast = 0;
        if ( header_i_cnt >= (MPIF_HEADER_LENGTH/8) - 1)
        {
          tmp.tlast = 1;
          fsmMpeState = SEND_CLEAR;
          expect_more_data = false;
          current_data_src_node_id = 0xFFF;
          current_data_src_port = 0x0;
          current_data_dst_port = 0x0;
          header = MPI_Header();
          expected_call = MPI_SEND_INT;
          if(currentDataType == MPI_FLOAT)
          {
            expected_call = MPI_SEND_FLOAT;
          }
          expected_type = DATA;
        }
        sFifoDataTX.write(tmp);
        header_i_cnt++;

      }
      break;
    case SEND_CLEAR:
      //      //dequeue
      //      if( !soTcp_data.full() && !sFifoDataTX.empty() )
      //      {
      //        NetworkWord word = NetworkWord();
      //        convertAxisToNtsWidth(sFifoDataTX, word);
      //        printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
      //        soTcp_data.write(word);
      //        handshakeLinesCnt--;
      //
      //      }
      //      //if( handshakeLinesCnt <= 0)
      //      if( handshakeLinesCnt <= 0 || sFifoDataTX.empty())
      //      {
      //        fsmMpeState = RECV_DATA_START;
      //        expect_more_data = false;
      //        current_data_src_node_id = 0xFFF;
      //        current_data_src_port = 0x0;
      //        current_data_dst_port = 0x0;
      //        //start subFSM
      //        //fsmReceiveState = READ_IDLE;
      //        //read_timeout_cnt  = 0;
      //      }
      if(!sDeqSendDone.empty())
      {
        if(sDeqSendDone.read())
        {
          fsmMpeState = RECV_DATA_START;
          expect_more_data = false;
          current_data_src_node_id = 0xFFF;
          current_data_src_port = 0x0;
          current_data_dst_port = 0x0;
          header = MPI_Header();
          expected_call = MPI_SEND_INT;
          if(currentDataType == MPI_FLOAT)
          {
            expected_call = MPI_SEND_FLOAT;
          }
          expected_type = DATA;
        }
      }
      break;
    case RECV_DATA_START:
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
          //break;
        } else {

            header_i_cnt = 0;
          //read header
          //for(int i = 0; i< (MPIF_HEADER_LENGTH+7)/8; i++)
          //{

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
              #pragma HLS unroll
              bytes[header_i_cnt*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
              //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
            }
          //}
            header_i_cnt = 1;
            fsmMpeState = RECV_DATA_START_1;
        }
      }
      break;

    case RECV_DATA_START_1:
    	 if( !siTcp_data.empty()
    		 && !sExpectedLength.full()
    	 )
    	 {
           NetworkWord tmp = siTcp_data.read();
           for(int j = 0; j<8; j++)
           {
             #pragma HLS unroll
             bytes[header_i_cnt*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
             //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
           }
           header_i_cnt++;

    		 if(header_i_cnt >= (MPIF_HEADER_LENGTH+7)/8)
    		 {
    			 ret = checkHeader(sFifoHeaderCache, current_cache_data_cnt, bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
				  if(ret == 0
					  && !expect_more_data) //we don't start a new data packet here
				  {
					//valid header && valid source
					//expected_recv_count = header.size;
					  sExpectedLength.write(header.size);

					  exp_recv_count_enqueue = header.size;
					printf("[MPI_Recv] expect %d bytes.\n",exp_recv_count_enqueue);
					//recv_total_cnt = 0;
					enqueue_recv_total_cnt = 0;
					current_data_src_node_id = metaSrc.src_rank;
					current_data_src_port = metaSrc.src_port;
					current_data_dst_port = metaSrc.dst_port;

					fsmMpeState = RECV_DATA_RD;
					//read_timeout_cnt = 0;
				  } else {
					  //we received another header and queued it
					  fsmMpeState = RECV_DATA_START;
				  }
    		 }
    	 }
    	break;

    case RECV_DATA_RD:
//      if(recvDeqFsm == DEQ_DONE)
//      {
//        fsmMpeState = RECV_DATA_DONE;
//        recvDeqFsm = DEQ_IDLE;
//        break;
//      }
//      if( !rx_overflow_fifo.empty() )
//      {
//        while(!rx_overflow_fifo.empty())
//        {
//          uint32_t current_word = rx_overflow_fifo.read();
//          if(!sFifoDataRX.write_nb(current_word))
//          {
//            rx_overflow_fifo.write(current_word);
//            break;
//          }
//        }
//      }
      if( !siTcp_data.empty() && !sFifoDataRX.full() 
         // && rx_overflow_fifo.empty()
        )
      {
        NetworkWord word = siTcp_data.read();
        printf("READ: tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        //convertAxisToMpiWidth(word, sFifoDataRX);
//        for(int i = 0; i < 2; i++)
//        {
//          //#pragma HLS unroll factor=2
//          if((word.tkeep >> i*4) == 0)
//          {
//            continue;
//          }
//          //with swap
//          //bufferIn[bufferInPtrWrite] = (ap_uint<8>) (big.tdata >> (7-i)*8);
//          //default
//          ap_uint<32> current_word = (ap_uint<32>) (word.tdata >> i*32);
        	sFifoDataRX.write(word.tdata);
          enqueue_recv_total_cnt++;
//          if(!sFifoDataRX.write_nb(current_word))
//          {
//            rx_overflow_fifo.write(current_word);
//          }
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
      //}
      break;
    case RECV_DATA_WRD:
      //wait for dequeue FSM
      if(!sDeqRecvDone.empty())
      {
    	  if(sDeqRecvDone.read())
    	  {
           fsmMpeState = RECV_DATA_DONE;
    	  }
      }
      break;
    case RECV_DATA_DONE:
      //if(fsmReceiveState == READ_STANDBY && !soTcp_meta.full() && !sFifoDataTX.full() )
      if( //!soTcp_meta.full() &&
    		  !sFifoDataTX.full()
			  && !sDeqSendDestId.full()
			  )
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
        //metaDst = NetworkMeta(header.dst_rank, ZRLMPI_DEFAULT_PORT, header.src_rank, ZRLMPI_DEFAULT_PORT, 0); //we set tlast
        //soTcp_meta.write(NetworkMetaStream(metaDst));
        sDeqSendDestId.write(header.dst_rank);

        header_i_cnt = 0;

        //write header
       // for(int i = 0; i < MPIF_HEADER_LENGTH/4; i++)
      //  {
          //Axis<8> tmp = Axis<8>(bytes[i]);
          Axis<64> tmp = Axis<64>();
          tmp.tdata = 0x0;
          tmp.tkeep = 0xFFF;
          for(int j = 0; j<8; j++)
          {
#pragma HLS unroll
            tmp.tdata |= ((ap_uint<64>) bytes[header_i_cnt*8+j]) << j*8;
          }
          tmp.tlast = 0;
//          if ( i == (MPIF_HEADER_LENGTH/4) - 1)
//          {
//            tmp.tlast = 1;
//          }
          sFifoDataTX.write(tmp);
          header_i_cnt = 1;

          //printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        //}
        //handshakeLinesCnt = (MPIF_HEADER_LENGTH + 7) /8;

//        //dequeue
//        if( !soTcp_data.full() && !sFifoDataTX.empty() )
//        {
//          NetworkWord word = NetworkWord();
//          convertAxisToNtsWidth(sFifoDataTX, word);
//          printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
//          soTcp_data.write(word);
//          handshakeLinesCnt--;
//
//        }

        fsmMpeState = SEND_ACK_0;
      }
      break;
    case SEND_ACK_0:
    	if(!sFifoDataTX.full())
    	{
    		Axis<64> tmp = Axis<64>();
        tmp.tdata = 0x0;
        tmp.tkeep = 0xFFF;
        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          tmp.tdata |= ((ap_uint<64>) bytes[header_i_cnt*8+j]) << j*8;
        }
        tmp.tlast = 0;
        header_i_cnt++;

            if ( header_i_cnt >= (MPIF_HEADER_LENGTH/8))
            {
              tmp.tlast = 1;
              fsmMpeState = SEND_ACK;
            }
            sFifoDataTX.write(tmp);
    	}
    	break;
      case SEND_ACK:
          //dequeue
//          if( !soTcp_data.full() && !sFifoDataTX.empty() )
//          {
//            NetworkWord word = NetworkWord();
//            convertAxisToNtsWidth(sFifoDataTX, word);
//            printf("tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
//            soTcp_data.write(word);
//            handshakeLinesCnt--;
//
//          }
//          if( handshakeLinesCnt <= 0 || sFifoDataTX.empty())
//          {
//            fsmMpeState = IDLE;
//          }
    	  if(!sDeqSendDone.empty())
    	  {
    		  if(sDeqSendDone.read())
    		  {
    			  fsmMpeState = IDLE;
    		  }
    	  }
          break;
//    case RECV_DATA_ERROR:
//      //empty streams
//      printf("Read error occured.\n");
//      if( !siTcp_meta.empty())
//      {
//        siTcp_meta.read();
//      }
//
//      if( !siTcp_data.empty())
//      {
//        siTcp_data.read();
//      } else {
//        fsmMpeState = IDLE;
//      }
//      break;

  }
  printf("fsmMpeState after FSM: %d\n", fsmMpeState);

}


void mpe_main(
    // ----- NAL Interface -----
    stream<NetworkWord>            &siTcp_data,
    stream<NetworkMetaStream>      &siTcp_meta,
    stream<NetworkWord>            &soTcp_data,
    stream<NetworkMetaStream>      &soTcp_meta,
    ap_uint<32>                   *po_rx_ports,

    ap_uint<32> *own_rank,
    // ----- for debugging  ------
    //ap_uint<32> *MMIO_out,

    // ----- MPI_Interface -----
    stream<MPI_Interface> &siMPIif,
    stream<Axis<64> > &siMPI_data,
    stream<Axis<64> > &soMPI_data
    )
{
#pragma HLS INTERFACE axis register both port=siTcp_data
#pragma HLS INTERFACE axis register both port=siTcp_meta
#pragma HLS INTERFACE axis register both port=soTcp_data
#pragma HLS INTERFACE axis register both port=soTcp_meta
#pragma HLS INTERFACE ap_ovld register port=po_rx_ports name=poROL_NRC_Rx_ports

#pragma HLS INTERFACE ap_vld register port=own_rank name=piFMC_rank

  //#pragma HLS INTERFACE ap_ovld register port=MMIO_out name=poMMIO

#pragma HLS INTERFACE ap_fifo port=siMPIif
#pragma HLS DATA_PACK     variable=siMPIif
#pragma HLS INTERFACE ap_fifo port=siMPI_data
#pragma HLS DATA_PACK     variable=siMPI_data
#pragma HLS INTERFACE ap_fifo port=soMPI_data
#pragma HLS DATA_PACK     variable=soMPI_data

  //===========================================================
  // Core-wide pragmas

#pragma HLS DATAFLOW
#pragma HLS INTERFACE ap_ctrl_none port=return


  //===========================================================
  // static variables

	static stream<Axis<64> > sFifoDataTX("sFifoDataTX");
	static stream<uint64_t> sFifoDataRX("sFifoDataRX");
	static stream<NodeId>        sDeqSendDestId("sDeqSendDestId");
	static stream<bool>          sDeqSendDone("sDeqSendDone");
	static stream<uint16_t>  	 sExpectedLength("sExpectedLength"); //in LINES!
	static stream<bool>      	 sDeqRecvDone("sDeqRecvDone");

#pragma HLS STREAM variable=sFifoDataTX     depth=128
#pragma HLS STREAM variable=sFifoDataRX     depth=128
#pragma HLS STREAM variable=sDeqSendDestId  depth=2
#pragma HLS STREAM variable=sDeqSendDone    depth=2
#pragma HLS STREAM variable=sExpectedLength depth=2
#pragma HLS STREAM variable=sDeqRecvDone    depth=2


  //===========================================================
  // Assign Debug Port
  //
  //    uint32_t debug_out = 0;
  //    //debug_out = fsmReceiveState;
  //    //debug_out |= ((uint32_t) fsmSendState) << 8;
  //    debug_out |= ((uint32_t) fsmMpeState) << 16;
  //    debug_out |= 0xAC000000;
  //
  //    *MMIO_out = (ap_uint<32>) debug_out;


  *po_rx_ports = 0x1; //currently work only with default ports...

  //===========================================================
  // MPE GLOBAL STATE

  pMpeGlobal(siMPIif, own_rank, sFifoDataTX, sDeqSendDestId, sDeqSendDone,
		     siTcp_data, siTcp_meta, siMPI_data, sFifoDataRX,
			 sExpectedLength, sDeqRecvDone);

  //===========================================================
  // DEQUEUE FSM SEND

  pDeqSend(sFifoDataTX, soTcp_data, soTcp_meta, own_rank, sDeqSendDestId,
		   sDeqSendDone);

  //===========================================================
  // DEQUEUE FSM RECV

  pDeqRecv(sFifoDataRX, soMPI_data, sExpectedLength, sDeqRecvDone);

}



