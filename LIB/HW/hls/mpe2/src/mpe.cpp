
#include "mpe.hpp"
#include <stdint.h>


using namespace hls;



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


uint16_t get_next_cache_line(
    ap_uint<256> header_cache[HEADER_CACHE_LENGTH],
    bool header_cache_valid[HEADER_CACHE_LENGTH],
    uint16_t start_value,
    ap_uint<256> &next_cache_line
    )
{
#pragma HLS inline off

  uint16_t ret = INVALID_CACHE_LINE_NUMBER;
  for(uint16_t i = 0; i < HEADER_CACHE_LENGTH; i++)
    //for(uint16_t i = start_value; i < HEADER_CACHE_LENGTH; i++)
  {
#pragma HLS unroll
    if(i < start_value)
    {
      continue;
    }
    if(header_cache_valid[i])
    {
      next_cache_line = header_cache[i];
      ret = i;
      break;
    }
  }
  return ret;
}


void add_cache_line(
    ap_uint<256> header_cache[HEADER_CACHE_LENGTH],
    bool header_cache_valid[HEADER_CACHE_LENGTH],
    ap_uint<256> &new_cache_line
    )
{
#pragma HLS inline off
  for(uint16_t i = 0; i < HEADER_CACHE_LENGTH; i++)
  {
#pragma HLS unroll
    if(!header_cache_valid[i])
    {
      header_cache[i] = new_cache_line;
      header_cache_valid[i] = true;
      break;
    }
  }

}

void delete_cache_line(
    ap_uint<256> header_cache[HEADER_CACHE_LENGTH],
    bool header_cache_valid[HEADER_CACHE_LENGTH],
    uint16_t index_to_delete
    )
{
#pragma HLS inline off
  if(index_to_delete < HEADER_CACHE_LENGTH)
  {
    header_cache_valid[index_to_delete] = false;
  }
}


//returns: 0 ok, 1 not ok
uint8_t checkHeader(ap_uint<8> bytes[MPIF_HEADER_LENGTH], MPI_Header &header, NetworkMeta &metaSrc,
    packetType expected_type, mpiCall expected_call, bool skip_meta, uint32_t expected_src_rank)
{
#pragma HLS inline off
#pragma HLS pipeline II=1

  int ret = bytesToHeader(bytes, header);
  bool unexpected_header = false;

  if(ret != 0)
  {
    printf("invalid header.\n");
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
    unexpected_header = true;
  }
  //check data type 
  //if((currentDataType == MPI_INT && header.call != MPI_RECV_INT) || (currentDataType == MPI_FLOAT && header.call != MPI_RECV_FLOAT))
  else if(header.call != expected_call)
  {
    printf("receiver expects different data type: %d.\n", (int) header.call);
    unexpected_header = true;
  }

  if(unexpected_header)
  {

    return 1;
  }
  return 0;
}

mpeState checkCache(
    uint16_t    &last_checked_cache_line,
    ap_uint<256>  header_cache[HEADER_CACHE_LENGTH],
    bool      header_cache_valid[HEADER_CACHE_LENGTH],
    ap_uint<8>    bytes[MPIF_HEADER_LENGTH],
    MPI_Header    &header,
    NetworkMeta   &metaSrc,
    packetType      expected_type,
    mpiCall         expected_call,
    uint32_t        expected_src_rank,
    const mpeState  found_state,
    const mpeState  miss_state,
    const mpeState  stay_state
    )
{
#pragma HLS inline off
  //#pragma HLS pipeline II=1


  ap_uint<256> next_cache_line = 0x0;
  uint16_t next_cache_line_number = INVALID_CACHE_LINE_NUMBER;
  uint8_t ret = 255;
  mpeState ret_val = IDLE;

  next_cache_line_number = get_next_cache_line(header_cache, header_cache_valid,
      last_checked_cache_line, next_cache_line);
  if(next_cache_line_number != INVALID_CACHE_LINE_NUMBER)
  {
    printf("check cache line %d\n", (uint16_t) next_cache_line_number);
    last_checked_cache_line = next_cache_line_number;
    printf("check cache entry: ");
    for(int j = 0; j < MPIF_HEADER_LENGTH; j++)
    {
#pragma HLS unroll
      bytes[j] = (uint8_t) (next_cache_line >> (31-j)*8);
      printf("%02x", (int) bytes[j]);
    }
    printf("\n");
    ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, true, expected_src_rank);
    if(ret == 0)
    {//found desired header
      printf("Cache HIT\n");
      ret_val = found_state;
      delete_cache_line(header_cache, header_cache_valid, last_checked_cache_line);
    } else {
      //else, we continue
      ret_val = stay_state;
    }
    last_checked_cache_line++;
  } else {
    ret_val = miss_state;
  }
  return ret_val;
}


