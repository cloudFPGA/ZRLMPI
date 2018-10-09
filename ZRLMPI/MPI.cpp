
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


static sockaddr_in rank_socks[3];
int udp_sock = 0;

void MPI_Init()
{
  //we are all set 
  return;
}

void MPI_Comm_rank(MPI_Comm communicator, int* rank)
{
  *rank = MPI_OWN_RANK;
}

void MPI_Comm_size( MPI_Comm communicator, int* size)
{
  *size = MPI_CLUSTER_SIZE;
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

  struct sockaddr_in src_addr;
  socklen_t slen = sizeof(src_addr);

  res = recvfrom(udp_sock, &bytes, MPIF_HEADER_LENGTH, 0, (sockaddr*)&src_addr, &slen);

  printf("received packet from %s:%d \n",inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));
  header = MPI_Header();
  int ret = bytesToHeader(bytes, header);

  if(ret != 0)
  {
    printf("invalid header.\n");
    //TODO
    exit(EXIT_FAILURE);
  }


  if(ntohl(src_addr.sin_addr.s_addr) != ntohl(rank_socks[destination].sin_addr.s_addr))
  {
    printf("header does not match ipAddress. \n");
    //TODO
    exit(EXIT_FAILURE);
  }

  if(header.dst_rank != MPI_OWN_RANK)
  {
    printf("I'm not the right recepient!\n");
    //TODO
    exit(EXIT_FAILURE);
  }

  if(header.type != CLEAR_TO_SEND)
  {
    printf("Expected CLEAR_TO_SEND, got %d!\n", (int) header.type);
    //TODO
    exit(EXIT_FAILURE);
  }

  //check data type 
  //TODO: generic
  if(header.call != MPI_RECV_INT)
  {
    printf("receiver expects different data type: %d.\n", (int) header.call);
    exit(EXIT_FAILURE);
  }
  
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

  res = recvfrom(udp_sock, &bytes, MPIF_HEADER_LENGTH, 0, (sockaddr*)&src_addr, &slen);

  printf("received packet from %s:%d \n",inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));
  header = MPI_Header();
  ret = bytesToHeader(bytes, header);

  if(ret != 0)
  {
    printf("invalid header.\n");
    //TODO
    exit(EXIT_FAILURE);
  }


  if(ntohl(src_addr.sin_addr.s_addr) != ntohl(rank_socks[destination].sin_addr.s_addr))
  {
    printf("header does not match ipAddress. \n");
    //TODO
    exit(EXIT_FAILURE);
  }

  if(header.dst_rank != MPI_OWN_RANK)
  {
    printf("I'm not the right recepient!\n");
    //TODO
    exit(EXIT_FAILURE);
  }

  if(header.type != ACK)
  {
    printf("Expected ACK, got %d!\n", (int) header.type);
    //TODO
    exit(EXIT_FAILURE);
  }
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
  struct sockaddr_in src_addr;
  socklen_t slen = sizeof(src_addr);

  int res = recvfrom(udp_sock, &bytes, MPIF_HEADER_LENGTH, 0, (sockaddr*)&src_addr, &slen);

  printf("received packet from %s:%d \n",inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));
  MPI_Header header = MPI_Header();
  int ret = bytesToHeader(bytes, header);

  if(ret != 0)
  {
    printf("invalid header.\n");
    //TODO
    exit(EXIT_FAILURE);
  }


  if(ntohl(src_addr.sin_addr.s_addr) != ntohl(rank_socks[source].sin_addr.s_addr))
  {
    printf("header does not match ipAddress. \n");
    //TODO
    exit(EXIT_FAILURE);
  }

  if(header.dst_rank != MPI_OWN_RANK)
  {
    printf("I'm not the right recepient!\n");
    //TODO
    exit(EXIT_FAILURE);
  }

  if(header.type != SEND_REQUEST)
  {
    printf("Expected SEND_REQUEST, got %d!\n", (int) header.type);
    //TODO
    exit(EXIT_FAILURE);
  }

  //check data type 
  //TODO: generic
  if(header.call != MPI_SEND_INT)
  {
    printf("receiver expects different data type: %d.\n", (int) header.call);
    exit(EXIT_FAILURE);
  }
  
  printf("Got SEND_REQUEST\n");

  header = MPI_Header(); 
  header.dst_rank = source;
  header.src_rank = MPI_OWN_RANK;
  header.size = 0;
  header.call = MPI_RECV_INT;
  header.type = CLEAR_TO_SEND;

  headerToBytes(header, bytes);
  
  res = sendto(udp_sock, &bytes, MPIF_HEADER_LENGTH, 0, (sockaddr*)&rank_socks[source], sizeof(rank_socks[source]));

  std::cout << res << " bytes sent for CLEAR_TO_SEND" << std::endl;
  
  //recv data
  uint8_t buffer[count*4 + MPIF_HEADER_LENGTH];
  
  res = recvfrom(udp_sock, &buffer, count*4 + MPIF_HEADER_LENGTH, 0, (sockaddr*)&src_addr, &slen);

  printf("received packet from %s:%d \n",inet_ntoa(src_addr.sin_addr), ntohs(src_addr.sin_port));
  header = MPI_Header();
  ret = bytesToHeader(buffer, header);

  if(ret != 0)
  {
    printf("invalid header.\n");
    //TODO
    exit(EXIT_FAILURE);
  }


  if(ntohl(src_addr.sin_addr.s_addr) != ntohl(rank_socks[source].sin_addr.s_addr))
  {
    printf("header does not match ipAddress. \n");
    //TODO
    exit(EXIT_FAILURE);
  }

  if(header.dst_rank != MPI_OWN_RANK)
  {
    printf("I'm not the right recepient!\n");
    //TODO
    exit(EXIT_FAILURE);
  }

  if(header.type != DATA)
  {
    printf("Expected SEND_REQUEST, got %d!\n", (int) header.type);
    //TODO
    exit(EXIT_FAILURE);
  }

  //check data type 
  //TODO: generic
  if(header.call != MPI_SEND_INT)
  {
    printf("receiver expects different data type: %d.\n", (int) header.call);
    exit(EXIT_FAILURE);
  }
  
  printf("Receiving DATA ...\n");

  //copy only the number of bytes the sender send us 
  //TODO: potential heartbleed
  memcpy(data, &buffer[MPIF_HEADER_LENGTH], header.size);

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
  fprintf(stderr, "Usage: %s <slot-rank-1> <slot-rank-2>\n",argv0);
  exit(EXIT_FAILURE);
}



