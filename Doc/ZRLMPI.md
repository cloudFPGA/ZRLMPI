ZRLMPI
=============
**MPI for cloudFPGA** (from the Zurich Research Laboratory (ZRL))


C/HLS calls
-----------------

```cpp
MPI_Send(
    void* data,
    int count,
    MPI_Datatype datatype,
    int destination,
    int tag,
    MPI_Comm communicator)
```

```cpp
MPI_Recv(
    void* data,
    int count,
    MPI_Datatype datatype,
    int source,
    int tag,
    MPI_Comm communicator,
    MPI_Status* status)

```


MPE Interface
--------------------
**The Message Passing Engine in the FPGA** 



### Version 0.9: 

Since **all MPI calls are always started by the user**, there is *only one `MPI_Interface`* from the user's application wrapper to the MPE.
There is `MPI_data` in both directions, of course.

```vhdl
entity MPI_Interface is 
  port(
    mpi_call  : out    std_logic_vector( 7 downto 0);
    count     : out    std_logic_vector(31 downto 0);
    rank      : out    std_logic_vector(31 downto 0)
  );
end MPI_Interface; 

```

optional:
```vhdl 
  tag  : in/out   std_logic_vector( 7 downto 0);
```

**`MPI_data` is a `Axis<8 >`** bus. ZRLMPI uses a bus width of 1 Byte in order to avoid alignment issues. 


### Custom Data Types

#### `mpi_call` definition: 


| `mpi_call`: `uint8_t`  |  Name of Call |
| :--------------------: | ------------- | 
|    0                   | `MPI_Send_INT`| 
|    1                   | `MPI_Recv_INT`| 
|    2                   | `MPI_Send_FLOAT`| 
|    3                   | `MPI_Recv_FLOAT`| 
|    4                   | `MPI_Barrier` | 


#### `packet_type` definition:


| `packet_type`: `uint8_t`  |  Type |
| :------------------------: | ------------- | 
|    1                       | `SEND_REQUEST`| 
|    2                       | `CLEAR_TO_SEND`| 
|    3                       | `DATA`| 
|    4                       | `ACK`| 
|    5                       | `ERROR` | 




### ZRLMPI-Header definition: 

* Each data stream send over network must have a header (also called `Envelope`)
* inserted and removed by the MPE core 
* for easy detection: `0x96` at the beginning and end *and* a defined length 
* size and rank are represented in *Little Endian* 
* before each transmission -- send and receive -- a **handshake** takes place
  `SEND_REQUEST --> CLEAR_TO_SEND --> SEND_DATA --> ACK_DATA` 
* Handshake packets (`SEND_REQUEST`, `CLEAR_TO_SEND`, `ACK`, `ERROR` ) should have `length=0`
  (`ERROR` may contain some bytes to name the type of the error)


| **Byte** position | Description |
|:-----------------:| ----------- | 
|        0 --  3    | `0x96 0x96 0x96 0x96`     | 
|        4 --  7    |  Destination rank |
|        8 -- 11    |  Source rank | 
|       12 -- 15    |  Size *in Bytes* (excluding header) |
|          16       | type of call (see `mpi_call`) |
|          17       | type of packet (see `packet_type`) |
|          18       | (optional) Tag | 
|       19 -- 27    | (reserved) | 
|       28 -- 31    | `0x96 0x96 0x96 0x96`     | 
  
**Length: 32 Bytes**



