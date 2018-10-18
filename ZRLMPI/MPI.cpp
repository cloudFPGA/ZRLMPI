
#include <stdint.h>
#include "zrlmpi_common.hpp"
#include "MPI.hpp"
#include "test.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <memory.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <errno.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdint.h>


sockaddr_in rank_socks[MPI_CLUSTER_SIZE_MAX + 1];
int udp_sock = 0;
int cluster_size = 0;
int own_rank = 0;

MPI_Header header_recv_cache[MPI_CLUSTER_SIZE_MAX];
int cache_iter = 0;
int cache_num = 0;

//returns the size
int receiveHeader(unsigned long expAddr, packetType expType, mpiCall expCall, uint32_t expSrcRank, int buffer_length, uint8_t *buffer)
{
  uint8_t bytes[MPIF_HEADER_LENGTH];
  struct sockaddr_in src_addr;
  socklen_t slen = sizeof(src_addr);
  MPI_Header header = MPI_Header();

  bool copyToCache = false, checkedCache = false;
  int ret = 0, res = 0, cache_i = 0;


  while(true) { 

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
      header = header_recv_cache[cache_i];
      cache_i++;

    } else {

      //then wait for network 
      header = MPI_Header();
      if(buffer_length > 0)
      { 
        res = recvfrom(udp_sock, buffer, MPIF_HEADER_LENGTH + buffer_length, 0, (sockaddr*)&src_addr, &slen);
        ret = bytesToHeader(buffer, header);
      } else { 
        res = recvfrom(udp_sock, &bytes, MPIF_HEADER_LENGTH, 0, (sockaddr*)&src_addr, &slen);
        ret = bytesToHeader(bytes, header);
      }

      printf("received packet from %s:%d \n",inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));


      if(ret != 0)
      {
        printf("invalid header.\n");
        //copyToCache = true;
        continue;
      }

    }
    copyToCache = false;

    if(checkedCache)
    {
      if(ntohl(src_addr.sin_addr.s_addr) != expAddr)
      {
        printf("Header does not match ipAddress: Expected %ld, got %ld \n", expAddr, ntohl(src_addr.sin_addr.s_addr));
        copyToCache = true;
      }
    }

    if(header.src_rank != expSrcRank)
    {
      printf("Header does not match expected src rank: Expected %d, got %d \n", expSrcRank, header.src_rank);
      copyToCache = true;
    }

    if(header.dst_rank != MPI_OWN_RANK)
    {
      printf("I'm not the right recepient!\n");
      copyToCache = true;
    }

    if(header.type != expType)
    {
      printf("Expected type %d, got %d!\n",(int) expType, (int) header.type);
      copyToCache = true;
    }

    //check data type 
    //TODO: generic
    if(header.call != expCall)
    {
      printf("receiver expects different call: %d.\n", (int) header.call);
      copyToCache = true;
    }

    if(copyToCache && checkedCache)
    {
      header_recv_cache[cache_iter] = header; 
      cache_iter++; 
      if (cache_iter >= MPI_CLUSTER_SIZE_MAX)
      {
        //some type of clean 
        //TODO
        cache_iter = 0;
      }
      printf("put header to cache\n");
      cache_num++;
    } 
    if(!checkedCache && !copyToCache)
    {//delete from cache
      printf("found header in cache\n");
      header_recv_cache[cache_i-1] = MPI_Header();
      cache_num--;
    }
    if(!copyToCache)
    {
      break;
    }

  } 

  return header.size;
}