int main(int argc, char **argv)
{

  if(argc != 3)
  {
    printUsage(argv[0]);
  }


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
  result = bind(sock, (sockaddr*)&addrListen, sizeof(addrListen));
  if (result == -1)
  {
    std::cerr << "bind: " << errno;
    exit(1);
  }


  sockaddr_in addrDest_1 = {};
  sockaddr_in addrDest_2 = {};

  std::string rank1_addr = std::string(MPI_BASE_IP).append(argv[1]);
  std::string rank2_addr = std::string(MPI_BASE_IP).append(argv[2]);

  std::cout << "rank 1 addr: " << rank1_addr << std::endl;
  std::cout << "rank 2 addr: " << rank2_addr << std::endl;


  result = resolvehelper(rank1_addr.c_str(), AF_INET, MPI_SERVICE, &addrDest_1);
  if (result != 0)
  {
    std::cerr << "getaddrinfo: " << errno;
    exit(1);
  }

  result = resolvehelper(rank2_addr.c_str(), AF_INET, MPI_SERVICE, &addrDest_2);
  if (result != 0)
  {
    std::cerr << "getaddrinfo: " << errno;
    exit(1);
  }

  rank_socks[1] = addrDest_1;
  rank_socks[2] = addrDest_2;

  //call actual MPI code 
  app_main();


  return 0;

}

