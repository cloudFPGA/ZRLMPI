
#include <stdint.h>
#include "zrlmpi_common.hpp"
#include "ZRLMPI.hpp"
//#include "test.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <memory.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <errno.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

typedef unsigned long long timestamp_t;

static timestamp_t get_timestamp()
{
  struct timeval now;
  gettimeofday(&now, NULL);
  return now.tv_usec + (timestamp_t)now.tv_sec*1000000;
}

sockaddr_in rank_socks[MPI_CLUSTER_SIZE_MAX + 1];
int udp_sock = 0;
int cluster_size = 0;
int own_rank = 0;
bool use_udp = false;
bool use_tcp = false; //TODO

MPI_Header header_recv_cache[MPI_CLUSTER_SIZE_MAX];
int cache_iter = 0;
int cache_num = 0;
bool skip_cache_entry[MPI_CLUSTER_SIZE_MAX];
uint32_t max_udp_payload_bytes = 0;

//clock_t clock_begin = 0;
timestamp_t t0 = 0;

//returns the size
int receiveHeader(unsigned long expAddr, packetType expType, mpiCall expCall, uint32_t expSrcRank, int payload_length, uint8_t *buffer)
{
  uint8_t bytes[MPIF_HEADER_LENGTH];
  struct sockaddr_in src_addr;
  socklen_t slen = sizeof(src_addr);
  MPI_Header header = MPI_Header();

  bool copyToCache = false, checkedCache = false;
  int ret = 0, res = 0, cache_i = 0;

  bool multiple_packet_mode = false;
  uint32_t received_length = 0;
  uint32_t recv_packets_cnt = 0;
  uint32_t expected_length = MPIF_HEADER_LENGTH + payload_length;
  if(expected_length > max_udp_payload_bytes)
  {
    printf("recv with multi packet mode.\n");
    multiple_packet_mode = true;
  }


  while(true)
  {

    //first: look up cache:
    if(!checkedCache)
    {

      //if(cache_i >= MPI_CLUSTER_SIZE_MAX || cache_num == 0)
      if(cache_i >= cache_iter || cache_num == 0)
      {
        checkedCache = true;
        continue;
      }
      printf("checking cache entry %d\n", cache_i);
      if(skip_cache_entry[cache_i])
      {
        printf("\tskipping entry.\n");
        cache_i++;
        continue;
      }
      header = header_recv_cache[cache_i];
      cache_i++;

    } else {

      //then wait for network 
      header = MPI_Header();
      if(payload_length > 0)
      {
        //printf("received_length: %d; recv_packets_cnt %d\n", received_length, recv_packets_cnt);
        uint8_t *start_address = buffer + received_length;
        res = recvfrom(udp_sock, start_address, expected_length, 0, (sockaddr*)&src_addr, &slen);
        ret = bytesToHeader(start_address, header);
        //for debugging
        //for(int d = 0; d < 32; d++)
        //{
        //  printf(" %d", start_address[d]);
        //  if(d > 1 && d %8 == 0)
        //  {
        //    printf("\n");
        //  }
        //}
        //printf("\n");
      } else {
        res = recvfrom(udp_sock, &bytes, MPIF_HEADER_LENGTH, 0, (sockaddr*)&src_addr, &slen);
        ret = bytesToHeader(bytes, header);
      }

      printf("received packet from %s:%d with length %d\n",inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port), res);

      if(ret != 0 && recv_packets_cnt == 0)
      {
        printf("invalid header. Dropping packet.\n");
        //copyToCache = true;
        continue;
      } 
      else if(ret != 0 && recv_packets_cnt > 0 && multiple_packet_mode)
      {//is the continuation of a data packet
        printf("data_packet %d received.\n", recv_packets_cnt);
        received_length += res;
        recv_packets_cnt++;
        if(received_length >= expected_length)
        {
          break;
        } else {
          continue;
        }
      }
      else if(ret == 0 && multiple_packet_mode && recv_packets_cnt > 0)
      {
        printf("we've got another header in between.\n");
        copyToCache = true;
      }
      else {
        //valid header
        received_length += res;
        recv_packets_cnt++;
        copyToCache = false;
      }

    }

    if(!copyToCache && checkedCache)
    {
      if(ntohl(src_addr.sin_addr.s_addr) != expAddr)
      {
        printf("Header does not match ipAddress: Expected %ld, got %ld \n", expAddr, ntohl(src_addr.sin_addr.s_addr));
        copyToCache = true;
      }
    }

    if(!copyToCache && header.src_rank != expSrcRank)
    {
      printf("Header does not match expected src rank: Expected %d, got %d \n", expSrcRank, header.src_rank);
      copyToCache = true;
    }

    //if(header.dst_rank != MPI_OWN_RANK)
    if(!copyToCache && header.dst_rank != own_rank)
    {
      printf("I'm not the right recepient! Header is addressed for %d.\n", header.dst_rank);
      copyToCache = true;
    }

    if(!copyToCache && header.type != expType)
    {
      printf("Expected type %d, got %d!\n",(int) expType, (int) header.type);
      copyToCache = true;
    }

    //check data type 
    //TODO: generic
    if(!copyToCache && header.call != expCall)
    {
      printf("receiver expects different call: %d.\n", (int) header.call);
      copyToCache = true;
    }

    if(copyToCache && checkedCache)
    {
      header_recv_cache[cache_iter] = header;
      skip_cache_entry[cache_iter] = false;
      cache_iter++;
      received_length -= res;
      recv_packets_cnt--;
      if (cache_iter >= MPI_CLUSTER_SIZE_MAX)
      {
        //some type of clean 
        //TODO
        cache_iter = 0;
        for(int i = 0; i< MPI_CLUSTER_SIZE_MAX; i++)
        {
          skip_cache_entry[i] = false;
        }
      }
      printf("put header to cache\n");
      cache_num++;
    } 
    if(!checkedCache && !copyToCache)
    {//delete from cache
      printf("found header in cache\n");
      header_recv_cache[cache_i-1] = MPI_Header();
      skip_cache_entry[cache_i-1] = true;
      cache_num--;
    }
    if(!copyToCache)
    {//we got what we wanted
      if(!multiple_packet_mode 
          || (received_length >= expected_length)
        )
      {
        break;
      } 
      //else: continue
    }

  }

  if(multiple_packet_mode)
  {
    //if(received_length != header.size)
    //{
    //  printf("\t[WARNING] received DATA length (%d) doesn't match header size (%d)!\n", received_length, header.size);
    //}
    return received_length;
  }
  return header.size;
}