void MPI_Init()
{
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


void MPI_Send(
    int* data,
    int count,
    MPI_Datatype datatype,
    int destination,
    int tag,
    MPI_Comm communicator)
{

  uint8_t bytes[MPIF_HEADER_LENGTH];
  MPI_Header header = MPI_Header(); 
  header.dst_rank = destination;
  header.src_rank = MPI_OWN_RANK;
  header.size = 0;
  header.call = MPI_SEND_INT;
  header.type = SEND_REQUEST;

  headerToBytes(header, bytes);

  int res = sendto(udp_sock, &bytes, MPIF_HEADER_LENGTH, 0, (sockaddr*)&rank_socks[destination], sizeof(rank_socks[destination]));

  std::cout << res << " bytes sent for SEND_REQUEST" << std::endl;

  int ret = 0;

  ret = receiveHeader(ntohl(rank_socks[destination].sin_addr.s_addr), CLEAR_TO_SEND, MPI_RECV_INT, destination, 0, 0);
  printf("Got CLEAR to SEND\n");

  //Send data
  header = MPI_Header(); 
  header.dst_rank = destination;
  header.src_rank = MPI_OWN_RANK;
  header.size = count*4;
  header.call = MPI_SEND_INT;
  header.type = DATA;

  uint8_t buffer[count*4 + MPIF_HEADER_LENGTH];

  headerToBytes(header, buffer);
  //TODO generic
  memcpy(&buffer[MPIF_HEADER_LENGTH], data, count*4);
  
  res = sendto(udp_sock, &buffer, count*4 + MPIF_HEADER_LENGTH, 0, (sockaddr*)&rank_socks[destination], sizeof(rank_socks[destination]));

  std::cout << res << " bytes sent for DATA" <<std::endl;
  
  ret = receiveHeader(ntohl(rank_socks[destination].sin_addr.s_addr), ACK, MPI_RECV_INT, destination, 0, 0);
  printf("Got ACK\n");


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
  uint8_t bytes[MPIF_HEADER_LENGTH];
  
  int ret = receiveHeader(ntohl(rank_socks[source].sin_addr.s_addr), SEND_REQUEST, MPI_SEND_INT, source, 0, 0);

  printf("Got SEND_REQUEST\n");

  MPI_Header header = MPI_Header(); 
  header.dst_rank = source;
  header.src_rank = MPI_OWN_RANK;
  header.size = 0;
  header.call = MPI_RECV_INT;
  header.type = CLEAR_TO_SEND;

  headerToBytes(header, bytes);
  
  int res = sendto(udp_sock, &bytes, MPIF_HEADER_LENGTH, 0, (sockaddr*)&rank_socks[source], sizeof(rank_socks[source]));

  std::cout << res << " bytes sent for CLEAR_TO_SEND" << std::endl;
  
  //recv data
  uint8_t buffer[count*4 + MPIF_HEADER_LENGTH];

  ret = receiveHeader(ntohl(rank_socks[source].sin_addr.s_addr), DATA, MPI_SEND_INT, source, count*4, buffer);

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
  header.src_rank = MPI_OWN_RANK;
  header.size = 0;
  header.call = MPI_RECV_INT;
  header.type = ACK;

  headerToBytes(header, bytes);
  
  res = sendto(udp_sock, &bytes, MPIF_HEADER_LENGTH, 0, (sockaddr*)&rank_socks[source], sizeof(rank_socks[source]));

  std::cout << res << " bytes sent for ACK" << std::endl;

}

void MPI_Finalize()
{
  close(udp_sock);
  //TODO 
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
  fprintf(stderr, "Usage: %s <cluster-size> <slot-rank-1> <slot-rank-2> <...>\nCluster size must be at least two and smaller than %d.\n",argv0, MPI_CLUSTER_SIZE_MAX);
  exit(EXIT_FAILURE);
}



int main(int argc, char **argv)
{

  if(argc < 3 || atoi(argv[1]) < 2 || atoi(argv[1]) > MPI_CLUSTER_SIZE_MAX)
  {
    printUsage(argv[0]);
  }

  cluster_size = atoi(argv[1]);
  //own_rank = argv[2];
  own_rank = MPI_OWN_RANK;

  int result = 0;
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if(sock == -1)
  {
    std::cerr <<" error socket" << std::endl;
    exit(EXIT_FAILURE);
  }

  udp_sock = sock;

  sockaddr_in addrListen = {}; 
  addrListen.sin_family = AF_INET;
  addrListen.sin_port = htons(MPI_PORT);
  inet_aton(HOST_ADDRESS, (in_addr*) &addrListen.sin_addr.s_addr);
  result = bind(sock, (sockaddr*)&addrListen, sizeof(addrListen));
  if (result == -1)
  {
    std::cerr << "bind: " << errno << std::endl;
    exit(1);
  }


  for(int i = 1; i<cluster_size; i++)
  {
    sockaddr_in addrDest = {};
    std::string rank_addr = std::string(MPI_BASE_IP).append(argv[1 + i]);
    std::cout << "rank " << i <<" addr: " << rank_addr << std::endl;
    result = resolvehelper(rank_addr.c_str(), AF_INET, MPI_SERVICE, &addrDest);
    if (result != 0)
    {
      std::cerr << "getaddrinfo: " << errno;
      exit(1);
    }

    rank_socks[i] = addrDest;

  }

  //call actual MPI code 
  app_main();


  return 0;

}