void add_cache_bytes(
    ap_uint<256>  header_cache[HEADER_CACHE_LENGTH],
    bool      header_cache_valid[HEADER_CACHE_LENGTH],
    ap_uint<8>    bytes[MPIF_HEADER_LENGTH]
    )
{
#pragma HLS inline off
  //#pragma HLS pipeline II=1
  ap_uint<256> cache_tmp = 0x0;
  printf("[pMpeGlobal] adding data to cache:\n\t");
  for(int j = 0; j<MPIF_HEADER_LENGTH; j++)
  {
#pragma HLS unroll
    cache_tmp |= ((ap_uint<256>) bytes[j]) << (31-j)*8;
    printf("%02x", (int) bytes[j]);
  }
  printf("\n");
  add_cache_line(header_cache, header_cache_valid, cache_tmp);
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
      if( !sFifoDataRX.empty() && !soMPI_data.full()
        )
      {
        Axis<64> tmp = Axis<64>();

        uint64_t new_data = sFifoDataRX.read();

        tmp.tdata = new_data;
        tmp.tkeep = 0xFFF;

        recv_total_cnt++; //we are counting LINES!
        //if(tmp.tlast == 1)
        if(recv_total_cnt >= (expected_recv_count))
        {
          printf("[pDeqRecv] [MPI_Recv] expected byte count reached.\n");
          //word_tlast_occured = true;
          tmp.tlast = 1;
          recvDeqFsm = DEQ_DONE;
        } else {
          //in ALL other cases
          tmp.tlast = 0;
        }
        printf("[pDeqRecv] toAPP: tkeep %#04x, tdata %#0llx, tlast %d\n",(int) tmp.tkeep, (unsigned long long) tmp.tdata, (int) tmp.tlast);
        soMPI_data.write(tmp);
        //cnt++;
        //}
  }
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
      if( !soTcp_data.full() && !sFifoDataTX.empty()
          && !soTcp_meta.full() )
      {
        NetworkWord word = NetworkWord();
        word.tdata = 0x0;
        word.tlast = 0x0;
        word.tkeep = 0x0;

        Axis<64> tmpl1 = Axis<64>();
        tmpl1 = sFifoDataTX.read();
        word.tdata = tmpl1.tdata;
        word.tkeep = 0xFFF;
        word.tlast = tmpl1.tlast;

        //check before we split in parts
        if(word.tlast == 1)
        {
          printf("[pDeqSend] SEND_DATA finished writing.\n");
          //word_tlast_occured = true;
          sendDeqFsm = DEQ_DONE;
        }

        //if(current_packet_line_cnt >= ZRLMPI_MAX_MESSAGE_SIZE_LINES)
        if(current_packet_line_cnt == 0)
        {//last write was last one or first one
          NetworkMeta metaDst = NetworkMeta(current_send_dst_id, ZRLMPI_DEFAULT_PORT, *own_rank, ZRLMPI_DEFAULT_PORT, 0);
          soTcp_meta.write(NetworkMetaStream(metaDst));
          printf("[pDeqSend] started new DATA part packet\n");
        }

        if(current_packet_line_cnt >= (ZRLMPI_MAX_MESSAGE_SIZE_LINES - 1))
        {//last one in this packet
          word.tlast = 1;
          current_packet_line_cnt = 0;
        } else {
          current_packet_line_cnt++;
        }

        printf("[pDeqSend] tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);
        soTcp_data.write(word);
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
    ap_uint<32>                   *po_rx_ports,
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
  //#pragma HLS pipeline II=1 //TODO
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static mpeState fsmMpeState = MPE_RESET;
  static bool cache_invalidated = false;

#pragma HLS reset variable=fsmMpeState
#pragma HLS reset variable=cache_invalidated
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  //static stream<ap_uint<128> > sFifoHeaderCache("sFifoHeaderCache");
  static uint32_t expected_src_rank = 0;
  //static uint16_t current_cache_data_cnt = 0;

  static ap_uint<256> header_cache[HEADER_CACHE_LENGTH];
  static bool header_cache_valid[HEADER_CACHE_LENGTH];
  static uint16_t last_checked_cache_line = 0;


  static MPI_Interface currentInfo = MPI_Interface();
  static mpiType currentDataType = MPI_INT;
  //static int handshakeLinesCnt = 0;

  static uint8_t header_i_cnt = 0;
  static ap_uint<8> bytes[MPIF_HEADER_LENGTH];

  static mpiCall expected_call = 0;
  static packetType expected_type = 0;
  //static uint16_t checked_entries = 0;


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

  // #pragma HLS STREAM variable=sFifoHeaderCache depth=64
  //#pragma HLS STREAM variable=sFifoHeaderCache depth=2048 //HEADER_CACHE_LENTH*MPIF_HEADER_LENGTH
#pragma HLS array_partition variable=bytes complete dim=0
  //#pragma HLS array_partition variable=header_cache cyclic factor=32 dim=0
  //#pragma HLS array_partition variable=header_cache_valid cyclic factor=32 dim=0

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------

  uint8_t ret;
  Axis<64> current_read_word = Axis<64>();


  *po_rx_ports = 0x1; //currently work only with default ports...


  switch(fsmMpeState)
  {
    default:
    case MPE_RESET:
      if(!cache_invalidated)
      {
        for(int i = 0; i < HEADER_CACHE_LENGTH; i++)
        {
          header_cache_valid[i] = false;
        }
        cache_invalidated = true;
        fsmMpeState = IDLE;
      } else {
        fsmMpeState = IDLE;
      }
      break;
    case IDLE:
      expected_src_rank = 0xFFF;
      if ( !siMPIif.empty() )
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


        sDeqSendDestId.write((NodeId) header.dst_rank);
        header_i_cnt = 0;

        //write header
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
        printf("tdata64: %#0llx\n",(uint64_t) tmp.tdata);
        tmp.tlast = 0;

        sFifoDataTX.write(tmp);
        //printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        header_i_cnt++;

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
        printf("tdata64: %#0llx\n",(uint64_t) tmp.tdata);
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
      if(!sDeqSendDone.empty())
      {
        if(sDeqSendDone.read())
        {
          fsmMpeState = WAIT4CLEAR_CACHE;
          last_checked_cache_line = 0;
          header = MPI_Header();
          expected_call = MPI_RECV_INT;
          if(currentDataType == MPI_FLOAT)
          {
            expected_call = MPI_RECV_FLOAT;
          }
          expected_type = CLEAR_TO_SEND;
        }
      }
      break;
    case WAIT4CLEAR_CACHE:
      //check cache first
      fsmMpeState = checkCache(last_checked_cache_line, header_cache, header_cache_valid,
          bytes, header, metaSrc, expected_type, expected_call, expected_src_rank,
          SEND_DATA_START, WAIT4CLEAR, WAIT4CLEAR_CACHE);
      break;
    case WAIT4CLEAR:
      if( !siTcp_data.empty() && !siTcp_meta.empty()
        )
      {
        //start read header
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
      if( !siTcp_data.empty() //&& !siTcp_meta.empty()
        )
      {
        //read header cont.
        NetworkWord tmp = siTcp_data.read();

        printf("Data read: tkeep %#03x, tdata %#016llx, tlast %d\n",(int) tmp.tkeep, (unsigned long long) tmp.tdata, (int) tmp.tlast);
        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          bytes[header_i_cnt*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
        header_i_cnt++;

        if(header_i_cnt >= (MPIF_HEADER_LENGTH+7)/8)
        {
          ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
          if(ret == 0)
          {//got CLEAR_TO_SEND 
            printf("Got CLEAR to SEND\n");
            fsmMpeState = SEND_DATA_START;
          } else {
            //else, we continue to wait
            //and add it to the cache
            add_cache_bytes(header_cache, header_cache_valid, bytes);
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
        header_i_cnt = 1;

        sDeqSendDestId.write((NodeId) header.dst_rank);

        expected_send_count = header.size; //in WORDS
        printf("[MPI_send] expect %d words.\n",expected_send_count);
        send_total_cnt = 0;

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
      break;
    case SEND_DATA_RD:
      if(!sFifoDataTX.full() && !siMPI_data.empty())
      {
        current_read_word = siMPI_data.read();
        //send_total_cnt += 8; //we read EIGHT BYTES per line
        send_total_cnt += 2; //two WORDS ber line
        if(send_total_cnt >= (expected_send_count))
        {// to be sure...
          current_read_word.tlast = 1;
        }

        if(current_read_word.tlast == 1)
        {
          fsmMpeState = SEND_DATA_WRD;
          printf("\t[pMpeGlobal] tlast Occured.\n");
        }
        sFifoDataTX.write(current_read_word);
        printf("\t[pMpeGlobal] MPI read data: %#08x, tkeep: %d, tlast %d\n", (unsigned long long) current_read_word.tdata, (int) current_read_word.tkeep, (int) current_read_word.tlast);
      }
      break;
    case SEND_DATA_WRD:
      //wait for dequeue fsm
      if(!sDeqSendDone.empty())
      {
        if(sDeqSendDone.read())
        {
          fsmMpeState = WAIT4ACK_CACHE;
          header = MPI_Header();
          last_checked_cache_line = 0;
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
      fsmMpeState = checkCache(last_checked_cache_line, header_cache, header_cache_valid,
          bytes, header, metaSrc, expected_type, expected_call, expected_src_rank,
          IDLE, WAIT4ACK, WAIT4ACK_CACHE);
      break;
    case WAIT4ACK:
      if( !siTcp_data.empty() && !siTcp_meta.empty() )
      {
        //read header
        NetworkWord tmp = siTcp_data.read();
        header_i_cnt = 0;

        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          bytes[header_i_cnt*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
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
          ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
          if(ret == 0)
          {
            printf("ACK received.\n");
            fsmMpeState = IDLE;
          } else {
            //else, we continue to wait
            //and add it to the cache
            add_cache_bytes(header_cache, header_cache_valid, bytes);
            fsmMpeState = WAIT4ACK;
          }
        }
      }
      break;
    case START_RECV:
      header = MPI_Header();
      last_checked_cache_line = 0;
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
      fsmMpeState = checkCache(last_checked_cache_line, header_cache, header_cache_valid,
          bytes, header, metaSrc, expected_type, expected_call, expected_src_rank,
          ASSEMBLE_CLEAR, WAIT4REQ, WAIT4REQ_CACHE);

      break;
    case WAIT4REQ:
      if( !siTcp_data.empty() && !siTcp_meta.empty() )
      {
        //read header
        NetworkWord tmp = siTcp_data.read();
        header_i_cnt = 0;
        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          bytes[header_i_cnt*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          //bytes[i*8 + 7 -j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
        header_i_cnt = 1;

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
          ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
          if(ret == 0)
          {
            //got SEND_REQUEST 
            printf("Got SEND REQUEST\n");
            fsmMpeState = ASSEMBLE_CLEAR;
          } else {
            //else, we wait...
            //and add it to the cache
            ap_uint<256> cache_tmp = 0x0;
            add_cache_bytes(header_cache, header_cache_valid, bytes);
            fsmMpeState = WAIT4REQ;
          }
        }
      }
      break;
    case ASSEMBLE_CLEAR:
      if(//!soTcp_meta.full() &&
          !sFifoDataTX.full()
          && !sDeqSendDestId.full()
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

        sDeqSendDestId.write((NodeId) header.dst_rank);

        header_i_cnt = 0;

        //write header
        Axis<64> tmp = Axis<64>();
        tmp.tdata = 0x0;
        tmp.tkeep = 0xFFF;
        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          tmp.tdata |= ((ap_uint<64>) bytes[header_i_cnt*8+j]) << j*8;
        }
        tmp.tlast = 0;
        sFifoDataTX.write(tmp);
        header_i_cnt = 1;

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

          NetworkWord tmp = siTcp_data.read();

          for(int j = 0; j<8; j++)
          {
#pragma HLS unroll
            bytes[header_i_cnt*8 + j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
            //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          }
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
          ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
          if(ret == 0
              && !expect_more_data) //we don't start a new data packet here
          {
            //valid header && valid source
            //expected_recv_count = header.size;
            //uint16_t expected_length_in_lines = (header.size+7)/8;
            uint16_t expected_length_in_lines = (header.size+1)/2;
            printf("[pMpeGlobal] expect %d LINES of data.\n", expected_length_in_lines);
            sExpectedLength.write(expected_length_in_lines);

            exp_recv_count_enqueue = header.size; //in WORDS!!
            printf("[MPI_Recv] expect %d words.\n",exp_recv_count_enqueue);
            //recv_total_cnt = 0;
            enqueue_recv_total_cnt = 0;
            current_data_src_node_id = metaSrc.src_rank;
            current_data_src_port = metaSrc.src_port;
            current_data_dst_port = metaSrc.dst_port;

            fsmMpeState = RECV_DATA_RD;
            //read_timeout_cnt = 0;
          } else {
            //we received another header and add it to the cache
            ap_uint<256> cache_tmp = 0x0;
            add_cache_bytes(header_cache, header_cache_valid, bytes);
            fsmMpeState = RECV_DATA_START;
          }
        }
      }
      break;

    case RECV_DATA_RD:
      if( !siTcp_data.empty() && !sFifoDataRX.full() 
        )
      {
        NetworkWord word = siTcp_data.read();
        printf("\t[pMpeGlobal] READ: tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.tkeep, (unsigned long long) word.tdata, (int) word.tlast);

        sFifoDataRX.write(word.tdata);
        //enqueue_recv_total_cnt++;
        enqueue_recv_total_cnt += 2; //two WORDS per line
        //check if we have to receive a new packet meta
        if(word.tlast == 1)
        {
          if(enqueue_recv_total_cnt < exp_recv_count_enqueue) //not <=
          {//we expect more
            fsmMpeState = RECV_DATA_START;
            expect_more_data = true;
            printf("\t[pMpeGlobal] we expect more: received: %d, expected %d;", enqueue_recv_total_cnt, exp_recv_count_enqueue);
          } else {
            fsmMpeState = RECV_DATA_WRD;
          }
        }
      }
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

        sDeqSendDestId.write(header.dst_rank);

        header_i_cnt = 0;

        Axis<64> tmp = Axis<64>();
        tmp.tdata = 0x0;
        tmp.tkeep = 0xFFF;
        for(int j = 0; j<8; j++)
        {
#pragma HLS unroll
          tmp.tdata |= ((ap_uint<64>) bytes[header_i_cnt*8+j]) << j*8;
        }
        tmp.tlast = 0;
        sFifoDataTX.write(tmp);
        header_i_cnt = 1;


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
      if(!sDeqSendDone.empty())
      {
        if(sDeqSendDone.read())
        {
          fsmMpeState = IDLE;
        }
      }
      break;
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
  static stream<uint16_t>    sExpectedLength("sExpectedLength"); //in LINES!
  static stream<bool>        sDeqRecvDone("sDeqRecvDone");

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


  //===========================================================
  // MPE GLOBAL STATE

  pMpeGlobal(po_rx_ports, siMPIif, own_rank, sFifoDataTX,
      sDeqSendDestId, sDeqSendDone, siTcp_data, siTcp_meta,
      siMPI_data, sFifoDataRX,
      sExpectedLength, sDeqRecvDone);

  //===========================================================
  // DEQUEUE FSM SEND

  pDeqSend(sFifoDataTX, soTcp_data, soTcp_meta, own_rank, sDeqSendDestId,
      sDeqSendDone);

  //===========================================================
  // DEQUEUE FSM RECV

  pDeqRecv(sFifoDataRX, soMPI_data, sExpectedLength, sDeqRecvDone);

}