bool array_contains(const std::string &v, const std::vector<std::string> &array)
{
    return std::find(array.begin(), array.end(), v) != array.end();
}

int array_count(const std::string &v, const std::vector<std::string> &array)
{
    return count(array.begin(), array.end(), v);
}


void MPI_Init()
{
  //clock_begin = clock();
  t0 = get_timestamp();
  //we are all set 
  return;
}

void MPI_Comm_rank(MPI_Comm communicator, int* rank)
{
  *rank = own_rank;
}

void MPI_Comm_size( MPI_Comm communicator, int* size)
{
  *size = cluster_size;
}

void send_internal(
    int* data,
    int count,
    MPI_Datatype datatype,
    int destination,
    int tag,
    MPI_Comm communicator)
{
  uint8_t bytes[MPIF_HEADER_LENGTH];
  //TODO make generic
  int typewidth = 4;
  int corresponding_call_type = MPI_RECV_INT;
  int call_type = MPI_SEND_INT;
  
  MPI_Header header = MPI_Header(); 
  header.dst_rank = destination;
  //header.src_rank = MPI_OWN_RANK;
  header.src_rank = own_rank;
  header.size = 0;
  header.call = call_type;
  header.type = SEND_REQUEST;

  headerToBytes(header, bytes);

  int res = sendto(udp_sock, &bytes, MPIF_HEADER_LENGTH, 0, (sockaddr*)&rank_socks[destination], sizeof(rank_socks[destination]));

  std::cout << res << " bytes sent for SEND_REQUEST" << std::endl;

  int ret = 0;


  ret = receiveHeader(ntohl(rank_socks[destination].sin_addr.s_addr), CLEAR_TO_SEND, corresponding_call_type, destination, 0, 0);
  printf("Got CLEAR to SEND\n");

  //Send data
  header = MPI_Header(); 
  header.dst_rank = destination;
  //header.src_rank = MPI_OWN_RANK;
  header.src_rank = own_rank;
  header.size = count*4;
  header.call = call_type;
  header.type = DATA;

  uint8_t buffer[count*4 + MPIF_HEADER_LENGTH];

  headerToBytes(header, buffer);
  //TODO generic
  memcpy(&buffer[MPIF_HEADER_LENGTH], data, count*4);
  uint32_t byte_length = count*4 + MPIF_HEADER_LENGTH;
  int total_send = 0;
  int total_packets = 0;
  struct timespec sleep;
  sleep.tv_sec = 0;
  sleep.tv_nsec = 1000; //1ms
  
  //ensure ZRLMPI_MAX_MESSAGE_SIZE_BYTES (in case of udp)
  for(int i = 0; i < byte_length; i+=max_udp_payload_bytes)
  {
    int count_of_this_message = byte_length - i; //important for last message
    if(count_of_this_message > max_udp_payload_bytes)
    {
      count_of_this_message = max_udp_payload_bytes;
    }
    printf("sending %d bytes from address %d as data junk...\n",count_of_this_message, i);
    ret = sendto(udp_sock, &buffer[i], count_of_this_message, 0, (sockaddr*)&rank_socks[destination], sizeof(rank_socks[destination]));
    if(ret == -1)
    {
      printf("error with sendto\n");
      perror("sendto");
      exit(EXIT_FAILURE);
    } else {
      total_send += ret;
      total_packets++;
    }
    //make sure they stay in order
    nanosleep(&sleep, &sleep);
  }
  //res = sendto(udp_sock, &buffer, count*4 + MPIF_HEADER_LENGTH, 0, (sockaddr*)&rank_socks[destination], sizeof(rank_socks[destination]));
  //std::cout << res << " bytes sent for DATA" <<std::endl;

  std::cout << total_send << " bytes sent for DATA (in " << total_packets << " packets) " <<std::endl;
  
  ret = receiveHeader(ntohl(rank_socks[destination].sin_addr.s_addr), ACK, corresponding_call_type, destination, 0, 0);
  printf("Got ACK\n");

}

