
#include "mpe.hpp"
#include <stdint.h>


using namespace hls;



uint8_t extractByteCnt(Axis<64> currWord)
{
#pragma HLS INLINE

  uint8_t ret = 0;

  switch (currWord.getTKeep()) {
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


//uint16_t get_next_cache_line(
//    ap_uint<256> header_cache[HEADER_CACHE_LENGTH],
//    bool header_cache_valid[HEADER_CACHE_LENGTH],
//    uint16_t start_value,
//    ap_uint<256> &next_cache_line
//    )
//{
//#pragma HLS inline off
////#pragma HLS pipeline II=1
//
//  uint16_t ret = INVALID_CACHE_LINE_NUMBER;
//  for(uint16_t i = 0; i < HEADER_CACHE_LENGTH; i++)
//    //for(uint16_t i = start_value; i < HEADER_CACHE_LENGTH; i++)
//  {
//#pragma HLS unroll
//    if(i < start_value)
//    {
//      continue;
//    }
//    if(header_cache_valid[i])
//    {
//      next_cache_line = header_cache[i];
//      ret = i;
//      break;
//    }
//  }
//  return ret;
//}

//returns the ZERO-based bit position (so 0 for LSB)
//0xFE for no bit
//uint8_t getRightmostBitPos(ap_uint<64> num)
//{
//#pragma HLS INLINE// off
//  ap_uint<64> one_hot = num & -num;
//  if(one_hot == 0)
//  {
//    return 0xFE;
//  }
//  uint8_t pos = 0;
//  for (int i = 0; i < 64; i++)
//  {
//#pragma HLS unroll
//    if(one_hot <= 1)
//    {
//      break;
//    }
//    pos++;
//    one_hot >>= 1;
//  }
//  //printf("[NAL:RigthmostBit] returning %d for input %d\n", (int) pos, (int) num);
//  return pos;
//}
//
////returns the ZERO-based bit position (so 0 for LSB)
//uint8_t getRightmostFREEPos(ap_uint<64> num)
//{
//#pragma HLS INLINE// off
//  ap_uint<64> one_hot = num & -num;
//  if(one_hot == 0)
//  {
//    return 0;
//  }
//  uint8_t pos = 0;
//  for (int i = 0; i < 64; i++)
//  {
//#pragma HLS unroll
//    if(one_hot == 0)
//    {
//      break;
//    }
//    pos++;
//    one_hot >>= 1;
//  }
//  //printf("[NAL:RigthmostBit] returning %d for input %d\n", (int) pos, (int) num);
//  return pos;
//}
//
//uint64_t array_to_vector(bool header_cache_valid[HEADER_CACHE_LENGTH])
//{
//#pragma HLS inline
//
//#ifndef __SYNTHESIS__
//  if(HEADER_CACHE_LENGTH != 64)
//  {
//    printf("ERROR: Header cache is only working for pipeline II=1 with length 64");
//    exit(-1);
//  }
//#endif
//
//  uint64_t ret = 0;
//  for(int i = 0; i<64; i++)
//  {
//    #pragma HLS unroll
//    if(header_cache_valid[i])
//    {
//      ret |= ((uint64_t) 1) << (64 - i);
//    }
//  }
//return ret;
//
//}

//uint16_t get_next_cache_line(
//    ap_uint<256> header_cache[HEADER_CACHE_LENGTH],
//    bool header_cache_valid[HEADER_CACHE_LENGTH],
//    uint16_t start_value,
//    ap_uint<256> &next_cache_line
//    )
//{
//#pragma HLS inline
////#pragma HLS pipeline II=1
//
//  uint64_t encoded_vec = array_to_vector(header_cache_valid);
//  //printf("\t\t\t\tencoded vec: %#016llx\n",encoded_vec);
//
//  uint64_t start_value_encoded = (0xFFFFFFFFFFFFFFFF) << start_value;
//
//  encoded_vec &= start_value_encoded;
//  //printf("\t\t\t\tencoded vec with start value: %#016llx, start value %d\n",encoded_vec, start_value);
//
//  uint8_t pos = getRightmostBitPos(encoded_vec);
//  //printf("\t\t\t\t[get_next_cache_line] pos: %d\n",pos);
//  if(pos == 0xFE)
//  {
//    return INVALID_CACHE_LINE_NUMBER;
//  }
//  if(pos < HEADER_CACHE_LENGTH)
//  {
//    next_cache_line = header_cache[pos];
//  }
//  return pos;
//}
//
//void add_cache_line(
//    ap_uint<256> header_cache[HEADER_CACHE_LENGTH],
//    bool header_cache_valid[HEADER_CACHE_LENGTH],
//    ap_uint<256> &new_cache_line
//    )
//{
//#pragma HLS inline //off
//
//  uint64_t encoded_vec = array_to_vector(header_cache_valid);
//  //printf("\t\t\t\tencoded vec: %#016llx\n",encoded_vec);
//
//  uint8_t pos = getRightmostFREEPos(encoded_vec);
//  //printf("\t\t\t\t[add_cache_line] pos: %d\n",pos);
//
//  if( pos < HEADER_CACHE_LENGTH )
//  {
//      header_cache[pos] = new_cache_line;
//      header_cache_valid[pos] = true;
//  }
//
//}

//void add_cache_line(
//    ap_uint<256> header_cache[HEADER_CACHE_LENGTH],
//    bool header_cache_valid[HEADER_CACHE_LENGTH],
//    ap_uint<256> &new_cache_line
//    )
//{
//#pragma HLS inline off
//  for(uint16_t i = 0; i < HEADER_CACHE_LENGTH; i++)
//  {
//#pragma HLS unroll
//    if(!header_cache_valid[i])
//    {
//      header_cache[i] = new_cache_line;
//      header_cache_valid[i] = true;
//      break;
//    }
//  }
//
//}


void add_cache_line(
    ap_uint<256> header_cache[HEADER_CACHE_LENGTH],
    bool header_cache_valid[HEADER_CACHE_LENGTH],
    ap_uint<256> &new_cache_line,
    uint16_t remote_rank
    )
{
#pragma HLS inline
  if(remote_rank < HEADER_CACHE_LENGTH)
  {
    header_cache[remote_rank] = new_cache_line;
    header_cache_valid[remote_rank] = true;
    printf("\tadding line to cache @ %d\n", remote_rank);
  }

}

void delete_cache_line(
    ap_uint<256> header_cache[HEADER_CACHE_LENGTH],
    bool header_cache_valid[HEADER_CACHE_LENGTH],
    uint16_t index_to_delete
    )
{
#pragma HLS inline
  if(index_to_delete < HEADER_CACHE_LENGTH)
  {
    header_cache_valid[index_to_delete] = false;
  }
}


//returns: 0 ok, 1 not ok, 2 invalid
uint8_t checkHeader(ap_uint<8> bytes[MPIF_HEADER_LENGTH], MPI_Header &header, NetworkMeta &metaSrc,
    packetType expected_type, mpiCall expected_call, bool skip_meta, uint32_t expected_src_rank)
{
#pragma HLS inline //off
//#pragma HLS pipeline II=1
#pragma HLS latency max=1

  int ret = bytesToHeader(bytes, header);
  bool unexpected_header = false;

  if(ret != 0 || header.src_rank >= ZRLMPI_MAX_CLUSTER_SIZE)
  {
    printf("invalid header.\n");
    //unexpected_header = true;
    return 2;
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

//mpeState checkCache(
//    uint16_t    &last_checked_cache_line,
//    ap_uint<256>  header_cache[HEADER_CACHE_LENGTH],
//    bool      header_cache_valid[HEADER_CACHE_LENGTH],
//    ap_uint<8>    bytes[MPIF_HEADER_LENGTH],
//    MPI_Header    &header,
//    NetworkMeta   &metaSrc,
//    packetType      expected_type,
//    mpiCall         expected_call,
//    uint32_t        expected_src_rank,
//    const mpeState  found_state,
//    const mpeState  miss_state,
//    const mpeState  stay_state
//    )
//{
//#pragma HLS inline off
////#pragma HLS pipeline II=1
//
//
//  ap_uint<256> next_cache_line = 0x0;
//  uint16_t next_cache_line_number = INVALID_CACHE_LINE_NUMBER;
//  uint8_t ret = 255;
//  mpeState ret_val = IDLE;
//
//  next_cache_line_number = get_next_cache_line(header_cache, header_cache_valid,
//      last_checked_cache_line, next_cache_line);
//  if(next_cache_line_number != INVALID_CACHE_LINE_NUMBER)
//  {
//    printf("check cache line %d\n", (uint16_t) next_cache_line_number);
//    last_checked_cache_line = next_cache_line_number;
//    printf("check cache entry: ");
//    for(int j = 0; j < MPIF_HEADER_LENGTH; j++)
//    {
//#pragma HLS unroll
//      bytes[j] = (uint8_t) (next_cache_line >> (31-j)*8);
//      printf("%02x", (int) bytes[j]);
//    }
//    printf("\n");
//    ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, true, expected_src_rank);
//    if(ret == 0)
//    {//found desired header
//      printf("Cache HIT\n");
//      ret_val = found_state;
//      delete_cache_line(header_cache, header_cache_valid, last_checked_cache_line);
//    } else {
//      //else, we continue
//      ret_val = stay_state;
//    }
//    last_checked_cache_line++;
//  } else {
//    ret_val = miss_state;
//  }
//  return ret_val;
//}

bool getCache(
    ap_uint<256>  header_cache[HEADER_CACHE_LENGTH],
    bool      header_cache_valid[HEADER_CACHE_LENGTH],
    ap_uint<8>    bytes[MPIF_HEADER_LENGTH],
    MPI_Header    &header,
    NetworkMeta   &metaSrc,
    packetType      expected_type,
    mpiCall         expected_call,
    uint32_t        expected_src_rank//,
    //const mpeState  found_state,
    //const mpeState  miss_state//,
    //const mpeState  stay_state
    )
{
#pragma HLS inline
//#pragma HLS pipeline II=1
#pragma HLS latency max=1

  if(expected_src_rank < HEADER_CACHE_LENGTH)
  {
    if(header_cache_valid[expected_src_rank])
    {
      ap_uint<256> check_cache_line = header_cache[expected_src_rank];
      uint8_t ret = 255;
      printf("check cache entry: ");
      for(int j = 0; j < MPIF_HEADER_LENGTH; j++)
      {
#pragma HLS unroll
        bytes[j] = (uint8_t) (check_cache_line >> (31-j)*8);
        printf("%02x", (int) bytes[j]);
      }
      printf("\n");
      //ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, true, expected_src_rank);
      //if(ret == 0)
      //{//found desired header
      //  printf("Cache HIT\n");
      //  //delete_cache_line(header_cache, header_cache_valid, expected_src_rank);
      //  //return found_state;
      //  return true;
      //} else {
      //  printf("\tCache MISS (wrong type)\n");
      //  //else, we haven't found it
      //  //thanks to BSP, there is no other place where it could be
      //  //return miss_state;
      //  return false;
      //}
      printf("Cache FOUND\n");
      return true;
    } else {
      printf("\tCache MISS (no entry @ %d)\n", expected_src_rank);
      //return miss_state;
      return false;
    }
  }
  printf("INVALID SRC RANK!\n");
  //return miss_state;
  return false;
}


void add_cache_bytes(
    ap_uint<256>  header_cache[HEADER_CACHE_LENGTH],
    bool      header_cache_valid[HEADER_CACHE_LENGTH],
    ap_uint<8>    bytes[MPIF_HEADER_LENGTH],
    uint32_t remote_rank
    )
{
#pragma HLS inline //off
#pragma HLS pipeline II=1

  ap_uint<256> cache_tmp = 0x0;
  printf("[pMpeGlobal] adding data to cache:\n\t");
  for(int j = 0; j<MPIF_HEADER_LENGTH; j++)
  {
#pragma HLS unroll
    cache_tmp |= ((ap_uint<256>) bytes[j]) << (31-j)*8;
    printf("%02x", (int) bytes[j]);
  }
  printf("\n");
  add_cache_line(header_cache, header_cache_valid, cache_tmp, remote_rank);
}


uint8_t getWordCntfromTKeep(ap_uint<16> tkeep)
{
#pragma HLS inline
#pragma HLS latency max=1
  if(tkeep > 0 && tkeep <= 0xF)
  {
    return 1;
  } else if(tkeep <= 0xFF)
  {
    return 2;
  } else if(tkeep <= 0xFFF)
  {
    return 3;
  } else if(tkeep <= 0xFFFF)
  {
    return 4;
  } else {
    return 0;
  }
}


void pEnqMpiData(
    stream<Axis<64> >  &siMPI_data,
    stream<Axis<128> > &sFifoMpiDataIn
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static deqState enqMpiDataFsm = DEQ_WRITE;
  //TODO: add state to drain FIFO after reset?
#pragma HLS reset variable=enqMpiDataFsm
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static uint64_t first_line = 0;
  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------

  switch(enqMpiDataFsm)
  {
    default:
    case DEQ_WRITE:
      if(!siMPI_data.empty() && !sFifoMpiDataIn.full())
      {
        Axis<128> tmp = Axis<128>();
        Axis<64> inWord = siMPI_data.read();
        if(inWord.getTLast() == 1)
        {
          ap_uint<128> tmp_tdata = 0;
          tmp_tdata |= (ap_uint<128>) inWord.getTData();
          tmp.setTData(tmp_tdata);
          tmp.setTKeep(0xFF);
          tmp.setTLast(1);
          sFifoMpiDataIn.write(tmp);
        printf("[pEnqMpiData] tkeep %#04x, tdata %#032llx, tlast %d\n",(int) tmp.getTKeep(), (unsigned long long) tmp.getTData(), (int) tmp.getTLast());
          //stay here
        } else {
          first_line = inWord.getTData();
          enqMpiDataFsm = DEQ_WRITE_2;
        }
      }
      break;
    case DEQ_WRITE_2:
      if(!siMPI_data.empty() && !sFifoMpiDataIn.full())
      {
        Axis<128> tmp = Axis<128>();
        Axis<64> inWord = siMPI_data.read();
        ap_uint<128> tmp_tdata = 0;
        tmp_tdata |= (ap_uint<128>) first_line;
        tmp_tdata |= ((ap_uint<128>) inWord.getTData()) << 64;
        tmp.setTData(tmp_tdata);
        tmp.setTKeep(0xFFFF);
        tmp.setTLast(inWord.getTLast());
        printf("[pEnqMpiData] tkeep %#04x, tdata %#032llx, tlast %d\n",(int) tmp.getTKeep(), (unsigned long long) tmp.getTData(), (int) tmp.getTLast());
        sFifoMpiDataIn.write(tmp);
        enqMpiDataFsm = DEQ_WRITE;
      }
      break;
  }

  printf("pEnqMpiDat after FSM: %d\n", enqMpiDataFsm);

}

void pEnqTcpIn(
    stream<NetworkWord> &siTcp_data,
    stream<NetworkMetaStream>      &siTcp_meta,
    stream<Axis<128> >  &sFifoTcpIn,
    stream<NetworkMetaStream>      &sFifoTcpMetaIn
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static deqState enqTcpDataFsm = DEQ_START;
  //TODO: add state to drain FIFO after reset?
#pragma HLS reset variable=enqTcpDataFsm
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static uint64_t first_line = 0;
  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------

  switch(enqTcpDataFsm)
  {
    default:
    case DEQ_START:
      if(!siTcp_data.empty() && !sFifoTcpIn.full()
          && !siTcp_meta.empty() && !sFifoTcpMetaIn.full()
          )
      {
        Axis<128> tmp = Axis<128>();
        NetworkWord inWord = siTcp_data.read();
        NetworkMetaStream meta_tmp = siTcp_meta.read();
        sFifoTcpMetaIn.write(meta_tmp);

        if(inWord.tlast == 1)
        {
          ap_uint<128> tmp_tdata = 0;
          tmp_tdata |= (ap_uint<128>) inWord.getTData();
          tmp.setTData(tmp_tdata);
          tmp.setTKeep(0xFF);
          tmp.setTLast(1);
          printf("[pEnqTcpIn] tkeep %#04x, tdata %#032llx, tlast %d\n",(int) tmp.getTKeep(), (unsigned long long) tmp.getTData(), (int) tmp.getTLast());
          sFifoTcpIn.write(tmp);
          //stay here
        } else {
          first_line = inWord.tdata;
          enqTcpDataFsm = DEQ_WRITE_2;
        }
      }
      break;

    case DEQ_WRITE:
      if(!siTcp_data.empty() && !sFifoTcpIn.full())
      {
        Axis<128> tmp = Axis<128>();
        NetworkWord inWord = siTcp_data.read();
        if(inWord.tlast == 1)
        {
          ap_uint<128> tmp_tdata = 0;
          tmp_tdata |= (ap_uint<128>) inWord.getTData();
          tmp.setTData(tmp_tdata);
          tmp.setTKeep(0xFF);
          tmp.setTLast(1);
          printf("[pEnqTcpIn] tkeep %#04x, tdata %#032llx, tlast %d\n",(int) tmp.getTKeep(), (unsigned long long) tmp.getTData(), (int) tmp.getTLast());
          sFifoTcpIn.write(tmp);
          enqTcpDataFsm = DEQ_START;
        } else {
          first_line = inWord.tdata;
          enqTcpDataFsm = DEQ_WRITE_2;
        }
      }
      break;
    case DEQ_WRITE_2:
      if(!siTcp_data.empty() && !sFifoTcpIn.full())
      {
        Axis<128> tmp = Axis<128>();
        NetworkWord inWord = siTcp_data.read();
        ap_uint<128> tmp_tdata = 0;
        tmp_tdata |= (ap_uint<128>) first_line;
        tmp_tdata |= ((ap_uint<128>) inWord.getTData()) << 64;
        tmp.setTData(tmp_tdata);
        tmp.setTKeep(0xFFFF);
        tmp.setTLast(inWord.getTLast());
        printf("[pEnqTcpIn] tkeep %#04x, tdata %#032llx, tlast %d\n",(int) tmp.getTKeep(), (unsigned long long) tmp.getTData(), (int) tmp.getTLast());
        sFifoTcpIn.write(tmp);
        if(inWord.getTLast() == 1)
        {
          enqTcpDataFsm = DEQ_START;
        } else {
          enqTcpDataFsm = DEQ_WRITE;
        }
      }
      break;
  }

  printf("pEnqTcpIn after FSM: %d\n", enqTcpDataFsm);

}


void pDeqRecv(
    stream<Axis<128> > &sFifoDataRX,
    stream<Axis<64> > &soMPI_data,
    stream<uint32_t>  &sExpectedLength, //in LINES! (original network lines, so 8 bytes per line)
    stream<bool>      &sDeqRecvDone
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
#pragma HLS pipeline II=1
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static deqState recvDeqFsm = DEQ_IDLE;
  //TODO: add state to drain FIFO after reset?
#pragma HLS reset variable=recvDeqFsm
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static uint32_t expected_recv_count = 0;
  static uint32_t recv_total_cnt = 0;
  static uint64_t second_line = 0;
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

        //uint64_t new_data = sFifoDataRX.read();
        Axis<128> inWord = sFifoDataRX.read();

        uint64_t new_data = (uint64_t) (inWord.getTData());
        second_line = (uint64_t) (inWord.getTData() >> 64);

        tmp.setTData(new_data);
        tmp.setTKeep(0xFF);

        recv_total_cnt++; //we are counting LINES!
        //if(tmp.tlast == 1)
        if(recv_total_cnt >= expected_recv_count)
        {
          printf("[pDeqRecv] [MPI_Recv] expected byte count reached.\n");
          //word_tlast_occured = true;
          tmp.setTLast(1);
          recvDeqFsm = DEQ_DONE;
        } else {
          //in ALL other cases
          tmp.setTLast(0);
          if(inWord.getTKeep() > 0xFF)
          {
            recvDeqFsm = DEQ_WRITE_2;
          }
        }
        printf("[pDeqRecv] toAPP: tkeep %#04x, tdata %#0llx, tlast %d\n",(int) tmp.getTKeep(), (unsigned long long) tmp.getTData(), (int) tmp.getTLast());
        soMPI_data.write(tmp);
      }
      break;

    case DEQ_WRITE_2:
      if( !soMPI_data.full() )
      {
        Axis<64> tmp = Axis<64>();
        tmp.setTData(second_line);
        tmp.setTKeep(0xFF);

        recv_total_cnt++; //we are counting LINES!
        //if(tmp.tlast == 1)
        if(recv_total_cnt >= expected_recv_count)
        {
          printf("[pDeqRecv] [MPI_Recv] expected byte count reached.\n");
          //word_tlast_occured = true;
          tmp.setTLast(1);
          recvDeqFsm = DEQ_DONE;
        } else {
          //in ALL other cases
          tmp.setTLast(0);
          recvDeqFsm = DEQ_WRITE;
        }
        printf("[pDeqRecv] toAPP: tkeep %#04x, tdata %#0llx, tlast %d\n",(int) tmp.getTKeep(), (unsigned long long) tmp.getTData(), (int) tmp.getTLast());
        soMPI_data.write(tmp);
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
    stream<Axis<128> >  &sFifoDataTX,
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
  //TODO: add state to drain FIFO after reset?
#pragma HLS reset variable=sendDeqFsm
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  static uint32_t current_packet_line_cnt = 0x0;
  static NodeId current_send_dst_id = 0xFFF;
  static uint64_t second_line = 0;
  static bool go_to_done = false;
  static bool start_with_second_line = false;
  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------

  switch(sendDeqFsm)
  {
    default:
    case DEQ_IDLE:
      if(!sDeqSendDestId.empty())
      {
        current_send_dst_id = sDeqSendDestId.read();
        current_packet_line_cnt = 0;
        sendDeqFsm = DEQ_START;
        start_with_second_line = false;
      }
      break;

    case DEQ_START:
      if( !soTcp_data.full() && !sFifoDataTX.empty()
          && !soTcp_meta.full() )
      {
        NetworkWord word = NetworkWord();
        //word.tdata = 0x0;
        //word.tlast = 0x0;
        //word.tkeep = 0x0;

        //Axis<64> tmpl1 = Axis<64>();
        //tmpl1 = sFifoDataTX.read();
        Axis<128> inWord = Axis<128>();
        //inWord.tdata = 0x0;
        //inWord.tkeep = 0x0;
        //inWord.tlast = 0x0;
        if(!start_with_second_line)
        {
          inWord = sFifoDataTX.read();

          uint64_t new_data = (uint64_t) (inWord.getTData());
          second_line = (uint64_t) (inWord.getTData() >> 64);
          word.setTData(new_data);
          word.setTLast(inWord.getTLast());
        } else {
          word.setTData(second_line);
          word.setTLast(0);
        }
        word.setTKeep(0xFF);

        //check before we split in parts
        if(word.getTLast() == 1)
        {
          printf("[pDeqSend] SEND_DATA finished writing.\n");
          //word_tlast_occured = true;
          //sendDeqFsm = DEQ_DONE;
          go_to_done = true;
        } else {
          go_to_done = false;
        }

        //if(current_packet_line_cnt >= ZRLMPI_MAX_MESSAGE_SIZE_LINES)
        //if(current_packet_line_cnt == 0)
        //{//last write was last one or first one
          NetworkMeta metaDst = NetworkMeta(current_send_dst_id, ZRLMPI_DEFAULT_PORT, *own_rank, ZRLMPI_DEFAULT_PORT, 0);
          soTcp_meta.write(NetworkMetaStream(metaDst));
          printf("[pDeqSend] started new DATA part packet\n");
        //}

        if(go_to_done)
        {
          if(!start_with_second_line
              && inWord.getTKeep() > 0xFF
            )
          {
            sendDeqFsm = DEQ_WRITE_2;
            word.setTLast(0);
          } else {
            sendDeqFsm = DEQ_DONE;
          }
        } else if(current_packet_line_cnt >= (ZRLMPI_MAX_MESSAGE_SIZE_LINES - 1))
        {//last one in this packet
          word.setTLast(1);
          current_packet_line_cnt = 0;
          //stay here
        } else {
          current_packet_line_cnt++;
          if(!start_with_second_line)
          {
            sendDeqFsm = DEQ_WRITE_2;
          } else {
            sendDeqFsm = DEQ_WRITE;
          }
        }
        start_with_second_line = false;

        printf("[pDeqSend] tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.getTKeep(), (unsigned long long) word.getTData(), (int) word.getTLast());
        soTcp_data.write(word);
      }
      break;
    
  case DEQ_WRITE:
      if( !soTcp_data.full() && !sFifoDataTX.empty()
        )
      {
        NetworkWord word = NetworkWord();
        //word.tdata = 0x0;
        //word.tlast = 0x0;
        //word.tkeep = 0x0;

        //Axis<64> tmpl1 = Axis<64>();
        //tmpl1 = sFifoDataTX.read();
        Axis<128> inWord = sFifoDataTX.read();

        uint64_t new_data = (uint64_t) (inWord.getTData());
        second_line = (uint64_t) (inWord.getTData() >> 64);
        word.setTData(new_data);
        word.setTKeep(0xFF);
        word.setTLast(inWord.getTLast());

        //check before we split in parts
        if(word.getTLast() == 1)
        {
          printf("[pDeqSend] SEND_DATA finished writing.\n");
          //word_tlast_occured = true;
          //sendDeqFsm = DEQ_DONE;
          go_to_done = true;
        } else {
          go_to_done = false;
        }

        if (go_to_done)
        {
          if(inWord.getTKeep() > 0xFF)
          {
            sendDeqFsm = DEQ_WRITE_2;
            word.setTLast(0);
          } else {
            sendDeqFsm = DEQ_DONE;
          }
        } else if(current_packet_line_cnt >= (ZRLMPI_MAX_MESSAGE_SIZE_LINES - 1))
        {//last one in this packet
          word.setTLast(1);
          current_packet_line_cnt = 0;
          sendDeqFsm = DEQ_START;
          if(inWord.getTKeep() > 0xFF)
          {
            start_with_second_line = true;
          } else {
            start_with_second_line = false;
          }
        } else {
          current_packet_line_cnt++;
          sendDeqFsm = DEQ_WRITE_2;
        }

        printf("[pDeqSend] tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.getTKeep(), (unsigned long long) word.getTData(), (int) word.getTLast());
        soTcp_data.write(word);
      }
      break;

  case DEQ_WRITE_2:
      if( !soTcp_data.full() )
      {
        NetworkWord word = NetworkWord();
        //word.tdata = 0x0;
        //word.tlast = 0x0;
        //word.tkeep = 0x0;

        word.setTData(second_line);
        word.setTKeep(0xFF);
        word.setTLast(0);

        if(go_to_done)
        {
          sendDeqFsm = DEQ_DONE;
          word.setTLast(1);
        } else if(current_packet_line_cnt >= (ZRLMPI_MAX_MESSAGE_SIZE_LINES - 1))
        {//last one in this packet
          word.setTLast(1);
          current_packet_line_cnt = 0;
          sendDeqFsm = DEQ_START;
          start_with_second_line = false;
        } else {
          current_packet_line_cnt++;
          sendDeqFsm = DEQ_WRITE;
        }

        printf("[pDeqSend] tkeep %#03x, tdata %#016llx, tlast %d\n",(int) word.getTKeep(), (unsigned long long) word.getTData(), (int) word.getTLast());
        soTcp_data.write(word);
      }
      break;

    case DEQ_DONE:
      if(!sDeqSendDone.full())
      {
        sDeqSendDone.write(true);
        sendDeqFsm = DEQ_IDLE;
        go_to_done = false;
      }
      break;
  }

  printf("sendDeqFsm after FSM: %d\n", sendDeqFsm);
}


void pMpeGlobal(
    ap_uint<32>                   *po_rx_ports,
    stream<MPI_Interface> &siMPIif,
    stream<MPI_Feedback> &soMPIFeB,
    ap_uint<32> *own_rank,
    //stream<Axis<64> >     &sFifoDataTX,
    stream<Axis<128> >     &sFifoDataTX,
    stream<NodeId>        &sDeqSendDestId,
    stream<bool>        &sDeqSendDone,
    //stream<NetworkWord>            &siTcp_data,
    stream<Axis<128> >            &siTcp_data,
    stream<NetworkMetaStream>      &siTcp_meta,
    //stream<Axis<64> > &siMPI_data,
    stream<Axis<128> > &siMPI_data,
    //stream<uint64_t> &sFifoDataRX,
    stream<Axis<128> > &sFifoDataRX,
    stream<uint32_t>  &sExpectedLength, //in LINES!
    stream<bool>      &sDeqRecvDone
    )
{
  //-- DIRECTIVES FOR THIS PROCESS ------------------------------------------
#pragma HLS INLINE off
//#pragma HLS pipeline II=1
#pragma HLS pipeline II=2
  //-- STATIC CONTROL VARIABLES (with RESET) --------------------------------
  static mpeState fsmMpeState = MPE_RESET;
  static bool cache_invalidated = false;
  static uint64_t protocol_timeout_cnt = ZRLMPI_PROTOCOL_TIMEOUT_CYCLES;
  static uint32_t protocol_timeout_inc = 0;

#pragma HLS reset variable=fsmMpeState
#pragma HLS reset variable=cache_invalidated
#pragma HLS reset variable=protocol_timeout_cnt
#pragma HLS reset variable=protocol_timeout_inc
  //-- STATIC DATAFLOW VARIABLES --------------------------------------------
  //static stream<ap_uint<128> > sFifoHeaderCache("sFifoHeaderCache");
  static uint32_t expected_src_rank = 0;
  //static uint16_t current_cache_data_cnt = 0;

  static ap_uint<256> header_cache[HEADER_CACHE_LENGTH];
  static bool header_cache_valid[HEADER_CACHE_LENGTH];
  //static uint16_t last_checked_cache_line = 0;


  static MPI_Interface currentInfo = MPI_Interface();
  static mpiType currentDataType = MPI_INT;
  //static int handshakeLinesCnt = 0;

  static uint8_t header_i_cnt = 0;
  static ap_uint<8> bytes[MPIF_HEADER_LENGTH];
  static ap_uint<8> bytes_c1[MPIF_HEADER_LENGTH];
  static ap_uint<8> bytes_c2[MPIF_HEADER_LENGTH];
  static ap_uint<8> bytes_c3[MPIF_HEADER_LENGTH];

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

  static uint32_t exp_recv_count_enqueue = 0;
  static uint32_t enqueue_recv_total_cnt = 0;

  static bool receive_right_data_started = false;

  static mpeState after_drain_recovery_state = MPE_RESET;

  // #pragma HLS STREAM variable=sFifoHeaderCache depth=64
  //#pragma HLS STREAM variable=sFifoHeaderCache depth=2048 //HEADER_CACHE_LENTH*MPIF_HEADER_LENGTH
#pragma HLS array_partition variable=bytes complete dim=0
#pragma HLS array_partition variable=bytes_c1 complete dim=0
#pragma HLS array_partition variable=bytes_c2 complete dim=0
#pragma HLS array_partition variable=bytes_c3 complete dim=0
  //#pragma HLS array_partition variable=header_cache cyclic factor=32 dim=0
  //#pragma HLS array_partition variable=header_cache_valid cyclic factor=32 dim=0
#pragma HLS array_partition variable=header_cache_valid complete dim=0

  //-- LOCAL DATAFLOW VARIABLES ---------------------------------------------

  uint8_t ret;

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
      protocol_timeout_inc = 0;
      after_drain_recovery_state = MPE_RESET;
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
        Axis<128> tmp = Axis<128>();
        //tmp.setTData(0x0);
        tmp.setTKeep(0xFFFF);
        ap_uint<128> tmp_tdata = 0x0;
        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          //tmp.tdata |= ((ap_uint<32>) bytes[i*4+j]) << (3-j)*8;
          tmp_tdata |= ((ap_uint<128>) bytes[header_i_cnt*BPL+j]) << j*8;
        }
        tmp.setTData(tmp_tdata);
        tmp.setTLast(0);
        //printf("tdata32: %#08x\n",(uint32_t) tmp.tdata);
        printf("tdata64: %#0llx\n",(uint64_t) tmp.getTData());

        sFifoDataTX.write(tmp);
        //printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        header_i_cnt++;

        fsmMpeState = START_SEND_1;
      }
      break;
    case START_SEND_1:
      if(!sFifoDataTX.full())
      {
        Axis<128> tmp = Axis<128>();
        tmp.setTData(0x0);
        tmp.setTKeep(0xFFFF);
        ap_uint<128> tmp_tdata = 0x0;
        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          //tmp.tdata |= ((ap_uint<32>) bytes[i*4+j]) << (3-j)*8;
          tmp_tdata |= ((ap_uint<128>) bytes[header_i_cnt*BPL+j]) << j*8;
        }
        tmp.setTData(tmp_tdata);
        printf("tdata64: %#0llx\n",(uint64_t) tmp.getTData());
        tmp.setTLast(0);
        if ( header_i_cnt >= (MPIF_HEADER_LENGTH/BPL) - 1)
        {
          tmp.setTLast(1);
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
          fsmMpeState = WAIT4CLEAR_CACHE_LOOKUP;
          //last_checked_cache_line = 0;
          header = MPI_Header();
          expected_call = MPI_RECV_INT;
          protocol_timeout_cnt = ZRLMPI_PROTOCOL_TIMEOUT_CYCLES + (ZRLMPI_PROTOCOL_TIMEOUT_CYCLES * protocol_timeout_inc * ZRLMPI_PROTOCOL_TIMEOUT_INC_FACTOR);
          if(currentDataType == MPI_FLOAT)
          {
            expected_call = MPI_RECV_FLOAT;
          }
          expected_type = CLEAR_TO_SEND;
        }
      }
      break;

    case WAIT4CLEAR_CACHE_LOOKUP:
      //check cache first
      if(getCache(header_cache, header_cache_valid, 
            bytes_c1, header, metaSrc, expected_type, expected_call, expected_src_rank))
      {
        fsmMpeState = WAIT4CLEAR_CACHE;
      } else {
        fsmMpeState = WAIT4CLEAR;
      }
      break;

    case WAIT4CLEAR_CACHE:
      ret = checkHeader(bytes_c1, header, metaSrc, expected_type, expected_call, true, expected_src_rank);
      if(ret == 0)
      {//found desired header
        printf("Cache HIT\n");
        //delete_cache_line(header_cache, header_cache_valid, expected_src_rank);
        fsmMpeState = SEND_DATA_START;
      } else {
        printf("\tCache MISS (wrong type)\n");
        //else, we haven't found it
        //thanks to BSP, there is no other place where it could be
        fsmMpeState = WAIT4CLEAR;
      }
      break;
    case WAIT4CLEAR:
      if( !siTcp_data.empty() && !siTcp_meta.empty()
        )
      {
        //start read header
        metaSrc = siTcp_meta.read().tdata;
        header_i_cnt = 0;
        fsmMpeState = WAIT4CLEAR_1;

        //NetworkWord tmp = siTcp_data.read();
        Axis<128> tmp = siTcp_data.read();
        printf("Data read: tkeep %#03x, tdata %#032llx, tlast %d\n",(int) tmp.getTKeep(), (unsigned long long) tmp.getTData(), (int) tmp.getTLast());
        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          bytes[header_i_cnt*BPL + j] = (ap_uint<8>) ( tmp.getTData() >> j*8) ;
          //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
        header_i_cnt = 1;
      } else {
        protocol_timeout_cnt--;
        if(protocol_timeout_cnt == 0 && protocol_timeout_inc < ZRLMPI_PROTOCOL_MAX_INC)
        {
          fsmMpeState = START_SEND;
          protocol_timeout_inc++;
        }
        //else, we wait
      }
      break;

    case WAIT4CLEAR_1:
      if( !siTcp_data.empty() //&& !siTcp_meta.empty()
        )
      {
        //read header cont.
        //NetworkWord tmp = siTcp_data.read();
        Axis<128> tmp = siTcp_data.read();

        printf("Data read: tkeep %#03x, tdata %#032llx, tlast %d\n",(int) tmp.getTKeep(), (unsigned long long) tmp.getTData(), (int) tmp.getTLast());
        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          bytes[header_i_cnt*BPL + j] = (ap_uint<8>) ( tmp.getTData() >> j*8) ;
          //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
        header_i_cnt++;

        if(header_i_cnt >= (MPIF_HEADER_LENGTH + (BPL-1))/BPL)
        {
          if(tmp.getTLast() != 1)
          {
            fsmMpeState = MPE_DRAIN_DATA_STREAM;
            after_drain_recovery_state = WAIT4CLEAR;
          } else {
            ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
            if(ret == 0)
            {//got CLEAR_TO_SEND 
              printf("Got CLEAR to SEND\n");
              fsmMpeState = SEND_DATA_START;
            } else {
              //else, we continue to wait
              //and add it to the cache
              if(ret == 1)
              {
                add_cache_bytes(header_cache, header_cache_valid, bytes, header.src_rank);
              }
              fsmMpeState = WAIT4CLEAR;
              //no timeout reset
            }
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
        Axis<128> tmp = Axis<128>();
        //tmp.setTData(0x0);
        tmp.setTKeep(0xFFFF);
        ap_uint<128> tmp_tdata = 0x0;
        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          tmp_tdata |= ((ap_uint<128>) bytes[header_i_cnt*BPL+j]) << j*8;
        }
        tmp.setTData(tmp_tdata);
        tmp.setTLast(0); //in this case, always
        sFifoDataTX.write(tmp);
        header_i_cnt = 1;

        sDeqSendDestId.write((NodeId) header.dst_rank);

        expected_send_count = header.size; //in WORDS
        printf("[MPI_send] expect %d words.\n",expected_send_count);
        send_total_cnt = 0;

        fsmMpeState = SEND_DATA_START_1;
        //we can delete the cache in all cases
        delete_cache_line(header_cache, header_cache_valid, expected_src_rank);
      }
      break;
    case SEND_DATA_START_1:
      if(!sFifoDataTX.full())
      {
        Axis<128> tmp = Axis<128>();
        //tmp.setTData(0x0);
        tmp.setTKeep(0xFFFF);
        ap_uint<128> tmp_tdata = 0x0;
        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          tmp_tdata |= ((ap_uint<128>) bytes[header_i_cnt*BPL+j]) << j*8;
        }
        tmp.setTData(tmp_tdata);
        tmp.setTLast(0); //in this case, always
        sFifoDataTX.write(tmp);
        //printf("Writing Header byte: %#02x\n", (int) bytes[i]);
        header_i_cnt++;
        if(header_i_cnt >= MPIF_HEADER_LENGTH/BPL)
        {
          fsmMpeState = SEND_DATA_RD;
        }

      }
      break;
    case SEND_DATA_RD:
      if(!sFifoDataTX.full() && !siMPI_data.empty())
      {
        Axis<128> current_read_word = siMPI_data.read();
        //send_total_cnt += 8; //we read EIGHT BYTES per line
        //send_total_cnt += 2; //two WORDS ber line
        //send_total_cnt += WPL; //four WORDS ber line
        //since this comes from the app, this should always be true...
        //anyways, just to be sure
        send_total_cnt += getWordCntfromTKeep(current_read_word.getTKeep());

        if(send_total_cnt >= expected_send_count)
        {
          current_read_word.setTLast(1);
          fsmMpeState = SEND_DATA_WRD;
          printf("\t[pMpeGlobal] given length reached.\n");
        } else {
          current_read_word.setTLast(0);
        }

        //if(current_read_word.tlast == 1)
        //{
        //  fsmMpeState = SEND_DATA_WRD;
        //  printf("\t[pMpeGlobal] tlast Occured.\n");
        //}
        sFifoDataTX.write(current_read_word);
        printf("\t[pMpeGlobal] MPI read data: %#08x, tkeep: %d, tlast %d\n", (unsigned long long) current_read_word.getTData(), (int) current_read_word.getTKeep(), (int) current_read_word.getTLast());
      }
      break;
    case SEND_DATA_WRD:
      //wait for dequeue fsm
      if(!sDeqSendDone.empty())
      {
        if(sDeqSendDone.read())
        {
          fsmMpeState = WAIT4ACK_CACHE_LOOKUP;
          header = MPI_Header();
          //last_checked_cache_line = 0;
          expected_call = MPI_RECV_INT;
          protocol_timeout_cnt = PROTOCOL_ACK_DELAY_FACTOR * ZRLMPI_PROTOCOL_TIMEOUT_CYCLES + (ZRLMPI_PROTOCOL_TIMEOUT_CYCLES * protocol_timeout_inc * ZRLMPI_PROTOCOL_TIMEOUT_INC_FACTOR);
          if(currentDataType == MPI_FLOAT)
          {
            expected_call = MPI_RECV_FLOAT;
          }
          expected_type = ACK;
        }
      }
      break;

    case WAIT4ACK_CACHE_LOOKUP:
      //check cache first
      if(getCache(header_cache, header_cache_valid, 
            bytes_c2, header, metaSrc, expected_type, expected_call, expected_src_rank))
      {
        fsmMpeState = WAIT4ACK_CACHE;
      } else {
        fsmMpeState = WAIT4ACK;
      }
      break;

    case WAIT4ACK_CACHE:
      ret = checkHeader(bytes_c2, header, metaSrc, expected_type, expected_call, true, expected_src_rank);
      if(ret == 0)
      {//found desired header
        printf("Cache HIT\n");
        //delete_cache_line(header_cache, header_cache_valid, expected_src_rank);
        fsmMpeState = WAIT4ACK_CACHE_2;
      } else {
        printf("\tCache MISS (wrong type)\n");
        //else, we haven't found it
        //thanks to BSP, there is no other place where it could be
        fsmMpeState = WAIT4ACK;
      }
      break;

    case WAIT4ACK_CACHE_2:
      //we can delete the cache in all cases
      delete_cache_line(header_cache, header_cache_valid, expected_src_rank);
      fsmMpeState = IDLE;
      break;

    case WAIT4ACK:
      if( !siTcp_data.empty() && !siTcp_meta.empty() )
      {
        //read header
        //NetworkWord tmp = siTcp_data.read();
        Axis<128> tmp = siTcp_data.read();
        header_i_cnt = 0;

        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          bytes[header_i_cnt*BPL + j] = (ap_uint<8>) ( tmp.getTData() >> j*8) ;
          //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
        header_i_cnt = 1;

        metaSrc = siTcp_meta.read().tdata;
        fsmMpeState = WAIT4ACK_1;
      } else {
        if( !soMPIFeB.full() )
        {
          protocol_timeout_cnt--;
          if(protocol_timeout_cnt == 0 && protocol_timeout_inc < ZRLMPI_PROTOCOL_MAX_INC)
          {
            soMPIFeB.write(ZRLMPI_FEEDBACK_FAIL);
            fsmMpeState = SEND_DATA_START;
            protocol_timeout_inc++;
          }
        }
      }
      break;
    case WAIT4ACK_1:
      if( !siTcp_data.empty() && !soMPIFeB.full() )
      {

        //NetworkWord tmp = siTcp_data.read();
        Axis<128> tmp = siTcp_data.read();

        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          bytes[header_i_cnt*BPL + j] = (ap_uint<8>) ( tmp.getTData() >> j*8) ;
          //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
        header_i_cnt++;

        if(header_i_cnt >= (MPIF_HEADER_LENGTH + (BPL-1))/BPL)
        {
          if(tmp.getTLast() != 1)
          {
            fsmMpeState = MPE_DRAIN_DATA_STREAM;
            after_drain_recovery_state = WAIT4ACK;
          } else {
            ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
            if(ret == 0)
            {
              printf("ACK received.\n");
              soMPIFeB.write(ZRLMPI_FEEDBACK_OK);
              fsmMpeState = IDLE;
            } else {
              //else, we continue to wait
              //and add it to the cache
              if(ret == 1)
              {
                add_cache_bytes(header_cache, header_cache_valid, bytes, header.src_rank);
              }
              fsmMpeState = WAIT4ACK;
              //no timeout reset
            }
          }
        }
      }
      break;
    case START_RECV:
      header = MPI_Header();
      //last_checked_cache_line = 0;
      expected_call = MPI_SEND_INT;
      if(currentDataType == MPI_FLOAT)
      {
        expected_call = MPI_SEND_FLOAT;
      }
      expected_type = SEND_REQUEST;
      fsmMpeState = WAIT4REQ_CACHE_LOOKUP;
      break;
    
    case WAIT4REQ_CACHE_LOOKUP:
      //check cache first
      if(getCache(header_cache, header_cache_valid, 
            bytes_c3, header, metaSrc, expected_type, expected_call, expected_src_rank))
      {
        fsmMpeState = WAIT4REQ_CACHE;
      } else {
        fsmMpeState = WAIT4REQ;
      }
      break;
   
    case WAIT4REQ_CACHE:
      ret = checkHeader(bytes_c3, header, metaSrc, expected_type, expected_call, true, expected_src_rank);
      if(ret == 0)
      {//found desired header
        printf("Cache HIT\n");
        //delete_cache_line(header_cache, header_cache_valid, expected_src_rank);
        fsmMpeState = ASSEMBLE_CLEAR;
      } else {
        printf("\tCache MISS (wrong type)\n");
        //else, we haven't found it
        //thanks to BSP, there is no other place where it could be
        fsmMpeState = WAIT4ACK;
      }
      break;
    case WAIT4REQ:
      if( !siTcp_data.empty() && !siTcp_meta.empty() )
      {
        //read header
        //NetworkWord tmp = siTcp_data.read();
        Axis<128> tmp = siTcp_data.read();

        header_i_cnt = 0;
        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          bytes[header_i_cnt*BPL + j] = (ap_uint<8>) ( tmp.getTData() >> j*8) ;
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
        //NetworkWord tmp = siTcp_data.read();
        Axis<128> tmp = siTcp_data.read();
        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          bytes[header_i_cnt*BPL + j] = (ap_uint<8>) ( tmp.getTData() >> j*8) ;
          //bytes[i*8 + 7 -j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
        header_i_cnt++;

        if(header_i_cnt >= (MPIF_HEADER_LENGTH + (BPL-1))/BPL)
        {
          if(tmp.getTLast() != 1)
          {
            fsmMpeState = MPE_DRAIN_DATA_STREAM;
            after_drain_recovery_state = WAIT4REQ;
          } else {
            ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
            if(ret == 0)
            {
              //got SEND_REQUEST 
              printf("Got SEND REQUEST\n");
              fsmMpeState = ASSEMBLE_CLEAR;
            } else {
              //else, we wait...
              //and add it to the cache
              if(ret == 1)
              {
                add_cache_bytes(header_cache, header_cache_valid, bytes, header.src_rank);
              }
              fsmMpeState = WAIT4REQ;
            }
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
        Axis<128> tmp = Axis<128>();
        //tmp.setTData(0x0);
        tmp.setTKeep(0xFFFF);
        ap_uint<128> tmp_tdata = 0x0;
        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          tmp_tdata |= ((ap_uint<128>) bytes[header_i_cnt*BPL+j]) << j*8;
        }
        tmp.setTData(tmp_tdata);
        tmp.setTLast(0);
        sFifoDataTX.write(tmp);
        header_i_cnt = 1;

        fsmMpeState = ASSEMBLE_CLEAR_1;
        //we can delete the cache in all cases
        delete_cache_line(header_cache, header_cache_valid, expected_src_rank);
      }
      break;
    case ASSEMBLE_CLEAR_1:
      if(!sFifoDataTX.full())
      {
        Axis<128> tmp = Axis<128>();
        //tmp.setTData(0x0);
        tmp.setTKeep(0xFFFF);
        ap_uint<128> tmp_tdata = 0x0;
        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          tmp_tdata |= ((ap_uint<128>) bytes[header_i_cnt*BPL+j]) << j*8;
        }
        tmp.setTData(tmp_tdata);
        tmp.setTLast(0);
        if ( header_i_cnt >= (MPIF_HEADER_LENGTH/BPL) - 1)
        {
          tmp.setTLast(1);
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
          protocol_timeout_cnt = ZRLMPI_PROTOCOL_TIMEOUT_CYCLES + (ZRLMPI_PROTOCOL_TIMEOUT_CYCLES * protocol_timeout_inc * ZRLMPI_PROTOCOL_TIMEOUT_INC_FACTOR);
          receive_right_data_started = false;
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

          //NetworkWord tmp = siTcp_data.read();
          Axis<128> tmp = siTcp_data.read();

          for(int j = 0; j<BPL; j++)
          {
#pragma HLS unroll
            bytes[header_i_cnt*BPL + j] = (ap_uint<8>) ( tmp.getTData() >> j*8) ;
            //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
          }
          header_i_cnt = 1;
          fsmMpeState = RECV_DATA_START_1;
        }
      } else {
        if(!soMPIFeB.full())
        {
          protocol_timeout_cnt--;
          if(protocol_timeout_cnt == 0 && protocol_timeout_inc < ZRLMPI_PROTOCOL_MAX_INC)
          {
            protocol_timeout_inc++;
            if(receive_right_data_started)
            {
              //re-start receive completely
              soMPIFeB.write(ZRLMPI_FEEDBACK_FAIL);
              fsmMpeState = RECV_DATA_START;
              receive_right_data_started = false;
              expect_more_data = false;
              current_data_src_node_id = 0xFFF;
              current_data_src_port = 0x0;
              current_data_dst_port = 0x0;
              header = MPI_Header();
            } else {
              fsmMpeState = ASSEMBLE_CLEAR;
            }
          }
        }
      }
      break;

    case RECV_DATA_START_1:
      if( !siTcp_data.empty()
          && !sExpectedLength.full()
        )
      {
        //NetworkWord tmp = siTcp_data.read();
        Axis<128> tmp = siTcp_data.read();
        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          bytes[header_i_cnt*BPL + j] = (ap_uint<8>) ( tmp.getTData() >> j*8) ;
          //bytes[i*8 + 7-j] = (ap_uint<8>) ( tmp.tdata >> j*8) ;
        }
        header_i_cnt++;

        if(header_i_cnt >= (MPIF_HEADER_LENGTH + (BPL-1))/BPL)
        {
          //here, NO TLAST check!
          ret = checkHeader(bytes, header, metaSrc, expected_type, expected_call, false, expected_src_rank);
          if(ret == 0
              && !expect_more_data) //we don't start a new data packet here
          {
            //valid header && valid source
            //expected_recv_count = header.size;
            //uint16_t expected_length_in_lines = (header.size+7)/8;
            uint32_t expected_length_in_lines = (uint32_t) (header.size+1)/2;
            //uint32_t expected_length_in_lines = (uint32_t) (header.size+(WPL-1))/WPL;
            //here, we count the original networklines -> so 2 is correct

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
            if(ret == 1)
            {
              add_cache_bytes(header_cache, header_cache_valid, bytes, header.src_rank);
            }
            fsmMpeState = RECV_DATA_START;
          }
        }
      }
      break;

    case RECV_DATA_RD:
      if( !siTcp_data.empty() && !sFifoDataRX.full()
        )
      {
        //NetworkWord word = siTcp_data.read();
        Axis<128> word = siTcp_data.read();
        printf("\t[pMpeGlobal] READ: tkeep %#03x, tdata %#032llx, tlast %d\n",(int) word.getTKeep(), (unsigned long long) word.getTData(), (int) word.getTLast());

        //sFifoDataRX.write(word.tdata);
        sFifoDataRX.write(word);
        //enqueue_recv_total_cnt++;
        //enqueue_recv_total_cnt += 2; //two WORDS per line
        //enqueue_recv_total_cnt += WPL; //four WORDS per line NO, not always!!
        enqueue_recv_total_cnt += getWordCntfromTKeep(word.getTKeep());
        //check if we have to receive a new packet meta
        if(word.getTLast() == 1)
        {
          if(enqueue_recv_total_cnt < exp_recv_count_enqueue) //not <=
          {//we expect more
            fsmMpeState = RECV_DATA_START;
            protocol_timeout_cnt = ZRLMPI_PROTOCOL_TIMEOUT_CYCLES + (ZRLMPI_PROTOCOL_TIMEOUT_CYCLES * protocol_timeout_inc * ZRLMPI_PROTOCOL_TIMEOUT_INC_FACTOR);
            receive_right_data_started = true;
            //so timeout also for multiple packets
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

        Axis<128> tmp = Axis<128>();
        //tmp.setTData(0x0);
        tmp.setTKeep(0xFFFF);
        ap_uint<128> tmp_tdata = 0x0;
        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          tmp_tdata |= ((ap_uint<128>) bytes[header_i_cnt*BPL+j]) << j*8;
        }
        tmp.setTData(tmp_tdata);
        tmp.setTLast(0);
        sFifoDataTX.write(tmp);
        header_i_cnt = 1;


        fsmMpeState = SEND_ACK_0;
      }
      break;
    case SEND_ACK_0:
      if(!sFifoDataTX.full())
      {
        Axis<128> tmp = Axis<128>();
        //tmp.setTData(0x0);
        tmp.setTKeep(0xFFFF);
        ap_uint<128> tmp_tdata = 0x0;
        for(int j = 0; j<BPL; j++)
        {
#pragma HLS unroll
          tmp_tdata |= ((ap_uint<128>) bytes[header_i_cnt*BPL+j]) << j*8;
        }
        tmp.setTData(tmp_tdata);
        tmp.setTLast(0);
        header_i_cnt++;

        if ( header_i_cnt >= (MPIF_HEADER_LENGTH/BPL))
        {
          tmp.setTLast(1);
          fsmMpeState = SEND_ACK;
        }
        sFifoDataTX.write(tmp);
      }
      break;
    case SEND_ACK:
      if(!sDeqSendDone.empty() && !soMPIFeB.full() )
      {
        if(sDeqSendDone.read())
        {
          soMPIFeB.write(ZRLMPI_FEEDBACK_OK);
          fsmMpeState = IDLE;
        }
      }
      break;
    case MPE_DRAIN_DATA_STREAM:
      if( !siTcp_data.empty() )
      {
        //NetworkWord word = siTcp_data.read();
        Axis<128> word = siTcp_data.read();
        printf("\t[pMpeGlobal] DROP: tkeep %#03x, tdata %#032llx, tlast %d\n",(int) word.getTKeep(), (unsigned long long) word.getTData(), (int) word.getTLast());
        if(word.getTLast() == 1)
        {
          fsmMpeState = after_drain_recovery_state;
          after_drain_recovery_state = MPE_RESET;
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
    stream<MPI_Feedback> &soMPIFeB,
    stream<Axis<64> > &siMPI_data,
    stream<Axis<64> > &soMPI_data
    )
{
#pragma HLS INTERFACE axis register both port=siTcp_data
#pragma HLS INTERFACE axis register both port=siTcp_meta
#pragma HLS INTERFACE axis register both port=soTcp_data
#pragma HLS INTERFACE axis register both port=soTcp_meta
//#pragma HLS INTERFACE axis register off port=siTcp_data
//#pragma HLS INTERFACE axis register off port=siTcp_meta
//#pragma HLS INTERFACE axis register off port=soTcp_data
//#pragma HLS INTERFACE axis register off port=soTcp_meta

#pragma HLS INTERFACE ap_ovld register port=po_rx_ports name=poROL_NRC_Rx_ports

#pragma HLS INTERFACE ap_vld register port=own_rank name=piFMC_rank

  //#pragma HLS INTERFACE ap_ovld register port=MMIO_out name=poMMIO

#pragma HLS INTERFACE ap_fifo port=siMPIif
#pragma HLS DATA_PACK     variable=siMPIif
#pragma HLS INTERFACE ap_fifo port=soMPIFeB
//#pragma HLS DATA_PACK     variable=soMPIFeB
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

  static stream<Axis<128> > sFifoDataTX("sFifoDataTX");
  static stream<Axis<128> > sFifoDataRX("sFifoDataRX");
  static stream<NodeId>        sDeqSendDestId("sDeqSendDestId");
  static stream<bool>          sDeqSendDone("sDeqSendDone");
  static stream<uint32_t>    sExpectedLength("sExpectedLength"); //in LINES!
  static stream<bool>        sDeqRecvDone("sDeqRecvDone");
  static stream<Axis<128> > sFifoTcpIn("sFifoTcpIn");
  static stream<Axis<128> > sFifoMpiDataIn("sFifoMpiDataIn");
  static stream<NetworkMetaStream> sFifoTcpMetaIn("sFifoTcpMetaIn");

//#pragma HLS STREAM variable=sFifoDataTX     depth=128
//#pragma HLS STREAM variable=sFifoDataRX     depth=128
#pragma HLS STREAM variable=sFifoDataTX     depth=256
#pragma HLS STREAM variable=sFifoDataRX     depth=256
#pragma HLS STREAM variable=sDeqSendDestId  depth=4
#pragma HLS STREAM variable=sDeqSendDone    depth=4
#pragma HLS STREAM variable=sExpectedLength depth=4
#pragma HLS STREAM variable=sDeqRecvDone    depth=4

#pragma HLS STREAM variable=sFifoTcpIn      depth=128
#pragma HLS STREAM variable=sFifoMpiDataIn  depth=128
#pragma HLS STREAM variable=sFifoTcpMetaIn  depth=32

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
  // ENQUEUE DATA (new width)

  pEnqMpiData(siMPI_data, sFifoMpiDataIn);

  pEnqTcpIn(siTcp_data, siTcp_meta, sFifoTcpIn, sFifoTcpMetaIn);

  //===========================================================
  // DEQUEUE FSM SEND

  pDeqSend(sFifoDataTX, soTcp_data, soTcp_meta, own_rank, sDeqSendDestId,
      sDeqSendDone);

  //===========================================================
  // DEQUEUE FSM RECV

  pDeqRecv(sFifoDataRX, soMPI_data, sExpectedLength, sDeqRecvDone);
  
  //===========================================================
  // MPE GLOBAL STATE
  // (put the slow process last...)

  pMpeGlobal(po_rx_ports, siMPIif, soMPIFeB, own_rank, sFifoDataTX,
      sDeqSendDestId, sDeqSendDone, sFifoTcpIn, sFifoTcpMetaIn,
      sFifoMpiDataIn, sFifoDataRX,
      sExpectedLength, sDeqRecvDone);

}