void MPI_Send(
    int* data,
    int count,
    MPI_Datatype datatype,
    int destination,
    int tag,
    MPI_Comm communicator)
{
  printf("[MPI_Send] count: %d, datatype %d, destination %d, tag %d, comm: %d.\n",count,(int) datatype, destination, tag, (int) communicator);
  //ensure ZRLMPI_MAX_MESSAGE_SIZE_BYTES
  //for now, only datatypes of size 4
  //ZRLMPI_MAX_MESSAGE_SIZE is divideable by 4
  //for(int i = 0; i < count; i+=ZRLMPI_MAX_MESSAGE_SIZE_WORDS)
  //{
  //  int count_of_this_message = count - i; //important for last message
  //  if(count_of_this_message > ZRLMPI_MAX_MESSAGE_SIZE_WORDS)
  //  {
  //    count_of_this_message = ZRLMPI_MAX_MESSAGE_SIZE_WORDS;
  //  }
  //  send_internal(&data[i], count_of_this_message, datatype, destination, tag, communicator);
  //}
  
  send_internal(data, count, datatype, destination, tag, communicator);


}

void recv_internal(
    int* data,
    int count,
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm communicator,
    MPI_Status* status)
{
  uint8_t bytes[MPIF_HEADER_LENGTH];
  //TODO make generic
  int typewidth = 4;
  int corresponding_call_type = MPI_SEND_INT;
  int call_type = MPI_RECV_INT;
  
  int ret = receiveHeader(ntohl(rank_socks[source].sin_addr.s_addr), SEND_REQUEST, corresponding_call_type, source, 0, 0);

  printf("Got SEND_REQUEST\n");

  MPI_Header header = MPI_Header(); 
  header.dst_rank = source;
  //header.src_rank = MPI_OWN_RANK;
  header.src_rank = own_rank;
  header.size = 0;
  header.call = call_type;
  header.type = CLEAR_TO_SEND;

  headerToBytes(header, bytes);
  
  int res = sendto(udp_sock, &bytes, MPIF_HEADER_LENGTH, 0, (sockaddr*)&rank_socks[source], sizeof(rank_socks[source]));

  std::cout << res << " bytes sent for CLEAR_TO_SEND" << std::endl;
  
  //recv data
  uint8_t buffer[count*typewidth + MPIF_HEADER_LENGTH];

  ret = receiveHeader(ntohl(rank_socks[source].sin_addr.s_addr), DATA, corresponding_call_type, source, count*typewidth, buffer);

  printf("Receiving DATA ...\n");

  //copy only the number of bytes the sender send us but at most count*4 
  if(ret > count*4)
  {
    memcpy(data, &buffer[MPIF_HEADER_LENGTH], count*4);
  } else {
    memcpy(data, &buffer[MPIF_HEADER_LENGTH], ret);
  }

  //send ACK
  header = MPI_Header(); 
  header.dst_rank = source;
  //header.src_rank = MPI_OWN_RANK;
  header.src_rank = own_rank;
  header.size = 0;
  header.call = call_type;
  header.type = ACK;

  headerToBytes(header, bytes);
  
  res = sendto(udp_sock, &bytes, MPIF_HEADER_LENGTH, 0, (sockaddr*)&rank_socks[source], sizeof(rank_socks[source]));

  std::cout << res << " bytes sent for ACK" << std::endl;

}

void MPI_Recv(
    int* data,
    int count,
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm communicator,
    MPI_Status* status)
{
  printf("[MPI_Recv] count: %d, datatype %d, source %d, tag %d, comm: %d.\n",count,(int) datatype, source, tag, (int) communicator);
  //ensure ZRLMPI_MAX_MESSAGE_SIZE_BYTES
  //for now, only datatypes of size 4
  //ZRLMPI_MAX_MESSAGE_SIZE is divideable by 4
  //for(int i = 0; i < count; i+=ZRLMPI_MAX_MESSAGE_SIZE_WORDS)
  //{
  //  int count_of_this_message = count - i; //important for last message
  //  if(count_of_this_message > ZRLMPI_MAX_MESSAGE_SIZE_WORDS)
  //  {
  //    count_of_this_message = ZRLMPI_MAX_MESSAGE_SIZE_WORDS;
  //  }
  //  recv_internal(&data[i], count_of_this_message, datatype, source, tag, communicator, status);
  //}

  recv_internal(data, count, datatype, source, tag, communicator, status);

  if(status != MPI_STATUS_IGNORE)
  {
    *status = 1; //success TODO
  }
}


void MPI_Finalize()
{
  close(udp_sock);
  //TODO 
  //clock_t clock_end = clock();
  timestamp_t t1 = get_timestamp();
  //double elapsed_time = (double)(clock_end - clock_begin) / CLOCKS_PER_SEC;
  double elapsed_time_secs = (double)(t1 - t0) / 1000000.0L;
  printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
  printf("\tZRLMPI execution time: %lfs\n", elapsed_time_secs); 
  printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
  return;
}





int resolvehelper(const char* hostname, int family, const char* service, sockaddr_in* pAddr)
{
  int result;
  addrinfo* result_list = NULL;
  addrinfo hints = {};
  hints.ai_family = family;
  hints.ai_socktype = SOCK_DGRAM; // without this flag, getaddrinfo will return 3x the number of addresses (one for each socket type).
  result = getaddrinfo(hostname, service, &hints, &result_list);
  if (result == 0)
  {
    //ASSERT(result_list->ai_addrlen <= sizeof(sockaddr_in));
    memcpy(pAddr, result_list->ai_addr, result_list->ai_addrlen);
    freeaddrinfo(result_list);
  }

  return result;
}

static void printUsage(const char* argv0)
{
  fprintf(stderr, "Usage: %s <tcp|udp> <host-address> <cluster-size> <own-rank> <ip-rank-1> <ip-rank-2> <...>\nCluster size must be at least two and smaller than %d.\n",argv0, MPI_CLUSTER_SIZE_MAX);
  exit(EXIT_FAILURE);
}



int main(int argc, char **argv)
{

  if(argc < 6 || atoi(argv[3]) < 2 || atoi(argv[3]) > MPI_CLUSTER_SIZE_MAX)
  {
    printUsage(argv[0]);
  }

  //char protocol[4];
  char *protocol = argv[1];
  char* host_address = argv[2];
  cluster_size = atoi(argv[3]);
  own_rank = atoi(argv[4]);
  //own_rank = MPI_OWN_RANK;
  if(own_rank < 0)
  {
    fprintf(stderr, "invaldi own-rank given.\n");
    printUsage(argv[0]);
  }

  if(strncmp("udp",protocol,3) == 0)
  {
    printf("using udp...\n");
    use_udp = true;
  } else if(strncmp("tcp",protocol,3) == 0)
  {
    printf("using tcp...\n");
    use_tcp = true;
    
    printf("   NOT YET IMPLEMENTED   ");
    exit(EXIT_FAILURE);
  } else {
    fprintf(stderr,"invalid protocol given: %s.\n",protocol);
    printUsage(argv[0]);
  }



  int result = 0;
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  //from: https://stackoverflow.com/questions/973439/how-to-set-the-dont-fragment-df-flag-on-a-socket
  //TODO: seems to kill sendto from time to time?
  //int val = IP_PMTUDISC_DO;
  //setsockopt(sock, IPPROTO_IP, IP_MTU_DISCOVER, &val, sizeof(val));

  if(sock == -1)
  {
    std::cerr <<" error socket" << std::endl;
    exit(EXIT_FAILURE);
  }


  udp_sock = sock;
  uint16_t own_port = MPI_PORT;
  std::vector<std::string> rank_ip_addrs;

  //first, determine ports
  int offset = 0;
  for(int i = 0; i<cluster_size; i++)
  {
    if(i == own_rank)
    {
      offset = -1;
      int multiple = array_count(host_address, rank_ip_addrs);
      if(multiple > 0)
      {
        //std::cout << "found multiple (" << multiple << ") ip-addr for own_rank " << std::endl;
        own_port += multiple;
      }
      rank_ip_addrs.push_back(host_address);
      continue;
    }
    if(i+5+offset >= argc)
    { //if CPU is rank0, avoid out_of_bounce reference (nullptr)
      break;
    }
    sockaddr_in addrDest = {};
    uint16_t rank_port = MPI_PORT;
    std::string rank_addr = std::string(argv[5 + i + offset]);
    //increment port, if IP is alreadu used
    int multiple = array_count(rank_addr, rank_ip_addrs);
    if(multiple > 0)
    {
      //std::cout << "found multiple (" << multiple << ") ip-addr for " << rank_addr << std::endl;
      rank_port += multiple;
    }
    rank_ip_addrs.push_back(rank_addr);
    std::cout << "rank " << i <<" addr: " << rank_addr << " port: " << rank_port << std::endl;
    //result = resolvehelper(rank_addr.c_str(), AF_INET, MPI_SERVICE, &addrDest);
    const char *service_name = std::to_string(rank_port).c_str();
    result = resolvehelper(rank_addr.c_str(), AF_INET, service_name, &addrDest);
    if (result != 0)
    {
      std::cerr << "getaddrinfo: " << errno;
      exit(1);
    }

    rank_socks[i] = addrDest;

  }

  //now, listen on this port
  std::cout << "I'm rank " << own_rank << " on address " << host_address << " and listening on port: " << own_port << std::endl;
  sockaddr_in addrListen = {}; 
  addrListen.sin_family = AF_INET;
  //addrListen.sin_port = htons(MPI_PORT);
  addrListen.sin_port = htons(own_port);
  //inet_aton(HOST_ADDRESS, (in_addr*) &addrListen.sin_addr.s_addr);
  inet_aton(host_address, (in_addr*) &addrListen.sin_addr.s_addr);
  result = bind(sock, (sockaddr*)&addrListen, sizeof(addrListen));
  if (result == -1)
  {
    std::cerr << "bind: " << errno << std::endl;
    exit(1);
  }
  
  //get MTU
  //1. get if name of ip addr https://man7.org/linux/man-pages/man3/getifaddrs.3.html
  //TODO
  //TODO: if we get the mtu automatically, all nodes have to agree on one...--> header?
  //https://stackoverflow.com/questions/38817023/how-to-send-packets-according-to-the-mtu-value
  //struct ifreq ifr;
  ////ifr.ifr_addr.sa_family = AF_INET;
  //memcpy(&ifr.ifr_addr, &addrListen, sizeof(addrListen));
  //strcpy(ifr.ifr_name, "vpn0");
  //if (ioctl(sock, SIOCGIFMTU, (caddr_t)&ifr) < 0) 
  //{
  //  perror("ioctl");
  //  exit(EXIT_FAILURE);
  //}

  //printf("MTU is %d.\n", ifr.ifr_mtu);
  //max_udp_payload_bytes = ifr.ifr_mtu - UDP_HEADER_SIZE_BYTES;
  
  max_udp_payload_bytes = CUSTOM_MTU - UDP_HEADER_SIZE_BYTES;
  if(max_udp_payload_bytes > ZRLMPI_MAX_MESSAGE_SIZE_BYTES)
  {
    max_udp_payload_bytes = ZRLMPI_MAX_MESSAGE_SIZE_BYTES;
  }
  //printf("max payload bytes: %d.\n", max_udp_payload_bytes);

  //init cache
  for(int i = 0; i< MPI_CLUSTER_SIZE_MAX; i++)
  {
    skip_cache_entry[i] = false;
  }

  std::cerr << "----- starting MPI app -----" << std::endl;
  //call actual MPI code 
  app_main();


  return 0;

}

